#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum sensor_modes
{
    GET_BOTH,
	GET_TEMPERATURE,
    GET_HUMIDITY,
};

enum sensor_modes sensor_mode = -1;
int repeat = 1;
int adder = -1;

//error when arg input is wrong
void arg_error_print_information(void)
{
    printf("[temp_humid_checker] arg error\n -t : get temperature\n -h : get humidity\n -n [times] : how many times to get data (default '1')\n -n inf : get data continuously\n");
    exit(EXIT_FAILURE);
}

//error when driver is not working properly or not exist.
void driver_error(int error_ret)
{
    printf("There is error when using device driver\nError return: %d\n",error_ret);
}

//print mode information for user.
void print_mode(void)
{
    printf("Get ");
    if(sensor_mode == GET_BOTH)
    {
        printf("temperature and humidity ");
    }
    else if(sensor_mode == GET_TEMPERATURE)
    {
        printf("temperature ");
    }
    else if(sensor_mode == GET_HUMIDITY)
    {
        printf("humidity ");
    }
    printf("data ");

    if(adder == 0)
    {
        printf("continuously\n");
    }
    else
    {
        printf("%d times\n",repeat);
    }
}


//check args and set mode.
void check_args(int argc, char* argv[])
{
    for(int i=1; i < argc; i++)
    {
        if(strcmp(argv[i],"-t") == 0)
        {
            if(sensor_mode == -1)
            {
                sensor_mode = GET_TEMPERATURE;
            }
            else if(sensor_mode == GET_HUMIDITY)
            {
                sensor_mode = GET_BOTH;
            }
        }
        else if(strcmp(argv[i],"-h") == 0)
        {
            if(sensor_mode == -1)
            {
                sensor_mode = GET_HUMIDITY;
            }
            else if(sensor_mode == GET_TEMPERATURE)
            {
                sensor_mode = GET_BOTH;
            }
        }
        else if(strcmp(argv[i],"-n") == 0)
        {
            if(i+1 >= argc || adder != -1) // no data after '-n' or second '-n'
            {
                arg_error_print_information();
            }
            i++;
            if(strcmp(argv[i],"inf") == 0) //inf loop
            {
                adder = 0;
            }
            else//set repeat num
            {
                repeat = atoi(argv[i]);
                adder = 1;
                if(repeat == 0)
                {
                    arg_error_print_information();
                }
            }
        }
        else
        {
            arg_error_print_information();
        }
    }

    if(sensor_mode == -1)
    {
        arg_error_print_information();
    }

    if(adder == -1)
    {
        adder = 1;
    }
}
 


//main function.
int main(int argc, char* argv[])
{    
    //check arg
    check_args(argc, argv);

    //main logic
    int sht31_dev, lcd1602_dev;
    char buff[100];
    int ret;
    

    lcd1602_dev = open("/dev/lcd1602_driver", O_WRONLY);
    if(lcd1602_dev <= 0)
    {
        driver_error(lcd1602_dev);
        exit(EXIT_FAILURE);
    }

    sht31_dev = open("/dev/sht31_driver", O_RDONLY);

    if(sht31_dev <= 0) // sensor driver open fail
    {
        driver_error(sht31_dev);

        strcpy(buff, "SHT31 Open Error\nExit with Enter.");

        ret = write(lcd1602_dev, buff, strlen(buff)+1);
        if(ret != strlen(buff))
        {
            driver_error(ret);
            exit(EXIT_FAILURE);
        }
    }
    else // sensor driver open success
    {
        ret = ioctl(sht31_dev, 0x44, sensor_mode);
        if(ret != 0)
        {
            driver_error(ret);
            exit(EXIT_FAILURE);
        }

        printf("Connect to SHT31 & LCD1602 success\n");
        print_mode();


        for(int i = 0; i < repeat; i += adder)
        {
            memset(buff, '\0', sizeof(buff));
            ret = read(sht31_dev, buff, 100);
            if(ret <= 0)
            {
                driver_error(ret);
                i -= adder;
                continue;
            }
            
            ret = write(lcd1602_dev, buff, strlen(buff)+1);
            if(ret != strlen(buff))
            {
                driver_error(ret);
                i -= adder;
                continue;
            }

            sleep(1);
        }
    }

    printf("Press enter to close device\n");
    getchar();


    if(sht31_dev > 0)
    {
        close(sht31_dev);
    }
    close(lcd1602_dev);
    printf("Close success\n");

    exit(EXIT_SUCCESS);
}
