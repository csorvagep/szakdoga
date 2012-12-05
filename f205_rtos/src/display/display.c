/*
 * display.c
 *
 *  Created on: Jul 29, 2012
 *      Author: Gábor
 */

#include "display.h"
#include "stm32f2xx.h"
#include "font.h"

#define DISP_SetLine(Line, ChipSel)		DISP_ByteWrite(DISP_PAGEADDR_REG | (Line), \
				DISP_DataType_Instruction, (ChipSel))

#define DISP_SetCol(Column, ChipSel)	DISP_ByteWrite(DISP_COLADDR_REG | (Column), \
				DISP_DataType_Instruction, (ChipSel))

#define DISP_DataWrite(Data, ChipSel)	DISP_ByteWrite((Data), \
				DISP_DataType_Data, (ChipSel))

static uint8_t LineCounter[] =
{ 0, 0 };
static uint8_t ColumnCounter[] =
{ 0, 0 };

/* TODO 16 elemû fényerõ szint */
static const uint8_t DutyList[8] = {4,5,9,17,28,45,66,100};

static uint8_t BacklightDuty = 3;

static void DISP_ByteWrite(uint8_t Byte, DISPDataType_TypeDef DISP_DataType, DISPCs_TypeDef DISP_CS);

static uint8_t Duty = 17;

/**
 * @brief	This function initializes the display backlight's GPIO pins and the timer for PWM
 * @param	None
 * @retval	None
 */
void Backlight_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct;
	TIM_TimeBaseInitTypeDef TIM_InitStruct;
	TIM_OCInitTypeDef TIM_OCInitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;
	uint16_t PrescalerValue = 0;

//Enable clocks
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

//Set display backlight PWM
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;

	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM12 );

//Set TIM12 to PWM
	PrescalerValue = (uint16_t) 0/* (SystemCoreClock / 12000000) - 1*/;
	TIM_InitStruct.TIM_Period = 100;
	TIM_InitStruct.TIM_Prescaler = PrescalerValue;
	TIM_InitStruct.TIM_ClockDivision = 0;
	TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM12, &TIM_InitStruct);

	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStruct.TIM_Pulse = DutyList[BacklightDuty];
	TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;

	TIM_OC2Init(TIM12, &TIM_OCInitStruct);
	TIM_OC2PreloadConfig(TIM12, TIM_OCPreload_Enable );

	TIM_ARRPreloadConfig(TIM12, ENABLE);

	TIM_Cmd(TIM12, ENABLE);

	TIM_TimeBaseStructInit(&TIM_InitStruct);
	TIM_InitStruct.TIM_Period = SystemCoreClock / 60000 - 1;
	TIM_InitStruct.TIM_Prescaler = 1000;
	TIM_TimeBaseInit(TIM3, &TIM_InitStruct);

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 4;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStruct);
}

/**
 * @brief	Initializes the display
 * @note	Enables the necessary GPIO pins in DISP_DATA_PORT, DISP_CTR_PORT1 and DISP_CTR_PORT2
 * 		with the given pins. (Seen in header file)
 * @param	None
 * @retval	None
 */
void DISP_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	RCC_AHB1PeriphClockCmd(DISP_RCC_Periph, ENABLE);   //Enable peripheral clock

	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Pin = DISP_DATA_Pins;
	GPIO_Init(DISP_DATA_PORT, &GPIO_InitStruct);  //Enable the display data port
	GPIO_Write(DISP_DATA_PORT, GPIO_ReadInputData(DISP_DATA_PORT ) & 0xff00);

	GPIO_InitStruct.GPIO_Pin = DISP_RW_Pin | DISP_CS1_Pin | DISP_CS2_Pin | DISP_RST_Pin;
	GPIO_Init(DISP_CTR_PORT1, &GPIO_InitStruct); //Enable the first control port
	GPIO_SetBits(DISP_CTR_PORT1, DISP_RST_Pin );

	GPIO_InitStruct.GPIO_Pin = DISP_E_Pin | DISP_DI_Pin;
	GPIO_Init(DISP_CTR_PORT2, &GPIO_InitStruct); //Enable the second control port
	GPIO_ResetBits(DISP_CTR_PORT2, DISP_E_Pin );

	DISP_ByteWrite(DISP_ON_REG, DISP_DataType_Instruction, DISP_CSAll); //Turn on the display
	DISP_ByteWrite(DISP_STARTLINE_REG, DISP_DataType_Instruction, DISP_CSAll); //Set the start line to 0

	DISP_Clear();          //Clear the display
}

