# Makefile

# Specify the GCC compiler 
# CC := x86_64-linux-gnu-gcc-13

# called by kernel
ifneq ($(KERNELRELEASE),)
# obj-m := hello.o
# obj-m := alloc_test.o
	obj-m := example.o

# called by make
else
    KERNELRELEASE ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

all:
#	@echo "kernel release: $(KERNELRELEASE)"
	$(MAKE) -C $(KERNELRELEASE) M=$(PWD) modules
# $(MAKE) -C $(KERNELRELEASE) M=$(PWD) CC=$(CC) modules
clean:
	$(MAKE) -C $(KERNELRELEASE) M=$(PWD) clean
#$(MAKE) -C $(KERNELRELEASE) M=$(PWD) CC=$(CC) clean
endif