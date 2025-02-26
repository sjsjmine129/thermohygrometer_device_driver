#include "sensor_operations.h"

#define SHT31_I2C_ADDRESS 0x44
#define DEVICE_NAME "sht31_driver"

enum command_codes 
{
    STOP_MEASUREMENT = 0x3093,
    START_MEASURMENT = 0x2126,
    SOFT_RESET = 0x30a2,
    REQUEST_DATA = 0xe000,
};

static dev_t device_dev;
static struct class *device_class;
static struct cdev device_cdev;

int checker =0;

int sensor_driver_open(struct inode *inode, struct file *file)
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
    snprintf(client->name, I2C_NAME_SIZE, "i2c-dev %d",adap->nr);


    client->adapter = adap;
    file->private_data = client;
    client->addr = SHT31_I2C_ADDRESS;

    init_client(client);

    send_command_to_sensor(STOP_MEASUREMENT);
    usleep_range(1000, 1000);
    send_command_to_sensor(SOFT_RESET);
    usleep_range(100000, 101000);
    send_command_to_sensor(START_MEASURMENT);
    usleep_range(7000, 8000);

	printk(KERN_ALERT "Open sht31 device! %u: i2c-dev %d\n",minor, adap->nr);
	return 0;
}

// release function for device driver
int sensor_driver_release(struct inode *inode, struct file *file)
{
    struct i2c_client *client = file->private_data;

    set_sensor_mode(GET_BOTH);
    send_command_to_sensor(STOP_MEASUREMENT);

    i2c_put_adapter(client->adapter);
    kfree(client);
    file->private_data = NULL;

	printk(KERN_ALERT "Release sht31 Device\n");
	return 0;
}


static ssize_t sensor_driver_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{ checker++;
        if(checker%3==0){

    return -1;
}
    int ret;

    if(send_command_to_sensor(REQUEST_DATA) != 0)
    { 
        printk(KERN_INFO "Read fail\n");
        return -EIO;
    }

    char *tmp;
    tmp = kzalloc(count, GFP_KERNEL);
    if(tmp == NULL)
    {
        printk(KERN_INFO "Read fail\n");
        return -ENOMEM;
    }

    ret = get_data_from_sensor(tmp, count);

    //check count and send len
    if(ret > count)
    {
        printk(KERN_INFO "Read fail: buffer is too small\n");
        kfree(tmp);
        return -EIO;
    }
    printk(KERN_ALERT "success read:%s\n", tmp);

    if(copy_to_user(buf, tmp, ret))
    {
        printk(KERN_INFO "Copy to user fail\n");
        ret = -EFAULT;
    }

    kfree(tmp);
    return ret;
}


static long sensor_driver_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if(cmd != SHT31_I2C_ADDRESS)
    {
        printk(KERN_ALERT "cmd is not correct sensor address\n");
        return -ENXIO;
    }

    int ret = set_sensor_mode(arg);

    return ret;
}



static struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = sensor_driver_open,
    .release = sensor_driver_release,
    .read = sensor_driver_read,
    .unlocked_ioctl = sensor_driver_ioctl,
};



static int sensor_probe(struct i2c_client *client)
{
    printk(KERN_INFO "SHT31 sensor detected\n");
    return 0;
}

static void sensor_remove(struct i2c_client *client)
{
    printk(KERN_INFO "SHT31 sensor removed\n");
    return;
}

static const struct i2c_device_id sensor_id[] = 
{
    {"sht31", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = 
{
    .driver = 
    {
        .name = "sht31_driver",
    },
    .probe = sensor_probe,
    .remove = sensor_remove,
    .id_table = sensor_id,
};


static int __init sensor_driver_init(void)
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
    if ((ret = i2c_add_driver(&sensor_driver) < 0)) 
    {
        device_destroy(device_class, device_dev);
        class_destroy(device_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(device_dev, 1);
        printk(KERN_INFO "No i2c driver");
        return ret;
    }

    printk(KERN_INFO "Successfully Load SHT31 Device Driver: Major = %d, Minor = %d\n", MAJOR(device_dev), MINOR(device_dev));
    return 0;
}


static void __exit sensor_driver_exit(void)
{
    i2c_del_driver(&sensor_driver);
    device_destroy(device_class, device_dev);
    class_destroy(device_class);
    cdev_del(&device_cdev);
    unregister_chrdev_region(device_dev, 1);
    printk(KERN_ALERT "Unload SHT31 sensor Driver\n");
}

module_init(sensor_driver_init);
module_exit(sensor_driver_exit);

MODULE_AUTHOR("seungju.eum");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Character device driver for SHT31 sensor with I2C support");