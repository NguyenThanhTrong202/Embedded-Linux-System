ARM_CC ?= arm-linux-gnueabihf-gcc
CFLAGS = -std=gnu11 -O2 -g -Wall -Wextra -Werror
.PHONY: arm clean
arm:
	$(ARM_CC) $(CFLAGS) -static -o test_sms_ioctl test_sms_ioctl.c
clean:
	rm -f test_sms_ioctl
