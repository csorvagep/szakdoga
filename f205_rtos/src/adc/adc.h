/*
 * adc.h
 *
 *  Created on: 2012.10.23.
 *      Author: Gábor
 */

#ifndef EADC_H_
#define EADC_H_

#include "stm32f2xx.h"
#include "FreeRTOS.h"
#include "semphr.h"

/* ADS1246 Register Addresses */
typedef enum
{
	EADC_Register_BCS 	= 0x00,
	EADC_Register_VBIAS  = 0x01,
	EADC_Register_MUX1 	= 0x02,
	EADC_Register_SYS0 	= 0x03,
	EADC_Register_OFC0 	= 0x04,
	EADC_Register_OFC1 	= 0x05,
	EADC_Register_OFC2 	= 0x06,
	EADC_Register_FSC0 	= 0x07,
	EADC_Register_FSC1 	= 0x08,
	EADC_Register_FSC2 	= 0x09,
	EADC_Register_ID 	= 0x0A
} EADCRegister_TypeDef;

/* System control commands */
#define EADC_COMMAND_WAKEUP	0x00
#define EADC_COMMAND_SLEEP		0x02
#define EADC_COMMAND_SYNC1		0x04
#define EADC_COMMAND_SYNC2		0x04
#define EADC_COMMAND_RESET		0x06
#define EADC_COMMAND_NOP		0xFF

/* Data read commands */
#define EADC_COMMAND_RDATA		0x12
#define EADC_COMMAND_RDATAC	0x14
#define EADC_COMMAND_SDATAC	0x16

/* Read register command */
#define EADC_COMMAND_RREG1		0x20
#define EADC_COMMAND_RREG2		0x00

/* Write register command */
#define EADC_COMMAND_WREG1		0x40
#define EADC_COMMAND_WREG2		0x00

/* Calibration commands */
#define EADC_COMMAND_SYSOCAL	0x60
#define EADC_COMMAND_SYSGCAL	0x61
#define EADC_COMMAND_SELFOCAL	0x62

#define EADC_NSS_ON()		GPIO_ResetBits(GPIOA, GPIO_Pin_15)
#define EADC_NSS_OFF()		GPIO_SetBits(GPIOA, GPIO_Pin_15)

#define EADC_CORR					1.008634f


void EADC_Init(void);
int32_t EADC_GetTemperature(void);
void EADC_SetSPI(void);
void EADC_ITCmd(FunctionalState NewState);


#endif /* EADC_H_ */
