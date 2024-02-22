#include "led.h"

/* Global Variable Declarations ----------------------------------------------*/


/* Static Software Interface Declarations ------------------------------------*/


/* Software Interface Definitions ---------------------------------------------*/

void LED_Turn_On( uint16_t GPIO_Pin ){
	HAL_GPIO_WritePin(GPIOD, GPIO_Pin, GPIO_PIN_SET);
}
void LED_Turn_Off( uint16_t GPIO_Pin ){
	HAL_GPIO_WritePin(GPIOD, GPIO_Pin, GPIO_PIN_RESET);
}

/* Static Software Interface Defintions --------------------------------------*/

