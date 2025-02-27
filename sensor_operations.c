#include "sensor_operations.h"

#define COMMAND_LENGTH 2
#define READ_LENGTH 6
#define MEASURE_TIME_GAP 105000

uint8_t i2c_data_buffer[6] = {0};
struct i2c_client *client;
enum sensor_modes sensor_mode = GET_BOTH;


int init_client(struct i2c_client *init_client)
{
    client = init_client;
    return 0;
}

// set sensor mode
int set_sensor_mode(enum sensor_modes mode)
{
    if(!(mode == GET_BOTH || mode == GET_TEMPERATURE || mode == GET_HUMIDITY))
    {
        printk(KERN_ALERT "wrong mode code!\n");
        return -1;
    }
    sensor_mode = mode;
    return 0;
}


int send_command_to_sensor(uint16_t command_code)
{
    //set command
    i2c_data_buffer[0] = (uint8_t)((command_code & 0xFF00) >> 8);
    i2c_data_buffer[1] = (uint8_t)((command_code & 0x00FF) >> 0); 
    
    //send data
    if(i2c_master_send(client, i2c_data_buffer, COMMAND_LENGTH) != COMMAND_LENGTH)
    {
        printk(KERN_ALERT "fail send %x\n", command_code);
        return -1;
    }

    return 0;
}

static void extract_temp_humid_data(int32_t* temperature, int32_t* temperature_decimal, int32_t* humidity, int32_t* humidity_decimal)
{
    int32_t tmp;
    if(temperature && temperature_decimal)
    {
        tmp = ((21875 * (int32_t)((uint16_t)i2c_data_buffer[0] << 8 | (uint16_t)i2c_data_buffer[1])) >> 13) - 45000;
        *temperature_decimal = tmp % 1000;
        *temperature = tmp / 1000;
    }

    if(humidity && humidity_decimal)
    {
        tmp =  ((12500 * (int32_t)((uint16_t)i2c_data_buffer[2] << 8 | (uint16_t)i2c_data_buffer[3])) >> 13);
        *humidity_decimal = tmp % 1000;
        *humidity = tmp / 1000;
    }
}

int get_data_from_sensor(char *tmp_buf, size_t count)
{

    int ret = i2c_master_recv(client, i2c_data_buffer, READ_LENGTH);
    if(ret != READ_LENGTH)
    {
        printk(KERN_ALERT "fail read: ret %d\n", ret);
        return -1;
    }

    //remove checksum
    int i, j;
    for (i = 0, j = 0; i < ret; i += 2 + 1) 
    {
        i2c_data_buffer[j++] = i2c_data_buffer[i];
        i2c_data_buffer[j++] = i2c_data_buffer[i + 1];
    }

    int32_t temperature, temperature_decimal, humidity, humidity_decimal;
    switch (sensor_mode) 
    {
        case GET_BOTH:
            extract_temp_humid_data(&temperature, &temperature_decimal, &humidity, &humidity_decimal);
            ret = snprintf(tmp_buf, count,"%d.%03d C\n%d.%03dRH%%", temperature, temperature_decimal, humidity, humidity_decimal);
            tmp_buf[6] = 0xDF;
            return ret;

        case GET_TEMPERATURE:
            extract_temp_humid_data(&temperature, &temperature_decimal, NULL, NULL);
            ret = snprintf(tmp_buf, count, "%d.%03d C", temperature, temperature_decimal);
            tmp_buf[6] = 0xDF;
            return ret;

        case GET_HUMIDITY:
            extract_temp_humid_data(NULL, NULL, &humidity, &humidity_decimal);
            ret = snprintf(tmp_buf, count, "%d.%03dRH%%", humidity, humidity_decimal);
            return ret;

        default:
            return -1;
    }
}

MODULE_AUTHOR("seungju.eum");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Character device driver for SHT31 sensor with I2C support");