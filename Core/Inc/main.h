/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

#include <stdbool.h>

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

extern TIM_HandleTypeDef htim2;

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BTN_R_Pin GPIO_PIN_8
#define BTN_R_GPIO_Port GPIOC
#define BTN_R_EXTI_IRQn EXTI9_5_IRQn
#define BTN_DOWN_Pin GPIO_PIN_9
#define BTN_DOWN_GPIO_Port GPIOC
#define BTN_DOWN_EXTI_IRQn EXTI9_5_IRQn
#define BTN_UP_Pin GPIO_PIN_10
#define BTN_UP_GPIO_Port GPIOC
#define BTN_UP_EXTI_IRQn EXTI15_10_IRQn
#define BTN_L_Pin GPIO_PIN_11
#define BTN_L_GPIO_Port GPIOC
#define BTN_L_EXTI_IRQn EXTI15_10_IRQn

/* USER CODE BEGIN Private defines */

typedef struct
{
  bool DOWN_Flag ;
  bool DOWN_Bit ;
  bool UP_Flag;
  bool UP_Bit;
  bool RIGHT_Flag;
  bool RIGHT_Bit;
  bool LEFT_Flag;
  bool LEFT_Bit;
} button_t;

extern button_t Buttons;

extern uint8_t USB_Status_For_Menu_Item;
extern uint8_t Menu_Proces_Status;

extern char UsbFileName_For_Display[32][32];
extern char Name_For_File_Open[32][32];

extern uint32_t File_Size_Mas[];

extern uint32_t File_Size_Current;

extern char SwNewName [];
extern char SwCurrName [];

extern uint16_t DispFileNum;		// 10 file in list -> it is for DEMO only
extern uint16_t DispFilePos;		// current position in file list

extern uint8_t Finde_BIN_Files;

extern uint8_t Firmware_Upgrase_Allowed;
extern uint8_t Firmware_Upgrase_Allowed_Counter;


extern uint8_t Read_Version_Allowed;
extern uint8_t Read_Version_Allowed_Counter;

extern uint8_t Counter_For_String;

extern uint8_t File_Number_Counter;

extern uint8_t Need_Do_Onse;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
