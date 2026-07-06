/*
 * AP3216C是一款集成了环境光传感器(ALS)、接近传感器(PS)和红外LED的三合一传感器
 * 本驱动使用IIO框架实现，符合Linux内核标准
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/regmap.h>

/* AP3216C 寄存器定义 */
#define AP3216C_SYSTEMCONG      0x00    /* 系统配置寄存器 */
#define AP3216C_INTSTATUS       0x01    /* 中断状态寄存器 */
#define AP3216C_INTCLEAR        0x02    /* 中断清除寄存器 */
#define AP3216C_IRDATALOW       0x0A    /* IR数据低字节 */
#define AP3216C_IRDATAHIGH      0x0B    /* IR数据高字节 */
#define AP3216C_ALSDATALOW      0x0C    /* ALS数据低字节 */
#define AP3216C_ALSDATAHIGH     0x0D    /* ALS数据高字节 */
#define AP3216C_PSDATALOW       0x0E    /* PS数据低字节 */
#define AP3216C_PSDATAHIGH      0x0F    /* PS数据高字节 */

/* 系统配置寄存器值 */
#define AP3216C_SYS_RESET       0x04    /* 软件复位 */
#define AP3216C_SYS_ALS_PS_IR   0x03    /* 开启 ALS + PS + IR */
#define AP3216C_SYS_ALS_ONLY    0x01    /* 仅开启 ALS */
#define AP3216C_SYS_PS_IR_ONLY  0x02    /* 仅开启 PS + IR */
#define AP3216C_SYS_POWERDOWN   0x00    /* 掉电模式 */

/* 数据有效位掩码 */
#define AP3216C_IR_OF_MASK      0x80    /* IR溢出标志 */
#define AP3216C_PS_OF_MASK      0x40    /* PS溢出标志 */

struct ap3216c_data {
    struct i2c_client *client;
    struct regmap *regmap;
    struct mutex lock;          /* 保护读取操作 */
};

/* Regmap配置 */
static const struct regmap_config ap3216c_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x2D,
};

/*
 * IIO通道定义
 * AP3216C有三个传感器：ALS(环境光)、PS(接近)、IR(红外)
 */
static const struct iio_chan_spec ap3216c_channels[] = {
    {
        /* ALS - 环境光传感器 */
        .type = IIO_LIGHT,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
                              BIT(IIO_CHAN_INFO_SCALE),
        .indexed = 0,
    },
    {
        /* PS - 接近传感器 */
        .type = IIO_PROXIMITY,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .indexed = 0,
    },
    {
        /* IR - 红外传感器 */
        .type = IIO_INTENSITY,
        .modified = 1,
        .channel2 = IIO_MOD_LIGHT_IR,
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
        .indexed = 0,
    },
};

/**
 * ap3216c_read_als - 读取环境光传感器数据
 * @data: 设备私有数据
 * @val: 存储读取值的指针
 *
 * 返回值: 0表示成功，负值表示错误
 */
static int ap3216c_read_als(struct ap3216c_data *data, int *val)
{
    unsigned int lsb, msb;
    int ret;

    ret = regmap_read(data->regmap, AP3216C_ALSDATALOW, &lsb);
    if (ret < 0)
        return ret;

    ret = regmap_read(data->regmap, AP3216C_ALSDATAHIGH, &msb);
    if (ret < 0)
        return ret;

    *val = (msb << 8) | lsb;
    return 0;
}

/**
 * ap3216c_read_ps - 读取接近传感器数据
 * @data: 设备私有数据
 * @val: 存储读取值的指针
 *
 * 返回值: 0表示成功，负值表示错误
 */
static int ap3216c_read_ps(struct ap3216c_data *data, int *val)
{
    unsigned int lsb, msb;
    int ret;

    ret = regmap_read(data->regmap, AP3216C_PSDATALOW, &lsb);
    if (ret < 0)
        return ret;

    ret = regmap_read(data->regmap, AP3216C_PSDATAHIGH, &msb);
    if (ret < 0)
        return ret;

    /* 检查溢出标志 */
    if (msb & AP3216C_PS_OF_MASK) {
        *val = 0;
        return 0;
    }

    /* PS数据: 高6位 + 低4位 = 10位 */
    *val = ((msb & 0x3F) << 4) | (lsb & 0x0F);
    return 0;
}

/**
 * ap3216c_read_ir - 读取红外传感器数据
 * @data: 设备私有数据
 * @val: 存储读取值的指针
 *
 * 返回值: 0表示成功，负值表示错误
 */
static int ap3216c_read_ir(struct ap3216c_data *data, int *val)
{
    unsigned int lsb, msb;
    int ret;

    ret = regmap_read(data->regmap, AP3216C_IRDATALOW, &lsb);
    if (ret < 0)
        return ret;

    ret = regmap_read(data->regmap, AP3216C_IRDATAHIGH, &msb);
    if (ret < 0)
        return ret;

    /* 检查溢出标志 */
    if (lsb & AP3216C_IR_OF_MASK) {
        *val = 0;
        return 0;
    }

    /* IR数据: 高8位 + 低2位 = 10位 */
    *val = (msb << 2) | (lsb & 0x03);
    return 0;
}

/**
 * ap3216c_read_raw - IIO read_raw回调函数
 * @indio_dev: IIO设备
 * @chan: 通道信息
 * @val: 整数部分
 * @val2: 小数部分
 * @mask: 信息掩码
 *
 * 返回值: IIO值类型
 */
