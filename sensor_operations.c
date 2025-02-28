#include "sensor_operations.h"

#define COMMAND_LENGTH 2
#define READ_LENGTH 6


uint8_t i2c_data_buffer[6] = {0};
struct i2c_client *client;
enum sensor_data_types sensor_data_type = GET_BOTH;
int timeout_second = 1;


int init_client(struct i2c_client *init_client)
{
    client = init_client;
    return 0;
}

// set sensor data_type
int set_sensor_data_type(enum sensor_data_types new_data_type)
{
    if(!(new_data_type == GET_BOTH || new_data_type == GET_TEMPERATURE || new_data_type == GET_HUMIDITY))
    {
        printk(KERN_ALERT "wrong datatype code!\n");
        return -1;
    }
    sensor_data_type = new_data_type;
    return 0;
}

// set timeout_second
int set_measure_time(int new_time)
{
    if(new_time <= 0 || new_time > 60 )
    {
        printk(KERN_ALERT "wrong measure time gap\n");
        return -1;
    }

    timeout_second = new_time;
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
    //remove checksum
    int i, j;
    for (i = 0, j = 0; i < READ_LENGTH; i += 2 + 1) 
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

int get_data_from_sensor(char *tmp_buf, size_t count)
{
    ktime_t start_time = ktime_get();
    ktime_t timeout_time = ktime_add(start_time, ktime_set(timeout_second, 0));
    ktime_t current_time;
    int ret;

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
            return -1;
    }

}

MODULE_AUTHOR("seungju.eum");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Character device driver for SHT31 sensor with I2C support");