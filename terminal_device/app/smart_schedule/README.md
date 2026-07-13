# 智慧教务终端 - 基于 i.MX6ULL 的校园物联网终端设备

## 项目简介

本项目是一款运行在 **ARM i.MX6ULL 嵌入式开发板** 上的 Qt 终端应用程序，部署于教室门口。终端通过 **TCP** 与网关服务器通信，提供课表查询、教学留言、座位预约、公告浏览、课堂考勤等核心功能，并具备完整的**网络可靠性保障机制**。


## 一.交叉编译
## 1.确保有交叉编译环境
/opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi

## 2.将当前终端会话切换成针对 ARM 架构的交叉编译环境
source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi

## 3.使能qmake
qmake smart_schedule.pro

## 4.执行make
make


# 二.网络可靠性保障 
## 1.断网自动重连（心跳机制） 
每30秒发送heartbeat消息，检测是否和网关设备连通，不连通则重新连接

## 2.超时重传: 
发送失败后自动重试，最多 3 次，间隔 3 秒

## 3.发送失败，离线缓存
消息自动缓存到 /var/cache/，防止数据丢失


## 技术栈:
1.Qt GUI  QWidget
2.Qt多媒体  Qsound
3.Qt网络  Qtcpsocket网络通信 
4.Qt虚拟键盘  Virtual Keyboard
5.Qt定时器  QTimer