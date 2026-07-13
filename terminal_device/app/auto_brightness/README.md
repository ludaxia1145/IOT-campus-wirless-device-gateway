# auto_brightness - AP3216C 自动亮度调节应用程序+驱动程序

## 功能简介
基于环境光传感器 AP3216C 的自动背光亮度调节程序。程序通过 Linux IIO 框架读取 ALS 原始值，经线性映射后动态调整屏幕背光亮度

## 编译方法

### 1.确保存在交叉编译工具链
CROSS_COMPILE = arm-linux-gnueabihf-

### 2.执行make
make

## 自动亮度调节方法 ：线性映射
亮度 = ALS原始值 × 最大亮度等级 / ALS最大原始值


