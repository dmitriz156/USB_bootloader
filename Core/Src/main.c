/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_device.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "display.h"
#include "flash_r_w.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VECTOR_TABLE_SIZE                (31 + 1 + 7 + 9)

#define APPLICATION_ADDRESS              0x0800C000
#define READ_BLOCK_SIZE					         128
#define STORAGE_LUN_NBR                  1
#define STORAGE_BLK_NBR                  128 //num Kbytes*2
#define STORAGE_BLK_SIZ                  512
#define FLASH_START_ADDRESS				       0x0800C000
#define FILE_OFFSET						           0xC000

#define MAX_FILES                        32
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim5;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
extern ApplicationTypeDef Appli_state;
extern USBH_HandleTypeDef hUsbHostFS;

FATFS USBDISKFatFs;
FIL File;

DIR Main_Dir;
FILINFO Main_Dir_Fileinfo;

USBSTAT USB_Status_For_Display;
uint8_t USB_Status_For_Menu_Item;
uint8_t Menu_Proces_Status=0;

button_t Buttons = {0};

char UsbFileName_For_Display[32][32];
char Name_For_File_Open[32][32];

uint32_t File_Size_Mas[32];

uint32_t File_Size_Current;

char SwNewName[32];
char SwCurrName[32];

uint16_t DispFileNum;		// 10 file in list -> it is for DEMO only
uint16_t DispFilePos;		// current position in file list

uint8_t Finde_BIN_Files=0;

uint8_t Firmware_Upgrase_Allowed = 0;
uint8_t Firmware_Upgrase_Allowed_Counter = 0;


uint8_t Read_Version_Allowed = 0;
uint8_t Read_Version_Allowed_Counter;

uint8_t Counter_For_String;

uint8_t File_Number_Counter;

uint8_t Need_Do_Onse;

union  {uint8_t Type_u8_t[2]; int16_t Type_u16_t;} Value;

int16_t Value_int16_t;


uint8_t Verification_Error;
uint8_t Exit_Bit;

uint8_t Exit_Flag_FR;

uint8_t buffer[STORAGE_BLK_NBR*STORAGE_BLK_SIZ];
uint8_t USB_data_read_flag = 0;
uint32_t USB_live_counter = 0;
uint32_t buffer_size = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM5_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
void FLASH_Program_Byte_Customized(uint32_t Address, uint8_t Data);
void Jump_To_Main_Application(void);
void Check_If_Need_Start_Main_Program(void);
void Work_With_File_in_the_USB_Flash(void);
void Read_Firmware_Version_From_File(void);
uint8_t Try_Finde_BIN_File(void);

#define FLASH_SOURCE_ADDR  ((uint32_t)0x08100000)
#define FLASH_DEST_ADDR    ((uint32_t)0x08020000)

void Data_From_USB_To_Flash(uint32_t data_size)
{
    //memcpy(RAM_buf, (uint8_t*)BIN_SOURCE_ADDR, BIN_MAX_SIZE);

    HAL_FLASH_Unlock();

    // Очистити сектор (сектор 5 починається з 0x08020000, розмір – KB)
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t SectorError;

    eraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    eraseInit.Sector       = FLASH_SECTOR_5;
    eraseInit.NbSectors    = 1;

    if (HAL_FLASHEx_Erase(&eraseInit, &SectorError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return;
    }
    // Запис байт у флеш по 4 байти
    for (uint32_t i = 0; i < data_size; i += 4)
    {
        uint32_t data;
        memcpy(&data, buffer[i], 4);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DEST_ADDR + i, data) != HAL_OK)
        {
        	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, (!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)));
        	break;
        }
    }

    HAL_FLASH_Lock();
}