static int ap3216c_read_raw(struct iio_dev *indio_dev,
                            struct iio_chan_spec const *chan,
                            int *val, int *val2, long mask)
{
    struct ap3216c_data *data = iio_priv(indio_dev);
    int ret;

    switch (mask) {
    case IIO_CHAN_INFO_RAW:
        mutex_lock(&data->lock);
        switch (chan->type) {
        case IIO_LIGHT:
            ret = ap3216c_read_als(data, val);
            break;
        case IIO_PROXIMITY:
            ret = ap3216c_read_ps(data, val);
            break;
        case IIO_INTENSITY:
            ret = ap3216c_read_ir(data, val);
            break;
        default:
            ret = -EINVAL;
        }
        mutex_unlock(&data->lock);

        if (ret < 0)
            return ret;
        return IIO_VAL_INT;

    case IIO_CHAN_INFO_SCALE:
        if (chan->type == IIO_LIGHT) {
            /*
             * ALS分辨率: 根据数据手册
             * 默认范围下，1 LSB = 0.35 lux
             * 返回 0.35 = 35/100
             */
            *val = 0;
            *val2 = 350000;  /* 0.35 lux */
            return IIO_VAL_INT_PLUS_MICRO;
        }
        return -EINVAL;

    default:
        return -EINVAL;
    }
}

static const struct iio_info ap3216c_info = {
    .read_raw = ap3216c_read_raw,
};

/**
 * ap3216c_chip_init - 初始化AP3216C芯片
 * @data: 设备私有数据
 *
 * 返回值: 0表示成功，负值表示错误
 */
static int ap3216c_chip_init(struct ap3216c_data *data)
{
    int ret;

    /* 软件复位 */
    ret = regmap_write(data->regmap, AP3216C_SYSTEMCONG, AP3216C_SYS_RESET);
    if (ret < 0) {
        dev_err(&data->client->dev, "Failed to reset chip\n");
        return ret;
    }

    /* 等待复位完成，数据手册要求至少10ms */
    msleep(50);

    /* 开启所有传感器 (ALS + PS + IR) */
    ret = regmap_write(data->regmap, AP3216C_SYSTEMCONG, AP3216C_SYS_ALS_PS_IR);
    if (ret < 0) {
        dev_err(&data->client->dev, "Failed to enable sensors\n");
        return ret;
    }

    /* 等待首次数据转换完成 */
    msleep(150);

    return 0;
}

static int ap3216c_probe(struct i2c_client *client)
{
    struct ap3216c_data *data;
    struct iio_dev *indio_dev;
    int ret;

    /* 分配IIO设备 */
    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
    if (!indio_dev)
        return -ENOMEM;

    data = iio_priv(indio_dev);
    data->client = client;
    mutex_init(&data->lock);

    /* 初始化regmap */
    data->regmap = devm_regmap_init_i2c(client, &ap3216c_regmap_config);
    if (IS_ERR(data->regmap)) {
        dev_err(&client->dev, "Failed to initialize regmap\n");
        return PTR_ERR(data->regmap);
    }

    i2c_set_clientdata(client, indio_dev);

    /* 初始化芯片 */
    ret = ap3216c_chip_init(data);
    if (ret < 0)
        return ret;

    /* 配置IIO设备 */
    indio_dev->name = "ap3216c";
    indio_dev->info = &ap3216c_info;
    indio_dev->modes = INDIO_DIRECT_MODE;
    indio_dev->channels = ap3216c_channels;
    indio_dev->num_channels = ARRAY_SIZE(ap3216c_channels);

    /* 注册IIO设备 */
    ret = devm_iio_device_register(&client->dev, indio_dev);
    if (ret) {
        dev_err(&client->dev, "Failed to register IIO device\n");
        return ret;
    }

    dev_info(&client->dev, "AP3216C IIO driver probed successfully\n");
    return 0;
}

static void ap3216c_remove(struct i2c_client *client)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(client);
    struct ap3216c_data *data = iio_priv(indio_dev);

    /* 关闭传感器 */
    regmap_write(data->regmap, AP3216C_SYSTEMCONG, AP3216C_SYS_POWERDOWN);

    dev_info(&client->dev, "AP3216C IIO driver removed\n");
}

static int ap3216c_suspend(struct device *dev)
{
    struct iio_dev *indio_dev = dev_get_drvdata(dev);
    struct ap3216c_data *data = iio_priv(indio_dev);

    return regmap_write(data->regmap, AP3216C_SYSTEMCONG, AP3216C_SYS_POWERDOWN);
}

static int ap3216c_resume(struct device *dev)
{
    struct iio_dev *indio_dev = dev_get_drvdata(dev);
    struct ap3216c_data *data = iio_priv(indio_dev);

    return ap3216c_chip_init(data);
}

static SIMPLE_DEV_PM_OPS(ap3216c_pm_ops, ap3216c_suspend, ap3216c_resume);
static const struct of_device_id ap3216c_of_match[] = {
    { .compatible = "Lu,ap3216c" },  /* 兼容旧设备树 */
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ap3216c_of_match);

static const struct i2c_device_id ap3216c_id[] = {
    { "ap3216c", 0 },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, ap3216c_id);

static struct i2c_driver ap3216c_driver = {
    .driver = {
        .name = "ap3216c",
        .of_match_table = ap3216c_of_match,
    	.pm = &ap3216c_pm_ops,
    },
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .id_table = ap3216c_id,
};

//将驱动注册到I2C子系统
module_i2c_driver(ap3216c_driver);

MODULE_AUTHOR("Lu");
MODULE_DESCRIPTION("AP3216C Ambient Light, Proximity and IR Sensor IIO Driver");
MODULE_LICENSE("GPL");
