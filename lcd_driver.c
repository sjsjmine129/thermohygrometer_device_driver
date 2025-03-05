#include "lcd_driver.h"

static dev_t device_dev;
static struct class *device_class;
static struct cdev device_cdev;

uint8_t i2c_data_buffer[2] = {0};

// announce send the data or command to lcd
int lcdToggleEnable(int bits, struct i2c_client *client) 
{
    char data = bits | ENABLE; // make enable bit high
    if(i2c_master_send(client, &data, SEND_LENGTH) != 1)
    {
        return DRIVER_FAIL;
    }
    usleep_range(500, 500);

    data = bits & ~ENABLE; // set enable bit low
    if(i2c_master_send(client, &data, SEND_LENGTH) != 1)
    {
        return DRIVER_FAIL;
    }
    usleep_range(500, 500);

    return DRIVER_SUCCUSS;
}

int send_data_to_lcd(uint16_t data, uint16_t data_type, struct i2c_client *client)
{
    //set data
    i2c_data_buffer[0] = data_type | (data & 0xF0) | LCD_BACKLIGHT; 
    i2c_data_buffer[1] = data_type | ((data << 4) & 0xF0) | LCD_BACKLIGHT;
    
    //send bit data twice
    for(int i = 0; i < 2; i++)
    {
        if(i2c_master_send(client, &i2c_data_buffer[i], SEND_LENGTH) != 1)
        {
            return DRIVER_FAIL;
        }

        if(lcdToggleEnable(i2c_data_buffer[i], client) != DRIVER_SUCCUSS)
        {
            return DRIVER_FAIL;
        }
    }

    return DRIVER_SUCCUSS;
}

int send_command_to_lcd(uint16_t command, struct i2c_client *client)
{
    if(send_data_to_lcd(command, LCD_CMD, client) != DRIVER_SUCCUSS)
    {
        printk(KERN_ALERT "fail to send command: %x\n", command);
        return DRIVER_FAIL;
    }

    return DRIVER_SUCCUSS;
}

int write_text_to_lcd(const char *text, int length, struct i2c_client *client) 
{
    int write_len = 0;

    for (int i = 0; i < length && *text; i++) 
    {
        if(send_data_to_lcd(*text++, LCD_CHR, client) != 0)
        {
            printk(KERN_ALERT "fail to write text: %s\n", text);
            return write_len;
        }
        write_len++;
    }

    return write_len;
}

int lcd_driver_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct i2c_client *client = NULL;
	struct i2c_adapter *adap = NULL;

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

    send_command_to_lcd(SET_8BIT_MODE, client);
    send_command_to_lcd(SET_4BIT_MODE, client);
    send_command_to_lcd(SET_CURSOR_MOVERIGHT, client);
    send_command_to_lcd(SET_DEFALUT_THEME, client);
    send_command_to_lcd(SET_2LINE_MODE, client);
    send_command_to_lcd(CLEAR_SCREEN, client);
    usleep_range(2000, 2100);

	printk(KERN_ALERT "Open lcd device %u: i2c-dev %d\n",minor, adap->nr);
	return DRIVER_SUCCUSS;
}

// release function for device driver
int lcd_driver_release(struct inode *inode, struct file *file)
{
   struct i2c_client *client = file->private_data;

    i2c_put_adapter(client->adapter);
    kfree(client);
    file->private_data = NULL;

	printk(KERN_ALERT "Release lcd device\n");
	return DRIVER_SUCCUSS;
}

static ssize_t lcd_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0;
	char *str;
    struct i2c_client *client = file->private_data;

	str = memdup_user(buf, count);
    if(str == NULL)
    {
        printk(KERN_INFO "write fail: memory error\n");
        return -ENOMEM;
    }
    printk(KERN_ALERT "data from user: %s\n",str);

    send_command_to_lcd(CLEAR_SCREEN, client);

    //seperate data to line
    const char *newline_pos = strchr(str, '\n');

    if(newline_pos != NULL) //two line
    { 
        //write first line
        int len = (int)(newline_pos - str);
        send_command_to_lcd(MOVE_CURSOR_1LINE, client); //set curcor first line
        ret += write_text_to_lcd(str, len, client); //error?

        //write second line
        len = strlen(newline_pos +1);
        send_command_to_lcd(MOVE_CURSOR_2LINE, client); //set curcor second line
        ret += write_text_to_lcd(newline_pos + 1, len, client);
        ret += 1; // "\n"
    }
    else  //oneline
    {
        send_command_to_lcd(MOVE_CURSOR_1LINE, client); //set curcor first line
        ret = write_text_to_lcd(str, count - 1, client);
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
    return DRIVER_SUCCUSS;
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
    int ret = -1;
    
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
        return DRIVER_FAIL;
    }

    if (device_create(device_class, NULL, device_dev, NULL, DEVICE_NAME) == NULL) 
    {
        class_destroy(device_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(device_dev, 1);
        return DRIVER_FAIL;
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
    return DRIVER_SUCCUSS;
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
