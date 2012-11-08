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
void EADC_SPIInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStruct;
	uint8_t i;

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

	//GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_SPI3);

	SPI_StructInit(&SPI_InitStruct);
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
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

	EADC_SPI_NSS_ON();

	SPI_I2S_SendData(SPI3, EADC_COMMAND_SLEEP);
	while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE ))
		;

	EADC_SPI_NSS_OFF();
}