bool Is_Flash_Data_Matching(uint32_t size)
{
    uint32_t *flash_ptr = (uint32_t*)FLASH_DEST_ADDR;

    if (memcmp(flash_ptr, buffer, size) == 0)
    {
        return true; // Дані збігаються
    } else {
        return false;
    }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_DMA_Init();
  MX_FATFS_Init();
  MX_USART2_UART_Init();
  MX_USB_HOST_Init();
  MX_TIM5_Init();
  //MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  //Check_If_Need_Start_Main_Program();
  HAL_Delay(100);

  if ( ( HAL_GPIO_ReadPin(BTN_L_GPIO_Port, BTN_L_Pin) == 1 ) || ( HAL_GPIO_ReadPin(BTN_R_GPIO_Port, BTN_R_Pin) == 1 ) )
  {
  	Jump_To_Main_Application();
  }
  
  HAL_TIM_Base_Start_IT(&htim5);
  HAL_UART_Transmit_DMA(&huart2, DispUart.txBuff, DISP_TX_BUFF);
  DispInit();
  USB_Status_For_Menu_Item = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */

    if(Appli_state==APPLICATION_READY)
    	//------------------------------------
    {
    	//--------------------------------------------------------------
    	if ( Finde_BIN_Files )
    	{
    		if ( Need_Do_Onse == 0 )
    		{
    			//Check if file is present
    			if ( Try_Finde_BIN_File()) { USB_Status_For_Menu_Item = 3; } else { USB_Status_For_Menu_Item = 2; }
          DispFileNum = File_Number_Counter;

    			Need_Do_Onse = 1;
    		}
    	}
    	//--------------------------------------------------------------

    	//--------------------------------------------------------------
    	if ( Read_Version_Allowed )
    	{
    		Read_Firmware_Version_From_File();
    	}
    	//--------------------------------------------------------------

    	//--------------------------------------------------------------
    	if ( Firmware_Upgrase_Allowed  )
    	{
    		Work_With_File_in_the_USB_Flash();
    	}
    	//--------------------------------------------------------------

    }
    else
    {
    	if ( Finde_BIN_Files )
    	{
    		USB_Status_For_Menu_Item = 0;
    		USB_Status_For_Display=USB_STAT_NO_USB;

    		USB_Status_For_Menu_Item = 0;
    		Need_Do_Onse=0;
    		DispFilePos=0;
    	}

    }

//	  if(USB_data_read_flag){
//		  uint32_t *flash_ptr = (uint32_t*)FLASH_DEST_ADDR;
//		  if (memcmp(flash_ptr, buffer, sizeof(buffer)) != 0)
//		  {
//			  //Data_From_USB_To_Flash(buffer, sizeof(buffer));
//			  buffer_size = sizeof(buffer);
//			  Data_From_USB_To_Flash(sizeof(buffer));
//			  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, 1);
//			  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 0);
//		  }else{
//			  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, 1);
//		  }
//	  }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 840-1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 100-1;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : PG6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_R_Pin BTN_DOWN_Pin BTN_UP_Pin BTN_L_Pin */
  GPIO_InitStruct.Pin = BTN_R_Pin|BTN_DOWN_Pin|BTN_UP_Pin|BTN_L_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void FLASH_Program_Byte_Customized(uint32_t Address, uint8_t Data)
{	
	CLEAR_BIT(FLASH->CR, FLASH_CR_PSIZE);
	FLASH->CR |= FLASH_PSIZE_BYTE;
	FLASH->CR |= FLASH_CR_PG;

	*(__IO uint8_t*)Address = Data;

	while ((FLASH->SR & FLASH_SR_BSY) != 0 ) {  }
}

void Jump_To_Main_Application(void)
{
	uint32_t app_jump_address;
	app_jump_address = *((volatile uint32_t*)(APPLICATION_ADDRESS + 4)); //reset handler

	HAL_RCC_DeInit();
	HAL_DeInit();
	void(*GoToApp)(void);
	GoToApp = (void(*)(void)) app_jump_address;

	__disable_irq();
	memset((uint32_t *)NVIC->ICER, 0xFF, sizeof(NVIC->ICER));
	memset((uint32_t *)NVIC->ICPR, 0xFF, sizeof(NVIC->ICPR));
	SysTick->CTRL = 0;
	SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
	SCB->VTOR = APPLICATION_ADDRESS;
	//__set_MSP( (uint32_t)&_App);
	__set_MSP((uint32_t)APPLICATION_ADDRESS);
	GoToApp();
}

void Check_If_Need_Start_Main_Program(void)
{
	if ( ( HAL_GPIO_ReadPin(BTN_L_GPIO_Port, BTN_L_Pin) == 1 ) || ( HAL_GPIO_ReadPin(BTN_R_GPIO_Port, BTN_R_Pin) == 1 ) )
	{
		Jump_To_Main_Application();
	}
}

void Work_With_File_in_the_USB_Flash(void)
{
    UINT bytesRead;
    uint8_t buffer[READ_BLOCK_SIZE];
    uint32_t byteOffset = 0;
    uint32_t fileSize = 0;
    Verification_Error = 1;
    Exit_Flag_FR = 0;

    while (Verification_Error)
    {
        if (f_mount(&USBDISKFatFs, "0:", 0) != FR_OK) {
            Exit_Flag_FR = 1;
            break;
        }

        if (f_open(&File, Name_For_File_Open[DispFilePos], FA_READ) != FR_OK) {
            Exit_Flag_FR = 2;
            break;
        }

        if (f_lseek(&File, FILE_OFFSET) != FR_OK) {
            f_close(&File);
            Exit_Flag_FR = 3;
            break;
        }

        USB_Status_For_Display = USB_STAT_PROC_ERASE;
        HAL_Delay(50);
        HAL_FLASH_Unlock();

        // Erase necessary sectors (adjust if more than 1 sector is needed)
        FLASH_EraseInitTypeDef eraseInit = {
            .TypeErase = FLASH_TYPEERASE_SECTORS,
            .VoltageRange = FLASH_VOLTAGE_RANGE_3,
            .Sector = FLASH_SECTOR_3,
            .NbSectors = 5  // Sectors 3–7
        };
        uint32_t SectorError;
        if (HAL_FLASHEx_Erase(&eraseInit, &SectorError) != HAL_OK) {
            HAL_FLASH_Lock();
            f_close(&File);
            Exit_Flag_FR = 4;
            break;
        }

        // --- Programming Flash ---
        USB_Status_For_Display = USB_STAT_PROC_LOAD;
        HAL_Delay(100);
        byteOffset = 0;
		//read READ_BLOCK_SIZE 128 byte of data from file
        while ((f_read(&File, buffer, READ_BLOCK_SIZE, &bytesRead) == FR_OK) && bytesRead > 0)
        {
            for (uint32_t i = 0; i < bytesRead; i++) {
                HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, FLASH_START_ADDRESS + byteOffset + i, buffer[i]);
            }
            byteOffset += bytesRead;
            fileSize += bytesRead;
        }

        HAL_FLASH_Lock();
        f_close(&File);

		    USB_Status_For_Display = USB_STAT_PROC_VERIF; //status for display massege
        HAL_Delay(100);
        // --- Verification phase ---
        Verification_Error = 0;
        byteOffset = 0;

        if (f_open(&File, Name_For_File_Open[DispFilePos], FA_READ) != FR_OK ||
            f_lseek(&File, FILE_OFFSET) != FR_OK) {
            Verification_Error = 1;
            Exit_Flag_FR = 5;
            break;
        }
		//===data verification===
        while ((f_read(&File, buffer, READ_BLOCK_SIZE, &bytesRead) == FR_OK) && bytesRead > 0)
        {
            for (uint32_t i = 0; i < bytesRead; i++) {
				//checking data which is written in flash
                uint8_t flash_byte = *(__IO uint8_t*)(FLASH_START_ADDRESS + byteOffset + i);
                if (flash_byte != buffer[i]) {
                    Verification_Error = 1;
                    break;
                }
            }
            if (Verification_Error) break;
            byteOffset += bytesRead;
        }
		//======================

        f_close(&File);
		//__enable_irq();

        if (fileSize == 0 || byteOffset != fileSize) {
            Verification_Error = 1;
        }

        // --- Status handling ---
        if (Verification_Error == 0) {
            USB_Status_For_Display = USB_STAT_UPDATE_OK;
            while (1) {
            	HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
                // if (Buttons.RIGHT_Bit == 1) {
                //     HAL_NVIC_SystemReset();
                // }
            }
        } else {
            USB_Status_For_Display = USB_STAT_UPDATE_FAIL;
            Exit_Bit = 0;
            while (!Exit_Bit) {
                if (Buttons.DOWN_Bit) {
                    Exit_Bit = 1;
                    Buttons.DOWN_Bit = 0;
                    USB_Status_For_Display = USB_STAT_PROC_ERASE;
                    for (volatile uint32_t i = 0; i < 4500000; i++) {}
                }
                if (Buttons.UP_Bit) {
                    Buttons.UP_Bit = 0;
                    HAL_NVIC_SystemReset();
                }
            }
        }
    }

    // Error message handling
    if (Exit_Flag_FR != 0) {
        USB_Status_For_Display = USB_STAT_TIMEOUT;
        while (1) {
            if (Buttons.RIGHT_Bit == 1) {
                HAL_NVIC_SystemReset();
            }
        }
    }

    while (1) {}
}

