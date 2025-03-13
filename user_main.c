#include "thermohygrometer.h"
//This is example usage code for thermohygrometer library

enum sensor_data_types sensor_mode = -1;
int repeat = 1;
int time_gap = 1;

//error when arg input is wrong
void arg_error_print_information(void)
{
    printf("[temp_humid_checker] arg error\n -t : get temperature\n -h : get humidity\n -n [times] : how many times to get data (default '1')\n -g [second] :time gap between measurement (default '1s', 1s~60s)\n");
    exit(EXIT_FAILURE);
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
                sensor_mode = GET_TEMPERATURE_HUMIDITY;
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
                sensor_mode = GET_TEMPERATURE_HUMIDITY;
            }
        }
        else if(strcmp(argv[i],"-n") == 0)
        {
            if(i+1 >= argc) // no data after '-n'
            {
                arg_error_print_information();
            }
            i++;

            repeat = atoi(argv[i]);
            if(repeat < 1)
            {
                arg_error_print_information();
            }
            
        }
        else if(strcmp(argv[i],"-g") == 0)
        {
            if(i+1 >= argc) // no data after '-g'
            {
                arg_error_print_information();
            }
            i++;

            time_gap = atoi(argv[i]);
            if(time_gap < 1 || time_gap >60)
            {
                arg_error_print_information();
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
}

int main(int argc, char* argv[])
{    
    //check arg
    check_args(argc, argv);

    int ret = measure_air_condition(sensor_mode, repeat, time_gap);
    if(ret != THERMO_SUCCESS)
    {
        return ret;
    }

    printf("Press enter to end program\n");
    getchar();

    ret = clear_screen();
    if(ret != THERMO_SUCCESS)
    {
        return ret;
    }

    return THERMO_SUCCESS;
}
