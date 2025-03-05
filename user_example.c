#include "thermohygrometer.h"

int main()
{
    int ret = TH_FAIL;
    ret = measure_air_condition(GET_TEMPERATURE_HUMIDITY, 3, 1);
    if(ret != TH_SUCCESS){
        return TH_FAIL;
    }

    printf("Press enter to end program\n");
    getchar();

    ret = clear_screen();
    if(ret != TH_SUCCESS){
        return TH_FAIL;
    }

    return TH_SUCCESS;
}
