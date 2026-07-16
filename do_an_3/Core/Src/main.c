/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>
#include "ds18b20.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t g_ms = 0;

uint16_t ADC_Value = 0;
uint8_t SpeedFactor = 1;

char oledStr[32];

uint32_t LastTick = 0;

/* WASH */
uint8_t WashReady = 0;
uint8_t DryReady = 0;
uint8_t WashDryReady = 0;

uint8_t WashBusy = 0;
uint8_t DryBusy = 0;
uint8_t WashDryBusy = 0;

uint32_t WashTime = 0;
uint32_t DryTime = 0;
uint32_t WashDryTime = 0;

uint8_t WashDryStage = 0;

uint8_t LocalMode = 0;

uint8_t Cursor = 0;
uint32_t OkPressStart = 0;
uint8_t LocalMenu = 0;
uint8_t OkOld = 0;
float TempDry = 0;
float TempWashDry = 0;

uint32_t TempTick = 0;

char uartRxBuf[32];
uint8_t uartRxIdx = 0;
char uartPendingCmd[32];
volatile uint8_t uartCmdPending = 0;
volatile uint8_t uartRxByte = 0;

uint32_t lastUartTimeValue = 255;
/* Bộ đếm riêng giới hạn gửi TIME qua UART tối đa 1 lần/giây */
uint32_t lastUartReportTick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void Relay_All_Off(void)
{
    HAL_GPIO_WritePin(GPIOB,
                      RELAY_WASH_Pin |
                      RELAY_DRY_Pin |
                      RELAY_WASHDRY_Pin,
                      GPIO_PIN_SET);
}

void Wash_On(void)
{
    HAL_GPIO_WritePin(RELAY_WASH_GPIO_Port,
                      RELAY_WASH_Pin,
                      GPIO_PIN_RESET);
}

void Dry_On(void)
{
    HAL_GPIO_WritePin(RELAY_DRY_GPIO_Port,
                      RELAY_DRY_Pin,
                      GPIO_PIN_RESET);
}

void WashDry_On(void)
{
    HAL_GPIO_WritePin(RELAY_WASHDRY_GPIO_Port,
                      RELAY_WASHDRY_Pin,
                      GPIO_PIN_RESET);
}
uint8_t BtnUP(void)
{
    return (HAL_GPIO_ReadPin(UP_GPIO_Port,UP_Pin)==GPIO_PIN_RESET);
}

uint8_t BtnDOWN(void)
{
    return (HAL_GPIO_ReadPin(DOWN_GPIO_Port,DOWN_Pin)==GPIO_PIN_RESET);
}

uint8_t BtnOK(void)
{
    return (HAL_GPIO_ReadPin(OK_GPIO_Port,OK_Pin)==GPIO_PIN_RESET);
}

uint8_t BtnPOWER(void)
{
    return (HAL_GPIO_ReadPin(POWER_GPIO_Port,POWER_Pin)==GPIO_PIN_RESET);
}
uint16_t ADC_Read(void)
{
    HAL_ADC_Start(&hadc1);

    HAL_ADC_PollForConversion(&hadc1,10);

    return HAL_ADC_GetValue(&hadc1);
	
}
uint8_t DoorWashClosed(void)
{
    return (HAL_GPIO_ReadPin(DOOR_WASH_GPIO_Port,
                             DOOR_WASH_Pin)
            == GPIO_PIN_RESET);
}

uint8_t DoorDryClosed(void)
{
    return (HAL_GPIO_ReadPin(DOOR_DRY_GPIO_Port,
                             DOOR_DRY_Pin)
            == GPIO_PIN_RESET);
}

uint8_t DoorWashDryClosed(void)
{
    return (HAL_GPIO_ReadPin(DOOR_WASHDRY_GPIO_Port,
                             DOOR_WASHDRY_Pin)
            == GPIO_PIN_RESET);
}
void Buzzer_On(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port,
                      BUZZER_Pin,
                      GPIO_PIN_SET);
}

void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port,
                      BUZZER_Pin,
                      GPIO_PIN_RESET);
}

void Buzzer_Beep(uint8_t times)
{
    for(uint8_t i=0;i<times;i++)
    {
        Buzzer_On();
        HAL_Delay(150);

        Buzzer_Off();
        HAL_Delay(150);
    }
}
void StartWash(void)
{
    if(WashBusy == 0)
    {
        WashBusy = 1;
        WashTime = 60;

        Wash_On();
    }
}

void StartDry(void)
{
    if(DryBusy == 0)
    {
        DryBusy = 1;
        DryTime = 45;

        Dry_On();
    }
}

void StartWashDry(void)
{
    if(WashDryBusy == 0)
    {
        WashDryBusy = 1;

        WashDryStage = 1;

        WashDryTime = 60;

        WashDry_On();
    }
}
void GrantWash(void)
{
    WashReady = 1;
}

