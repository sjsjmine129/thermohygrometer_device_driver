#include "thermohygrometer.h"
//This is example usage code for thermohygrometer library

int main()
{
    int ret = measure_air_condition(GET_TEMPERATURE_HUMIDITY, 5, 1);
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
