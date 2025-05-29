
#include "stm32f4xx_hal.h"


typedef enum
{
	USB_STAT_BOOT=0,			// start bootloader? 								Buttons: NO-YES
	USB_STAT_SELECT_MODE,       // select USB mode Mass Storage Class or USB HOST
	USB_STAT_HOLD_FILE,			// hold file uploading in MSC mod
	USB_STAT_INVALID_FILE,		// worning massage about the invalid uploaded file
	USB_STAT_NO_USB,			// no USB-flash drive. 							Buttons: OK
	USB_STAT_NO_FILE,			// correct file is not found. 			Buttons: OK
	USB_STAT_UPDATE,			// Update firmware?									Buttons: NO-YES
	USB_STAT_PROC_ERASE,	// processing. Sector erasing.			LIST PAGE				
	USB_STAT_PROC_LOAD,		// processing. Sector writing.			LIST PAGE
	USB_STAT_PROC_VERIF,	// processing. Sector verification	LIST PAGE
	USB_STAT_UPDATE_OK,		// Firmware is updated successfully	Buttons: OK
	USB_STAT_UPDATE_FAIL,	// Firmware updating failed.Repeat?	Buttons: NO-YES
	USB_STAT_TRN_FAIL,		// Turnstile is not available				No buttons
	USB_STAT_PCBERR,			// PCB code is failed
	USB_STAT_FILESEL,			// selecting file list
	USB_STAT_NUM,
	USB_STAT_TIMEOUT	
} USBSTAT;


// !!! This file is common for two projects:
// 				- BldcControl
// 				- OledDisplay
// !!! Edit it only under BdlcControl project - this is parent project for this file

// -- Display buttons
#define DISP_BTN_L			0
#define DISP_BTN_R			1
#define DISP_BTN_UP			2
#define DISP_BTN_DN			3
	#define DISP_BTN_PUSH				0			// button is push just now
	#define DISP_BTN_SHRT				1			// button is released -> short push
	#define DISP_BTN_LONG				2			// button is still pushed -> long push event
	#define DISP_BTN_LONG_R			3			// button is released after long push event
	
#define DISP_TX_BUFF 	64			// bytes

typedef enum		// type of message (label in message)
{
	DISP_SYS_MSG_NO=0,		// No message
	// message WITHOUT buttons 
	DISP_SYS_MSG_WRN,			// Blink. 		Message with warning-lable. 								
	DISP_SYS_MSG_BIRD,		// Blink. 		Success (bird-lable) message. 							
	DISP_SYS_MSG_NOMARK,	// NO Blink.	Message without marks
	// message WITH buttons 
		// 1 button
		DISP_SYS_MSG_WRN_OK,		// Blink. 		Message with warning-lable.				OK button
		DISP_SYS_MSG_BIRD_OK,		// Blink. 		Success (bird-lable) message.			OK button	
		DISP_SYS_MSG_INFO_OK,		// NO Blink. 	Message with INFO marks,					OK button
		DISP_SYS_MSG_INFO_BACK,	// NO Blink. 	Message with INFO marks,					BACK button
		// 2 button
		DISP_SYS_MSG_WRN_NY,			// Blink. 		Message with warning-lable.									NO-YES buttons
		DISP_SYS_MSG_WRN_OKINFO,	// Blink.			Message with warning-lable.									OK-INFO buttons
		DISP_SYS_MSG_SETT,				// NO Blink. 	Message with arrows pointed in the center. 	BACK-OK buttons
		DISP_SYS_MSG_QUE,					// Blink. 		Question-lable message. 										NO-YES buttons
}DISPSYSMSG;

typedef enum		// button in message
{
	DISP_SYS_MSG_BTN_NO=0,		// no button
	DISP_SYS_MSG_BTN_NOYES,
	DISP_SYS_MSG_BTN_BACKOK,
	DISP_SYS_MSG_BTN_OK,
}DISPSYSMSGBTN;

#define DISP_BTN_OPENA	0
#define DISP_BTN_OPENB	1
#define DISP_BTN_CLOSE	2
#define DISP_BTN_SAFETY	3

