#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"
#include "tim.h"

float DS18B20_Read(GPIO_TypeDef *GPIOx,
                   uint16_t GPIO_Pin);

#endif