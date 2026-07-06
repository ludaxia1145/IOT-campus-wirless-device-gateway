# auto_brightness - AP3216C 自动亮度调节应用程序+驱动程序

## 功能简介
基于环境光传感器 AP3216C 的自动背光亮度调节程序。程序通过 Linux IIO 框架读取 ALS 原始值，经线性映射后动态调整屏幕背光亮度

## 编译方法
### 1.确保板卡上的内核在如下目录中:
/home/doge/project/IOT_wirlessdevice_and_gateway/linuxkernel

### 2.确保存在交叉编译工具链
CROSS_COMPILE = arm-linux-gnueabihf-

### 执行make



### 一.应用程序与驱动程序的所有交互节点

# 节点1 - /sys/bus/iio/devices/iio:deviceX/name 
作用:识别设备是否为AP3216C

# 节点2 - /sys/bus/iio/devices/iio:deviceX/in_illuminance_raw
作用:读取环境光传感器(ALS)的原始值

# 节点3 - /sys/bus/iio/devices/iio:deviceX/in_illuminance_scale
作用:获取原始值到lux的转换系数

# 节点4 - /sys/bus/iio/devices/iio:deviceX/in_proximity_raw
作用:读取接近传感器(PS)原始值

# 节点5 - /sys/bus/iio/devices/iio:deviceX/in_intensity_ir_raw
作用:读取红外传感器(IR)的原始值

# 节点6 - /sys/class/backlight/backlight/brightness
作用:设置/读取屏幕背光亮度


### 驱动程序的调用步骤

# 1.i2c_driver 注册，实现probe、remove，实现设备树匹配表
static const struct of_device_id ap3216c_of_match[] = {
    { .compatible = "Lu,ap3216c" },  /* 兼容设备树 */
    { /* sentinel */ }
};

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

module_i2c_driver(ap3216c_driver);

# 2.probe() 被调用（匹配到设备时，自动赋值spi_device *spi）
static int icm20608_probe(struct spi_device *spi)  

# 分配IIO设备
indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));

# 初始化regmap ，之后可以用regmap_read、regmap_write读取
# 将i2c总线权限交给regmap 
data->regmap = devm_regmap_init_i2c(client, &ap3216c_regmap_config);

# 初始化硬件(自定义封装函数)
ret = ap3216c_chip_init(data);

# 配置IIO设备并且注册IIO设备
indio_dev->name = "ap3216c";           // 设备名 → 生成 name 节点
indio_dev->info = &ap3216c_info;       // 绑定读写回调
indio_dev->modes = INDIO_DIRECT_MODE;  // 直接读取模式
indio_dev->channels = ap3216c_channels;      // 绑定通道定义
indio_dev->num_channels = ARRAY_SIZE(ap3216c_channels);  // 通道数量
ret = devm_iio_device_register(&client->dev, indio_dev);

# IIO设备节点生成，等待应用程序读取:/sys/bus/iio/devices/iio:deviceX/ 

# 实现IIO设备信息结构体iio_info与IIO回调函数ap3216c_read_raw
# 只要用户 cat 任何一个由 IIO 框架创建的节点，最终都会调用 ap3216c_read_raw
static const struct iio_info ap3216c_info = {
    //核心读取函数，当用户读取任何 sysfs 节点时，IIO 框架都会调用它
    .read_raw = ap3216c_read_raw,
};


### 三.自动亮度调节应用程序

# 调节方法
亮度 = ALS原始值 × 最大亮度等级 / ALS最大原始值

# 前台+后台支持
## 后台程序:
两次fork，让进程脱离终端，避免终端关闭终止进程

## 前台:
printf