typedef enum
{
	DISP_PAGE_HELLO=0,	// PH - start page
	DISP_PAGE_MAIN,			// PM - main page
	DISP_PAGE_LIST,			// PL - list pages
	DISP_PAGE_EMPTY,		// empty page
	DISP_PAGE_UNDEF,		// undefined list
	DISP_PAGE_NUM
}DISPPAGE;

#define DISP_PAGE_HELLO_PACK_NUM		3
#define DISP_PAGE_MAIN_PACK_NUM			5
#define DISP_PAGE_LIST_PACK_NUM			8
#define DISP_PAGE_EMPTY_PACK_NUM		2
#define DISP_PAGE_UNDEF_PACK_NUM		1

typedef struct
{
	uint8_t txBuff[DISP_TX_BUFF];				// tx buffer for display
	uint8_t txPackNum[DISP_PAGE_NUM];		// amount of tx pack for display for each type of display page
	uint8_t txPackPnt;										// current tx-pack point
	uint8_t pageIndex;										// active page of display
	uint16_t packTxCnt;									// counter of transmitted packet (for debug)
	uint16_t pauseTmr;										// pause after each transmitting
	uint16_t pauseTmrMax;								// max value of pause
	uint16_t shiftTmr;
	uint8_t shiftChar0;
	uint8_t shiftChar1;
} DispUartTypeDef;

// common register
typedef enum
{
	DISP_REG_PACKTYPE=0,	// type of packet
	DISP_REG_PAGE,				// page index
	DISP_REG_CMN_NUM
}DISPREG;
#define DISP_PAGE_MSK				0x3F
#define DISP_PAGE_M_SHARE		0x80			// master is sharing screen of slave
#define DISP_PAGE_S_SHARE		0x40			// and vice versa
#define PageIndx	(Uart.rxOledData0[DISP_REG_PAGE]&DISP_PAGE_MSK)

typedef enum
{	
	DISP_PACK_DATA_0=0,			// main packet
	DISP_PACK_SYS_MGS,			// system message
	// string packets
	DISP_PACK_STR_0,
	DISP_PACK_STR_1,
	DISP_PACK_STR_2,
	DISP_PACK_STR_3,
	DISP_PACK_STR_4,
	DISP_PACK_STR_5,
	DISP_PACK_STR_6,
	DISP_PACK_NUM
}DISPPACKTYPE;

// --- DISP_PACKTYPE_SYS_MESSAGE - common packet for all page
	#define DISP_SYS_MSG_STR_LEN			12
	typedef enum
	{
		DISP_SYS_MSG_TYPE=DISP_REG_CMN_NUM,
		DISP_SYS_MSG_STR0,
		DISP_SYS_MSG_STR1=DISP_SYS_MSG_STR0+DISP_SYS_MSG_STR_LEN,
		DISP_SYS_MSG_STR2=DISP_SYS_MSG_STR1+DISP_SYS_MSG_STR_LEN,
		DISP_SYS_MSG_STR3=DISP_SYS_MSG_STR2+DISP_SYS_MSG_STR_LEN,
		DISP_SYS_MSG_STR4=DISP_SYS_MSG_STR3+DISP_SYS_MSG_STR_LEN,
	}DISP_SYSMSG;

