#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h> // data send to user space & from user space
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h> // character device

#define MINOR_BASE 0

static dev_t device_dev;
static struct class *device_class;
static struct cdev device_cdev;

static char *device_name = "SHT31_Driver";
static char *device_record = NULL;
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

static ssize_t device_driver_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	printk(KERN_ALERT "Read Device\n");

	if(!device_record){
		printk(KERN_ALERT "No Record\n");
		return -ENOMEM; // no memory available for allocation
	}
	
	if (copy_to_user(buf, device_record, count))
	{
		printk(KERN_ALERT "Copy to User Failed\n");
		return -EFAULT; 
	}
	printk(KERN_ALERT "Read: %s\n",device_record);

	return min(count, strlen(device_record));
}

static ssize_t device_driver_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	printk(KERN_ALERT "Write Device\n");

	if(device_record != NULL)
	{
		kfree(device_record);
	}

	if((device_record = kmalloc(count, GFP_KERNEL)) == NULL){
		return -ENOMEM;
	}

	if(copy_from_user(device_record, buf, count))
	{
		printk(KERN_ALERT "Copy from User Failed\n");
		return -EFAULT;
	}

	printk(KERN_ALERT "Write: %s\n",device_record);

	return min(count, strlen(device_record));
}


/* 
 * operations for device driver
 */
static struct file_operations fops = {
	.owner = THIS_MODULE, // owner of the module
// 	.llseek = device_driver_llseek,
    .read = device_driver_read,
    .write = device_driver_write,
    .open = device_driver_open,
    .release = device_driver_release
};


static int __init device_driver_init(void)
{
	int ret;
	printk(KERN_INFO "Load Device Driver: %s\n", device_name);

    /* try allocating character device */
	if (alloc_chrdev_region(&device_dev, MINOR_BASE, 1, device_name)) {
		printk(KERN_ALERT "[%s] alloc_chrdev_region failed\n");
		return -1;
	}

    /* init cdev */
	cdev_init(&device_cdev, &fops); // to use cdev in my Device Driver
	device_cdev.owner = THIS_MODULE; // owner of the module 


    /* add cdev */
	if (cdev_add(&device_cdev, device_dev, 1)) {
		printk(KERN_ALERT "[%s] cdev_add failed\n");
        unregister_chrdev_region(device_dev, 1);
        return -1;
	}

    if ((device_class = class_create(THIS_MODULE, device_name)) == NULL) {
		printk(KERN_ALERT "[%s] class_add failed\n");
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



	printk(KERN_INFO "Successfully Load Device Driver: Major = %d, Minor = %d\n", MAJOR(device_dev), MINOR(device_dev));
    return 0;
}


static void __exit device_driver_exit(void)    
{	
	if(device_record != NULL)
	{
		kfree(device_record);
	}

	// Unregister I2C driver
	//i2c_del_driver(&sht31_driver);

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
MODULE_DESCRIPTION("character device driver for SHT31 sensor");