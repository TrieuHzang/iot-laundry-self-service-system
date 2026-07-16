/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define UP_Pin GPIO_PIN_0
#define UP_GPIO_Port GPIOA
#define DOWN_Pin GPIO_PIN_1
#define DOWN_GPIO_Port GPIOA
#define OK_Pin GPIO_PIN_2
#define OK_GPIO_Port GPIOA
#define POWER_Pin GPIO_PIN_3
#define POWER_GPIO_Port GPIOA
#define DOOR_WASH_Pin GPIO_PIN_4
#define DOOR_WASH_GPIO_Port GPIOA
#define DOOR_DRY_Pin GPIO_PIN_5
#define DOOR_DRY_GPIO_Port GPIOA
#define DOOR_WASHDRY_Pin GPIO_PIN_6
#define DOOR_WASHDRY_GPIO_Port GPIOA
#define RELAY_WASH_Pin GPIO_PIN_0
#define RELAY_WASH_GPIO_Port GPIOB
#define RELAY_DRY_Pin GPIO_PIN_1
#define RELAY_DRY_GPIO_Port GPIOB
#define RELAY_WASHDRY_Pin GPIO_PIN_10
#define RELAY_WASHDRY_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_11
#define BUZZER_GPIO_Port GPIOB
#define DS18B20_1_Pin GPIO_PIN_12
#define DS18B20_1_GPIO_Port GPIOB
#define DS18B20_2_Pin GPIO_PIN_13
#define DS18B20_2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
