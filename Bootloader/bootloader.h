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
#define CBL_OTP_READ_CMD						0x20
#define CBL_CHANGE_ROP_LEVEL_CMD		0x21


#define BL_VENDOR_ID								0x15
#define MAJOR_VERSION								0x07
#define MINOR_VERSION								0x06
#define PATCH_VERSION								0x02

#define CRC_VERIFIED								1
#define CRC_VERIFAILED							0

#define BL_ACK											0xCD
#define BL_NACK											0xAB

/*	naming conventions for verifying address received from host	*/
#define ADDRESS_VERIFIED						1
#define ADDRESS_VERIFAILED					0

#define ADD_FLASH_START							0x08000000
#define ADD_FLASH_END               0x080FFFFF
#define ADD_CCM_START               0x10000000
#define ADD_CCM_END                 0x1000FFFF
#define ADD_SRAM16KB_START          0x2001C000
#define ADD_SRAM16KB_END            0x2001FFFF
#define ADD_SRAM112KB_START         0x20000000
#define ADD_SRAM112KB_END           0x2001BFFF
#define THUMB_INSTRUCTION_ADDITIVE	0x01
typedef void (*jumpToAddressFunction)(void);

/*	naming conventions for erasing the flash function	*/
#define SECTORS_VALIDATED						0x01
#define SECTORS_NOT_VALIDATED				0x00
#define ERASE_OK										0xFFFFFFFFU
#define MASS_ERASE_TRUE							0x01
#define MASS_ERASE_FALSE						0x00


/*	naming conventions for writing the flash function	*/
#define OVERWRITE_POSITIVE					0x01
#define OVERWRITE_NEGATIVE					0x00
#define WRITING_FAILURE							0x00
#define WRITING_SUCCESS							0x01
#define SECTOR2_START_ADDRESS				0x08008000						

/*	MACRO to enable or disable debugging prints 	*/
#define BootLoader_Debugging				BootLoader_Debugging
#define SWO_DEBUGGING								SWO_DEBUGGING
/*	MACRO to enable or disable red LED Status Debugging	*/
#define BootLoader_LED_STATUS_Debugging				BootLoader_LED_STATUS_Debugging

/* HW modules assigned to the bootloader	*/
#define BL_HOST_COMM_UART 					&huart2	
#define BL_DEBUG_UART 							&huart3		
#define BL_CRC &hcrc	

/*	MACRO for size of host buffer	*/
#define HOST_BUFFER_LEN							300

/*	typedef for BootLoader error status	*/
typedef enum{	
	BL_OK,
	BL_ERROR,
}BL_StatusTypeDef;

/*	Address for Identication Data Address	*/
#define DBGMCU_IDCODE			      		((*((volatile uint32_t*)0xE0042000)))   
/*	ID Code for this Device is 	*/	
#define ID_CODE											((uint16_t)(DBGMCU_IDCODE & 0x00000FFF))

/* Macro Functions------------------------------------------------------------*/

/* Software Interface Decalarations ------------------------------------------*/
			

BL_StatusTypeDef bootloader_Debug_Display(char* str, ...);
BL_StatusTypeDef bootloader_Receive_From_Host(void);

void jump_To_Application(void);

/* Static Function Declarations ----------------------------------------------*/

#endif /*BOOTLOADER_H__*/
