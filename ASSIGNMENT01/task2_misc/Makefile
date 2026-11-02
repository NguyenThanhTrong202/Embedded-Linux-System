KDIR := $(HOME)/Embedded-Linux-System/LAB01/sources/linux-5.15
KBUILD_OUTPUT := $(HOME)/Embedded-Linux-System/LAB01/build/kernel

ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

obj-m += counter_driver.o

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) O=$(KBUILD_OUTPUT) \
		M=$(CURDIR) ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		KCFLAGS="-march=armv7-a" modules

clean:
	$(MAKE) -C $(KDIR) O=$(KBUILD_OUTPUT) \
		M=$(CURDIR) ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) clean
