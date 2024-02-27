#include "bootloader.h"

/* Global Variable Declarations ----------------------------------------------*/

static uint8_t uartHostBuffer[HOST_BUFFER_LEN];
void jump_To_Application(void);

/* Static Software Interface Declarations ------------------------------------*/
static BL_StatusTypeDef BootLoader_Get_Version                    (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Get_Help                       (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Get_Chip_Identification_Number (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Read_Protection_Level          (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Jump_To_Address                (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Erase_Flash                    (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Memory_Write                   (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Enable_RW_Protection           (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Memory_Read                    (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Get_Sector_Protection_Status   (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Read_OTP                       (uint8_t *hostBuffer);
static BL_StatusTypeDef BootLoader_Disable_RW_Protection					(uint8_t *hostBuffer);

static uint8_t verify_CRC(uint8_t *hostBuffer, uint8_t frameLength, uint32_t crcHost);
static uint8_t verify_Address(uint32_t hostAddress);
static uint8_t verify_Application_Doesnot_Overwrite_Bootloader(uint32_t hostAddress);
static uint8_t write_Application_on_Flash(uint32_t host_Start_Address, uint16_t data_Number_Bytes, uint8_t *pToStartData)	;
static void jump_To_Address(uint32_t hostAddress);
static uint8_t verify_Sectors(uint8_t sector, uint8_t numberOfSectors);
static uint8_t verify_Mass_Erase(uint8_t sector, uint8_t numberOfSectors);
static uint8_t verify_Accurate_Remaining_Sectors(uint8_t sector, uint8_t numberOfSectors);
static BL_StatusTypeDef BootLoader_Send_ACK(uint8_t replyLen);
static BL_StatusTypeDef BootLoader_Send_NACK(void);
static BL_StatusTypeDef BootLoader_Send_To_Host(uint8_t *data, uint8_t dataLen);

/* Software Interface Defintions ---------------------------------------------*/
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
			blStatus |= BootLoader_Get_Help(uartHostBuffer);
			break;
		
		case( CBL_GET_CID_CMD						):
			blStatus |= BootLoader_Get_Chip_Identification_Number(uartHostBuffer);
			break;
		
		case( CBL_GET_RDP_STATUS_CMD		):
			blStatus |= BootLoader_Read_Protection_Level(uartHostBuffer);
			break;
		
		case( CBL_GO_TO_ADDR_CMD				):
			blStatus |= BootLoader_Jump_To_Address(uartHostBuffer);
			break;
		
		case( CBL_FLASH_ERASE_CMD				):
			blStatus |= BootLoader_Erase_Flash(uartHostBuffer);
			break;
		
		case( CBL_MEM_WRITE_CMD					):
			blStatus |= BootLoader_Memory_Write(uartHostBuffer);
			break;
		
		case( CBL_EN_R_W_PROTECT_CMD		):
			break;
		
		case( CBL_MEM_READ_CMD					):
			break;
		
		case( CBL_READ_SECTOR_STATUS_CMD):
			break;
		
		default:
#ifdef SWO_DEBUGGING
		printf("Wrong input Default switch case \r\n");
#endif
			break;		
	}
	blStatus = (BL_StatusTypeDef)uartStatus;
	
#ifdef BootLoader_LED_STATUS_Debugging
if(blStatus)
	LED_Turn_On(LED_RED);
#endif

	return blStatus;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
BL_StatusTypeDef bootloader_Debug_Display(char* formatStr, ...)
{
	HAL_StatusTypeDef uartStatus= HAL_OK;
#if BOOTLOADER_DEBUGGING == ENABLED
	char sentence[300] = {0};
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
static uint8_t verify_CRC(uint8_t *hostBuffer, uint8_t frameLength, uint32_t crcHost)
{
	uint16_t dataLength = frameLength - 4;																		/*	var for length of data	*/
	uint32_t calculatedCrc = 0;																								/*	var for calculated CRC Buffer	*/
				
	uint32_t data = 0;																												/*	host data is uint8 , we need uint32, transform type to give to CRC function	*/
				
	for(int i = 0; i < dataLength; i++)	
	{ 
		data = (uint32_t)(hostBuffer[i]);
		calculatedCrc = HAL_CRC_Accumulate(BL_CRC, &data, 1);										/*	calculate CRC on data and return result in CRC buffer	*/
	}	
	
	__HAL_CRC_DR_RESET(BL_CRC);																								/*	remove accumulated data in CRC Buffer by resetting data register	*/																								/*	free allocated memory in heap	*/
#ifdef SWO_DEBUGGING
	printf("CRC Sent by Host is:      0x%X\r\n", crcHost);
	printf("CRC Calculated by MCU is: 0x%X\r\n", calculatedCrc);
#endif
	if( crcHost == calculatedCrc )	{ return CRC_VERIFIED; }									/*	return boolean of success or failure	*/
	else { return CRC_VERIFAILED; }							
}


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static uint8_t verify_Address(uint32_t hostAddress)
{
	uint8_t isAddressVerified = ADDRESS_VERIFAILED;

	if(ADD_FLASH_START <= hostAddress && hostAddress <= ADD_FLASH_END)
	{
		isAddressVerified = ADDRESS_VERIFIED;
	}
	else if(ADD_CCM_START <= hostAddress && hostAddress <= ADD_CCM_END)
	{
		isAddressVerified = ADDRESS_VERIFIED;
	}
	else if(ADD_SRAM16KB_START <= hostAddress && hostAddress <= ADD_SRAM16KB_END)
	{
		isAddressVerified = ADDRESS_VERIFIED;
	}
	else if(ADD_SRAM112KB_START <= hostAddress && hostAddress <= ADD_SRAM112KB_END)
	{
		isAddressVerified = ADDRESS_VERIFIED;
	}
	else
	{
		isAddressVerified = ADDRESS_VERIFAILED;
	}
	
	return isAddressVerified;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static void jump_To_Address(uint32_t hostAddress)
{
	jumpToAddressFunction jump = (jumpToAddressFunction)hostAddress;
	jump();
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static uint8_t verify_Sectors(uint8_t sector, uint8_t numberOfSectors)
{
	if(sector >= 0 && sector <= 11 && numberOfSectors <= 12)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static uint8_t verify_Mass_Erase(uint8_t sector, uint8_t numberOfSectors)
{
	if( sector == 0 && numberOfSectors == 12 )
		return 1;
	else
		return 0;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static uint8_t verify_Accurate_Remaining_Sectors(uint8_t sector, uint8_t numberOfSectors)
{
	if(sector + numberOfSectors > 12)
	{
		return (12 - sector);
	}
	else
	{
		return numberOfSectors;
	}
}



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
#ifdef SWO_DEBUGGING
		printf("==========================================================================================\r\n");
		printf( "Bootloader Get Version Number\r\n");
#endif
	/*	verify CRC	*/
	if(verify_CRC(hostBuffer, frameLength, crc_Host) == CRC_VERIFIED)
	{					
#ifdef SWO_DEBUGGING
		printf( "CRC Verified Passed \r\n");
#endif		
		/*	send ACK and data, then send version data	*/
		blStatus |= BootLoader_Send_ACK( sizeof(blVersionData)/sizeof(blVersionData[0])  );
		blStatus |= BootLoader_Send_To_Host( blVersionData, 4 );
		
	}
	else 
	{
#ifdef SWO_DEBUGGING
		printf( "CRC Verified Failed \r\n");
#endif
		/*	send NACK	*/
		blStatus |= BootLoader_Send_NACK();
	}
	
#ifdef BootLoader_LED_STATUS_Debugging
if(blStatus)
	LED_Turn_On(LED_RED);
#endif
	
	return blStatus;	
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Get_Help(uint8_t *hostBuffer)
{
	BL_StatusTypeDef blStatus = BL_OK;
	
	/*	calculate frame length from first byte of frame and add one	*/
	uint8_t frameLength = hostBuffer[0] + 1;
#ifdef SWO_DEBUGGING
		printf("==========================================================================================\r\n");
		printf( "Bootloader Get Help Menu \r\n");
#endif
	
	/*	extract CRC from Frame : it is the last 4 bytes	*/
	uint32_t crc_Host = *((uint32_t*)(hostBuffer + frameLength - 4));
	
	/*	array for commands 	*/
	uint8_t bootLoaderCommandsArray[12] = {
																CBL_GET_VER_CMD							
																, CBL_GET_HELP_CMD						
																, CBL_GET_CID_CMD							
																, CBL_GET_RDP_STATUS_CMD			
																, CBL_GO_TO_ADDR_CMD					
																, CBL_FLASH_ERASE_CMD					
																, CBL_MEM_WRITE_CMD						
																, CBL_EN_R_W_PROTECT_CMD			
																, CBL_MEM_READ_CMD						
																, CBL_READ_SECTOR_STATUS_CMD	
																, CBL_OTP_READ_CMD						
																, CBL_CHANGE_ROP_LEVEL_CMD		
																};
																
	/*	verify CRC	*/
	if( CRC_VERIFIED == verify_CRC( hostBuffer, frameLength, crc_Host) )
	{
#ifdef SWO_DEBUGGING
		printf( "CRC Verified Passed \r\n");
#endif
		
		/*	send ACK and data, then a list of bootloader commands that acts as a help menu*/
		blStatus |= BootLoader_Send_ACK( 12 );
		blStatus |= BootLoader_Send_To_Host(bootLoaderCommandsArray, 12);
	}	
	else
	{
#ifdef SWO_DEBUGGING
		printf( "CRC Verified Failed \r\n");
#endif
		
		/*	send NACK	*/
		blStatus |= BootLoader_Send_NACK();
	}
	
#ifdef BootLoader_LED_STATUS_Debugging
	if(blStatus)
			LED_Turn_On(LED_RED);
#endif
												
	return blStatus;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Get_Chip_Identification_Number (uint8_t *hostBuffer)
{
	BL_StatusTypeDef blStatus = BL_OK;
	
	/*	calculate frame length from first byte of frame and add one	*/
	uint8_t frameLength = hostBuffer[0] + 1;
	
	/*	extract CRC from Frame : it is the last 4 bytes	*/
	uint32_t CRC_Host = *((uint32_t*)(hostBuffer + frameLength - 4));
	
	/* chip Identification number fetched from certain CPU address in Macro	*/
	uint16_t chip_Id_Data = ID_CODE;\
	
#ifdef SWO_DEBUGGING
	printf("==========================================================================================\r\n");
	printf( "Bootloader Get Chip Identification Number\r\n");
#endif
	
	/*	verify CRC	*/
	if(CRC_VERIFIED == verify_CRC(hostBuffer, frameLength, CRC_Host))
	{
#ifdef SWO_DEBUGGING
		printf( "CRC Verified Passed \r\n");
#endif
		
		/*	send ACK and data, then send chip identification number data	*/
		blStatus |= BootLoader_Send_ACK(2);
		blStatus |= BootLoader_Send_To_Host( (uint8_t*)(&chip_Id_Data), 2);
	}
	else
	{
#ifdef SWO_DEBUGGING
		printf( "CRC Verified Failed \r\n");
#endif
		/*	send NACK	*/
		blStatus |= BootLoader_Send_NACK();
	}

#ifdef BootLoader_LED_STATUS_Debugging
	if(blStatus)
		LED_Turn_On(LED_RED);
#endif
	
	return blStatus;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Jump_To_Address(uint8_t *hostBuffer)
{
	BL_StatusTypeDef blStatus = BL_OK;
	
	/*	calculate frame length from first byte of frame and add one	*/
	uint8_t frameLength = hostBuffer[0] + 1;
	
	/*	extract CRC from Frame : it is the last 4 bytes	*/
	uint32_t crc_Host = *((uint32_t*)(hostBuffer + frameLength - 4));
	
	/*	extract address from host frame	*/
	uint32_t hostDesiredAddress = *((uint32_t *)(&hostBuffer[2]));
	
	uint8_t isAddressVerified = ADDRESS_VERIFAILED;
	
	if(verify_CRC(hostBuffer, frameLength, crc_Host) == CRC_VERIFIED)
	{							
			/*	send ACK and data, then send version data	*/
			blStatus |= BootLoader_Send_ACK( 1 );
			
			/*	check validity of Address	*/
			isAddressVerified = verify_Address(hostDesiredAddress);
			if(ADDRESS_VERIFIED == isAddressVerified)
			{
				blStatus |= BootLoader_Send_To_Host( &isAddressVerified, 1 );
				jump_To_Address(hostDesiredAddress + THUMB_INSTRUCTION_ADDITIVE );
			}
			else
			{
				blStatus |= BootLoader_Send_To_Host( &isAddressVerified, 1 );
			}
		
	}
	else 
	{
		/*	send NACK	*/
		blStatus |= BootLoader_Send_NACK();
	}
	
#ifdef BootLoader_LED_STATUS_Debugging
	if(blStatus)
			LED_Turn_On(LED_RED);
#endif	
	
	return blStatus;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Erase_Flash(uint8_t *hostBuffer)
{
	BL_StatusTypeDef blStatus = BL_OK;
	HAL_StatusTypeDef halStatus = HAL_OK;
	uint32_t sectorErrorStatus = 0xAA;
	uint8_t EraseProcessResult;
	uint8_t frameLength;
	uint32_t crc_Host;
	uint8_t isSectorVerified;
	
#ifdef SWO_DEBUGGING
	printf("==========================================================================================\r\n");
	printf("Bootloader Erase Flash \r\n");
#endif
	/*	calculate frame length from first byte of frame and add one	*/
	frameLength = hostBuffer[0] + 1;
	
	/*	extract CRC from Frame : it is the last 4 bytes	*/
	crc_Host = *((uint32_t*)(hostBuffer + frameLength - 4));
	
		/*	verify CRC	*/
	if( CRC_VERIFIED == verify_CRC( hostBuffer, frameLength, crc_Host) )
	{
#ifdef SWO_DEBUGGING
		printf("CRC Verification Pased \r\n");
#endif		
		/*	send ACK and data, then a list of bootloader commands that acts as a help menu*/
		blStatus |= BootLoader_Send_ACK( 12 );
		
#ifdef SWO_DEBUGGING
		printf("sector Number is %d , Number of sectors is %d \r\n",hostBuffer[2], hostBuffer[3] );
#endif	
		
		/*	Verify Sector Numbers are in Actual Range	*/
		isSectorVerified = verify_Sectors(hostBuffer[2], hostBuffer[3]);
		
		/*	verify reamining Sectors doesnot exceed last sector, else the function returns a maximum correct number	*/
		hostBuffer[3] = verify_Accurate_Remaining_Sectors(hostBuffer[2], hostBuffer[3]);
#ifdef SWO_DEBUGGING
		printf("Actual Accurate Remaining Sectors is %d\r\n" ,hostBuffer[3] );
#endif		
		if(isSectorVerified)
		{
			
			/*	struct for configuration param of erase function	*/
			FLASH_EraseInitTypeDef erase_cfg;
			erase_cfg.Banks = FLASH_BANK_1;
			erase_cfg.VoltageRange = FLASH_VOLTAGE_RANGE_3;
			erase_cfg.Sector = hostBuffer[2];
			
			uint8_t isThisMassErase = verify_Mass_Erase(hostBuffer[2], hostBuffer[3]);
			if(isThisMassErase)
			{
#ifdef SWO_DEBUGGING
				printf("This Erase is a mass erase\r\n");
#endif	
				erase_cfg.TypeErase = FLASH_TYPEERASE_MASSERASE;
			}
			else
			{
#ifdef SWO_DEBUGGING
				printf("This Erase is sector erase\r\n");
#endif	
				erase_cfg.TypeErase = FLASH_TYPEERASE_SECTORS;
			}
			
			/*	unlock flash for erasing sectors	*/
			halStatus |= HAL_FLASH_Unlock();
#ifdef SWO_DEBUGGING
			if(0 == halStatus)
				printf("HAL Status after unlocking flash is OK \r\n");
			else 
				printf("HAL Status after unlocking flash is ERROR \r\n");
#endif			
			
			halStatus |= HAL_FLASHEx_Erase( &erase_cfg, &sectorErrorStatus);
#ifdef SWO_DEBUGGING
			if(0 == halStatus)
				printf("HAL Status after erasing flash is OK \r\n");
			else 
				printf("HAL Status after erasing flash is ERROR \r\n");
			printf("Sector Status before Erase = 0xAA, After Erase = 0x%X\r\n", sectorErrorStatus);
#endif	
			/*	unlock flash for erasing sectors	*/
			halStatus |= HAL_FLASH_Lock();
#ifdef SWO_DEBUGGING
			if(0 == halStatus)
				printf("HAL Status after locking flash is OK \r\n");
			else 
				printf("HAL Status after locking flash is ERROR \r\n");
#endif				
			
			
			/*	return erase status to user	as true(0x03) / false(0x02)	*/
			if(sectorErrorStatus == 0xFFFFFFFFU)
			{
				EraseProcessResult = 0x03;
			}
			else
			{
				EraseProcessResult = 0x02;
			}
			
			/*	send result back to host	*/
			blStatus |= BootLoader_Send_To_Host( &EraseProcessResult , 1);
#ifdef SWO_DEBUGGING
			printf("Send to host: Erase Process Result is %d \r\n", EraseProcessResult);
#endif	
		}
		else
		{
			EraseProcessResult = 0x02;
			
			/*	send result back to host	*/
			blStatus |= BootLoader_Send_To_Host( &EraseProcessResult , 1);
			
#ifdef SWO_DEBUGGING
			printf("Sectors Not Verified Send to host: Erase Process Result is %d \r\n", EraseProcessResult);
#endif
		}
		
	}	
	else
	{
#ifdef SWO_DEBUGGING
	printf("CRC Verification Failed \r\n");
#endif
		
		/*	send NACK	*/
		blStatus |= BootLoader_Send_NACK();
	}
	
#ifdef BootLoader_LED_STATUS_Debugging
	if(blStatus)
			LED_Turn_On(LED_RED);
	if(halStatus)
			LED_Turn_On(LED_ORANGE);
	if(sectorErrorStatus != 0xFFFFFFFFU)
			LED_Turn_On(LED_BLUE);
#endif
												
	return blStatus;
}
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static uint8_t Bootloader_CRC_Verify(uint8_t *pData, uint32_t Data_Len, uint32_t Host_CRC);
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Memory_Write(uint8_t *hostBuffer)
{
	BL_StatusTypeDef blStatus = BL_OK;
	uint8_t isAddressVerified = ADDRESS_VERIFAILED;
	uint8_t writingStatus = WRITING_SUCCESS;
#ifdef SWO_DEBUGGING
	printf("==========================================================================================\r\n");
	printf("Bootloader Write Flash \r\n");
#endif
	/*	calculate frame length from first byte of frame and add one	*/
	uint16_t frameLength = hostBuffer[0] + 1;
	
	/*	extract CRC from Frame : it is the last 4 bytes	*/
	uint32_t crc_Host = *((uint32_t*)(hostBuffer + frameLength - 4));
#ifdef SWO_DEBUGGING
	printf("Order of CRC byte in frame is %d\r\n", frameLength - 4 );
	printf("CRC Sent from HOST is 0x%X\r\n", crc_Host);
#endif
	
	/*	extract address from host frame	*/
	uint32_t hostDesiredAddress = *((uint32_t *)(&hostBuffer[2]));
	
	/*	how many bytes stored in this frame to be written in memory	*/
	uint16_t hostNumberOfBytesToWrite = hostBuffer[6];
	
	/*	start address of data to be written	*/
	uint8_t *pToData = &(hostBuffer[7]);

	
	/*	verify CRC	*/	
	uint8_t calculatedCRC = verify_CRC(hostBuffer, frameLength, crc_Host);
	
	if( calculatedCRC == CRC_VERIFIED) 
	{			
#ifdef SWO_DEBUGGING
			printf("Send ACK to Host \r\n");
#endif			
			/*	send ACK and data, then send version data	*/
			blStatus |= BootLoader_Send_ACK( 1 );
		
		
			/*	check validity of Address	*/
			isAddressVerified = verify_Address(hostDesiredAddress);
			if(ADDRESS_VERIFIED == isAddressVerified)
			{
#ifdef SWO_DEBUGGING
				printf("Send to Host Address Verification Success \r\n");
#endif
				blStatus |= BootLoader_Send_To_Host( &isAddressVerified, 1 );
				
				/*	verify no overwriting happens between bootloader and application	*/
				uint8_t overwrite = verify_Application_Doesnot_Overwrite_Bootloader(hostDesiredAddress);
				
				if(OVERWRITE_NEGATIVE == overwrite)
				{
#ifdef SWO_DEBUGGING
					printf("Send to Host Overwrite Negative \r\n");
#endif
					writingStatus = write_Application_on_Flash(hostDesiredAddress, hostNumberOfBytesToWrite, pToData)	;
				}
				else
				{
#ifdef SWO_DEBUGGING
					printf("Send to Host Overwrite Positive \r\n");
#endif
					writingStatus = WRITING_FAILURE;				
					blStatus |= BootLoader_Send_To_Host( &writingStatus, 1 );
				}
			}
			else
			{
#ifdef SWO_DEBUGGING
				printf("Send to Host Address Verification Failed \r\n");
#endif
				writingStatus = WRITING_FAILURE;				
				blStatus |= BootLoader_Send_To_Host( &writingStatus, 1 );
			}
	}
	else 
	{
#ifdef SWO_DEBUGGING
		printf("Send to Host NACK \r\n");
#endif
		/*	send NACK	*/
		blStatus |= BootLoader_Send_NACK();
	}
	
	return blStatus;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static uint8_t verify_Application_Doesnot_Overwrite_Bootloader(uint32_t hostAddress)
{
	uint8_t isAddressInBootloader = OVERWRITE_NEGATIVE;
	if(hostAddress < 0x08008000 )
	{
		isAddressInBootloader = OVERWRITE_POSITIVE;
	}
	return isAddressInBootloader;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static uint8_t write_Application_on_Flash(uint32_t host_Start_Address, uint16_t data_Number_Bytes, uint8_t *pToStartData)	
{	
	uint8_t writingStatus = WRITING_SUCCESS;
	HAL_StatusTypeDef halStatus = HAL_OK;
	
	/*	unlock flash for erasing sectors	*/
	halStatus |= HAL_FLASH_Unlock();
#ifdef SWO_DEBUGGING
	if(0 == halStatus)
		printf("HAL Status after unlocking flash is OK \r\n");
	else 
		printf("HAL Status after unlocking flash is ERROR \r\n");
#endif			
	
	/*	write on flash	*/
	for(uint16_t i = 0; i < data_Number_Bytes; i+=4)
	{
		halStatus |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, host_Start_Address+i, *( (uint32_t*)(pToStartData+ i) ) );
		if(halStatus)
		{
#ifdef SWO_DEBUGGING
			printf("Flash Writing failed on WORD number %d\r\n", i);
#endif	
			break;
		}
	}
#ifdef SWO_DEBUGGING
	if(0 == halStatus)
		printf("HAL Status after writing on flash is OK \r\n");
	else 
		printf("HAL Status after writing on flash is ERROR \r\n");
#endif	
	
	/*	lock flash after writing sectors	*/
	halStatus |= HAL_FLASH_Lock();
#ifdef SWO_DEBUGGING
	if(0 == halStatus)
		printf("HAL Status after locking flash is OK \r\n");
	else 
		printf("HAL Status after locking flash is ERROR \r\n");
#endif	
	
	if(halStatus)
	{
#ifdef SWO_DEBUGGING
		printf("Writing Failed Return failure \r\n");
#endif	
		writingStatus = WRITING_FAILURE;
	}
	else
	{
#ifdef SWO_DEBUGGING
		printf("Writing Success Return Success \r\n");
#endif	
		writingStatus = WRITING_SUCCESS;
	}
	
	return writingStatus;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
static BL_StatusTypeDef BootLoader_Read_Protection_Level(uint8_t *hostBuffer)
{
	BL_StatusTypeDef blStatus = BL_OK;
	FLASH_OBProgramInitTypeDef Flash_RDB_Cfg;
	uint8_t RDB_Value = 0x11;
	
	/*	calculate frame length from first byte of frame and add one	*/
	uint8_t frameLength = hostBuffer[0] + 1;
	
	/*	extract CRC from Frame : it is the last 4 bytes	*/
	uint32_t crc_Host = *((uint32_t*)(hostBuffer + frameLength - 4));
	
#ifdef SWO_DEBUGGING
		printf("==========================================================================================\r\n");
		printf( "Bootloader Get Flash Protection Level Number\r\n");
#endif
	/*	verify CRC	*/
	if(verify_CRC(hostBuffer, frameLength, crc_Host) == CRC_VERIFIED)
	{					
#ifdef SWO_DEBUGGING
		printf( "CRC Verified Passed \r\n");
#endif		
		/*	send ACK and data, then send read protection data	*/
		blStatus |= BootLoader_Send_ACK( 1  );
		
		/*	read flash Option bytes configuration	*/
		HAL_FLASHEx_OBGetConfig(&Flash_RDB_Cfg);
		
		/*	send flash RDB to host	*/
		RDB_Value = (uint8_t)Flash_RDB_Cfg.RDPLevel;
#ifdef SWO_DEBUGGING
		printf( "Option Bytes Protection Level is %d \r\n", RDB_Value);
#endif	
		blStatus |= BootLoader_Send_To_Host( &RDB_Value ,1 );
		
	}
	else 
	{
#ifdef SWO_DEBUGGING
		printf( "CRC Verified Failed \r\n");
#endif
		/*	send NACK	*/
		blStatus |= BootLoader_Send_NACK();
	}
	
#ifdef BootLoader_LED_STATUS_Debugging
if(blStatus)
	LED_Turn_On(LED_RED);
#endif
	
	return blStatus;	
}
