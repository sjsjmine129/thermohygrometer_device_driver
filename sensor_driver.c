#include "sensor_driver.h"

static dev_t device_dev;
static struct class *device_class;
static struct cdev device_cdev;

uint8_t i2c_data_buffer[6] = {0};
enum sensor_data_types sensor_data_type = GET_BOTH;
int timeout_second = 1;

static struct file_operations fops = 
{
    .owner = THIS_MODULE,
    .open = sensor_driver_open,
    .release = sensor_driver_release,
    .read = sensor_driver_read,
    .unlocked_ioctl = sensor_driver_ioctl,
};

static const struct i2c_device_id sensor_id[] = 
{
    {"sht31", 0},
    {}
};

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

// set sensor data_type
int set_sensor_data_type(enum sensor_data_types new_data_type)
{
    if(!(new_data_type == GET_BOTH || new_data_type == GET_TEMPERATURE || new_data_type == GET_HUMIDITY))
    {
        printk(KERN_ALERT "wrong datatype code!\n");
        return DRIVER_FAIL;
    }
    sensor_data_type = new_data_type;
    return DRIVER_SUCCUSS;
}

// set timeout_second
int set_measure_time(int new_time)
{
    if(new_time <= 0 || new_time > 60 )
    {
        printk(KERN_ALERT "wrong measure time gap\n");
        return DRIVER_FAIL;
    }

    timeout_second = new_time;
    return DRIVER_SUCCUSS;
}


int send_command_to_sensor(uint16_t command_code, struct i2c_client *client)
{
    //set command
    i2c_data_buffer[0] = (uint8_t)((command_code & 0xFF00) >> 8);
    i2c_data_buffer[1] = (uint8_t)((command_code & 0x00FF) >> 0); 
    
    //send data
    if(i2c_master_send(client, i2c_data_buffer, COMMAND_LENGTH) != COMMAND_LENGTH)
    {
        printk(KERN_ALERT "fail send %x\n", command_code);
        return DRIVER_FAIL;
    }

    return DRIVER_SUCCUSS;
}

static void extract_temp_humid_data(int32_t* temperature, int32_t* humidity)
{
    //remove checksum
    int i = 0, j = 0;
    for (; i < READ_LENGTH; i += 2 + 1) 
    {
        i2c_data_buffer[j++] = i2c_data_buffer[i];
        i2c_data_buffer[j++] = i2c_data_buffer[i + 1];
    }

    // calcualte data
    if(temperature)
    {
        *temperature = ((21875 * (int32_t)((uint16_t)i2c_data_buffer[0] << 8 | (uint16_t)i2c_data_buffer[1])) >> 13) - 45000;
    }

    if(humidity)
    {
        *humidity =  ((12500 * (int32_t)((uint16_t)i2c_data_buffer[2] << 8 | (uint16_t)i2c_data_buffer[3])) >> 13);
    }
}

