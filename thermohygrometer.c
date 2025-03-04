#include "thermohygrometer.h"


int sht31_dev = -1;
int lcd1602_dev = -1;


int measure_air_condition(enum sensor_data_types sensor_data_type, int repeat, int time_gap)
{    
    //check params
    if((sensor_data_type != GET_HUMIDITY && sensor_data_type != GET_TEMPERATURE && sensor_data_type != GET_TEMPERATURE_HUMIDITY)
        ||(repeat <= 1)
        ||(time_gap < 1 || time_gap > 60))
    {
        return -EROOR_WRONG_PARAMS;
    }

    char buff[100];

    //open dev
    lcd1602_dev = open("/dev/lcd1602_driver", O_WRONLY);
    if(lcd1602_dev <= 0)
    {
        return -EROOR_OPEN_LCD_DEVICE;
    }

    sht31_dev = open("/dev/sht31_driver", O_RDONLY);
    if(sht31_dev <= 0)
    {
        close(lcd1602_dev);
        lcd1602_dev = -1;
        return -EROOR_OPEN_SENSOR_DEVICE;
    }

    
    //set data type & time gap of measure temperature or humidity
    if(ioctl(sht31_dev, 0x0010, sensor_data_type))
    {
        close(sht31_dev);
        close(lcd1602_dev);
        return -EROOR_SETTING_DEVICE;
    }
    if(ioctl(sht31_dev, 0x0011, time_gap))
    {
        close(sht31_dev);
        close(lcd1602_dev);
        return -EROOR_SETTING_DEVICE;
    }
    

    //measure loop
    for(int i = 0; i < repeat; i += 1)
    {
        memset(buff, '\0', sizeof(buff));

        if(read(sht31_dev, buff, 100) <= 0)
        {
            i -= 1;
            continue;
        }
        
        if(write(lcd1602_dev, buff, strlen(buff)+1) != strlen(buff))
        {
            i -= 1;
            continue;
        }

    }

    close(sht31_dev);
    close(lcd1602_dev);
    return 0;
}


int clear_screen()
{
    //open dev
    lcd1602_dev = open("/dev/lcd1602_driver", O_WRONLY);
    if(lcd1602_dev <= 0)
    {
        return -EROOR_OPEN_LCD_DEVICE;
    }

    close(lcd1602_dev);
    return 0;
}


