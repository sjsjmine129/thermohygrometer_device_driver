#include "sensor_operations.h"

#define COMMAND_LENGTH 2
#define READ_LENGTH 6
#define MEASURE_TIME_GAP 105000
#define WINDOW_SIZE 5
#define TIMEOUT_SECOND 1

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

static void extract_temp_humid_data(int32_t* temperature, int32_t* humidity)
{
    if(temperature)
    {
        *temperature = ((21875 * (int32_t)((uint16_t)i2c_data_buffer[0] << 8 | (uint16_t)i2c_data_buffer[1])) >> 13) - 45000;
    }

    if(humidity)
    {
        *humidity =  ((12500 * (int32_t)((uint16_t)i2c_data_buffer[2] << 8 | (uint16_t)i2c_data_buffer[3])) >> 13);
    }
}

int get_data_from_sensor(char *tmp_buf, size_t count)
{
    ktime_t start_time = ktime_get();
    ktime_t timeout_time = ktime_add(start_time, ktime_set(TIMEOUT_SECOND, 0));
    ktime_t current_time;
    int ret;

    int32_t *temp_datas;
    int32_t *humid_datas;
    int temp_num = 0;
    int humid_num = 0;

    switch (sensor_mode) 
    {
        case GET_BOTH:
            temp_datas = kmalloc(sizeof(int32_t)*11*TIMEOUT_SECOND, GFP_KERNEL);
            if(!temp_datas)
            {
                printk(KERN_ALERT "memory error\n");
                return -ENOMEM;
            }
            humid_datas = kmalloc(sizeof(int32_t)*11*TIMEOUT_SECOND, GFP_KERNEL);
            if(!humid_datas)
            {
                kfree(temp_datas);
                printk(KERN_ALERT "memory error\n");
                return -ENOMEM;
            }
            break;

        case GET_TEMPERATURE:
            temp_datas = kmalloc(sizeof(int32_t)*11*TIMEOUT_SECOND, GFP_KERNEL);
            if(!temp_datas)
            {
                printk(KERN_ALERT "memory error\n");
                return -ENOMEM;
            }
            break;

        case GET_HUMIDITY:
            humid_datas = kmalloc(sizeof(int32_t)*11*TIMEOUT_SECOND, GFP_KERNEL);
            if(!humid_datas)
            {
                printk(KERN_ALERT "memory error\n");
                return -ENOMEM;
            }
            break;

        default:
            break;
    }


    do // read data untile time is over
    {
        ret = i2c_master_recv(client, i2c_data_buffer, READ_LENGTH);
        if(ret != READ_LENGTH) // new data is not arrived
        {
            current_time = ktime_get();
            continue;
        }

        //remove checksum
        int i, j;
        for (i = 0, j = 0; i < ret; i += 2 + 1) 
        {
            i2c_data_buffer[j++] = i2c_data_buffer[i];
            i2c_data_buffer[j++] = i2c_data_buffer[i + 1];
        }
        
        int32_t new_temperature, new_humidity;
        switch (sensor_mode) 
        {
            case GET_BOTH:
                extract_temp_humid_data(&new_temperature, &new_humidity);
                printk(KERN_ALERT "[read] temperature: %d, humid: %d\n", new_temperature, new_humidity);
                temp_datas[temp_num++] = new_temperature;
                humid_datas[humid_num++] = new_humidity;
                break;
    
            case GET_TEMPERATURE:
                extract_temp_humid_data(&new_temperature, NULL);
                printk(KERN_ALERT "[read] temperature: %d\n", new_temperature);
                temp_datas[temp_num++] = new_temperature;
                break;
    
            case GET_HUMIDITY:
                extract_temp_humid_data(NULL, &new_humidity);
                printk(KERN_ALERT "[read] humid: %d\n", new_humidity);
                humid_datas[humid_num++] = new_humidity;
                break;
    
            default:
                break;
        }
    
        current_time = ktime_get();


    } while(ktime_compare(current_time, timeout_time) < 0);



    // cal valid data average and make text
    switch (sensor_mode) 
    {
        case GET_BOTH:

            ret = snprintf(tmp_buf, count,"%d.%03d C\n%d.%03dRH%%", temperature, temperature_decimal, humidity, humidity_decimal);
            tmp_buf[6] = 0xDF;
            return ret;

        case GET_TEMPERATURE:

            ret = snprintf(tmp_buf, count, "%d.%03d C", temperature, temperature_decimal);
            tmp_buf[6] = 0xDF;
            return ret;

        case GET_HUMIDITY:

            ret = snprintf(tmp_buf, count, "%d.%03dRH%%", humidity, humidity_decimal);
            return ret;

        default:
            return -1;
    }


    if(temp_datas)
    {
        kfree(temp_datas);
    }
    if(humid_datas)
    {
        kfree(humid_datas);
    }

    return count;
}

MODULE_AUTHOR("seungju.eum");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Character device driver for SHT31 sensor with I2C support");