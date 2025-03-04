# Makefile

# called by kernel
ifneq ($(KERNELRELEASE),)
	obj-m := sensor_driver.o lcd_driver.o

# called by make
else
    KERNELRELEASE ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

all:
	@echo "kernel release: $(KERNELRELEASE)"
	$(MAKE) -C $(KERNELRELEASE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELRELEASE) M=$(PWD) clean

endif