int get_data_from_sensor(char *tmp_buf, size_t count, struct i2c_client *client)
{
    ktime_t start_time = ktime_get();
    ktime_t timeout_time = ktime_add(start_time, ktime_set(timeout_second, 0));
    ktime_t current_time = NULL;
    int ret = -1;

    int32_t temp_sum = 0;
    int32_t humid_sum = 0;
    int temp_num = 0;
    int humid_num = 0;

    do // read data untile time is over
    {
        ret = i2c_master_recv(client, i2c_data_buffer, READ_LENGTH);
        if(ret != READ_LENGTH) // new data is not arrived
        {
            current_time = ktime_get();
            continue; 
        }
        
        int32_t new_temperature, new_humidity;
        switch (sensor_data_type) 
        {
            case GET_BOTH:
                extract_temp_humid_data(&new_temperature, &new_humidity);
                printk(KERN_ALERT "[read] temperature: %d, humid: %d\n", new_temperature, new_humidity);
                temp_sum += new_temperature;
                humid_sum += new_humidity;
                temp_num++;
                humid_num++;
                break;
    
            case GET_TEMPERATURE:
                extract_temp_humid_data(&new_temperature, NULL);
                printk(KERN_ALERT "[read] temperature: %d\n", new_temperature);
                temp_sum += new_temperature;
                temp_num++;
                break;
    
            case GET_HUMIDITY:
                extract_temp_humid_data(NULL, &new_humidity);
                printk(KERN_ALERT "[read] humid: %d\n", new_humidity);
                humid_sum += new_humidity;
                humid_num++;
                break;
    
            default:
                break;
        }

        current_time = ktime_get();
    } while(ktime_compare(current_time, timeout_time) < 0);

    // calculate data average and make text
    switch (sensor_data_type) 
    {
        case GET_BOTH:
            temp_sum = temp_sum / temp_num;
            humid_sum = humid_sum / humid_num;

            ret = snprintf(tmp_buf, count,"%d.%03d C\n%d.%03dRH%%", temp_sum/1000, temp_sum%1000, humid_sum/1000, humid_sum%1000);
            tmp_buf[6] = 0xDF; // add '°'  
            return ret;

        case GET_TEMPERATURE:
            temp_sum = temp_sum / temp_num;

            ret = snprintf(tmp_buf, count, "%d.%03d C", temp_sum/1000, temp_sum%1000);
            tmp_buf[6] = 0xDF; // add '°' 
            return ret;

        case GET_HUMIDITY:
            humid_sum = humid_sum / humid_num;

            ret = snprintf(tmp_buf, count, "%d.%03dRH%%", humid_sum/1000, humid_sum%1000);
            return ret;

        default:
            return DRIVER_FAIL;
    }
}


static int sensor_driver_open(struct inode *inode, struct file *file)
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
    snprintf(client->name, I2C_NAME_SIZE, "i2c-dev %d",adap->nr);

    client->adapter = adap;
    file->private_data = client;
    client->addr = 0x44; //set sensor i2c address

    set_sensor_data_type(GET_BOTH); 
    set_measure_time(1);

    send_command_to_sensor(STOP_MEASUREMENT, client);
    usleep_range(1000, 1000);
    send_command_to_sensor(SOFT_RESET, client);
    usleep_range(100000, 101000);
    send_command_to_sensor(START_MEASURMENT, client);
    usleep_range(7000, 8000);

	printk(KERN_ALERT "Open sht31 device! %u: i2c-dev %d\n",minor, adap->nr);
	return DRIVER_SUCCUSS;
}

// release function for device driver
static int sensor_driver_release(struct inode *inode, struct file *file)
{
    struct i2c_client *client = file->private_data;

    send_command_to_sensor(STOP_MEASUREMENT, client);

    i2c_put_adapter(client->adapter);
    kfree(client);
    file->private_data = NULL;

	printk(KERN_ALERT "Release sht31 Device\n");
	return DRIVER_SUCCUSS;
}


static ssize_t sensor_driver_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    int ret = -1;

    if(send_command_to_sensor(REQUEST_DATA, file->private_data) != DRIVER_SUCCUSS)
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

    ret = get_data_from_sensor(tmp, count, file->private_data);

    //check count and send len
    if(ret > count || ret < 0)
    {
        printk(KERN_INFO "Read fail\n");
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
    int ret = -1;

    switch (cmd) 
    {
        case 0x0010: //sensor mode
            ret = set_sensor_data_type(arg); 
            break;

        case 0x0011: // sensing time gap
            ret = set_measure_time(arg);
            break;

        default:
            printk(KERN_ALERT "cmd is not correct\n");
            ret -ENXIO;
    }

    return ret;
}

static int sensor_probe(struct i2c_client *client)
{
    printk(KERN_INFO "SHT31 sensor detected\n");
    return DRIVER_SUCCUSS;
}

static void sensor_remove(struct i2c_client *client)
{
    printk(KERN_INFO "SHT31 sensor removed\n");
    return;
}


MODULE_DEVICE_TABLE(i2c, sensor_id);


static int __init sensor_driver_init(void)
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
    return DRIVER_SUCCUSS;
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
