# =============================================================================
# Makefile  ——  交叉编译至 imx6ull
# =============================================================================

# 交叉编译工具链路径
TOOLCHAIN_DIR = /usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf
CROSS_COMPILE = $(TOOLCHAIN_DIR)/bin/arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc

# Buildroot 的 staging 目录
BUILDROOT_DIR = /home/doge/buildroot2021/buildroot-2021.02.x
SYSROOT = $(BUILDROOT_DIR)/output/staging

CFLAGS  = -I$(SYSROOT)/usr/include -Wall -O2 -std=c11
LDFLAGS = -L$(SYSROOT)/usr/lib -lpthread -lsqlite3 -lmongoose -lssl -lcrypto -lm

TARGET = gateway
OBJS = main.o server.o database.o cJSON.o http_mongoose.o http_api.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) --sysroot=$(SYSROOT) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean