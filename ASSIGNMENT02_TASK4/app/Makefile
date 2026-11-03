ARM_CC ?= arm-linux-gnueabihf-gcc
HOST_CC ?= gcc
CFLAGS = -std=gnu11 -O2 -g -Wall -Wextra -Werror -pthread -D_GNU_SOURCE
LDLIBS = -pthread -lrt -lm
SOURCES = sms_app.c sensor_thread.c processor_thread.c alert_thread.c logger_thread.c circular_buffer.c
.PHONY: all arm host clean
all: arm host
arm:
	$(ARM_CC) $(CFLAGS) -static -o sms_app $(SOURCES) $(LDLIBS)
host:
	$(HOST_CC) $(CFLAGS) -o sms_app_x86 $(SOURCES) $(LDLIBS)
clean:
	rm -f sms_app sms_app_x86
