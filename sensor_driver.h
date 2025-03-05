#pragma once
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/delay.h> 
#include <linux/string.h>

#define DEVICE_NAME "sht31_driver"
#define COMMAND_LENGTH 2
#define READ_LENGTH 6

enum sensor_data_types 
{
    GET_BOTH,
	GET_TEMPERATURE,
    GET_HUMIDITY,
};

enum command_codes 
{
    STOP_MEASUREMENT = 0x3093,
    START_MEASURMENT = 0x2737,
    SOFT_RESET = 0x30a2,
    REQUEST_DATA = 0xe000,
};