// --- LIST MESSAGE - common packet for all page
	#define DISP_LIST_LINE_MAX			5		// maximum value of lines (string) per page
	// allowed length of string
	#define DISP_LISTPARAM_LEN			32		// allowed length of parameter name
	#define DISP_LISTVALUE_LEN			16		// allowed length of parameter value
	#define DISP_LIST_TTL_LEN_VIS		20		// visible length of list name (in case of NoValueExist)
	#define DISP_LISTPARAM_LEN_VIS	14		// visible length of parameter name (in case of ValueExist)
	#define DISP_LISTVALUE_LEN_VIS	6			// visible length of parameter value (in case of ValueExist)
	
	// DISP_PACKTYPE_DATA_0 - common data
	#define DISP_LIST_VALUE_NO			0		// list without values
	#define DISP_LIST_VALUE_YES			1		// list with values
		#define DISP_LIST_SYMB_NO				0		// no symbol
		#define DISP_LIST_SYMB_L				1		// symbol stays left of parameter name
		#define DISP_LIST_SYMB_R				2		// symbol stays right of parameter name
		#define DISP_LIST_SYMB_LR				3
	#define DISP_LIST_LINE_HIDE			1
	#define DISP_LIST_LINE_SHOW			0
	#define DISP_LIST_GEAR_HIDE			0
	#define DISP_LIST_GEAR_SHOW			1
	typedef enum
	{
		DISP_PLD0_SELLINE=DISP_REG_CMN_NUM,		// selected line in the list
		DISP_PLD0_VALUE_EDIT,									// is parameter under edit?
		DISP_PLD0_VALUE_EXIST,								// is parameter exist?
		DISP_PLD0_SYMB_MODE,									// symbol position
		DISP_PLD0_LINE_HIDE,									// hide line
		DISP_PLD0_GEAR_SHOW,									// gear - settings symbol
	}DISP_PLD0_MODE;		
	// typ of symbols
	typedef enum
	{	
		DISP_LISTMSG_SYMB_NO=0,
		DISP_LISTMSG_SYMB_LOCK,
		DISP_LISTMSG_SYMB_ARROW,
		DISP_LISTMSG_SYMB_CHECK_BIRD,
		DISP_LISTMSG_SYMB_CHECK_FILL,
		DISP_LISTMSG_SYMB_UNCHECK,
		DISP_LISTMSG_SYMB_FAIL,
		DISP_LISTMSG_SYMB_RUN,
		DISP_LISTMSG_SYMB_INFO,
		DISP_LISTMSG_SYMB_QUE,
		DISP_LISTMSG_SYMB_SYNC,		
	}DISP_LISTSYMB;
	// registers for list message
	// DISP_PACKTYPE_STR_0		- name of list (upper bar)
	// DISP_PACKTYPE_STR_1		- list string 0
	// ......
	// DISP_PACKTYPE_STR_5		- list string 4
	typedef enum
	{
		DISP_LISTMSG_SYMB_L=DISP_REG_CMN_NUM,													// 
		DISP_LISTMSG_SYMB_R,
		DISP_LISTMSG_STR0,																						// parameter name
		DISP_LISTMSG_STR1=DISP_LISTMSG_STR0+DISP_LISTPARAM_LEN,	// parameter value
	}DISP_LISTMSG;
	#define ListValueExist			Uart.rxOledData0[DISP_PLD0_VALUE_EXIST]
	#define	ListValueEdit				Uart.rxOledData0[DISP_PLD0_VALUE_EDIT]
	#define ListLine						Uart.rxOledData0[DISP_PLD0_SELLINE]
	#define ListSymbExist				Uart.rxOledData0[DISP_PLD0_SYMB_MODE]
	#define ListLineHide				Uart.rxOledData0[DISP_PLD0_LINE_HIDE]
	#define ListGearShow				Uart.rxOledData0[DISP_PLD0_GEAR_SHOW]
	
// --- DISP_PAGE_HELLO registers
	// Used only DISP_PACKTYPE_DATA_0
	typedef enum
	{
		DISP_PHD0_TMR=DISP_REG_CMN_NUM,		// init timer
		DISP_PHD0_TMRMAX,							// init timer max
	}DISP_RP_HELLO;

