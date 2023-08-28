KERNELDIR := /usr/src/linux-headers-5.4.0-148-generic
CURRENT_PATH := $(shell pwd)

obj-m := binder_pro.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean