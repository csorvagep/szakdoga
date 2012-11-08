/*
 * rotary.c
 *
 *  Created on: 2012.10.09.
 *      Author: Gábor
 */

#include "rotary.h"
#include "stm32f2xx.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

static int16_t LastValue = 0;
int16_t DeltaValue = 0;

void ROT_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	TIM_TimeBaseInitTypeDef TIM_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;
	int8_t FirstValue;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	/* Set push button, and rotaryA and B as input */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	//Set timer for rotary
	TIM_InitStruct.TIM_Prescaler = 0;
	TIM_InitStruct.TIM_Period = (uint16_t) ((SystemCoreClock / 2) / 1000) - 1;
	TIM_InitStruct.TIM_ClockDivision = 0;
	TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Down;

	TIM_TimeBaseInit(TIM2, &TIM_InitStruct);
	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;

	NVIC_Init(&NVIC_InitStruct);

	TIM_Cmd(TIM2, ENABLE);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource0 );

	EXTI_InitStruct.EXTI_Line = EXTI_Line0;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;

	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 2;

	NVIC_Init(&NVIC_InitStruct);

	//Read initial value
	FirstValue = 0;
	if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2 ))
		FirstValue = 3;
	if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1 ))
		FirstValue ^= 1;
	LastValue = FirstValue;
}

void TIM2_IRQHandler(void)
{
	int16_t NewValue, DiffValue;

	if(TIM_GetITStatus(TIM2, TIM_IT_Update ))
	{
		NewValue = 0;
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2 ))
			NewValue = 3;
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1 ))
			NewValue ^= 1;
		DiffValue = LastValue - NewValue;
		if(DiffValue & 1)
		{
			LastValue = NewValue;
			DeltaValue += (DiffValue & 2) - 1;
		}
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update );
	}
}

extern xQueueHandle xQueueMenu;
extern xTimerHandle xTimerRotaryAllow;
void EXTI0_IRQHandler(void)
{
	static portBASE_TYPE xHigherPriorityTaskWoken;
	static signed short sToSend = 0;
	NVIC_InitTypeDef NVIC_InitStruct;

	if(EXTI_GetITStatus(EXTI_Line0 ))
	{
		xQueueSendToBackFromISR(xQueueMenu, &sToSend,
				&xHigherPriorityTaskWoken);

		xTimerResetFromISR( xTimerRotaryAllow, &xHigherPriorityTaskWoken );

		NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
		NVIC_InitStruct.NVIC_IRQChannelCmd = DISABLE;
		NVIC_Init(&NVIC_InitStruct);

		EXTI_ClearFlag(EXTI_Line0 );

		portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
	}
}
