# gesture_monotor - ICM20608姿态检测应用程序+驱动程序

## 功能简介
进行姿态检测，读取姿态角度，设备倾倒状态（比如墙面维修、安装不稳）则终止触控UI，触发保护UI


## 编译、运行应用程序 gesture_monitor
### 确保有交叉编译工具链 arm-linux-gnueabihf- :
make

### 运行方法
./gesture_monitor debug    # 前台调试模式
./gesture_monitor daemon   # 后台监测模式


## 编译驱动程序 icm20608.ko
### 确保如下目录有板卡上运行的Linux内核
KERNEL_DIR := /home/doge/project/IOT_wirlessdevice_and_gateway/linuxkernel


### 确保有交叉编译工具链 arm-linux-gnueabihf-，执行 :
make

## 编译Qt保护UI
### 1.确保有这个NXP IMX交叉编译环境
ls /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi


### 2.将当前终端会话切换成针对 ARM 架构的交叉编译环境
source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi


### 3.编写smart_schedule.pro,给项目文件生成 Makefile
qmake protect_ui.pro


### 4.执行make
make

## 三.姿态检测应用程序
### 1.保护UI在倾角>15度的情况下触发
最后计算的角度，只包含x,y轴的倾角(Roll和Pitch)，通过两个角度合成，检测是否>15度
不包含绕Z轴的旋转分量（Yaw），但是z轴必须参与计算
(通过 X/Y轴分量与Z轴分量的比值，得到x，y的倾角)

angle_x = atan2((float)accel_y, (float)accel_z) * 180.0 / M_PI;
angle_y = atan2((float)accel_x, (float)accel_z) * 180.0 / M_PI;
tilt_angle = sqrt(angle_x * angle_x + angle_y * angle_y);

### 2.防抖机制（触发保护后，10秒内不会再重复触发）
if (current_time - last_time > MONITOR_INTERVAL) {
    last_time = current_time;
}

#### 保护UI的处理(通过 fork() + execl() 启动独立进程)
static pid_t start_protect_ui(void)
{
    pid_t pid = fork();     
    if (pid == 0) {
        execl("/usr/app/protect_ui", "protect_ui", NULL);       
        fprintf(stderr, "Failed to execute protect_ui\n");
        exit(1);
    } else if (pid > 0) {
        return pid;
    } else {
        perror("fork");
        return -1;
    }
}


#### 恢复逻辑
断电之后，扶正重启即可恢复


#### 后台模式: 调用fork脱离终端的影响
static void daemonize(void) {
    fork();   // 第一次：脱离父进程
    setsid(); // 创建新会话，脱离终端
    fork();   // 第二次：防止重新获得控制终端
}


#### 前台模式：调试
printf


### 四.设备树节点实现
&ecspi3 {
	fsl,spi-num-chipselects = <1>;				/* 1个片选 */
	cs-gpios = <&gpio1 20 GPIO_ACTIVE_LOW>;		/* 片选引脚，软件片选！ */
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_ecspi3>;
	status = "okay";

	/* 对应的SPI芯片子节点 */
	spidev0: icm20608@0 {			/* @后面的0表示次SPI芯片接到哪个硬件片选上 */
		reg = <0>;
		compatible = "Lu,icm20608";
		spi-max-frequency = <8000000>;	/*  SPI时钟频率8MHz*/
	};
};







