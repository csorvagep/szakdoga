/*
 * display.h
 *
 *  Created on: Jul 29, 2012
 *      Author: Gábor
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "stm32f2xx.h"

/* Register values */
#define DISP_ON_REG			0x3F
#define DISP_OFF_REG		0x3E

/* Register masks */
#define DISP_STARTLINE_REG	0xC0
#define DISP_PAGEADDR_REG	0xB8
#define DISP_COLADDR_REG	0x40
#define DISP_BUSY			0x80
#define DISP_ONOFF			0x20
#define DISP_RESET			0x10

/* Used ports */
#define DISP_DATA_PORT		GPIOC
#define DISP_CTR_PORT1		GPIOA
#define DISP_CTR_PORT2		GPIOB

/* Pins for CTR_PORT1 */
#define DISP_CS1_Pin		GPIO_Pin_4
#define DISP_CS2_Pin		GPIO_Pin_5
#define DISP_RST_Pin		GPIO_Pin_6
#define DISP_RW_Pin			GPIO_Pin_7

/* Pins for CTR_PORT2 */
#define DISP_DI_Pin			GPIO_Pin_0
#define DISP_E_Pin			GPIO_Pin_1

/* Constans */
#define DISP_MAX_DUTY	7

/* Defines */
typedef enum
{
	DISP_CS1 = 0x00,
	DISP_CS2 = 0x01,
	DISP_CSAll = 0x02
} DISPCs_TypeDef;

typedef enum
{
	DISP_DataType_Instruction = 0x00, DISP_DataType_Data = 0x01
} DISPDataType_TypeDef;

#define DISP_RCC_Periph		RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB
#define DISP_DATA_Pins		GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7

typedef enum
{
	DISP_WriteMode_Normal = 0x00,
	DISP_WriteMode_Invert = 0x01
}DISPWriteMode_TypeDef;

#define DISP_CharWrite(Line, Column, Ch)	DISP_CharWriteGeneric((Line), \
	(Column), (Ch), DISP_WriteMode_Normal)

#define DISP_CharWriteInvert(Line, Column, Ch)	DISP_CharWriteGeneric((Line), \
	(Column), (Ch), DISP_WriteMode_Invert)

#define DISP_StringWrite(Line, Column, String)	DISP_StringWriteGeneric((Line), \
		(Column), (String), DISP_WriteMode_Normal)

#define DISP_StringWriteInvert(Line, Column, String)	DISP_StringWriteGeneric((Line), \
		(Column), (String), DISP_WriteMode_Invert)


/* Functions */
void Backlight_Init(void);
void DISP_Init(void);
void DISP_BlockWrite(uint8_t Line, uint8_t Column, uint8_t Block);
void DISP_Clear(void);
void DISP_Delay(uint8_t delay);
void DISP_CharWriteGeneric(uint8_t Line, uint8_t Column, uint8_t Ch, DISPWriteMode_TypeDef Mode);
void DISP_StringWriteGeneric(uint8_t Line, uint8_t Column, const char* String,
		DISPWriteMode_TypeDef Mode);
uint8_t DISP_ReadBlock(uint8_t Line, uint8_t Column);
uint8_t DISP_2LineNumWrite(uint8_t Line, uint8_t Column, uint8_t Number);
void DISP_2LineStringWrite(uint8_t Line, uint8_t Column, char * String);
uint8_t DISP_GetBacklight(void);
void DISP_SetBacklight(uint8_t Val);


#endif /* DISPLAY_H_ */
