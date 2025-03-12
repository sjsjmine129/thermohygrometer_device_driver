#include "thermohygrometer.h"

thermo_results measure_air_condition(enum sensor_data_types sensor_data_type, int repeat, int time_gap)
{
    int sht31_dev = -1;
    int lcd1602_dev = -1;

    //check params
    if((sensor_data_type != GET_HUMIDITY && sensor_data_type != GET_TEMPERATURE && sensor_data_type != GET_TEMPERATURE_HUMIDITY)
        ||(repeat <= 1)
        ||(time_gap <= 0 || time_gap > 60))
    {
        printf("Fail: Wrong params\n");
        return THERMO_FAIL_WRONG_PARAMS;
    }

    char buff[100] = {0};

    //open dev
    sht31_dev = open("/dev/sht31_driver", O_RDONLY);
    if(sht31_dev <= 0)
    {
        printf("Fail: Fail while connect sensor device\n");
        return THERMO_FAIL_OPEN_SENSOR_DEVICE;
    }

    //set data type & time gap of measure temperature or humidity
    if(ioctl(sht31_dev, 0x0010, sensor_data_type) != 0)
    {
        close(sht31_dev);
        printf("Fail: Fail while setting sensor device\n");
        printf("For third param, use sensor_data_types in thermohygrometer.h\n");
        return THERMO_FAIL_SETTING_DEVICE;
    }
    if(ioctl(sht31_dev, 0x0011, time_gap) != 0)
    {
        close(sht31_dev);
        printf("Fail: Fail while setting sensor device\n");
        return THERMO_FAIL_SETTING_DEVICE;
    }

    lcd1602_dev = open("/dev/lcd1602_driver", O_WRONLY);
    if(lcd1602_dev <= 0)
    {
        close(sht31_dev);
        printf("Fail: Fail while connect LCD device\n");
        return THERMO_FAIL_OPEN_LCD_DEVICE;
    }


    
    //measure loop
    for(int i = 0; i < repeat; i++)
    {
        memset(buff, '\0', sizeof(buff));

        if(read(sht31_dev, buff, sizeof(buff)) <= 0)
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
    return THERMO_SUCCESS;
}


thermo_results clear_screen()
{
    int lcd1602_dev = -1;
    //open dev
    lcd1602_dev = open("/dev/lcd1602_driver", O_WRONLY);
    if(lcd1602_dev <= 0)
    {
        printf("Fail: Fail while connect LCD device\n");
        return THERMO_FAIL_OPEN_LCD_DEVICE;
    }

    close(lcd1602_dev);
    return THERMO_SUCCESS;
}


