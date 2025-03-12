#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    THERMO_SUCCESS = 0,
    THERMO_FAIL_WRONG_PARAMS = -2,
    THERMO_FAIL_OPEN_SENSOR_DEVICE = -3,
    THERMO_FAIL_OPEN_LCD_DEVICE = -4,
    THERMO_FAIL_SETTING_DEVICE = -5,
}thermo_results;

enum sensor_data_types
{
    GET_TEMPERATURE_HUMIDITY,
	GET_TEMPERATURE,
    GET_HUMIDITY,
};

struct air_condition_data
{
    int temperature;
    int humidity;
};

int measure_air_condition(enum sensor_data_types sensor_data_type, int repeat, int time_gap);

int clear_screen();

