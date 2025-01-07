#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/slab.h>

#define SHT31_ADDR 0x44
#define CMD_MEASURE_HIGHREP 0x2400


struct sht31_data {
    struct i2c_client *client;
    int temperature;
    int humidity;
};


static int sht31_read_sensor_data(struct i2c_client *client, int *temperature, int *humidity) {
    int ret;
    u8 command[2];
    u8 buffer[6];

    // Send measurement command
    command[0] = CMD_MEASURE_HIGHREP >> 8;
    command[1] = CMD_MEASURE_HIGHREP & 0xFF;

    ret = i2c_master_send(client, command, 2);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to send measurement command\n");
        return ret;
    }

    // Wait for measurement to complete
    msleep(20);


    // Read sensor data
    ret = i2c_master_recv(client, buffer, 6);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read sensor data\n");
        return ret;
    }


    // Extract temperature and humidity
    *temperature = ((buffer[0] << 8) | buffer[1]);
    *humidity = ((buffer[3] << 8) | buffer[4]);

    return 0;
}



// Sysfs attribute to expose temperature
static ssize_t temperature_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct i2c_client *client = to_i2c_client(dev);
    struct sht31_data *data = i2c_get_clientdata(client);
    int temperature, humidity;


    if (sht31_read_sensor_data(client, &temperature, &humidity) == 0) {
        data->temperature = -4500 + 17500 * temperature / 65535;
        return sprintf(buf, "%d.%02d\n", data->temperature / 100, data->temperature % 100);
    }

    return sprintf(buf, "Error reading temperature\n");
}



// Sysfs attribute to expose humidity
static ssize_t humidity_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct i2c_client *client = to_i2c_client(dev);
    struct sht31_data *data = i2c_get_clientdata(client);
    int temperature, humidity;

    if (sht31_read_sensor_data(client, &temperature, &humidity) == 0) {
        data->humidity = 10000 * humidity / 65535;
        return sprintf(buf, "%d.%02d\n", data->humidity / 100, data->humidity % 100);
    }

    return sprintf(buf, "Error reading humidity\n");
}


static DEVICE_ATTR_RO(temperature);
static DEVICE_ATTR_RO(humidity);

static struct attribute *sht31_attrs[] = {
    &dev_attr_temperature.attr,
    &dev_attr_humidity.attr,
    NULL,
};


static const struct attribute_group sht31_attr_group = {
    .attrs = sht31_attrs,
};


static int sht31_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    struct sht31_data *data;
    int ret;

    // Allocate driver data
    data = devm_kzalloc(&client->dev, sizeof(struct sht31_data), GFP_KERNEL);

    if (!data)
        return -ENOMEM;

    data->client = client;

    i2c_set_clientdata(client, data);


    // Create sysfs entries
    ret = sysfs_create_group(&client->dev.kobj, &sht31_attr_group);

    if (ret) {
        dev_err(&client->dev, "Failed to create sysfs group\n");
        return ret;
    }

    dev_info(&client->dev, "SHT31 sensor driver probed\n");
    return 0;

}



static int sht31_remove(struct i2c_client *client) {
    sysfs_remove_group(&client->dev.kobj, &sht31_attr_group);
    dev_info(&client->dev, "SHT31 sensor driver removed\n");

    return 0;
}



static const struct i2c_device_id sht31_id[] = {
    { "sht31", 0 },
    { }
};


MODULE_DEVICE_TABLE(i2c, sht31_id);


static struct i2c_driver sht31_driver = {
    .driver = {
        .name = "sht31",
    },
    .probe = sht31_probe,
    .remove = sht31_remove,
    .id_table = sht31_id,
};



module_i2c_driver(sht31_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("SHT31 I2C Driver");
MODULE_VERSION("1.0");