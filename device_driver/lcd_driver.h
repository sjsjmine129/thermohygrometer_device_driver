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

#define DEVICE_NAME "lcd1602_driver"
#define SEND_LENGTH 1
#define LCD_CHR  1
#define LCD_CMD  4 
#define LCD_BACKLIGHT 8 
#define ENABLE 4

enum command_codes
{
    SET_CURSOR_MOVERIGHT = 0x06,
    SET_DEFALUT_THEME= 0x0C, // display on, cursor off, blink off
    SET_2LINE_MODE= 0x28,
    SET_4BIT_MODE = 0x32,
    SET_8BIT_MODE = 0x33,
    CLEAR_SCREEN = 0x01,
    MOVE_CURSOR_1LINE = 0x80,
    MOVE_CURSOR_2LINE = 0xC0,
};

static int lcd_driver_open(struct inode *inode, struct file *file);

static int lcd_driver_release(struct inode *inode, struct file *file);

static ssize_t lcd_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static int lcd_probe(struct i2c_client *client);

static void lcd_remove(struct i2c_client *client);