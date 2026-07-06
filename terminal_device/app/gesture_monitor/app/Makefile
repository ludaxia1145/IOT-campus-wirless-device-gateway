CROSS_COMPILE := arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc

CFLAGS := -static -Wall -O2
LDFLAGS := -lm

TARGET := gesture_monitor
SRC := gesture_monitor.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