void GrantDry(void)
{
    DryReady = 1;
}

void GrantWashDry(void)
{
    WashDryReady = 1;
}

void UART_SendLine(const char *msg)
{
    HAL_UART_Transmit(&huart1,
                      (uint8_t *)msg,
                      strlen(msg),
                      100);
    HAL_UART_Transmit(&huart1,
                      (uint8_t *)"\r\n",
                      2,
                      100);
}

uint8_t MachineBusy(void)
{
    return (WashBusy || DryBusy || WashDryBusy);
}

uint8_t MachineReady(void)
{
    return (WashReady || DryReady || WashDryReady);
}

void UART_ReportTime(void)
{
    uint32_t timeLeft = 0;

    if(WashBusy)
        timeLeft = WashTime;
    else if(DryBusy)
        timeLeft = DryTime;
    else if(WashDryBusy)
        timeLeft = WashDryTime;
    else
        return;

    if(timeLeft != lastUartTimeValue)
{
    char buf[16];

    lastUartTimeValue = timeLeft;

    sprintf(buf, "TIME:%lu", timeLeft);
    UART_SendLine(buf);
}
}

void UART_NormalizeCmd(char *cmd)
{
    char *start = cmd;
    char *end;

    while(*start == ' ' || *start == '\t')
        start++;

    if(start != cmd)
        memmove(cmd, start, strlen(start) + 1);

    end = cmd + strlen(cmd);
    while(end > cmd)
    {
        char c = *(end - 1);

        if(c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            *(--end) = '\0';
        }
        else
        {
            break;
        }
    }

    for(char *p = cmd; *p; p++)
    {
        if(*p >= 'a' && *p <= 'z')
            *p = (char)(*p - 'a' + 'A');
    }
}

void ProcessUARTCommand(char *cmd)
{
    UART_NormalizeCmd(cmd);

    if(strlen(cmd) == 0)
        return;

    if(strcmp(cmd, "WASH") == 0)
    {
        if(MachineBusy())
        {
            UART_SendLine("BUSY");
            return;
        }

        GrantWash();
        LocalMenu = 0;
        Buzzer_Beep(2);
        UART_SendLine("WASH READY");
				lastUartTimeValue = 255;
UART_ReportTime();
    }
    else if(strcmp(cmd, "DRY") == 0)
    {
        if(MachineBusy())
        {
            UART_SendLine("BUSY");
            return;
        }

        GrantDry();
        LocalMenu = 0;
        Buzzer_Beep(2);
        UART_SendLine("DRY READY");
				lastUartTimeValue = 255;
UART_ReportTime();
    }
    else if(strcmp(cmd, "WASHDRY") == 0)
    {
        if(MachineBusy())
        {
            UART_SendLine("BUSY");
            return;
        }

        GrantWashDry();
        LocalMenu = 0;
        Buzzer_Beep(2);
        UART_SendLine("WASHDRY READY");
				lastUartTimeValue = 255;
UART_ReportTime();
    }
    else if(strcmp(cmd, "STOP") == 0)
    {
        WashReady = 0;
        DryReady = 0;
        WashDryReady = 0;
        WashBusy = 0;
        DryBusy = 0;
        WashDryBusy = 0;
        WashDryStage = 0;

        Relay_All_Off();
        /* FIX: gửi TIME:0 trước FREE để Blynk reset V3 đồng bộ */
        UART_SendLine("TIME:0");
        lastUartTimeValue = 255;

        UART_SendLine("FREE");
    }
    else
    {
        UART_SendLine("ERROR");
    }
}

void UART_OnByte(uint8_t c)
{
    if(c == '\r' || c == '\n')
    {
        if(uartRxIdx > 0)
        {
            uartRxBuf[uartRxIdx] = '\0';
            strncpy(uartPendingCmd, uartRxBuf, sizeof(uartPendingCmd) - 1);
            uartPendingCmd[sizeof(uartPendingCmd) - 1] = '\0';
            uartCmdPending = 1;
            uartRxIdx = 0;
        }
    }
    else if(uartRxIdx < (sizeof(uartRxBuf) - 1))
    {
        uartRxBuf[uartRxIdx++] = c;
    }
    else
    {
        uartRxIdx = 0;
    }
}

void UART_StartReceive(void)
{
    uartRxIdx = 0;
    uartCmdPending = 0;
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&uartRxByte, 1);
}

