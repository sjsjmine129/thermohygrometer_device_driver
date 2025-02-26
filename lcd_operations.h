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

int init_client(struct i2c_client *init_client);

int send_command_to_lcd(uint16_t command);

int write_text_to_lcd(const char *text, int length);
