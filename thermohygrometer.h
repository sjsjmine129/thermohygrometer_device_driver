#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TH_SUCCESS 0
#define TH_FAIL -1
#define TH_FAIL_WRONG_PARAMS -2
#define TH_FAIL_OPEN_SENSOR_DEVICE -3
#define TH_FAIL_OPEN_LCD_DEVICE -4
#define TH_FAIL_SETTING_DEVICE -5

enum sensor_data_types
{
    GET_TEMPERATURE_HUMIDITY,
	GET_TEMPERATURE,
    GET_HUMIDITY,
};

int measure_air_condition(enum sensor_data_types sensor_data_type, int repeat, int time_gap);

int clear_screen();

