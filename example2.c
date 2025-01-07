#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/i2c.h> // I2C subsystem

#define MINOR_BASE 0
#define SHT31_I2C_ADDRESS 0x44 // Default I2C address of the SHT31


static dev_t device_dev;
static struct class *device_class;
static struct cdev device_cdev;


static char *device_name = "SHT31_Driver";
static struct i2c_client *sht31_client;
module_param(device_name, charp, S_IRUGO);

int device_driver_open(struct inode *inode, struct file *file)
{
	/* data to store state data between syscall */
	// file->private_data = kmalloc(1024, GFP_KERNEL); // you have to release this in release function
	
	printk(KERN_ALERT "Open Device!\n");
	return 0;
}

int device_driver_release(struct inode *inode, struct file *file)
{
	printk(KERN_ALERT "Release Device!\n");
	return 0;
}

// Function to read temperature and humidity from SHT31
static int sht31_read_sensor_data(char *buf)
{
    int ret;
    char cmd[2] = {0x2C, 0x06}; // Command to trigger measurement
    char data[6];              // Buffer to store raw sensor data

    // Send measurement command
    ret = i2c_master_send(sht31_client, cmd, 2);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to send I2C command\n");
        return ret;
    }

    // Delay to allow measurement to complete
    msleep(15);

    // Read 6 bytes of data: 2 bytes for temperature, 2 for humidity, 2 for CRC
    ret = i2c_master_recv(sht31_client, data, 6);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to read data from SHT31\n");
        return ret;
    }

    // Process temperature and humidity
    int temp_raw = (data[0] << 8) | data[1];
    int hum_raw = (data[3] << 8) | data[4];

    float temperature = -45 + 175 * (temp_raw / 65535.0);
    float humidity = 100 * (hum_raw / 65535.0);

    // Format and copy data to user buffer
    snprintf(buf, PAGE_SIZE, "Temperature: %.2f °C\nHumidity: %.2f %%\n", temperature, humidity);
    return 0;
}



static ssize_t device_driver_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    char *kbuf;
    int ret;

    kbuf = kzalloc(PAGE_SIZE, GFP_KERNEL);
    if (!kbuf)
    {
       return -ENOMEM;
    }

    ret = sht31_read_sensor_data(kbuf);

    if (ret < 0)
    {
        kfree(kbuf);
        return ret;
    }

    if (copy_to_user(buf, kbuf, strlen(kbuf)))
    {
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    return strlen(kbuf);
}



static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = device_driver_read,
    .open = device_driver_open,
    .release = device_driver_release,
};



static int sht31_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    sht31_client = client;
    printk(KERN_INFO "SHT31 sensor detected\n");
    return 0;
}


static int sht31_remove(struct i2c_client *client)
{
    printk(KERN_INFO "SHT31 sensor removed\n");
    return 0;
}



static const struct i2c_device_id sht31_id[] = {
    {"sht31", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, sht31_id);

static struct i2c_driver sht31_driver = {
    .driver = {
        .name = "sht31_driver",
    },
    .probe = sht31_probe,
    .remove = sht31_remove,
    .id_table = sht31_id,
};


static int __init device_driver_init(void)
{
    int ret;
    printk(KERN_INFO "Load Device Driver: %s\n", device_name);


    // Register character device
    if (alloc_chrdev_region(&device_dev, MINOR_BASE, 1, device_name)) {
        printk(KERN_ALERT "alloc_chrdev_region failed\n");
        return -1;
    }


    cdev_init(&device_cdev, &fops);
    device_cdev.owner = THIS_MODULE;


    if (cdev_add(&device_cdev, device_dev, 1)) {
        unregister_chrdev_region(device_dev, 1);
        return -1;
    }

    if ((device_class = class_create(THIS_MODULE, device_name)) == NULL) {
        cdev_del(&device_cdev);
        unregister_chrdev_region(device_dev, 1);
        return -1;
    }

    if (device_create(device_class, NULL, device_dev, NULL, device_name) == NULL) {
        class_destroy(device_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(device_dev, 1);
        return -1;
    }

    // Register I2C driver
    ret = i2c_add_driver(&sht31_driver);
    if (ret) {
        device_destroy(device_class, device_dev);
        class_destroy(device_class);
        cdev_del(&device_cdev);
        unregister_chrdev_region(device_dev, 1);
    }

    printk(KERN_INFO "Successfully Load Device Driver: Major = %d, Minor = %d\n", MAJOR(device_dev), MINOR(device_dev));
    return ret;
}


static void __exit device_driver_exit(void)
{
    i2c_del_driver(&sht31_driver);
    device_destroy(device_class, device_dev);
    class_destroy(device_class);
    cdev_del(&device_cdev);
    unregister_chrdev_region(device_dev, 1);
    printk(KERN_ALERT "Unload Device Driver\n");
}

module_init(device_driver_init);
module_exit(device_driver_exit);

MODULE_AUTHOR("sj");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Character device driver for SHT31 sensor with I2C support");