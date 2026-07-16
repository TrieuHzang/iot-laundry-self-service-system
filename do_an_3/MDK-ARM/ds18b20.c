#include "ds18b20.h"

extern TIM_HandleTypeDef htim3;

static void Delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim3,0);

    while(__HAL_TIM_GET_COUNTER(&htim3) < us);
}

static void Pin_Output(GPIO_TypeDef *GPIOx,
                       uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIOx,&GPIO_InitStruct);
}

static void Pin_Input(GPIO_TypeDef *GPIOx,
                      uint16_t GPIO_Pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOx,&GPIO_InitStruct);
}

static uint8_t DS18B20_Reset(GPIO_TypeDef *GPIOx,
                             uint16_t GPIO_Pin)
{
    uint8_t response = 0;

    Pin_Output(GPIOx,GPIO_Pin);

    HAL_GPIO_WritePin(GPIOx,
                      GPIO_Pin,
                      GPIO_PIN_RESET);

    Delay_us(480);

    Pin_Input(GPIOx,GPIO_Pin);

    Delay_us(80);

    if(HAL_GPIO_ReadPin(GPIOx,GPIO_Pin)
       == GPIO_PIN_RESET)
    {
        response = 1;
    }

    Delay_us(400);

    return response;
}

static void DS18B20_WriteByte(GPIO_TypeDef *GPIOx,
                              uint16_t GPIO_Pin,
                              uint8_t data)
{
    for(uint8_t i=0;i<8;i++)
    {
        Pin_Output(GPIOx,GPIO_Pin);

        HAL_GPIO_WritePin(GPIOx,
                          GPIO_Pin,
                          GPIO_PIN_RESET);

        if(data & (1<<i))
        {
            Delay_us(2);

            Pin_Input(GPIOx,GPIO_Pin);

            Delay_us(60);
        }
        else
        {
            Delay_us(60);

            Pin_Input(GPIOx,GPIO_Pin);

            Delay_us(2);
        }
    }
}

static uint8_t DS18B20_ReadByte(GPIO_TypeDef *GPIOx,
                                uint16_t GPIO_Pin)
{
    uint8_t value = 0;

    for(uint8_t i=0;i<8;i++)
    {
        Pin_Output(GPIOx,GPIO_Pin);

        HAL_GPIO_WritePin(GPIOx,
                          GPIO_Pin,
                          GPIO_PIN_RESET);

        Delay_us(2);

        Pin_Input(GPIOx,GPIO_Pin);

        Delay_us(10);

        if(HAL_GPIO_ReadPin(GPIOx,GPIO_Pin))
        {
            value |= (1<<i);
        }

        Delay_us(50);
    }

    return value;
}

float DS18B20_Read(GPIO_TypeDef *GPIOx,
                   uint16_t GPIO_Pin)
{
    uint8_t Temp_L;
    uint8_t Temp_H;

    int16_t Temp;

    if(!DS18B20_Reset(GPIOx,GPIO_Pin))
    {
        return -127.0f;
    }

    DS18B20_WriteByte(GPIOx,
                      GPIO_Pin,
                      0xCC);

    DS18B20_WriteByte(GPIOx,
                      GPIO_Pin,
                      0x44);

    HAL_Delay(750);

    DS18B20_Reset(GPIOx,
                  GPIO_Pin);

    DS18B20_WriteByte(GPIOx,
                      GPIO_Pin,
                      0xCC);

    DS18B20_WriteByte(GPIOx,
                      GPIO_Pin,
                      0xBE);

    Temp_L = DS18B20_ReadByte(GPIOx,
                              GPIO_Pin);

    Temp_H = DS18B20_ReadByte(GPIOx,
                              GPIO_Pin);

    Temp = (Temp_H << 8) | Temp_L;

    return (float)Temp / 16.0f;
}