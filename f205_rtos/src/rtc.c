/*
 * rtc.c
 *
 *  Created on: 2012.10.18.
 *      Author: Gábor
 */

#include "rtc.h"
#include "stm32f2xx.h"
#include "display/display.h"

#define USE_SECONDS 1

RTC_TimeTypeDef RTC_TimeStructure;
RTC_InitTypeDef RTC_InitStructure;
RTC_DateTypeDef RTC_DateStructure;
__IO uint32_t AsynchPrediv = 0, SynchPrediv = 0;

extern __IO int32_t *psTempLimit;
extern __IO float *pfTempLimit;
extern __IO uint8_t * pBacklightDuty;

void RTCInit(void)
{
	uint32_t i = 0;

	if(RTC_ReadBackupRegister(RTC_BKP_DR0 ) != 0x32F2)
	{
		/* RTC configuration  */
		RTC_Config();

		/* Configure the RTC data register and RTC prescaler */
		RTC_InitStructure.RTC_AsynchPrediv = AsynchPrediv;
		RTC_InitStructure.RTC_SynchPrediv = SynchPrediv;
		RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;

		/* Check on RTC init */
		if(RTC_Init(&RTC_InitStructure) == ERROR)
		{
			while(1)
				;
		}

		/* Configure the time register */

		RTC_TimeStructure.RTC_H12 = RTC_H12_AM;
		RTC_TimeStructure.RTC_Hours = 12;
		RTC_TimeStructure.RTC_Minutes = 24;
		RTC_TimeStructure.RTC_Seconds = 00;

		/* Configure the RTC time register */
		if(RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure) == ERROR)
		{
			DISP_StringWrite(5, 4, "Set fail");
		}
		else
		{
			RTC_WriteBackupRegister(RTC_BKP_DR0, 0x32F2);
		}

		/* Store initial data in the backup SRAM */
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);
		*(__IO int32_t *) (BKPSRAM_BASE + BKP_TEMP_LIMIT1_OFFSET) = 317005;
		*(__IO float *) (BKPSRAM_BASE + BKP_TEMP_LIMIT2_OFFSET) = 19.0f;
		*(__IO uint8_t *)(BKPSRAM_BASE + BKP_BACKLIGHT_OFFSET) = 3;
		PWR_BackupRegulatorCmd(ENABLE);
		/* Wait until the Backup SRAM low power Regulator is ready */
		while(PWR_GetFlagStatus(PWR_FLAG_BRR ) == RESET)
		{
		}

	}
	else
	{
		/* Enable the PWR clock */
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

		/* Allow access to RTC */
		PWR_BackupAccessCmd(ENABLE);

		/* Wait for RTC APB registers synchronisation */
		RTC_WaitForSynchro();

		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);
	}
	pfTempLimit = (__IO float *)(BKPSRAM_BASE + BKP_TEMP_LIMIT2_OFFSET);
	psTempLimit = (__IO int32_t *)(BKPSRAM_BASE + BKP_TEMP_LIMIT1_OFFSET);
	pBacklightDuty = (__IO uint8_t *)(BKPSRAM_BASE + BKP_BACKLIGHT_OFFSET);
}

void RTC_Config(void)
{
	/* Enable the PWR clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to RTC */
	PWR_BackupAccessCmd(ENABLE);

	RCC_LSEConfig(RCC_LSE_ON );

	/* Wait till LSE is ready */
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY ) == RESET)
	{
	}

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE );

	SynchPrediv = 0xFF;
	AsynchPrediv = 0x7F;

	/* Enable the RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC APB registers synchronization */
	RTC_WaitForSynchro();
}

void RTC_TimeToString(char* String, RTCShowSeconds_TypeDef Show)
{
	static uint8_t bLeadZero = 0;
	bLeadZero = 0;
	RTC_GetTime(RTC_Format_BCD, &RTC_TimeStructure);
	if((RTC_TimeStructure.RTC_Hours >> 4) == 0 && !Show)
		bLeadZero = 1;
	if(!bLeadZero || Show)
		String[0] = (char) ((RTC_TimeStructure.RTC_Hours >> 4) + '0');
	String[1 - bLeadZero] = (char) ((RTC_TimeStructure.RTC_Hours & 0x0f) + '0');
	String[2 - bLeadZero] = ':';
	String[3 - bLeadZero] = (char) ((RTC_TimeStructure.RTC_Minutes >> 4) + '0');
	String[4 - bLeadZero] = (char) ((RTC_TimeStructure.RTC_Minutes & 0x0f) + '0');
	if(Show)
	{
		String[5 - bLeadZero] = ':';
		String[6 - bLeadZero] = (char) ((RTC_TimeStructure.RTC_Seconds >> 4) + '0');
		String[7 - bLeadZero] = (char) ((RTC_TimeStructure.RTC_Seconds & 0x0f) + '0');
		String[8 - bLeadZero] = '\0';
	}
	else
	{
		String[5 - bLeadZero] = '\0';
	}
}

void RTC_DateToString(char* String)
{
	RTC_GetDate(RTC_Format_BCD, &RTC_DateStructure);
	String[0] = (char) ('2');
	String[1] = (char) ('0');
	String[2] = (char) ((RTC_DateStructure.RTC_Year >> 4) + '0');
	String[3] = (char) ((RTC_DateStructure.RTC_Year & 0x0f) + '0');
	String[4] = '.';
	String[5] = (char) ((RTC_DateStructure.RTC_Month >> 4) + '0');
	String[6] = (char) ((RTC_DateStructure.RTC_Month & 0x0f) + '0');
	String[7] = '.';
	String[8] = (char) ((RTC_DateStructure.RTC_Date >> 4) + '0');
	String[9] = (char) ((RTC_DateStructure.RTC_Date & 0x0f) + '0');
	String[10] = '.';
	String[11] = '\0';
}
