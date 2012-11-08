/*
 * relay.c
 *
 *  Created on: 2012.10.22.
 *      Author: Gábor
 */

#include "relay.h"


/*
 * @brief	Initializes the GPIO pins for the relay output, and switches off
 * @note	The GPIO pins and relay assignment are the below
 * 			PC13 - REL1
 * 			PB9 - REL2
 * 			PB8 - REL3
 * 			PB4 - REL4
 * 			PD2 - REL5
 * @param	None
 * @retval	None
 */
void RELAY_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	/* Enable the clock source for the GPIO ports */
	RCC_AHB1PeriphClockCmd(
			RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD,
			ENABLE);

	/* Set PB4, PB8, and PB9 to slow pull-down output (REL4, REL3, REL2) */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_8 | GPIO_Pin_9;

	GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* Set PC13 to slow pull-down output (REL1) */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;

	GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* Set PD2 to slow pull-down output (REL5) */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;

	GPIO_Init(GPIOD, &GPIO_InitStruct);

	/* Reset all bits */
	GPIO_ResetBits(GPIOB, GPIO_Pin_4 | GPIO_Pin_8 | GPIO_Pin_9);
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	GPIO_ResetBits(GPIOD, GPIO_Pin_2);
}

void RELAY_SetMode(RELAYMode_TypeDef Mode, RELAYFanSpeed_TypeDef FanSpeed)
{
	switch(FanSpeed)
	{
	case RELAY_FanSpeed_OFF:
		GPIO_ResetBits(RELAY_FAN1_PORT, RELAY_FAN1_PIN);
		GPIO_ResetBits(RELAY_FAN2_PORT, RELAY_FAN2_PIN);
		GPIO_ResetBits(RELAY_FAN3_PORT, RELAY_FAN3_PIN);
		break;

	case RELAY_FanSpeed_Slow:
		GPIO_ResetBits(RELAY_FAN2_PORT, RELAY_FAN2_PIN);
		GPIO_ResetBits(RELAY_FAN3_PORT, RELAY_FAN3_PIN);
		GPIO_SetBits(RELAY_FAN1_PORT, RELAY_FAN1_PIN);
		break;

	case RELAY_FanSpeed_Normal:
		GPIO_ResetBits(RELAY_FAN1_PORT, RELAY_FAN1_PIN);
		GPIO_ResetBits(RELAY_FAN3_PORT, RELAY_FAN3_PIN);
		GPIO_SetBits(RELAY_FAN2_PORT, RELAY_FAN2_PIN);
		break;

	case RELAY_FanSpeed_Fast:
		GPIO_ResetBits(RELAY_FAN1_PORT, RELAY_FAN1_PIN);
		GPIO_ResetBits(RELAY_FAN2_PORT, RELAY_FAN2_PIN);
		GPIO_SetBits(RELAY_FAN3_PORT, RELAY_FAN3_PIN);
		break;

	default:
		GPIO_ResetBits(RELAY_FAN1_PORT, RELAY_FAN1_PIN);
		GPIO_ResetBits(RELAY_FAN2_PORT, RELAY_FAN2_PIN);
		GPIO_ResetBits(RELAY_FAN3_PORT, RELAY_FAN3_PIN);
		while(1);
		break;
	}

	GPIO_ResetBits(RELAY_COLDWATER_PORT, RELAY_COLDWATER_PIN);
	GPIO_SetBits(RELAY_HOTWATER_PORT, RELAY_HOTWATER_PIN);
}
