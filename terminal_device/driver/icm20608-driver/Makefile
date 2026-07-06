# 驱动模块编译Makefile
# 适用平台: IMX6ULL

# 指定内核源码路径
KERNEL_DIR := /home/doge/project/IOT_wirlessdevice_and_gateway/linuxkernel

# 指定交叉编译工具链前缀
CROSS_COMPILE := arm-linux-gnueabihf-

# 当前目录
PWD := $(shell pwd)

# 驱动模块名称
obj-m := icm20608.o

# 默认目标: 编译驱动模块
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) modules

# 清理编译产物
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS_COMPILE) clean