/**
 * @brief	Writes the Byte in the selected control (DISP_CS) with the specified type (DISP_DataType)
 * @param	Byte: specifies the value to be written in the display control
 * @param	DISP_DataType: specifies the type of the given data
 * 			This parameter can be one of the DISPDataType_TypeDef enum values:
 * 				@arg DISP_DataType_Instruction: the Byte is a control byte
 * 				@arg DISP_DataType_Data: the Byte is data byte
 * @param	DISP_CS: specifies the selected control register
 * 			This parameter can be any combination of DISPCs_TypeDef enum
 * @retval	None
 */
static void DISP_ByteWrite(uint8_t Byte, DISPDataType_TypeDef DISP_DataType, DISPCs_TypeDef DISP_CS)
{
	uint16_t data = 0;
	// TODO: Optimise the delay value if possible

	switch(DISP_CS)
	//Sets the directon
	{
	case DISP_CS1:
		GPIO_SetBits(DISP_CTR_PORT1, DISP_CS1_Pin );
		break;

	case DISP_CS2:
		GPIO_SetBits(DISP_CTR_PORT1, DISP_CS2_Pin );
		break;

	case DISP_CSAll:
		GPIO_SetBits(DISP_CTR_PORT1, DISP_CS1_Pin | DISP_CS2_Pin );
		break;
	}

	switch(DISP_DataType)
	//Sets the type
	{
	case DISP_DataType_Instruction:
		GPIO_ResetBits(DISP_CTR_PORT2, DISP_DI_Pin );
		break;

	case DISP_DataType_Data:
		GPIO_SetBits(DISP_CTR_PORT2, DISP_DI_Pin );
		break;
	}

	data = GPIO_ReadInputData(DISP_DATA_PORT ); //Read the port value, not to ruin unused pins
	data &= 0xff00;
	data |= Byte;
	GPIO_Write(DISP_DATA_PORT, data);          //Write the specified data

	DISP_Delay(1);
	GPIO_SetBits(DISP_CTR_PORT2, DISP_E_Pin );          //Sets the E pin high
	DISP_Delay(2);
	GPIO_ResetBits(DISP_CTR_PORT2, DISP_E_Pin );          //Sets the E pin low
	DISP_Delay(4);
	GPIO_ResetBits(DISP_CTR_PORT1, DISP_CS1_Pin | DISP_CS2_Pin ); //Set back the CS pins
}

/**
 * @brief	Displays one segment in the specified place
 * @note	One segment is a 1 pixel wide 8 pixel high. The LSB is the top pixel.
 * 			Remembers the display's positions to hide the two memory blocks
 * @param	Line: specifies the line where the block is located (0-7)
 * @param	Column: specifies the column where the block is located (0-127)
 * @param	Block: specifies the value to be displayed (LSB - top ... MSB - bottom)
 * @retval	None
 */
void DISP_BlockWrite(uint8_t Line, uint8_t Column, uint8_t Block)
{
	static DISPCs_TypeDef ChipSelect = DISP_CS1;

	ChipSelect = DISP_CS1;

	if(Column > 127)
	{
		Line++;
		Column &= 0x7f;
	}
	if(Line > 7)
	{
		Line &= 0x07;
	}
	if(Column > 63)
	{
		ChipSelect = DISP_CS2;
		Column -= 64;
	}

	if(Line != LineCounter[ChipSelect])
	{
		DISP_SetLine(Line, ChipSelect);
		LineCounter[ChipSelect] = Line;
	}
	if(Column != ColumnCounter[ChipSelect])
	{
		DISP_SetCol(Column, ChipSelect);
		ColumnCounter[ChipSelect] = Column;
	}

	DISP_DataWrite(Block, ChipSelect);

	ColumnCounter[ChipSelect] = (ColumnCounter[ChipSelect] + 1) & 0x7f;
}

/**
 * @brief	Clears the entire display.
 * @note	Warning! This takes a long time!
 * @param	None
 * @retval	None
 */
void DISP_Clear(void)
{
	static uint8_t x = 0, y = 0;
	for(y = 0; y < 8; y++)          //Loop through the lines
	{
		DISP_SetCol(0, DISP_CSAll);
		DISP_SetLine(y, DISP_CSAll);
		for(x = 0; x < 64; x++)          //Loop through the blocks
			DISP_DataWrite(0x00, DISP_CSAll);
	}
	DISP_SetCol(0, DISP_CSAll);
	DISP_SetLine(0, DISP_CSAll);
	LineCounter[0] = 0;
	LineCounter[1] = 0;
	ColumnCounter[0] = 0;
	ColumnCounter[1] = 0;
}

/**
 * @brief	Software counter for some delay.
 * @param	delay: proportional value of wait
 * @retval	None
 */
void DISP_Delay(uint8_t delay)
{
	uint16_t i = delay * 10;
	while(i--)
		;
}

/**
 * @brief	Writes the specified character in the display
 * @param	Line: specifies the line where the character is written (0-7)
 * @param	Column: specifies the column where the character is written (0-127)
 * @param	Ch: specifies the ASCII character to be written
 * @param	Mode: specifies the write mode. This parameter can be a value of @ref DISP_WriteMode_TypeDef
 * @retval	None
 */
void DISP_CharWriteGeneric(uint8_t Line, uint8_t Column, uint8_t Ch, DISPWriteMode_TypeDef Mode)
{
	uint8_t i = 0;
	uint8_t temp = 0;

	/* TODO Ch ellenõzése */

	for(i = 0; i < 5; i++)
	{
		temp = fontdata[(Ch - 32) * 5 + i];
		if(Mode)
			temp = ~temp;
		DISP_BlockWrite(Line, Column + i, temp);
	}

	temp = 0x00;
	if(Mode)
		temp = ~temp;
	DISP_BlockWrite(Line, Column + 5, temp);
}

/**
 * @brief Writes a string in the display
 * @param	Line: specifies the line where the character is written (0-7)
 * @param	Column: specifies the column where the character is written (0-127)
 * @param	String: specifies the null-terminated string
 * @param	Mode: specifies the write mode. This parameter can be a value of @ref DISP_WriteMode_TypeDef
 * @retval	None
 */
void DISP_StringWriteGeneric(uint8_t Line, uint8_t Column, const char* String,
		DISPWriteMode_TypeDef Mode)
{
	uint8_t temp = 0;
	uint8_t i = 0;
	while(String[i])
	{
		temp = (uint8_t) String[i];
		if(Mode)
			DISP_CharWriteInvert(Line, Column + i * 6, temp);
		else
			DISP_CharWrite(Line, Column + i * 6, temp);
		i++;
	}
}

/* TODO Kommentelni, és BlockWrite mintájára a globális változók állítása */
uint8_t DISP_ReadBlock(uint8_t Line, uint8_t Column)
{
	DISPCs_TypeDef Disp_CS = DISP_CS1;
	GPIO_InitTypeDef GPIO_InitStruct;
	uint16_t retval = 0;

	if(Column >= 64)
	{
		Disp_CS = DISP_CS2;
		Column -= 64;
	}

	/* Set memory cursor position */
	DISP_ByteWrite(DISP_COLADDR_REG | Column, DISP_DataType_Instruction, Disp_CS);
	DISP_ByteWrite(DISP_PAGEADDR_REG | Line, DISP_DataType_Instruction, Disp_CS);

	/* Set DI to data */
	GPIO_SetBits(DISP_CTR_PORT2, DISP_DI_Pin );

	switch(Disp_CS)
	{
	case DISP_CS1:
		GPIO_SetBits(DISP_CTR_PORT1, DISP_CS1_Pin );
		break;

	case DISP_CS2:
		GPIO_SetBits(DISP_CTR_PORT1, DISP_CS2_Pin );
		break;

	case DISP_CSAll: //Can't read both
		GPIO_SetBits(DISP_CTR_PORT1, DISP_CS1_Pin );
		break;
	}
	GPIO_SetBits(DISP_CTR_PORT1, DISP_RW_Pin );

	/* Set GPIO mode to input */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Pin = DISP_DATA_Pins;
	GPIO_Init(DISP_DATA_PORT, &GPIO_InitStruct);

	/* Dummy read */
	DISP_Delay(2);
	GPIO_SetBits(DISP_CTR_PORT2, DISP_E_Pin );          //Sets the E pin high
	DISP_Delay(4);
	GPIO_ResetBits(DISP_CTR_PORT2, DISP_E_Pin );          //Sets the E pin low
	retval = GPIO_ReadInputData(DISP_DATA_PORT );
	DISP_Delay(4);

	/* Read */
	GPIO_SetBits(DISP_CTR_PORT2, DISP_E_Pin );          //Sets the E pin high
	DISP_Delay(4);
	retval = GPIO_ReadInputData(DISP_DATA_PORT );
	GPIO_ResetBits(DISP_CTR_PORT2, DISP_E_Pin );          //Sets the E pin low
	DISP_Delay(4);

	GPIO_ResetBits(DISP_CTR_PORT1, DISP_CS1_Pin | DISP_RW_Pin ); //Set back the CS pins

	/* Set GPIO mode back to output */
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Pin = DISP_DATA_Pins;
	GPIO_Init(DISP_DATA_PORT, &GPIO_InitStruct);

	return (uint8_t) retval;
}

