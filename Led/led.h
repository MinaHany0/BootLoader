#ifndef  LED_H__
#define	 LED_H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "gpio.h"

/* Macro Declarations---------------------------------------------------------*/

#define LED_GREEN				GPIO_PIN_12
#define LED_ORANGE			GPIO_PIN_13
#define LED_RED					GPIO_PIN_14
#define LED_BLUE				GPIO_PIN_15


/* Macro Functions------------------------------------------------------------*/


/* Software Interface Decalarations ------------------------------------------*/

void LED_Turn_On( uint16_t GPIO_Pin );
void LED_Turn_Off( uint16_t GPIO_Pin );

			
/* Static Function Declarations ----------------------------------------------*/

#endif /*LED_H__*/
