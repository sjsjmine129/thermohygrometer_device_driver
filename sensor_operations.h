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

enum sensor_data_types 
{
    GET_BOTH,
	GET_TEMPERATURE,
    GET_HUMIDITY,
};


int init_client(struct i2c_client *init_client);

int set_sensor_data_type(enum sensor_data_types mode);

int set_measure_time(int new_time);

int send_command_to_sensor(uint16_t command_code);

int get_data_from_sensor(char *tmp_buf, size_t count);
