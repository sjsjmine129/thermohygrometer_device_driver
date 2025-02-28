#include "lcd_operations.h"

#define DEVICE_NAME "lcd1602_driver"

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

static dev_t device_dev;
static struct class *device_class;
static struct cdev device_cdev;


int lcd_driver_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct i2c_client *client;
	struct i2c_adapter *adap;

	adap = i2c_get_adapter(minor);
    if(!adap)
    {
        printk(KERN_ALERT "No adapter %u\n",minor);
        return -ENODEV;
    }

    client = kzalloc(sizeof(*client), GFP_KERNEL);
    if(!client)
    {
        i2c_put_adapter(adap);
        return -ENOMEM;
    }
    snprintf(client->name, I2C_NAME_SIZE, "i2c-dev %d", adap->nr);


    client->adapter = adap;
    file->private_data = client;
    client->addr = 0x27; //set lcd i2c address

    init_client(client);

    send_command_to_lcd(SET_8BIT_MODE);
    send_command_to_lcd(SET_4BIT_MODE);
    send_command_to_lcd(SET_CURSOR_MOVERIGHT);
    send_command_to_lcd(SET_DEFALUT_THEME);
    send_command_to_lcd(SET_2LINE_MODE);
    send_command_to_lcd(CLEAR_SCREEN);
    usleep_range(2000, 2100);

	printk(KERN_ALERT "Open lcd device %u: i2c-dev %d\n",minor, adap->nr);
	return 0;
}


// release function for device driver
int lcd_driver_release(struct inode *inode, struct file *file)
{
   struct i2c_client *client = file->private_data;

    send_command_to_lcd(CLEAR_SCREEN);

    i2c_put_adapter(client->adapter);
    kfree(client);
    file->private_data = NULL;

	printk(KERN_ALERT "Release lcd device\n");
	return 0;
}

static ssize_t lcd_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0;
	char *str;

	str = memdup_user(buf, count);
    if(str == NULL)
    {
        printk(KERN_INFO "write fail: memory error\n");
        return -ENOMEM;
    }
    printk(KERN_ALERT "data from user: %s\n",str);

    send_command_to_lcd(CLEAR_SCREEN);

    //seperate data to line
    const char *newline_pos = strchr(str, '\n');

    if(newline_pos != NULL) //two line
    { 
        //write first line
        int len = (int)(newline_pos - str);
        send_command_to_lcd(MOVE_CURSOR_1LINE); //set curcor first line
        ret += write_text_to_lcd(str, len); //error?

        //write second line
        len = strlen(newline_pos +1);
        send_command_to_lcd(MOVE_CURSOR_2LINE); //set curcor second line
        ret += write_text_to_lcd(newline_pos + 1, len);
        ret += 1; // "\n"
    }
    else  //oneline
    {
        send_command_to_lcd(MOVE_CURSOR_1LINE); //set curcor first line
        ret = write_text_to_lcd(str, count - 1);
    }

	kfree(str);
	return ret;
}



static struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = lcd_driver_open,
    .release = lcd_driver_release,
    .write = lcd_driver_write
};


static int lcd_probe(struct i2c_client *client)
{
    printk(KERN_INFO "LCD 1602 detected\n");
    return 0;
}

static void lcd_remove(struct i2c_client *client)
{
    printk(KERN_INFO "LCD 1602 removed\n");
    return;
}

static const struct i2c_device_id lcd_id[] = 
{
    {"lcd1602", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, lcd_id);

static struct i2c_driver lcd_driver = 
{
    .driver = 
    {
        .name = "lcd1602_driver",
    },
    .probe = lcd_probe,
    .remove = lcd_remove,
    .id_table = lcd_id,
};


static int __init lcd_driver_init(void)
{
    int ret;
    
    // Register character device
    if ((ret = alloc_chrdev_region(&device_dev, 1, 1, DEVICE_NAME)) < 0) 
    {
        printk(KERN_ALERT "alloc_chrdev_region failed\n");
        return ret;
    }

    cdev_init(&device_cdev, &fops);
    device_cdev.owner = THIS_MODULE;

    if ((ret = cdev_add(&device_cdev, device_dev, 1)) < 0) 
    {
        unregister_chrdev_region(device_dev, 1);
        return ret;
    }

    if ((device_class = class_create(DEVICE_NAME)) == NULL) 
    {
        cdev_del(&device_cdev);
        unregister_chrdev_region(device_dev, 1);
        return -1;
    }

    if (device_create(device_class, NULL, device_dev, NULL, DEVICE_NAME) == NULL) 
    {
        class_destroy(device_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(device_dev, 1);
        return -1;
    }

    // Register I2C driver
    if ((ret = i2c_add_driver(&lcd_driver) < 0)) 
    {
        device_destroy(device_class, device_dev);
        class_destroy(device_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(device_dev, 1);
        printk(KERN_INFO "No i2c driver");
        return ret;
    }

    printk(KERN_INFO "Successfully Load LCD Device Driver: Major = %d, Minor = %d\n", MAJOR(device_dev), MINOR(device_dev));
    return 0;
}


static void __exit lcd_driver_exit(void)
{
    i2c_del_driver(&lcd_driver);
    device_destroy(device_class, device_dev);
    class_destroy(device_class);
    cdev_del(&device_cdev);
    unregister_chrdev_region(device_dev, 1);
    printk(KERN_ALERT "Unload LCD device driver\n");
}

module_init(lcd_driver_init);
module_exit(lcd_driver_exit);

MODULE_AUTHOR("seungju.eum");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Character device driver for lcd with I2C support");