# icm20608角度传感器驱动 - ICM20608姿态检测驱动程序

## 功能简介
驱动ICM20608传感器，检测设备的姿态角，提供应用程序的读取接口


## 编译驱动程序 icm20608.ko
### 1.确保如下目录有板卡上运行的Linux内核
KERNEL_DIR := /home/doge/project/IOT_wirlessdevice_and_gateway/linuxkernel

### 2.确保有交叉编译工具链 
arm-linux-gnueabihf-

### 3.执行make
make


## 一.驱动程序提供的主要接口

### X轴加速度原始值
cat /sys/bus/iio/devices/iio:device0/in_accel_x_raw

### Y轴加速度原始值
cat /sys/bus/iio/devices/iio:device0/in_accel_y_raw

### Z轴加速度原始值
cat /sys/bus/iio/devices/iio:device0/in_accel_z_raw



## 二.驱动程序的调用步骤  IIO + Regmap

### 1. spi_driver注册，实现probe、remove，实现设备树匹配表
static const struct of_device_id icm20608_of_match[] = {
	{ .compatible = "Lu,icm20608" },
	{ /* Sentinel */ }
};

static struct spi_driver icm20608_driver = {
	.probe = icm20608_probe,
	.remove = icm20608_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "icm20608",
		   	.of_match_table = icm20608_of_match,
		   },
	.id_table = icm20608_id,
};


### 2.probe() 被调用（匹配到设备时，自动赋值spi_device *spi）
static int icm20608_probe(struct spi_device *spi)

#### 分配IIO设备
indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*dev));

#### 初始化regmap ，之后可以用regmap_read、regmap_write读取
#### 将spi总线权限交给regmap 
dev->regmap = regmap_init_spi(spi, &dev->regmap_config);

#### 初始化硬件(自定义封装函数)
icm20608_reginit(dev);

#### 配置IIO设备并且注册IIO设备
indio_dev->dev.parent = &spi->dev;
indio_dev->info = &icm20608_info;
indio_dev->name = ICM20608_NAME;	
indio_dev->modes = INDIO_DIRECT_MODE;	
indio_dev->channels = icm20608_channels;
indio_dev->num_channels = ARRAY_SIZE(icm20608_channels);

ret = iio_device_register(indio_dev);

#### IIO设备节点生成，等待应用程序读取:/sys/bus/iio/devices/iio:deviceX/ 

#### 实现IIO设备信息结构体iio_info与IIO回调函数
static const struct iio_info icm20608_info = {
    .read_raw = icm20608_read_raw,      
    .write_raw = icm20608_write_raw,    
    .write_raw_get_fmt = icm20608_write_raw_get_fmt,  
};