void UART_ProcessPending(void)
{
    if(uartCmdPending)
    {
        uartCmdPending = 0;
        ProcessUARTCommand(uartPendingCmd);
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
HAL_TIM_Base_Start_IT(&htim2);
HAL_TIM_Base_Start(&htim3);

Relay_All_Off();
Buzzer_Off();


SSD1306_Init();

SSD1306_Fill(SSD1306_COLOR_BLACK);
SSD1306_UpdateScreen();


SSD1306_Fill(SSD1306_COLOR_BLACK);

SSD1306_GotoXY(0,0);

SSD1306_Puts("STM32 READY",
             &Font_7x10,
             SSD1306_COLOR_WHITE);

SSD1306_UpdateScreen();

UART_StartReceive();
UART_SendLine("FREE");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
while(1)
{
    UART_ProcessPending();

    ADC_Value = ADC_Read();

    SpeedFactor = 1 + (ADC_Value * 19) / 4095;
	if((g_ms - TempTick) >= 15000)
{
    TempTick = g_ms;

    TempDry =
        DS18B20_Read(DS18B20_1_GPIO_Port,
                     DS18B20_1_Pin);

    TempWashDry =
        DS18B20_Read(DS18B20_2_GPIO_Port,
                     DS18B20_2_Pin);
}
if(TempDry > 60)
{
    DryBusy = 0;
    DryReady = 0;

    HAL_GPIO_WritePin(RELAY_DRY_GPIO_Port,
                      RELAY_DRY_Pin,
                      GPIO_PIN_SET);

    Buzzer_Beep(5);
    UART_SendLine("ERROR");
    UART_SendLine("FREE");
    lastUartTimeValue = 255;
}

if(TempWashDry > 60)
{
    WashDryBusy = 0;
    WashDryReady = 0;

    HAL_GPIO_WritePin(RELAY_WASHDRY_GPIO_Port,
                      RELAY_WASHDRY_Pin,
                      GPIO_PIN_SET);

    Buzzer_Beep(5);
    UART_SendLine("ERROR");
    UART_SendLine("FREE");
    lastUartTimeValue = 255;
}
uint8_t OkNew = BtnOK();

if(OkNew && !OkOld)
{
    if(LocalMenu == 0)
    {
        LocalMenu = 1;

        while(BtnOK());

        HAL_Delay(20);
    }
}

OkOld = OkNew;
	
	if(BtnPOWER())
{
    if(WashReady &&
       DoorWashClosed())
    {
        WashReady = 0;
        StartWash();

UART_SendLine("BUSY");

lastUartTimeValue = 255;
UART_ReportTime();
    }

    if(DryReady &&
       DoorDryClosed())
    {
        DryReady = 0;
        StartDry();
UART_SendLine("BUSY");

lastUartTimeValue = 255;
UART_ReportTime();
    }

    if(WashDryReady &&
       DoorWashDryClosed())
    {
        WashDryReady = 0;
        StartWashDry();

UART_SendLine("BUSY");

lastUartTimeValue = 255;
UART_ReportTime();
    }

    HAL_Delay(20);
}
if(LocalMenu)
{
    if(BtnUP())
    {
        if(Cursor > 0)
            Cursor--;

        HAL_Delay(200);
    }

    if(BtnDOWN())
    {
        if(Cursor < 2)
            Cursor++;

        HAL_Delay(200);
    }

    if(BtnOK())
{
    while(BtnOK());

    switch(Cursor)
    {
        case 0:
    GrantWash();
    UART_SendLine("WASH READY");
    lastUartTimeValue = 255;
    UART_ReportTime();
    break;

case 1:
    GrantDry();
    UART_SendLine("DRY READY");
    lastUartTimeValue = 255;
    UART_ReportTime();
    break;

case 2:
    GrantWashDry();
    UART_SendLine("WASHDRY READY");
    lastUartTimeValue = 255;
    UART_ReportTime();
    break;
    }

    LocalMenu = 0;

    HAL_Delay(20);
}

		if(BtnPOWER())
{
    LocalMenu = 0;
    HAL_Delay(50);
}
}

    if(g_ms - LastTick >= (1000 / SpeedFactor))
{
    LastTick = g_ms;

    if(WashBusy && WashTime)
        WashTime--;

    if(DryBusy && DryTime)
        DryTime--;

    if(WashDryBusy && WashDryTime)
        WashDryTime--;

    /* Gửi TIME qua UART tối đa 1 lần/giây, không phụ thuộc SpeedFactor.
       Tránh flood Blynk khi SpeedFactor > 1 */
    if(MachineBusy() && (g_ms - lastUartReportTick >= 1000))
    {
        lastUartReportTick = g_ms;
        lastUartTimeValue = 255; /* force gửi TIME hiện tại */
        UART_ReportTime();
    }
}

    if(WashBusy && WashTime == 0)
    {
        WashBusy = 0;
			WashReady = 0;

        HAL_GPIO_WritePin(RELAY_WASH_GPIO_Port,
                          RELAY_WASH_Pin,
                          GPIO_PIN_SET);

        /* FIX: gửi TIME:0 trước để Blynk reset V3 đồng bộ */
        UART_SendLine("TIME:0");
        lastUartTimeValue = 0;
        Buzzer_Beep(3);
        UART_SendLine("FINISHED");
        UART_SendLine("FREE");
        lastUartTimeValue = 255;
    }

    if(DryBusy && DryTime == 0)
    {
        DryBusy = 0;
        DryReady = 0;

        HAL_GPIO_WritePin(RELAY_DRY_GPIO_Port,
                          RELAY_DRY_Pin,
                          GPIO_PIN_SET);

        /* FIX: gửi TIME:0 trước để Blynk reset V3 đồng bộ */
        UART_SendLine("TIME:0");
        lastUartTimeValue = 0;
        Buzzer_Beep(3);
        UART_SendLine("FINISHED");
        UART_SendLine("FREE");
        lastUartTimeValue = 255;
    }

    if(WashDryBusy && WashDryTime == 0)
    {
        if(WashDryStage == 1)
{
    WashDryStage = 2;

    WashDryTime = 45;

    lastUartTimeValue = 255;
    UART_SendLine("BUSY");
    UART_ReportTime();
}
        else
        {
            WashDryBusy = 0;
            WashDryReady = 0;
            HAL_GPIO_WritePin(RELAY_WASHDRY_GPIO_Port,
                              RELAY_WASHDRY_Pin,
                              GPIO_PIN_SET);

            /* FIX: gửi TIME:0 trước để Blynk reset V3 đồng bộ */
            UART_SendLine("TIME:0");
            lastUartTimeValue = 0;
            Buzzer_Beep(3);
            UART_SendLine("FINISHED");
            UART_SendLine("FREE");
            lastUartTimeValue = 255;
        }
    }


if(LocalMenu)
{
    SSD1306_Fill(SSD1306_COLOR_BLACK);

    SSD1306_GotoXY(0,0);
    SSD1306_Puts("LOCAL MODE",
                 &Font_7x10,
                 SSD1306_COLOR_WHITE);

    SSD1306_GotoXY(0,20);

    if(Cursor == 0)
        SSD1306_Puts(">WASH",
                     &Font_7x10,
                     SSD1306_COLOR_WHITE);
    else
        SSD1306_Puts(" WASH",
                     &Font_7x10,
                     SSD1306_COLOR_WHITE);

    SSD1306_GotoXY(0,35);

    if(Cursor == 1)
        SSD1306_Puts(">DRY",
                     &Font_7x10,
                     SSD1306_COLOR_WHITE);
    else
        SSD1306_Puts(" DRY",
                     &Font_7x10,
                     SSD1306_COLOR_WHITE);

    SSD1306_GotoXY(0,50);

    if(Cursor == 2)
        SSD1306_Puts(">W+D",
                     &Font_7x10,
                     SSD1306_COLOR_WHITE);
    else
        SSD1306_Puts(" W+D",
                     &Font_7x10,
                     SSD1306_COLOR_WHITE);

    SSD1306_UpdateScreen();

    HAL_Delay(50);

    continue;
}
    SSD1306_Fill(SSD1306_COLOR_BLACK);

  SSD1306_GotoXY(0,0);

if(WashBusy)
{
    sprintf(oledStr,"WASH:%lus",WashTime);
}
else if(WashReady)
{
    sprintf(oledStr,"WASH:READY");
}
else
{
    sprintf(oledStr,"WASH:FREE");
}

SSD1306_Puts(oledStr,
             &Font_7x10,
             SSD1306_COLOR_WHITE);

   SSD1306_GotoXY(0,20);

if(DryBusy)
{
    sprintf(oledStr,
            "DRY:%lus %.0fC",
            DryTime,
            TempDry);
}
else if(DryReady)
{
    sprintf(oledStr,
            "DRY:READY %.0fC",
            TempDry);
}
else
{
    sprintf(oledStr,
            "DRY:FREE %.0fC",
            TempDry);
}

SSD1306_Puts(oledStr,
             &Font_7x10,
             SSD1306_COLOR_WHITE);

   SSD1306_GotoXY(0,40);

if(WashDryBusy)
{
    if(WashDryStage == 1)
    {
        sprintf(oledStr,
                "W+D:W%lu %.0fC",
                WashDryTime,
                TempWashDry);
    }
    else
    {
        sprintf(oledStr,
                "W+D:D%lu %.0fC",
                WashDryTime,
                TempWashDry);
    }
}
else if(WashDryReady)
{
    sprintf(oledStr,
            "W+D:READY %.0fC",
            TempWashDry);
}
else
{
    sprintf(oledStr,
            "W+D:FREE %.0fC",
            TempWashDry);
}

SSD1306_Puts(oledStr,
             &Font_7x10,
             SSD1306_COLOR_WHITE);

    SSD1306_UpdateScreen();

    HAL_Delay(10);
}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
    {
        g_ms++;
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        UART_OnByte(uartRxByte);
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&uartRxByte, 1);
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
