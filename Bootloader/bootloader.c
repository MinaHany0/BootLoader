#include "bootloader.h"

/* Global Variable Declarations ----------------------------------------------*/
static uint8_t uartHostBuffer[HOST_BUFFER_LEN];

/* Static Software Interface Declarations ------------------------------------*/
static BL_StatusTypeDef BootLoader_Get_Version                    (uint8_t *hostBuffer);
static void             BootLoader_Get_Help                       (uint8_t *hostBuffer);
static void             BootLoader_Get_Chip_Identification_Number (uint8_t *hostBuffer);
static void             BootLoader_Read_Protection_Level          (uint8_t *hostBuffer);
static void             BootLoader_Jump_To_Address                (uint8_t *hostBuffer);
static void             BootLoader_Erase_Flash                    (uint8_t *hostBuffer);
static void             BootLoader_Memory_Write                   (uint8_t *hostBuffer);
static void             BootLoader_Enable_RW_Protection           (uint8_t *hostBuffer);
static void             BootLoader_Memory_Read                    (uint8_t *hostBuffer);
static void             BootLoader_Get_Sector_Protection_Status   (uint8_t *hostBuffer);
static void             BootLoader_Read_OTP                       (uint8_t *hostBuffer);
static void             BootLoader_Disable_RW_Protection					(uint8_t *hostBuffer);

static uint8_t verify_CRC(uint8_t *hostBuffer, uint8_t frameLength, uint32_t crcHost);
static BL_StatusTypeDef BootLoader_Send_ACK(uint8_t replyLen);
static BL_StatusTypeDef BootLoader_Send_NACK(void);
static BL_StatusTypeDef BootLoader_Send_To_Host(uint8_t *data, uint8_t dataLen);