void Read_Firmware_Version_From_File(void)
{


	UINT Bytes_Read;
	char Read_Bufer[36];

	uint8_t Result_1=0;
	uint8_t Result_2=0;
	uint8_t Result_3=0;


	if(f_mount(&USBDISKFatFs,"0:",0)==FR_OK)
		//---------------------------------------------------------
	{
		if(f_open(&File, Name_For_File_Open[DispFilePos] ,FA_READ)==FR_OK)
			//---------------------------------------------------
		{
			if ( f_lseek(&File, 0xC400) == FR_OK )
			{
				//------------------------------------------------------------------
				if ( (f_read(&File,Read_Bufer,2,&Bytes_Read)==FR_OK) && (Bytes_Read==2) )
				{
					Value.Type_u8_t[1] = Read_Bufer[1];
					Value.Type_u8_t[0] = Read_Bufer[0];

					strcpy(SwNewName,	DispIntToStr( Value.Type_u16_t ,0) );

					Value_int16_t = Value.Type_u16_t;

					Result_1 = 1;
				}
				//------------------------------------------------------------------
				//------------------------------------------------------------------
				if ( (f_read(&File,Read_Bufer,36,&Bytes_Read)==FR_OK) && (Bytes_Read==36) )
				{
					strcpy(SwNewName,	Read_Bufer );

					Result_2 = 1;
				}
				//------------------------------------------------------------------
			}

			f_close(&File);
			//---------------------------------------------------
		}
		//---------------------------------------------------
	}
	//---------------------------------------------------------

	File_Size_Current	= File_Size_Mas[DispFilePos];

	//if ( File_Size_Current < 524289 )

	if ( (Result_1==0) || (Result_2==0) )
	{
		memset((void*)&SwNewName,0x00,sizeof(SwNewName));
		Value_int16_t = 0;
	}

}


// Рекурсивна функція для пошуку .BIN файлів з повним шляхом
static void Find_BIN_Files_In_Dir(const char *base_path) {
    DIR dir;
    FILINFO fno;
    char path[128];

    if (f_opendir(&dir, base_path) != FR_OK)
        return;

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
        // Пропустити "." і ".."
        if (fno.fname[0] == '.' && (fno.fname[1] == 0 || (fno.fname[1] == '.' && fno.fname[2] == 0)))
            continue;

        snprintf(path, sizeof(path), "%s/%s", base_path, fno.fname);

        if (fno.fattrib & AM_DIR) {
            // Рекурсивно увійти в підкаталог
            Find_BIN_Files_In_Dir(path);
        } else {
            char *ext = strrchr(fno.fname, '.');
            if (ext && (strcasecmp(ext, ".bin") == 0)) {
                if (File_Number_Counter < MAX_FILES) {
                    // Зберегти повний шлях до файлу
                    const char *relative_path = (strncmp(path, "0:/", 3) == 0)? path + 3 : path;
                    strncpy(UsbFileName_For_Display[File_Number_Counter], relative_path, sizeof(UsbFileName_For_Display[0]) - 1);
                    UsbFileName_For_Display[File_Number_Counter][sizeof(UsbFileName_For_Display[0]) - 1] = '\0';

                    strcpy(Name_For_File_Open[File_Number_Counter], fno.altname);
                    File_Size_Mas[File_Number_Counter] = fno.fsize;
                    File_Number_Counter++;
                }
            } 
        }
    }

    f_closedir(&dir);
}

uint8_t Try_Finde_BIN_File(void) {
    File_Number_Counter = 0;

    // Очистити старі шляхи
    for (uint8_t i = 0; i < MAX_FILES; i++) {
        memset(UsbFileName_For_Display[i], 0x00, sizeof(UsbFileName_For_Display[i]));
    }

    if (f_mount(&USBDISKFatFs, "0:", 0) == FR_OK) {
        Find_BIN_Files_In_Dir("0:");
    }

    return (File_Number_Counter > 0) ? 1 : 0;
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

#ifdef  USE_FULL_ASSERT
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
