/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

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
#define BTN_SEND_DATA_Pin GPIO_PIN_13
#define BTN_SEND_DATA_GPIO_Port GPIOC
#define LED_SENSOR_3_Pin GPIO_PIN_0
#define LED_SENSOR_3_GPIO_Port GPIOC
#define LED_SENSOR_2_Pin GPIO_PIN_1
#define LED_SENSOR_2_GPIO_Port GPIOC
#define ANA_SENSOR_1_Pin GPIO_PIN_0
#define ANA_SENSOR_1_GPIO_Port GPIOA
#define LED_SENSOR_1_Pin GPIO_PIN_0
#define LED_SENSOR_1_GPIO_Port GPIOB
#define LED_FREQ_SEND_DATA_Pin GPIO_PIN_7
#define LED_FREQ_SEND_DATA_GPIO_Port GPIOC
#define PWM_BUZZER_Pin GPIO_PIN_12
#define PWM_BUZZER_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
