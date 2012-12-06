/*
 * adc.c
 *
 *  Created on: 2012.10.23.
 *      Author: Gábor
 */

#include "adc.h"

/*
 * @brief	Initializes the SPI3 and the connected GPIO pins
 * @param	None
 * @retval	None
 */
void EADC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	uint32_t i;

	/* Enable the SPI3, SYCFG and GPIO clock source */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	/* Set the MOSI, MISO SCK pins to alternate-function, push-pull, no push, high speed */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;

	GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* Connect the SPI3 to the specified GPIO pins */
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3 );
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3 );
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3 );

	/* Set the EADC_SEL pin to push-pull, pull-up, high-speed, output */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;

	GPIO_Init(GPIOA, &GPIO_InitStruct);

	SPI_StructInit(&SPI_InitStruct);
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;

	SPI_Init(SPI3, &SPI_InitStruct);
	SPI_Cmd(SPI3, ENABLE);

	/* Set the EADC_INT pin to push-pull, pull-up, input */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource8);

	EXTI_InitStruct.EXTI_Line = EXTI_Line8;
	EXTI_InitStruct.EXTI_LineCmd = DISABLE;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 6;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStruct);

	/* Wait for device startup */
	while(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8 ))
		;
	while(!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8 ))
		;

	/* Wait the empty TX flag */
	while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE ) == RESET)
		;

	/* Send self calibration command */
	SPI_I2S_SendData(SPI3, EADC_COMMAND_SELFOCAL);
	while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE ))
		;

	/* Wait some to calibrate */
	for(i=0;i<0xff;i++);

	/* Everything okay, return */
}

int32_t EADC_GetTemperature(void)
{
	uint8_t ret[3];

	/* Wait to transmit the latest byte */
	while(SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE ) == RESET)
		;

	EADC_NSS_ON();

	/* Set sleep mode then wake up to do a single measure */
	SPI_I2S_SendData(SPI3, EADC_COMMAND_NOP);
	while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE ))
		;

	/* Send read data command then send NOPs to read the conversation result */
	SPI_I2S_SendData(SPI3, EADC_COMMAND_RDATA);
	while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE ))
		;

	SPI_I2S_SendData(SPI3, EADC_COMMAND_NOP);
	while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE ))
		;
	ret[0] = SPI_I2S_ReceiveData(SPI3 );

	SPI_I2S_SendData(SPI3, EADC_COMMAND_NOP);
	while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE ))
		;
	ret[1] = SPI_I2S_ReceiveData(SPI3 );

	SPI_I2S_SendData(SPI3, EADC_COMMAND_NOP);
	while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE ))
		;
	ret[2] = SPI_I2S_ReceiveData(SPI3 );

	EADC_NSS_OFF();

	/* Return the result */
	return (ret[0] << 16) | (ret[1] << 8) | ret[2];
}

void EADC_SetSPI(void)
{
	SPI_InitTypeDef SPI_InitStruct;

	SPI_StructInit(&SPI_InitStruct);
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;

	SPI_Init(SPI3, &SPI_InitStruct);
}

void EADC_ITCmd(FunctionalState NewState)
{
	EXTI_InitTypeDef EXTI_InitStruct;

	EXTI_InitStruct.EXTI_Line = EXTI_Line8;
	EXTI_InitStruct.EXTI_LineCmd = NewState;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStruct);
}

extern xSemaphoreHandle xSemaphoreTempReady;
void EXTI9_5_IRQHandler(void)
{
	static portBASE_TYPE xHigherPriorityTaskWoken;
	if(EXTI_GetFlagStatus(EXTI_Line8))
	{
		xSemaphoreGiveFromISR(xSemaphoreTempReady, &xHigherPriorityTaskWoken);
		EXTI_ClearITPendingBit(EXTI_Line8);
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
}

