/*
 * relay.h
 *
 *  Created on: 2012.10.22.
 *      Author: Gábor
 */

#ifndef RELAY_H_
#define RELAY_H_

#include "stm32f2xx.h"

#define RELAY_HOTWATER_PIN		GPIO_Pin_13
#define RELAY_HOTWATER_PORT		GPIOC

#define RELAY_COLDWATER_PIN		GPIO_Pin_9
#define RELAY_COLDWATER_PORT	GPIOB

#define RELAY_FAN1_PIN			GPIO_Pin_8
#define RELAY_FAN1_PORT			GPIOB

#define RELAY_FAN2_PIN			GPIO_Pin_4
#define RELAY_FAN2_PORT			GPIOB

#define RELAY_FAN3_PIN			GPIO_Pin_2
#define RELAY_FAN3_PORT			GPIOD

#define RELAY_Heat(FanSpeed)	RELAY_SetMode(RELAY_Mode_Heat, (FanSpeed))
#define RELAY_Cool(FanSpeed)	RELAY_SetMode(RELAY_Mode_Cool, (FanSpeed))
#define RELAY_OFF()				RELAY_SetMode(RELAY_Mode_OFF, RELAY_FanSpeed_OFF)


typedef enum
{
	RELAY_FanSpeed_OFF		= 0x00,
	RELAY_FanSpeed_Slow		= 0x01,
	RELAY_FanSpeed_Normal	= 0x02,
	RELAY_FanSpeed_Fast		= 0x03
} RELAYFanSpeed_TypeDef;

typedef enum
{
	RELAY_Mode_Heat			= 0x00,
	RELAY_Mode_Cool			= 0x01,
	RELAY_Mode_OFF				= 0x02
} RELAYMode_TypeDef;

void RELAY_Init(void);
void RELAY_SetMode(RELAYMode_TypeDef Mode, RELAYFanSpeed_TypeDef FanSpeed);

#endif /* RELAY_H_ */
