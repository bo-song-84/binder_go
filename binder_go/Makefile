KERNELDIR := /usr/src/linux-headers-5.4.0-150-generic
CURRENT_PATH := $(shell pwd)

obj-m := binder_go.o
binder_go-objs =  binder_go.o binder_client_action.o binder_combin_test.o \
                                      binder_init.o binder_malloc.o binder_map.o binder_rb_tree.o \
									  binder_server_action.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean