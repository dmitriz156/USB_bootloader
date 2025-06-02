#include "main.h"

#include "display.h"
#include "menu.h"

#include <stdio.h>
#include <string.h>


extern TIM_HandleTypeDef htim6;
extern USBSTAT USB_Status_For_Display;

char* DispIntToStr(uint16_t data, uint8_t add);

uint32_t Operate_Led_Counter;

uint16_t UART_TX_counter = 0;
uint16_t Button_handler_counter = 0;

uint16_t DispTout=2;			// display timeout connection, sec

extern int16_t Value_int16_t;


#define DISP_PAUSE_TMR		25	// ms
#define DISP_SHIFT_TMR		80	

#define SetListSelLine(a)			DispUart.txBuff[DISP_PLD0_SELLINE]=a
#define SetListValueEdit(a)		DispUart.txBuff[DISP_PLD0_VALUE_EDIT]=a
#define SetListValueExist(a)	DispUart.txBuff[DISP_PLD0_VALUE_EXIST]=a	
#define SetListSymbMode(a)		DispUart.txBuff[DISP_PLD0_SYMB_MODE]=a	
#define SetListSymbL(a)				DispUart.txBuff[DISP_LISTMSG_SYMB_L]=a	
#define SetListSymbR(a)				DispUart.txBuff[DISP_LISTMSG_SYMB_R]=a	
#define SetListLineHide()			DispUart.txBuff[DISP_PLD0_LINE_HIDE]=DISP_LIST_LINE_HIDE
#define SetListLineShow()			DispUart.txBuff[DISP_PLD0_LINE_HIDE]=DISP_LIST_LINE_SHOW
#define SetListName(name)				strncpy((void*)&DispUart.txBuff[DISP_LISTMSG_STR0], name,	DISP_LISTPARAM_LEN)
#define SetListNameAdd(name)		strncat((void*)&DispUart.txBuff[DISP_LISTMSG_STR0], name,	DISP_LISTPARAM_LEN)
#define SetListParam(param)			strncpy((void*)&DispUart.txBuff[DISP_LISTMSG_STR0], param,	DISP_LISTPARAM_LEN)	
#define SetListParamAdd(param)	strncat((void*)&DispUart.txBuff[DISP_LISTMSG_STR0], param,	DISP_LISTPARAM_LEN-3)	

#define GetListPos(a)						(Menu.linePos+(DispUart.txPackPnt-a))

DispUartTypeDef DispUart;

MenuTypeDef Menu;

/**
  * @brief  Convert integer value into string
						value range  0..999 (3digits)
  * @param  u16 data 	- unsigned value
						u8 add 		- additional char in the end of string
  * @retval none
  */
char* DispIntToStr(uint16_t data, uint8_t add)
{
	#define DIG_NUM			3					// 
	#define DIV					100				// DIG_NUM-1
	#define DIG_BUFF		(DIG_NUM+1)			// + add
	static char dig[DIG_BUFF];
	uint8_t indx=0;
	uint8_t printEn=0;
	uint8_t len=DIG_NUM-1;
	uint32_t div10=DIV;
	
	memset((void*)&dig,0x00,DIG_BUFF);
	
	while(len)
	{
		dig[indx]=(data/div10)+'0';
		data%=div10;
		if((dig[indx]!='0')||printEn)
		{
			printEn=1; 
			indx++;
		}
		else;		
		len--;
		div10/=10;
	}
	dig[indx++]=data+'0';
		
	if(add) dig[indx++]=add;
	
	return &dig[0];
}


void ButtonHandler()
{
	if ( Operate_Led_Counter==1   ) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);	   }
		if ( Operate_Led_Counter==20 ) { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);	 }

		Operate_Led_Counter++;

		if ( Operate_Led_Counter==40 ) { Operate_Led_Counter=0; }
		//-------------------------------------------------------------------------
		if ( (HAL_GPIO_ReadPin(BTN_DOWN_GPIO_Port, BTN_DOWN_Pin)==0) && (Buttons.DOWN_Flag==0) )
		{
				Buttons.DOWN_Flag = 1;
				Buttons.DOWN_Bit = 1;
		}
		if (HAL_GPIO_ReadPin(BTN_DOWN_GPIO_Port, BTN_DOWN_Pin)!=0)  {  Buttons.DOWN_Flag = 0;  }
		//-------------------------------------------------------------------------
		//-------------------------------------------------------------------------
		if ( (HAL_GPIO_ReadPin(BTN_UP_GPIO_Port, BTN_UP_Pin)==0) && (Buttons.UP_Flag==0) )
		{
				Buttons.UP_Flag = 1;
				Buttons.UP_Bit = 1;
		}
		if (HAL_GPIO_ReadPin(BTN_UP_GPIO_Port, BTN_UP_Pin)!=0)  {  Buttons.UP_Flag = 0;  }
		//-------------------------------------------------------------------------
		//-------------------------------------------------------------------------
		if ( (HAL_GPIO_ReadPin(BTN_R_GPIO_Port, BTN_R_Pin)==0) && (Buttons.RIGHT_Flag==0) )
		{
				Buttons.RIGHT_Flag = 1;
				Buttons.RIGHT_Bit = 1;
		}
		if (HAL_GPIO_ReadPin(BTN_R_GPIO_Port, BTN_R_Pin)!=0)  {  Buttons.RIGHT_Flag = 0;  }
		//-------------------------------------------------------------------------
		//-------------------------------------------------------------------------
		if ( (HAL_GPIO_ReadPin(BTN_L_GPIO_Port, BTN_L_Pin)==0) && (Buttons.LEFT_Flag==0) )
		{
				Buttons.LEFT_Flag = 1;
				Buttons.LEFT_Bit = 1;
		}
		if (HAL_GPIO_ReadPin(BTN_L_GPIO_Port, BTN_L_Pin)!=0)  {  Buttons.LEFT_Flag = 0;  }
		//-------------------------------------------------------------------------
			//============================================================================
			if ( Menu_Proces_Status == 0 )
			{
				//------------------------------------------------------------------------
				if ( Buttons.DOWN_Bit )
				{
						Buttons.DOWN_Bit=0;
						Menu_Proces_Status = 1;

						Finde_BIN_Files = 1;
				}
				//------------------------------------------------------------------------
				//------------------------------------------------------------------------
				if ( Buttons.UP_Bit )
				{
						Buttons.UP_Bit=0;
						HAL_NVIC_SystemReset();
				}
				//------------------------------------------------------------------------
				Buttons.RIGHT_Flag = 0;
				Buttons.RIGHT_Bit = 0;
				Buttons.LEFT_Flag = 0;
				Buttons.LEFT_Bit = 0;
			}
			//============================================================================
			//============================================================================
			if ( Menu_Proces_Status == 1 )
			{
				//------------------------------------------------------------------------
				if ( USB_Status_For_Menu_Item == 0 )
				{
						USB_Status_For_Display = USB_STAT_NO_USB;
						//-----------------------------
						if ( Buttons.RIGHT_Bit )
						{
								Buttons.RIGHT_Bit=0;
								//HAL_NVIC_SystemReset();
								USB_Status_For_Display = USB_STAT_SELECT_USB_MODE;
						}
						//-----------------------------
				}
				//------------------------------------------------------------------------
				//------------------------------------------------------------------------
				if ( USB_Status_For_Menu_Item == 1 )
				{
						USB_Status_For_Display = USB_STAT_NO_USB;
						//-----------------------------
						if ( Buttons.RIGHT_Bit )
						{
								Buttons.RIGHT_Bit=0;
								//HAL_NVIC_SystemReset();
								USB_Status_For_Display = USB_STAT_SELECT_USB_MODE;
						}
						//-----------------------------
				}
				//------------------------------------------------------------------------
				//------------------------------------------------------------------------
				if ( USB_Status_For_Menu_Item == 2 )
				{
						USB_Status_For_Display = USB_STAT_SELECT_USB_MODE;
						//-----------------------------
						if ( Buttons.RIGHT_Bit )
						{
								Buttons.RIGHT_Bit=0;
								//FLAFG STATUS
						}
						//-----------------------------
				}
				//------------------------------------------------------------------------
				//------------------------------------------------------------------------
				if (USB_Status_For_Menu_Item == 3)
				{
						USB_Status_For_Display = USB_STAT_FILESEL;

						//-----------------------------
						if ( Buttons.UP_Bit )
						{
								Buttons.UP_Bit=0;

								if ( DispFilePos>0 ) { DispFilePos--; }
						}
						//-----------------------------
						//-----------------------------
						if ( Buttons.DOWN_Bit )
						{
								Buttons.DOWN_Bit=0;

								if ( DispFilePos<(DispFileNum-1) ) { DispFilePos++; }
						}
						//-----------------------------
						//-----------------------------
						if ( Buttons.RIGHT_Bit )
						{
								Buttons.RIGHT_Bit=0;

								Menu_Proces_Status = 2;

								Read_Version_Allowed_Counter = 10;

								USB_Status_For_Display = USB_STAT_UPDATE;

						}
						//-----------------------------
						//-----------------------------
						if ( Buttons.LEFT_Bit )
						{
								Buttons.LEFT_Bit=0;
								HAL_NVIC_SystemReset();

								Menu_Proces_Status = 0;
								USB_Status_For_Display = USB_STAT_BOOT;

						}
						//-----------------------------
				}
				//------------------------------------------------------------------------
			}
			//============================================================================

			//============================================================================
			if ( Menu_Proces_Status == 2 )
			{
				//-----------------------------------------
				if (( Value_int16_t == 429 ) && ( File_Size_Current < (0xFFFFF - 0xC000) ) )
				{
						USB_Status_For_Display = USB_STAT_UPDATE;
						//-----------------------------
						if ( Buttons.UP_Bit )
						{
								Buttons.UP_Bit=0;

								Menu_Proces_Status = 1;
								USB_Status_For_Display = USB_STAT_FILESEL;

						}
						//-----------------------------
						//-----------------------------
						if ( Buttons.DOWN_Bit )
						{
								Buttons.DOWN_Bit=0;

								Firmware_Upgrase_Allowed_Counter = 10;

								USB_Status_For_Display = USB_STAT_PROC_ERASE;

								Menu_Proces_Status = 3;
						}
						//-----------------------------

				}
				else
				{
						USB_Status_For_Display = USB_STAT_PCBERR;

						//-----------------------------
						if ( Buttons.RIGHT_Bit )
						{
								Buttons.RIGHT_Bit=0;

								Menu_Proces_Status = 1;
								USB_Status_For_Display = USB_STAT_FILESEL;
						}
						//-----------------------------
				}
				//-----------------------------------------
			}

			if (Menu_Proces_Status == 3)
			{
				if ( Buttons.RIGHT_Bit )
				{
					Buttons.RIGHT_Bit=0;
					HAL_NVIC_SystemReset();
				}
			}
			//============================================================================
			//-----------------------------------------------------------------------------------
			if ( Read_Version_Allowed_Counter >  0 ) { Read_Version_Allowed_Counter--; }
			if ( Read_Version_Allowed_Counter == 1 ) { Read_Version_Allowed = 1; 			 }
			//-----------------------------------------------------------------------------------
			//-----------------------------------------------------------------------------------
			if ( Firmware_Upgrase_Allowed_Counter >  0 ) { Firmware_Upgrase_Allowed_Counter--; }
			if ( Firmware_Upgrase_Allowed_Counter == 1 ) { Firmware_Upgrase_Allowed = 1; 			 }
			//-----------------------------------------------------------------------------------
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART2)
	{
		UART_TX_counter = 5;
		DispUart.packTxCnt++;		// go to next packet
		DispUart.pauseTmr = 1;		// start timeout
		//HAL_UART_Transmit_DMA(&huart2, DispUart.txBuff, DISP_TX_BUFF);
	}
}


//   1 ms timer
//---------------------------------------------------------
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (UART_TX_counter){
    	UART_TX_counter--;
    } else {
		DispUart.pauseTmr = 0;
		HAL_UART_Transmit_DMA(&huart2, DispUart.txBuff, DISP_TX_BUFF);
	}

    if(Button_handler_counter){
    	Button_handler_counter--;
    } else {
    	Button_handler_counter = 5;
    	DispTask();
		ButtonHandler();
    }

}



//======================================================	
void DispInit(void)
{
		char String_L32_For_Disp_Init[32];	
		uint8_t Counter_For_Disp_Init;

		memset((void*)&DispUart,0x00,sizeof(DispUart));
		
		// prepare number of packet for each type of display pages
		DispUart.txPackNum[DISP_PAGE_HELLO]	= DISP_PAGE_HELLO_PACK_NUM;
		DispUart.txPackNum[DISP_PAGE_MAIN]	= DISP_PAGE_MAIN_PACK_NUM;
		DispUart.txPackNum[DISP_PAGE_LIST]	= DISP_PAGE_LIST_PACK_NUM;
		DispUart.txPackNum[DISP_PAGE_EMPTY]	= DISP_PAGE_EMPTY_PACK_NUM;
		DispUart.txPackNum[DISP_PAGE_UNDEF]	= DISP_PAGE_UNDEF_PACK_NUM;
		
		DispUart.pauseTmr=1;		// start of pause
		
		memset((void*)&Menu,0x00,sizeof(Menu));
		Menu.startTmr=DISP_START_TMR;
							
		strcpy(SwCurrName,	DispIntToStr( *(__IO uint16_t*) 0x0800C400, 0) );//0x08008400
		
		for ( Counter_For_Disp_Init = 0; Counter_For_Disp_Init<32; Counter_For_Disp_Init++ )
		{		
				String_L32_For_Disp_Init[ Counter_For_Disp_Init ] = *(__IO uint8_t*) (0x0800C402 + Counter_For_Disp_Init);//0x08008402
		}
		
		strcpy(SwCurrName,	String_L32_For_Disp_Init );
											
}
//======================================================	


/**
	* @brief  Fill system message from DispTask
	* @param 	u8 type 	- type of message from DISPSYSMSG
						str0...4 	- stings of message
						Length of str0...4 Must not exceed DISP_SYS_MSG_STR_LEN.
	* @retval None
	*/	
void MenuSysMsgFill(uint8_t type, char* str0, char* str1, char* str2, char* str3, char* str4)
{
	DispUart.txBuff[DISP_SYS_MSG_TYPE]=type;
	if(str0)	strncpy((void*)&DispUart.txBuff[DISP_SYS_MSG_STR0],str0,DISP_SYS_MSG_STR_LEN);	
	if(str1)	strncpy((void*)&DispUart.txBuff[DISP_SYS_MSG_STR1],str1,DISP_SYS_MSG_STR_LEN);	
	if(str2)	strncpy((void*)&DispUart.txBuff[DISP_SYS_MSG_STR2],str2,DISP_SYS_MSG_STR_LEN);							
	if(str3)	strncpy((void*)&DispUart.txBuff[DISP_SYS_MSG_STR3],str3,DISP_SYS_MSG_STR_LEN);							
	if(str4)	strncpy((void*)&DispUart.txBuff[DISP_SYS_MSG_STR4],str4,DISP_SYS_MSG_STR_LEN);							
}

void DispTask(void)
{
	char str0[DISP_LISTPARAM_LEN];
	char str1[DISP_LISTPARAM_LEN];
	char str2[DISP_LISTPARAM_LEN];
	char str3[DISP_LISTPARAM_LEN];
	char str4[DISP_LISTPARAM_LEN];
	uint16_t len;
	
	if(DispUart.pauseTmr)
	{
		DispUart.pauseTmr=0;
		DispUart.shiftTmr++;
		if(DispUart.shiftTmr>=DISP_SHIFT_TMR)
		{
			DispUart.shiftTmr=0;
			DispUart.shiftChar0++;
			DispUart.shiftChar1++;
		}
		else;
		
		if(Menu.startTmr)
		{
			Menu.pageIndx=MENU_PAGE_HELLO;
			Menu.sysMsg=MENU_SM_NO;
		}
		else
		{
			switch(USB_Status_For_Display)
			{
				case USB_STAT_BOOT:					// start bootloader? 								Buttons: NO-YES
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_BOOT;
					break;
				case USB_STAT_NO_USB:				// no USB-flash drive. 							Buttons: OK
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_NO_USB;
					break;
				case USB_STAT_SELECT_USB_MODE:					// select USB mod? 								Buttons: HOST-FLASH
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_SELECT_USB_MODE;
					break;
				case USB_STAT_NO_FILE:			// correct file is not found. 			Buttons: OK
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_NO_FILE;
					break;
				case USB_STAT_UPDATE:					// file is found. Update firmware?	Buttons: NO-YES
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_UPDATE;
					break;
				case USB_STAT_PROC_ERASE:		// processing. Sector erasing.			LIST PAGE
					Menu.pageIndx=MENU_PAGE_PROC;
					Menu.sysMsg=MENU_SM_NO;
					Menu.lineNum=MENU_PROC_NUM;
					Menu.linePos=0;
					break;				
				case USB_STAT_PROC_LOAD:		// processing. Sector writing.			LIST PAGE
					Menu.pageIndx=MENU_PAGE_PROC;
					Menu.sysMsg=MENU_SM_NO;
					Menu.lineNum=MENU_PROC_NUM;
					Menu.linePos=0;
					break;
				case USB_STAT_PROC_VERIF:		// processing. Sector verification	LIST PAGE	
					Menu.pageIndx=MENU_PAGE_PROC;
					Menu.sysMsg=MENU_SM_NO;
					Menu.lineNum=MENU_PROC_NUM;
					Menu.linePos=0;
					Menu.lineSel=USB_Status_For_Display-USB_STAT_PROC_ERASE;
					break;
				case USB_STAT_FILESEL:
					Menu.pageIndx=MENU_PAGE_FILE;
					Menu.sysMsg=MENU_SM_NO;				
					Menu.lineNum=DispFileNum;
					if((DispFilePos>=Menu.linePos)&&(DispFilePos<(Menu.linePos+DISP_LIST_LINE_MAX)))
					{
						Menu.lineSel=DispFilePos-Menu.linePos;
					}
					else if(DispFilePos<Menu.linePos)
					{
						Menu.linePos=DispFilePos;
						Menu.lineSel=0;
					}
					else // Menu.lineSel>=(Menu.linePos+DISP_LIST_LINE_MAX)
					{
						Menu.linePos+=(DispFilePos-(Menu.linePos+DISP_LIST_LINE_MAX)+1);
						Menu.lineSel=4;
					}
					break;
				case USB_STAT_UPDATE_OK:		// Firmware is updated successfully	Buttons: OK
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_UPDATE_OK;
					break;
				case USB_STAT_UPDATE_FAIL:	// Firmware updating failed.Repeat?	Buttons: NO-YES
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_UPDATE_FAIL;
					break;
				case USB_STAT_TRN_FAIL:			// Turnstile is not available				No buttons		
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_TRN_FAIL;
					break;
				
				case USB_STAT_PCBERR:
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_PCBERR;					
					break;

				case USB_STAT_TIMEOUT:
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_TIMOUT_ERROR;					
					break;
				
				default:
					Menu.pageIndx=MENU_PAGE_EMPTY;
					Menu.sysMsg=MENU_SM_INTERR;
					break;
			}
		}
		
// --- next packet		
		DispUart.txPackPnt++;
		if(DispUart.txPackPnt>=DispUart.txPackNum[DispUart.pageIndex])
			DispUart.txPackPnt=0;
		else;
		
		if(Menu.pageIndx<=MENU_PAGE_EMPTY)
			DispUart.pageIndex=Menu.pageIndx;
		else //MENU_PAGE_FILE
			DispUart.pageIndex=DISP_PAGE_LIST;
		
		// prepare display Tx buffer
		memset((void*)&DispUart.txBuff[0],0x00,DISP_TX_BUFF);
		
		// common register for all packages
		DispUart.txBuff[DISP_REG_PACKTYPE]=DispUart.txPackPnt;			// number of packet
		DispUart.txBuff[DISP_REG_PAGE]=DispUart.pageIndex;					// current Display page index

		// clear it before using in code below
		memset(str0,0x00,sizeof(str0));
		memset(str1,0x00,sizeof(str1));
		memset(str2,0x00,sizeof(str2));					
		memset(str3,0x00,sizeof(str3));
		memset(str4,0x00,sizeof(str4));		
		
		// current packet is system message
		if(DispUart.txPackPnt==DISP_PACK_SYS_MGS)	
		{
			// code(index) of system message
			switch(Menu.sysMsg)
			{
				// !!! WARNING !!!
				// DO NOT EXCEED 12 SYMBOLS FOR EACH STRING
				case MENU_SM_BOOT:					// start bootloader? 								Buttons: NO-YES
					MenuSysMsgFill(DISP_SYS_MSG_QUE, "START","BOOTLOADER","RBv.1.2 ?",0,0);
					break;
				case MENU_SM_SELECT_USB_MODE:
					MenuSysMsgFill(DISP_SYS_MSG_QUE, "ENABLE USB","FLASH DEVICE", "MODE?",0,0);
					break;
				case MENU_SM_NO_USB:				// no USB-flash drive. 							Buttons: OK
					MenuSysMsgFill(DISP_SYS_MSG_WRN_OK,	"USB-FLASH","IS NOT","CONNECTED!",0,0);
					break;	
				case MENU_SM_NO_FILE:				// correct file is not found. 			Buttons: OK
					MenuSysMsgFill(DISP_SYS_MSG_WRN_OK, "BIN-FILES","ARE NOT","FOUND!",0,0);
					break;	
				case MENU_SM_UPDATE:				// Update firmware?									Buttons: NO-YES
					len=strlen(SwCurrName);
					if(len<=DISP_SYS_MSG_STR_LEN)
						DispUart.shiftChar0=0;
					else if(DispUart.shiftChar0>(len-DISP_SYS_MSG_STR_LEN))
						DispUart.shiftChar0=0;
					else;
					strncpy(str0,&SwCurrName[DispUart.shiftChar0],DISP_SYS_MSG_STR_LEN);		
					
					len=strlen(SwNewName);
					if(len<=DISP_SYS_MSG_STR_LEN)
						DispUart.shiftChar1=0;
					else if(DispUart.shiftChar1>(len-DISP_SYS_MSG_STR_LEN))
						DispUart.shiftChar1=0;
					else;
					strncpy(str1,&SwNewName[DispUart.shiftChar1],DISP_SYS_MSG_STR_LEN);		
					
					MenuSysMsgFill(DISP_SYS_MSG_QUE,"CURRENT FW:",str0,"NEW FW:",str1,"UPLOAD?");
					break;	
				case MENU_SM_UPDATE_OK:			// Firmware is updated successfully	Buttons: OK
					MenuSysMsgFill(DISP_SYS_MSG_BIRD_OK,"FIRMWARE","UPLOAD","SUCCESSFULLY",0,0);
					break;	
				case MENU_SM_UPDATE_FAIL:		// Firmware updating failed.Repeat?	Buttons: NO-YES
					MenuSysMsgFill(DISP_SYS_MSG_WRN_NY,"FIRMWARE","UPLOAD","FAILED!","REPEAT","UPLOADING?");
					break;	
				case MENU_SM_TRN_FAIL:			// Turnstile is not available				No buttons
					MenuSysMsgFill(DISP_SYS_MSG_WRN,"FIRMWARE","ERROR!","TURNSTILE","BLOCKED!",0);
					break;	
				case MENU_SM_INTERR:				// internal error of programm
					MenuSysMsgFill(DISP_SYS_MSG_WRN,"INTERNAL","ERROR!","RESET","CONTROLLER!",0);
					break;	
				
				case MENU_SM_PCBERR:			// PCB code is failed
					MenuSysMsgFill(DISP_SYS_MSG_WRN_OK,"NEW FIRMWARE","IS NOT","SUITABLE","FOR PCB429!",0);
					break;


				case MENU_SM_TIMOUT_ERROR:			// Timeout Error
					MenuSysMsgFill(DISP_SYS_MSG_WRN_OK,"An Error has","occurred.","Please try","Update Again",0);
					break;
				
				
				
				case MENU_SM_NO:		// no message -> do nothing
					break;				
				default:						// undefined message
					MenuSysMsgFill(DISP_SYS_MSG_WRN,"SYSTEM","MESSAGE","IS NOT DEF.!","CHECK","CODE!");
					break;
			}
		}
		else;
		
		// clear it before using in code below...again
		memset(str0,0x00,sizeof(str0));
		memset(str1,0x00,sizeof(str1));
		memset(str2,0x00,sizeof(str2));					
		memset(str3,0x00,sizeof(str3));
		memset(str4,0x00,sizeof(str4));		
		
		// current menu page
		switch(Menu.pageIndx)
		{
// --- HELLO PAGE
			case MENU_PAGE_HELLO:
				switch(DispUart.txPackPnt)
				{
					// DISP_PACKTYPE_DATA_0 - common data
					case DISP_PACK_DATA_0:		
						// status of loading
						DispUart.txBuff[DISP_PHD0_TMR]=(Menu.startTmr);	
						DispUart.txBuff[DISP_PHD0_TMRMAX]=(DISP_START_TMR);		
						break;
					case DISP_PACK_STR_0:
						// use this string if it need
						strncpy((void*)&DispUart.txBuff[DISP_REG_CMN_NUM], "BOOTLOADER RBv.1.2",	DISP_LISTPARAM_LEN);
						break;						
					default:
						break;						
				}				
				break;
		
// --- MENU_PAGE_EMPTY	- do not print anything			
			case MENU_PAGE_EMPTY:
				break;

// --- MENU_PAGE_PROC
			case MENU_PAGE_PROC:
				switch(DispUart.txPackPnt)
				{
					// DISP_PACKTYPE_DATA_0 - common data
					case DISP_PACK_DATA_0:		
						SetListSelLine(Menu.lineSel);
						SetListValueEdit(0);
						SetListValueExist(DISP_LIST_VALUE_NO);
						SetListSymbMode(DISP_LIST_SYMB_L);
						SetListLineShow();
						break;

					// DISP_PACKTYPE_STR_0		- name of list (upper bar)
					case DISP_PACK_STR_0:
						SetListName("FIRMWARE UPLOADING:");
						break;
					case DISP_PACK_STR_1:
					case DISP_PACK_STR_2:
					case DISP_PACK_STR_3:
					case DISP_PACK_STR_4:
					case DISP_PACK_STR_5:
						if(GetListPos(DISP_PACK_STR_1)<Menu.lineNum)
						{
							SetListParam(MenuProcName[GetListPos(DISP_PACK_STR_1)]);			// name in line
							if((USB_Status_For_Display-USB_STAT_PROC_ERASE)==GetListPos(DISP_PACK_STR_1))
								SetListSymbL(DISP_LISTMSG_SYMB_RUN);
							else if((USB_Status_For_Display-USB_STAT_PROC_ERASE)>GetListPos(DISP_PACK_STR_1))
								SetListSymbL(DISP_LISTMSG_SYMB_CHECK_BIRD);
							else
								SetListSymbL(DISP_LISTMSG_SYMB_UNCHECK);
						}
						else;
						break;		
						
					default:
						break;						
				}					
				break;
// --- MENU_PAGE_FILE
			case MENU_PAGE_FILE:
				switch(DispUart.txPackPnt)
				{
					// DISP_PACKTYPE_DATA_0 - common data
					case DISP_PACK_DATA_0:		
						SetListSelLine(Menu.lineSel);
						SetListValueEdit(0);
						SetListValueExist(DISP_LIST_VALUE_NO);
						SetListSymbMode(DISP_LIST_SYMB_R);
						SetListLineShow();
						break;

					// DISP_PACKTYPE_STR_0		- name of list (upper bar)
					case DISP_PACK_STR_0:
						SetListName("SELECT FILE:");
						break;
					case DISP_PACK_STR_1:
					case DISP_PACK_STR_2:
					case DISP_PACK_STR_3:
					case DISP_PACK_STR_4:
					case DISP_PACK_STR_5:
						if(GetListPos(DISP_PACK_STR_1)<Menu.lineNum)
						{
							SetListParam(DispIntToStr(GetListPos(DISP_PACK_STR_1)+1,'.'));
							SetListParamAdd(UsbFileName_For_Display[GetListPos(DISP_PACK_STR_1)]);
							SetListSymbR(DISP_LISTMSG_SYMB_ARROW);
						}
						else;
						break;		
						
					default:
						break;						
				}						
				break;
			
			default:
				break;
		}
		
		if(DispUart.txPackPnt==DISP_PACK_DATA_0)
			DispUart.txBuff[DISP_PMD0_UARTTOUT]=DispTout;		// seconds	
		else;

		//HAL_UART_Transmit_DMA(&huart2, DispUart.txBuff, DISP_TX_BUFF);
		//DMA1_Stream6->CR |= ((uint32_t)0x00000001);	
		
		if(Menu.startTmr)
			Menu.startTmr--;
		else;		
	}
	else;
}

void DispTmr1ms(void)
{
	if(DispUart.pauseTmr)
		DispUart.pauseTmr++;
	else;
}

