#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EROOR_WRONG_PARAMS 1
#define EROOR_OPEN_SENSOR_DEVICE 2
#define EROOR_OPEN_LCD_DEVICE 3
#define EROOR_SETTING_DEVICE 4


enum sensor_data_types
{
    GET_TEMPERATURE_HUMIDITY,
	GET_TEMPERATURE,
    GET_HUMIDITY,
};



int measure_air_condition(enum sensor_data_types sensor_data_type, int repeat, int time_gap);

int clear_screen();

