# Makefile for AP3216C driver
obj-m := ap3216c_iio.o

KERNELDIR := /home/doge/project/IOT_wirlessdevice_and_gateway/linuxkernel
PWD := $(shell pwd)

# 关键：指定目标架构和交叉编译器
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

ccflags-y := -fno-pic

all:
	$(MAKE) -C $(KERNELDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) clean

help:
	$(MAKE) -C $(KERNELDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) help
