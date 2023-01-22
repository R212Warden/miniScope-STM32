/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include <stdio.h>
#include <stdint.h>  
    

ADC_ChannelConfTypeDef adc_sConfig;
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

//UART_HandleTypeDef huart1;

int32_t getSupplyVoltageCounter;

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
//int MX_ADC1_Init(FunctionalState continuousmode, uint32_t samplingtime, uint32_t trigger, uint32_t channel);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define btn_power_Pin GPIO_PIN_0
#define btn_power_GPIO_Port GPIOA
#define btn_mode_Pin GPIO_PIN_1
#define btn_mode_GPIO_Port GPIOA
#define btn_down_Pin GPIO_PIN_2
#define btn_down_GPIO_Port GPIOA
#define btn_up_Pin GPIO_PIN_3
#define btn_up_GPIO_Port GPIOA
#define in_plugged_Pin GPIO_PIN_4
#define in_plugged_GPIO_Port GPIOA
#define LCD_CLK_Pin GPIO_PIN_5
#define LCD_CLK_GPIO_Port GPIOA
#define in_adc_batteryVoltage_Pin GPIO_PIN_6
#define in_adc_batteryVoltage_GPIO_Port GPIOA
#define LCD_DAT_Pin GPIO_PIN_7
#define LCD_DAT_GPIO_Port GPIOA
#define in_chargeCurrent_Pin GPIO_PIN_0
#define in_chargeCurrent_GPIO_Port GPIOB
#define in_scope_Pin GPIO_PIN_1
#define in_scope_GPIO_Port GPIOB
#define LCD_DC_Pin GPIO_PIN_10
#define LCD_DC_GPIO_Port GPIOB
#define LCD_RES_Pin GPIO_PIN_11
#define LCD_RES_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_4
#define LCD_CS_GPIO_Port GPIOB
#define in_counter_Pin GPIO_PIN_5
#define in_counter_GPIO_Port GPIOB
#define LCD_BL_Pin GPIO_PIN_6
#define LCD_BL_GPIO_Port GPIOB
#define out_charging_Pin GPIO_PIN_7
#define out_charging_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
