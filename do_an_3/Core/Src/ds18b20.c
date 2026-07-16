#include "ds18b20.h"

extern TIM_HandleTypeDef htim3;

static void Delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim3,0);

    while(__HAL_TIM_GET_COUNTER(&htim3) < us);
}