/* Software Interface Defintions ---------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
BL_StatusTypeDef bootloader_Debug_Display(char* formatStr, ...)
{
	HAL_StatusTypeDef uartStatus= HAL_OK;
#if BOOTLOADER_DEBUGGING == ENABLED
	char sentence[100] = {0};
	uint8_t charCount = 0;
	
	va_list listarg;
	va_start(listarg, formatStr);
	
	charCount = vsprintf(sentence, formatStr, listarg);
	
	uartStatus |= HAL_UART_Transmit(BL_DEBUG_UART, (const unsigned char*)sentence, charCount, HAL_MAX_DELAY);
	va_end(listarg); 
#endif
	return (BL_StatusTypeDef)uartStatus;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Send_To_Host(uint8_t *data, uint8_t dataLen)
{
	HAL_StatusTypeDef uartStatus= HAL_OK;
	uartStatus |= HAL_UART_Transmit(BL_HOST_COMM_UART, data, dataLen , HAL_MAX_DELAY);
	return (BL_StatusTypeDef)uartStatus;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Send_ACK(uint8_t replyLen)
{
	/*	send two bytes : ACK and length of next reply frame from BL to host	*/
	HAL_StatusTypeDef uartStatus= HAL_OK;
	uint8_t BL_ACK_Var = BL_ACK;												
	uartStatus |= HAL_UART_Transmit(BL_HOST_COMM_UART, &BL_ACK_Var, 1 , HAL_MAX_DELAY);
	uartStatus |= HAL_UART_Transmit(BL_HOST_COMM_UART, &replyLen, 	1 , HAL_MAX_DELAY);
	return (BL_StatusTypeDef)uartStatus;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Send_NACK(void)
{
	HAL_StatusTypeDef uartStatus= HAL_OK;
	uint8_t nack = BL_NACK;
	uartStatus |= HAL_UART_Transmit(BL_HOST_COMM_UART, (const unsigned char*)&nack, 1 , HAL_MAX_DELAY);
	return (BL_StatusTypeDef)uartStatus;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static uint8_t verify_CRC(uint8_t *hostBuffer, uint8_t frameLength, uint32_t crcHost)
{
	uint8_t dataLength = frameLength - 4;																			/*	var for length of data	*/
	uint32_t calculatedCrc = 0;																								/*	var for calculated CRC Buffer	*/
				
	uint32_t *data = (uint32_t*)calloc(dataLength , sizeof(uint32_t));				/*	host data is uint8 , we need uint32, transform type to give to CRC function	*/
				
	for(int i = 0; i < dataLength; i++)	{ data[i] = hostBuffer[i]; }					/*	loop & save data in new data buffer	*/
				
	calculatedCrc = HAL_CRC_Accumulate(BL_CRC, data, dataLength);							/*	calculate CRC on data and return result in CRC buffer	*/
	__HAL_CRC_DR_RESET(BL_CRC);																								/*	remove accumulated data in CRC Buffer by resetting data register	*/
	free(data);																																/*	free allocated memory in heap	*/
	
	if( crcHost == calculatedCrc )	{ return CRC_VERIFIED; }									/*	return boolean of success or failure	*/
	else { return CRC_VERIFAILED; }							
}



/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
BL_StatusTypeDef bootloader_Receive_From_Host(void)
{
	/*	initialize bootloaderStatusVar, uartStatusVar , variable for message length, SID	*/
	BL_StatusTypeDef blStatus = BL_OK;
	HAL_StatusTypeDef uartStatus = HAL_OK;
	uint8_t dataLen = 0, SID = 0;
	
	/*	re-initialize the data buffer	*/
	memset(uartHostBuffer, 0, HOST_BUFFER_LEN);
	
	/*	receive one byte for number of incoming bytes after it	*/
	uartStatus |= HAL_UART_Receive(BL_HOST_COMM_UART, uartHostBuffer, 1, HAL_MAX_DELAY); 
	
	/*	if receiving failed, return error status	else store length of incoming bytes	*/
	if(uartStatus){ return BL_ERROR; }
	else { dataLen = uartHostBuffer[0];}
	
	/*	receive specified number of bytes in buffer	*/
	uartStatus |= HAL_UART_Receive(BL_HOST_COMM_UART, uartHostBuffer+1 , dataLen, HAL_MAX_DELAY); 
	
	/*	if receiving failed, return error status else send 2 frames */
	/*	1 frame for ACK + reply Length */
	/*	1 frame for actual reply according to SID */
	if(uartStatus){ return BL_ERROR; }
	SID = uartHostBuffer[1];
	switch(SID)
	{
		case( CBL_GET_VER_CMD						):
		
			blStatus |= BootLoader_Get_Version(uartHostBuffer);
			break;
		
		case( CBL_GET_HELP_CMD					):
			break;
		
		case( CBL_GET_CID_CMD						):
			break;
		
		case( CBL_GET_RDP_STATUS_CMD		):
			break;
		
		case( CBL_GO_TO_ADDR_CMD				):
			break;
		
		case( CBL_FLASH_ERASE_CMD				):
			break;
		
		case( CBL_MEM_WRITE_CMD					):
			break;
		
		case( CBL_EN_R_W_PROTECT_CMD		):
			break;
		
		case( CBL_MEM_READ_CMD					):
			break;
		
		case( CBL_READ_SECTOR_STATUS_CMD):
			break;
		
		default:
			break;		
	}
	blStatus = (BL_StatusTypeDef)uartStatus;
	return blStatus;
}




/* Static Software Interface Defintions --------------------------------------*/

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/


	
	
static BL_StatusTypeDef BootLoader_Get_Version(uint8_t *hostBuffer)
{
	BL_StatusTypeDef blStatus = BL_OK;
	
	/*	calculate frame length from first byte of frame and add one	*/
	uint8_t frameLength = hostBuffer[0] + 1;
	
	/*	extract CRC from Frame : it is the last 4 bytes	*/
	uint32_t crc_Host = *((uint32_t*)(hostBuffer + frameLength - 4));
	
	/*	array for bootloader reply --version data	*/
	uint8_t blVersionData[] = {	(uint8_t)BL_VENDOR_ID,
															(uint8_t)MAJOR_VERSION,
															(uint8_t)MINOR_VERSION,
															(uint8_t)PATCH_VERSION	};
		
	/*	verify CRC	*/
	if(verify_CRC(hostBuffer, frameLength, crc_Host) == CRC_VERIFIED)
	{								
		/*	send ACK and data, then send version data	*/
		blStatus |= BootLoader_Send_ACK( sizeof(blVersionData)/sizeof(blVersionData[0])  );
		blStatus |= BootLoader_Send_To_Host( blVersionData, 4 );
		
	}
	else 
	{
		/*	turn on LED after verification	*/
		LED_Turn_On(LED_ORANGE);
		
		
		/*	send NACK	*/
		blStatus |= BootLoader_Send_NACK();
	}
	return blStatus;	
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/