// --- DISP_PAGE_MAIN registers
	// DISP_PACKTYPE_DATA_0
	typedef enum
	{
		DISP_PMD0_COMM=DISP_REG_CMN_NUM,	// command of movement
		DISP_PMD0_POSCURR,								// current position 
		DISP_PMD0_POSMIN,
		DISP_PMD0_POSMAX,
		DISP_PMD0_POSMIDDLE,
		DISP_PMD0_POS_FLAG,								// flag near position indication
		// additional parameter
		DISP_PMD0_AP_EN,			// enabled
		DISP_PMD0_AP_POS,			// progress bar position
		DISP_PMD0_AP_MAX,			// progress bar max value
		DISP_PMD0_AP_OL,			// overload
		// display contrast
		DISP_PMD0_CONTRAST,
		// cotroller in BootMode
		DISP_PMD0_UARTTOUT,		// sec, !!! for all Pages
	}DISP_PMD0_MAIN;
	#define AdParamEn							Uart.rxOledData0[DISP_PMD0_AP_EN]
	#define AdParamPos						Uart.rxOledData0[DISP_PMD0_AP_POS]
	#define AdParamMax						Uart.rxOledData0[DISP_PMD0_AP_MAX]
	#define AdParamOL							Uart.rxOledData0[DISP_PMD0_AP_OL]
	#define DispContrast					Uart.rxOledData0[DISP_PMD0_CONTRAST]
	#define DispUartTout					Uart.rxOledData0[DISP_PMD0_UARTTOUT]		
		#define DIST_UART_TOUT_MAX	60		//sec
	
	// DISP_PMD0_COMM
		// command bellow corresponde to GATECOMM
	#define DISP_PMD0_COMM_NO					0
	#define DISP_PMD0_COMM_MIDDLE			1		// gate is move in the middle of range
	#define DISP_PMD0_COMM_MIN				2		// gate is move in the min position
	#define DISP_PMD0_COMM_MAX				3		// gate is move in the max position
	// DISP_PMD0_POS_FLAG
	#define DISP_PMD0_POS_FLAG_NO			0		// no additional indication near current position window
	#define DISP_PMD0_POS_FLAG_CCW		1		// move forward
	#define DISP_PMD0_POS_FLAG_CW 		2		// move backward
	// display contrast
	#define DISP_PMD0_CONTRAST_LOW	5		
	#define DISP_PMD0_CONTRAST_HIGH		0xFF		
	
	#define DISP_PMS0_BTN_LEN			5		// length of buttons text
	#define DISP_PMS0_BTN_NUM			5
	#define DISP_PMS0_APN_LEN			15	// additinal parameter name length
	#define DISP_PMS0_APD_LEN			8		// additinal parameter data length

	// DISP_PACKTYPE_STR_0
	typedef enum
	{
		DISP_PMS0_BTN1=DISP_REG_CMN_NUM,
		DISP_PMS0_BTN2=DISP_PMS0_BTN1+DISP_PMS0_BTN_LEN,
		DISP_PMS0_BTN3=DISP_PMS0_BTN2+DISP_PMS0_BTN_LEN,
		DISP_PMS0_BTN4=DISP_PMS0_BTN3+DISP_PMS0_BTN_LEN,
		DISP_PMS0_BTN5=DISP_PMS0_BTN4+DISP_PMS0_BTN_LEN
	}DISP_PMS0_MAIN;
	#define DISP_PMS0_AP_NAME 	DISP_PMS0_BTN1
	#define DISP_PMS0_AP_DATA 	(DISP_PMS0_AP_NAME+DISP_PMS0_APN_LEN)
	
	#define DISP_PMS1_POS_LEN					8		// length of position buttons
	// DISP_PACKTYPE_STR_1
	typedef enum
	{
		DISP_PMS1_POS_CURR=DISP_REG_CMN_NUM,
		DISP_PMS1_POS_A=DISP_PMS1_POS_CURR+DISP_PMS1_POS_LEN,
		DISP_PMS1_POS_B=DISP_PMS1_POS_A+DISP_PMS1_POS_LEN,
		DISP_PMS1_POS_C=DISP_PMS1_POS_B+DISP_PMS1_POS_LEN,
		DISP_PMS1_NAME_A=DISP_PMS1_POS_C+DISP_PMS1_POS_LEN,
		DISP_PMS1_NAME_B=DISP_PMS1_NAME_A+DISP_PMS1_POS_LEN,
		DISP_PMS1_NAME_C=DISP_PMS1_NAME_B+DISP_PMS1_POS_LEN
	}DISP_PMS1_MAIN;
	// DISP_PACKTYPE_STR_2
	typedef enum
	{
		DISP_PMS2_ABOVE=DISP_REG_CMN_NUM,
		DISP_PMS2_BELOW_L=DISP_PMS2_ABOVE+DISP_LIST_TTL_LEN_VIS,
		DISP_PMS2_BELOW_R=DISP_PMS2_BELOW_L+DISP_LIST_TTL_LEN_VIS/2+1,
	}DISP_PMS2_MAIN;	

extern DispUartTypeDef DispUart;

void DispInit(void);
void DispTask(void);
void DispTmr1ms(void);
void DispTmr1sec(void);
void DispPushBtn(uint8_t btn, uint8_t mode);
uint8_t DispGetGateCtrl(void);
void DispWriteLog(uint8_t gr, uint8_t code);	
void DispPushGateCtrlBtn(uint8_t btn, uint8_t mode);
