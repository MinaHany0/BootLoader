#ifndef  BOOTLOADER_H__
#define	 BOOTLOADER_H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "usart.h"
#include "crc.h"
#include "led.h"

/* Macro Declarations---------------------------------------------------------*/
#define ENABLED 1
#define DISABLED 0
#define BOOTLOADER_DEBUGGING				ENABLED

#define CBL_GET_VER_CMD							0x10
#define CBL_GET_HELP_CMD						0x11
#define CBL_GET_CID_CMD							0x12
#define CBL_GET_RDP_STATUS_CMD			0x13
#define CBL_GO_TO_ADDR_CMD					0x14
#define CBL_FLASH_ERASE_CMD					0x15
#define CBL_MEM_WRITE_CMD						0x16
#define CBL_EN_R_W_PROTECT_CMD			0x17
#define CBL_MEM_READ_CMD						0x18
#define CBL_READ_SECTOR_STATUS_CMD	0x19


#define BL_VENDOR_ID								0x15
#define MAJOR_VERSION								0x07
#define MINOR_VERSION								0x06
#define PATCH_VERSION								0x02

#define CRC_VERIFIED								1
#define CRC_VERIFAILED							0

#define BL_ACK											0xCD
#define BL_NACK											0xAB

/*	MACRO to enable or disable debugging prints 	*/
#define BootLoader_Debugging				BootLoader_Debugging

/* HW modules assigned to the bootloader	*/
#define BL_HOST_COMM_UART 					&huart2	
#define BL_DEBUG_UART 							&huart3		
#define BL_CRC &hcrc	

/*	MACRO for size of host buffer	*/
#define HOST_BUFFER_LEN							100

/*	typedef for BootLoader error status	*/
typedef enum{	
	BL_OK,
	BL_ERROR,
}BL_StatusTypeDef;


/* Macro Functions------------------------------------------------------------*/

/* Software Interface Decalarations ------------------------------------------*/
			

BL_StatusTypeDef bootloader_Debug_Display(char* str, ...);
BL_StatusTypeDef bootloader_Receive_From_Host(void);


/* Static Function Declarations ----------------------------------------------*/

#endif /*BOOTLOADER_H__*/
