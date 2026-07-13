# gesture_monotor - ICM20608姿态检测应用程序+驱动程序

## 功能简介
进行姿态检测，读取姿态角度，设备倾倒状态（比如墙面维修、安装不稳）则终止触控UI，触发保护UI


## 编译、运行应用程序 gesture_monitor
1.确保有交叉编译工具链 arm-linux-gnueabihf- :
2.make

### 运行方法
./gesture_monitor debug    # 前台调试模式
./gesture_monitor daemon   # 后台监测模式


## 编译Qt保护UI
### 1.确保有这个NXP IMX交叉编译环境
ls /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi


### 2.将当前终端会话切换成针对 ARM 架构的交叉编译环境
source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi


### 3.使能smart_schedule.pro
qmake protect_ui.pro


### 4.执行make
make


## 姿态检测应用程序
### 1.保护UI在倾角>15度的情况下触发
x,y轴的倾角(Roll和Pitch)合成，计算角度，检测是否>15度


### 保护UI的处理
通过 execl() 执行进程跳转


### 恢复逻辑
断电之后，扶正重启即可恢复