/**
 * @brief	Writes the specified 2 line high number in the display and 2 column space (11 column wide)
 * @param	Line: specifies the line where the left bottom corenr of the number is written (1-7)
 * @param	Column: specifies the column where the character is written (0-127)
 * @param	Number: specifies the ASCII code of number to be written, or it could be colon or period
 * @retval	None
 */
uint8_t DISP_2LineNumWrite(uint8_t Line, uint8_t Column, uint8_t Number)
{
	uint8_t i = 0;
	uint8_t cnt = 0;

	/* Assert Number parameter */
	if((Number - '0') > 9 && Number != ':' && Number != '.')
		return 0;

	/* If Line = 0 set to the minimum */
	if(Line == 0)
		Line = 1;

	if(Number != ':' && Number != '.')
	{
		/* Write the second line of the number */
		for(i = 0; i < 9; i++)
			DISP_BlockWrite(Line, Column + i,
					(uint8_t) (twoline_nums[(Number - '0') * 9 + i] & 0x00ff));

		/* Write out 2 space block */
		DISP_BlockWrite(Line, Column + 10, 0x00);
		DISP_BlockWrite(Line, Column + 11, 0x00);

		Line--;

		for(i = 0; i < 9; i++)
			DISP_BlockWrite(Line, Column + i,
					(uint8_t) ((twoline_nums[(Number - '0') * 9 + i] & 0xff00) >> 8));

		/* Write out 2 space block */
		DISP_BlockWrite(Line, Column + 10, 0x00);
		DISP_BlockWrite(Line, Column + 11, 0x00);

		return 11;
	}
	else
	{
		if(Number == ':')
			cnt = 92;
		else if(Number == '.')
			cnt = 90;

		DISP_BlockWrite(Line, Column, (uint8_t) (twoline_nums[cnt] & 0x00ff));
		DISP_BlockWrite(Line, Column + 1, (uint8_t) (twoline_nums[cnt + 1] & 0x00ff));
		DISP_BlockWrite(Line, Column + 2, 0x00);
		DISP_BlockWrite(Line, Column + 3, 0x00);

		Line--;

		DISP_BlockWrite(Line, Column, (uint8_t) ((twoline_nums[cnt] & 0xff00) >> 8));
		DISP_BlockWrite(Line, Column + 1, (uint8_t) ((twoline_nums[cnt + 1] & 0xff00) >> 8));
		DISP_BlockWrite(Line, Column + 2, 0x00);
		DISP_BlockWrite(Line, Column + 3, 0x00);

		return 4;
	}
}

/*TODO comment*/
void DISP_2LineStringWrite(uint8_t Line, uint8_t Column, char * String)
{
	static uint8_t i = 0;
	for(i = 0; String[i]; i++)
		Column += DISP_2LineNumWrite(Line, Column, String[i]);
}

inline uint8_t DISP_GetBacklight(void)
{
	return BacklightDuty;
}

inline void DISP_SetBacklight(uint8_t Val)
{
	if(Val>DISP_MAX_DUTY)
		BacklightDuty = DISP_MAX_DUTY;
	else
		BacklightDuty = Val;

	TIM_SetCompare2(TIM12, DutyList[BacklightDuty]);
}

/* TODO comment */
void DISP_SetOff(void)
{
	Duty = DutyList[BacklightDuty];

	/* Enable the fade off timer */
	TIM_Cmd(TIM3, ENABLE);
}

void DISP_SetOn(void)
{
	DISP_ByteWrite(DISP_ON_REG, DISP_DataType_Instruction, DISP_CSAll);
	TIM_Cmd(TIM12, ENABLE);

	TIM_SelectOCxM(TIM12, TIM_Channel_2, TIM_OCMode_PWM2);
	TIM_CCxCmd(TIM12, TIM_Channel_2, TIM_CCx_Enable);

	TIM_SetCompare2(TIM12, DutyList[BacklightDuty]);
}

void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update ))
	{
		if(Duty == 0)
		{
			DISP_ByteWrite(DISP_OFF_REG, DISP_DataType_Instruction, DISP_CSAll);
			TIM_ForcedOC2Config(TIM12, TIM_ForcedAction_Active);
			TIM_Cmd(TIM12, DISABLE);
			TIM_Cmd(TIM3, DISABLE);
		}
		else
		{
			TIM_SetCompare2(TIM12, --Duty);
		}
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update );
	}
}
