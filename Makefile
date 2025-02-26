# Makefile

# called by kernel
ifneq ($(KERNELRELEASE),)
	obj-m := module_sensor_driver.o module_lcd_driver.o
	
	module_sensor_driver-objs := sensor_driver.o sensor_operations.o
	module_lcd_driver-objs := lcd_driver.o lcd_operations.o

# called by make
else
    KERNELRELEASE ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

all:
	@echo "kernel release: $(KERNELRELEASE)"
	$(MAKE) -C $(KERNELRELEASE) M=$(PWD) modules
	# rm -f *.o *.mod *.symvers *.order *.mod.c *o.cmd *.mod.* .*.cmd

clean:
	$(MAKE) -C $(KERNELRELEASE) M=$(PWD) clean

endif