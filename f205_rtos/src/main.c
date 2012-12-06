/**
 *****************************************************************************
 **
 **  File        : main.c
 **
 **  Abstract    : main function.
 **
 **  Functions   : main
 **
 **  Environment : Atollic TrueSTUDIO/STM32
 **                STMicroelectronics STM32F2xx Standard Peripherals Library
 **
 **  Distribution: The file is distributed “as is,” without any warranty
 **                of any kind.
 **
 **  (c)Copyright Atollic AB.
 **  You may use this file as-is or modify it according to the needs of your
 **  project. Distribution of this file (unmodified or modified) is not
 **  permitted. Atollic AB permit registered Atollic TrueSTUDIO(R) users the
 **  rights to distribute the assembled, compiled & linked contents of this
 **  file as part of an application binary file, provided that it is built
 **  using the Atollic TrueSTUDIO(R) toolchain.
 **
 **
 *****************************************************************************
 */

/* Application includes */
#include "main.h"

#include "display/display.h"
#include "rotary.h"
#include "menu.h"
#include "rtc.h"
#include "relay.h"
#include "adc/adc.h"
#include "rfm70/rfm70.h"

/* Queue Handles */
xQueueHandle xQueueMenu = NULL;

/* Timer Handles */
xTimerHandle xTimerRotaryAllow = NULL;
xTimerHandle xTimerUpdateDisplay = NULL;
xTimerHandle xTimerTempMeasure = NULL;

/* Mutex Handles */
xSemaphoreHandle xMutexTempMemory = NULL;
xSemaphoreHandle xMutexSPIUse = NULL;
xSemaphoreHandle xMutexTempLimit = NULL;

xSemaphoreHandle xMutexBattVolt = NULL;

/* Semaphore Handles */
xSemaphoreHandle xSemaphoreTempReady = NULL;
xSemaphoreHandle xSemaphoreRFInt = NULL;
xSemaphoreHandle xSemaphoreSleep = NULL;

/* Task Handles */
xTaskHandle xTaskHandleMainScreen = NULL;
xTaskHandle xTaskHandleMenuSelect = NULL;
xTaskHandle xTaskHandleSetTimeDate = NULL;
xTaskHandle xTaskHandleSetBrightness = NULL;
xTaskHandle xTaskHandleSetRFModule = NULL;

xTaskHandle xTaskHandleTurnOff = NULL;

xTaskHandle xTaskHandleRFMRead = NULL;

xTaskHandle xTaskHandleUSB = NULL;

xTaskHandle * aMenuTaskPtrs[MENU_MAX + 1] = {
		&xTaskHandleSetTimeDate, &xTaskHandleMainScreen, &xTaskHandleSetBrightness,
		&xTaskHandleSetRFModule, &xTaskHandleMainScreen, &xTaskHandleTurnOff };


xTaskHandle * aDisplayTasks[DISP_TASKS_COUNT] = {&xTaskHandleSetTimeDate, &xTaskHandleMainScreen, &xTaskHandleSetBrightness,
		&xTaskHandleSetRFModule, &xTaskHandleMenuSelect};

/* Global variables for temperature measuring */
int32_t *psStartOfBuffer = NULL;
uint8_t uBuffCntr = 0;
int32_t sTempLimit = 317005;
float fTempLimit = 19.0;

uint16_t uSensBattVoltage = 3700;
uint16_t uSensExtTemp = 20000;
uint8_t bSensPresent = 0;

/* Global variable used by Rotary */
extern int16_t DeltaValue;

/*USB*/
__ALIGN_BEGIN USB_OTG_CORE_HANDLE      USB_OTG_Core __ALIGN_END;
__ALIGN_BEGIN USBH_HOST                USB_Host __ALIGN_END;


/* Main Function, Program entry point */
int main(void)
{
	/* Ensure that all 4 interrupt priority bits are used as the pre-emption priority. */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4 );

	/* Initialize the components */
	Backlight_Init();
	DISP_Init();
	RELAY_Init();
	ROT_Init(); //TODO visszatérés, hogy ha be kell állítani akkor azzal kezdjen
	RTCInit();
	EADC_Init();

	USBH_Init(&USB_OTG_Core, USB_OTG_FS_CORE_ID, &USB_Host, &USBH_MSC_cb, &USR_cb);

	/* Create Queues */
	xQueueMenu = xQueueCreate( menuQUEUE_LENGTH, sizeof( int8_t ) );

	/* Create Mutexes */
	xMutexTempMemory = xSemaphoreCreateMutex();
	xMutexTempLimit = xSemaphoreCreateMutex();
	xMutexSPIUse = xSemaphoreCreateMutex();
	xMutexBattVolt = xSemaphoreCreateMutex();

	/* Create Semaphores */
	vSemaphoreCreateBinary(xSemaphoreTempReady);
	vSemaphoreCreateBinary(xSemaphoreRFInt);
	vSemaphoreCreateBinary(xSemaphoreSleep);

	xSemaphoreTake(xSemaphoreSleep, 0);

	/* Create Tasks */
	xTaskCreate( vTaskMainScreen, ( signed char * ) "MainScreen", 100, NULL, MAIN_SCREEN_PRIORITY,
			&xTaskHandleMainScreen);
	xTaskCreate( vTaskMenuSelect, ( signed char * ) "MenuSelect", 70, NULL, MENU_SELECT_PRIORITY,
			&xTaskHandleMenuSelect);
	xTaskCreate( vTaskSetTimeDate, ( signed char * ) "SetTimeDate", 100, NULL, SET_TIMEDATE_PRIORITY,
			&xTaskHandleSetTimeDate);
	xTaskCreate( vTaskSetBrightness, ( signed char * ) "SetBrightness", 70, NULL,
			SET_BRIGHTNESS_PRIORITY, &xTaskHandleSetBrightness);
	xTaskCreate( vTaskSetRFModule, ( signed char * ) "SetRF", 70, NULL, RFMODULE_PRIORITY,
			&xTaskHandleSetRFModule);
	xTaskCreate( vTaskTurnOff, ( signed char * ) "Off", 20, NULL, TURNOFF_PRIORITY,
			&xTaskHandleTurnOff);

//	xTaskCreate( vTaskUSB, ( signed char * ) "USB", 300, NULL, USB_PRIORITY,
//			&xTaskHandleUSB);

	xTaskCreate( prvRotaryChkTask, ( signed char * )"Rotary", 70, NULL, rotaryQUEUE_TASK_PRIORITY,
			NULL);
	xTaskCreate( prvTempStoreTask, ( signed char * ) "Temp", 70, NULL, tempMEASURE_TASK_PRIORITY,
			NULL);

	xTaskCreate(vTaskRFMRead, (signed char *)"rfm", 100, NULL, RFM_TASK_PRIORITY,
			&xTaskHandleRFMRead);

	/* Create Timers */
	xTimerRotaryAllow = xTimerCreate((signed char *) "Rotary", ROTARY_PB_DENY, pdFALSE, (void *) 0,
			vAllowRotary);

	vTaskSuspend(xTaskHandleMainScreen);
	vTaskSuspend(xTaskHandleMenuSelect);
	vTaskSuspend(xTaskHandleSetTimeDate);
	vTaskSuspend(xTaskHandleSetBrightness);
	vTaskSuspend(xTaskHandleSetRFModule);
	vTaskSuspend(xTaskHandleTurnOff);

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* Never should reach here */
	for(;;)
		;
}

/*
 * Timer callback functions
 */

void vAllowRotary(xTimerHandle pxTimer)
{
	NVIC_InitTypeDef NVIC_InitStruct;

	EXTI_ClearITPendingBit(EXTI_Line0 );

	NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 6;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;

	NVIC_Init(&NVIC_InitStruct);
}

/*
 * Tasks
 */

/* This task retrieves the value of the rotary switch and send it to the queue */
void prvRotaryChkTask(void *pvParameters)
{
	portTickType xNextWakeTime;
	signed short CurrentVal = 0;
	uint8_t xValueHolder;

	/* Initialize xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for(;;)
	{
		vTaskDelayUntil(&xNextWakeTime, ROTARY_CHK_FREQUENCY);

		TIM_Cmd(TIM2, DISABLE);
		CurrentVal = DeltaValue;
		DeltaValue = CurrentVal & 3;
		TIM_Cmd(TIM2, ENABLE);

		CurrentVal >>= 2;

		if(CurrentVal)
		{
			xValueHolder = CurrentVal;
			xQueueSend( xQueueMenu, &xValueHolder, 0);
			if(xSemaphoreTake(xSemaphoreSleep, 0) == pdTRUE)
			{
				/* Send back action to the queue, comment out if needed */
				//xQueueSendToFront(xQueueMenu, &xReceivedValue, 0);
				/* Turn back the display */
				DISP_SetOn();

				/* Switch back to main screen */
				vTaskResume(xTaskHandleMainScreen);
			}
		}
	}
}

/* This task displays the main screen, sets the temperature limit */
void vTaskMainScreen(void *pvParamters)
{
	int8_t sReceivedValue;
	uint16_t uIdleCntr = 0;
	uint8_t bUpdateNeed = 1;
	float prvfTempLimit;
	float fRt, fT;
	int32_t prvsTempLimit, prvTemp;
	char time_string[15];
	uint8_t i;
	uint16_t prvBattVoltage;
	uint16_t prvExtTemp;
	uint8_t prvbSensPresent;

	/* Main loop for this task */
	for(;;)
	{
		/* Do some update */
		/* Increase the idle counter, perform an update */
		if(bUpdateNeed)
		{
			/* Update the time and date */
			RTC_TimeToString(time_string, RTC_ShowSeconds_No);
			for(i=0; i<50; i++)
			{
				DISP_BlockWrite(0,i,0x00);
				DISP_BlockWrite(1,i,0x00);
			}
			DISP_2LineStringWrite(1, 0, time_string);
			RTC_DateToString(time_string);
			DISP_StringWrite(2, 0, time_string);

			/* Get average of the 5 latest temperature, then update */
			if(xSemaphoreTake(xMutexTempMemory, (portTickType) 5) == pdTRUE)
			{
				if(psStartOfBuffer)
				{
					prvTemp = 0;
					for(i = 0; i < 5; i++)
						prvTemp += psStartOfBuffer[(uBuffCntr - 5 + i) & 0x7f];
					xSemaphoreGive(xMutexTempMemory);

					/* Calculate average */
					prvTemp /= 5;

					/* Write out the temperature  */
					fRt = 27.E3 * (prvTemp * EADC_CORR) / (0x7fffff - prvTemp);
					fT = (fRt - 1000) / 3.85;
					DISP_2LineNumWrite(1, 79, ((uint8_t) floor(fT / 10.0)) + '0');
					DISP_2LineNumWrite(1, 90, ((uint8_t) fT % (uint8_t) 10.0) + '0');
					DISP_2LineNumWrite(1, 101, '.');
					fT -= (float) (uint8_t) fT;
					DISP_2LineNumWrite(1, 105, ((uint8_t) floor(fT * 10.0)) + '0');
					DISP_CharWrite(0, 115, 133);
					DISP_CharWrite(0, 121, 'C');

				}
				else
				{
					xSemaphoreGive(xMutexTempMemory);
				}
			}

			/* Get limit temperature, then update */
			if(xSemaphoreTake(xMutexTempLimit, 10 * portTICK_RATE_MS))
			{
				prvfTempLimit = fTempLimit;
				prvsTempLimit = sTempLimit;

				/* Give back the mutex */
				xSemaphoreGive(xMutexTempLimit);

				/* Write out the new limit */
				DISP_CharWrite(2, 89, ((uint8_t) floor(prvfTempLimit / 10.0)) + '0');
				DISP_CharWrite(2, 95, ((uint8_t) prvfTempLimit % (uint8_t) 10.0) + '0');
				DISP_CharWrite(2, 101, '.');
				DISP_CharWrite(2, 107,
						((uint8_t) floor((prvfTempLimit - (float) (uint8_t) prvfTempLimit) * 10.0)) + '0');
				DISP_CharWrite(2, 115, 133);
				DISP_CharWrite(2, 121, 'C');
			}

			/* Voltage of the wireless sensor */
			if(xSemaphoreTake(xMutexBattVolt, 10 * portTICK_RATE_MS))
			{
				prvBattVoltage = uSensBattVoltage;
				prvExtTemp = uSensExtTemp;
				prvbSensPresent = bSensPresent;
				xSemaphoreGive(xMutexBattVolt);

				if(prvbSensPresent)
				{
					DISP_StringWrite(4, 0, "RF:");
					prvBattVoltage += 50;
					DISP_CharWrite(5, 0, (prvBattVoltage / 1000) + '0');
					DISP_CharWrite(5, 6, '.');
					DISP_CharWrite(5, 12, (prvBattVoltage % 1000) / 100 + '0');
					DISP_CharWrite(5, 18, 'V');

					DISP_CharWrite(6, 0, (prvExtTemp / 10000) + '0');
					DISP_CharWrite(6, 6, (prvExtTemp % 10000) / 1000 + '0');
					DISP_CharWrite(6, 12, '.');
					DISP_CharWrite(6, 18, (prvExtTemp % 1000) / 100 + '0');
					DISP_CharWrite(6, 24, 133);
					DISP_CharWrite(6, 30, 'C');
				}
			}
			bUpdateNeed = 0;
		}

		/* Wait to receive element */
		if(pdTRUE == xQueueReceive(xQueueMenu, &sReceivedValue, MENU_UPDATE_FREQUENCY))
		{
			/* Push-button received */
			if(sReceivedValue == 0)
			{
				/* Clear display */
				DISP_Clear();
				uIdleCntr = 0;
				bUpdateNeed = 1;

				vTaskResume(xTaskHandleMenuSelect);
				vTaskSuspend(NULL );
			}
			/* Rotation received */
			else
			{
				/* Reset the idle counter */
				uIdleCntr = 0;

				/* Get the current temperature limit, store in the local private variable */
				if(xSemaphoreTake(xMutexTempLimit, 10 * portTICK_RATE_MS))
				{
					prvfTempLimit = fTempLimit;
					prvsTempLimit = sTempLimit;

					/* Give back the mutex */
					xSemaphoreGive(xMutexTempLimit);

					/* Set the temperature limit */
					prvfTempLimit = prvLimitInterval(prvfTempLimit + sReceivedValue * 0.5, 15.0, 30.0);

					/* Write out the new limit */
					DISP_CharWrite(2, 89, ((uint8_t) floor(prvfTempLimit / 10.0)) + '0');
					DISP_CharWrite(2, 95, ((uint8_t) prvfTempLimit % (uint8_t) 10.0) + '0');
					DISP_CharWrite(2, 101, '.');
					DISP_CharWrite(2, 107,
							((uint8_t) floor((prvfTempLimit - (float) (uint8_t) prvfTempLimit) * 10.0)) + '0');

					/* Compute the new ADC limit */
					fRt = (prvfTempLimit * 3.85 + 1000);
					prvsTempLimit = (int32_t) ((fRt * (float) 0x7fffff) / (fRt + 2.7e4));
					prvsTempLimit /= EADC_CORR;

					/* Store the new limits */
					if(xSemaphoreTake(xMutexTempLimit, 10 * portTICK_RATE_MS))
					{
						fTempLimit = prvfTempLimit;
						sTempLimit = prvsTempLimit;
						xSemaphoreGive(xMutexTempLimit);
					}
				}
			}
		}
		/* No rotary action, start over */
		else
		{
			if(++uIdleCntr < DISPLAY_OFF_TIME)
			{
				bUpdateNeed = 1;
			}
			else
			{
				/* Set off the display */
				DISP_SetOff();

				/* Update when come back */
				uIdleCntr = 0;
				bUpdateNeed = 1;

				//vTaskResume(xTaskHandleSleep);
				xSemaphoreGive(xSemaphoreSleep);
				vTaskSuspend(NULL );
			}
		}
	}
}

/* This task displays the main menu */
void vTaskMenuSelect(void *pvParamters)
{
	/* Declare local variables */
	int8_t sReceivedValue;
	int8_t sMenuPtr = 0, sLastMenuPtr = 1;
	uint8_t i;
	uint16_t uIdleCntr = 0;

	/* Main loop for this task */
	for(;;)
	{
		/* Update when the pointer changed */
		if(sMenuPtr != sLastMenuPtr)
		{
			/* Display left arrow if not the first element */
			if(sMenuPtr == 0)
				DISP_CharWrite(0, 0, ' ');
			else
				DISP_CharWrite(0, 0, '<');

			/* Display right arrow if not the last element */
			if(sMenuPtr == MENU_MAX)
				DISP_CharWrite(0, 122, ' ');
			else
				DISP_CharWrite(0, 122, '>');

			/* Clear first line */
			for(i = 0; i < 118; i++)
				DISP_BlockWrite(0, 5 + i, 0x00);

			/* Write out the menu name */
			DISP_StringWrite(0, (uint8_t) (64 - (strlen(first_row[sMenuPtr]) * 6 / 2)),
					first_row[sMenuPtr]);

			/* Save the menu pointer */
			sLastMenuPtr = sMenuPtr;
		}

		/* Wait for rotary action */
		if(pdTRUE == xQueueReceive(xQueueMenu, &sReceivedValue, MENU_UPDATE_FREQUENCY))
		{
			/* Rotation received */
			if(sReceivedValue != 0)
			{
				/* Set the menu pointer to the new value */
				sMenuPtr = prvLimit(sMenuPtr + sReceivedValue, MENU_MAX);

			}
			/* Push button received */
			else if(sReceivedValue == 0)
			{
				/* Clear display */
				DISP_Clear();

				/* Change to the selected menu */
				vTaskResume(*aMenuTaskPtrs[sMenuPtr]);

				/* Reset values */
				sLastMenuPtr = 1;
				sMenuPtr = 0;

				vTaskSuspend(NULL );
			}
			/* Reset the idle counter */
			uIdleCntr = 0;
		}
		/* There was no change */
		else
		{
			if(++uIdleCntr >= MENU_EXIT_COUNTER)
			{
				uIdleCntr = 0;
				DISP_Clear();
				sLastMenuPtr = 1;
				sMenuPtr = 0;

				/* Change back to main screen task */
				vTaskResume(xTaskHandleMainScreen);
				vTaskSuspend(NULL );
			}
		}
	} /* Task's main loop end */
} /* Task end */

/* This task sets the date and time */
void vTaskSetTimeDate(void *pvParamters)
{
	/* Local variables */
	int8_t sReceivedValue;
	uint16_t uIdleCntr = 0;
	uint8_t bInitNeed = 1;
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	uint16_t time[6] = {
			0, 0, 0, 0, 0, 0 };
	char strTime[15];
	char strTemp[15];
	uint8_t uDatePtr;
	const uint8_t aLimits[6] = {
			99, 12, 31, 23, 59, 59 };
	uint8_t uColNext = 0, uColPrev = 0;
	uint8_t uLineNext = 0, uLinePrev = 0;

	/* Main loop for this task */
	for(;;)
	{
		/* Do initialization if needed */
		if(bInitNeed)
		{
			/* Get the current date and time, and store in local array */
			RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
			RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
			time[0] = RTC_DateStructure.RTC_Year;
			time[1] = RTC_DateStructure.RTC_Month;
			time[2] = RTC_DateStructure.RTC_Date;
			time[3] = RTC_TimeStructure.RTC_Hours;
			time[4] = RTC_TimeStructure.RTC_Minutes;
			time[5] = RTC_TimeStructure.RTC_Seconds;

			/* Write out the date and time, invert the year */
			RTC_DateToString(strTime);
			strncpy(strTemp, strTime, 2);
			strTemp[2] = '\0';
			DISP_StringWrite(4, 31, strTemp);
			strncpy(strTemp, strTime + 2, 2);
			strTemp[2] = '\0';
			DISP_StringWriteInvert(4, 43, strTemp);
			strcpy(strTemp, strTime + 4);
			DISP_StringWrite(4, 55, strTemp);
			RTC_TimeToString(strTime, RTC_ShowSeconds_Yes);
			DISP_StringWrite(5, 40, strTime);

			/* Set variables */
			uDatePtr = 0;
			bInitNeed = 0;
		}

		/* Wait for rotary action */
		if(xQueueReceive(xQueueMenu, &sReceivedValue, MENU_UPDATE_FREQUENCY))
		{
			/* Rotation received */
			if(sReceivedValue != 0)
			{
				/* Set the new value */
				time[uDatePtr] = (uint8_t) prvLimit(time[uDatePtr] + sReceivedValue, aLimits[uDatePtr]);

				/* Fork in time and date */
				if(uDatePtr < 3)
				{
					DISP_CharWriteInvert(4, 43 + (uDatePtr * 18), (time[uDatePtr] / 10) + '0');
					DISP_CharWriteInvert(4, 49 + (uDatePtr * 18), (time[uDatePtr] % 10) + '0');
				}
				else
				{
					DISP_CharWriteInvert(5, 40 + ((uDatePtr-3) * 18), (time[uDatePtr] / 10) + '0');
					DISP_CharWriteInvert(5, 46 + ((uDatePtr-3) * 18), (time[uDatePtr] % 10) + '0');
				}

			}
			/* Push button received */
			else
			{
				/* If the setting is done, save and go back */
				if(uDatePtr == 5)
				{
					/* Store the settings */
					RTC_DateStructure.RTC_Year = time[0];
					RTC_DateStructure.RTC_Month = time[1];
					RTC_DateStructure.RTC_Date = time[2];

					RTC_TimeStructure.RTC_Hours = time[3];
					RTC_TimeStructure.RTC_Minutes = time[4];
					RTC_TimeStructure.RTC_Seconds = time[5];

					RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
					RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);

					/* Reset variables */
					uIdleCntr = 0;
					bInitNeed = 1;
					DISP_Clear();

					/* Change back to main screen task */
					vTaskResume(xTaskHandleMainScreen);
					vTaskSuspend(NULL );
				}
				/* Step to next part */
				else
				{
					/* If it's a date */
					if(uDatePtr < 2)
					{
						uColPrev = 43 + (uDatePtr * 18);
						uColNext = 43 + ((uDatePtr + 1) * 18);
						uLineNext = uLinePrev = 4;
					}
					/* If it's a switch between the date and time */
					else if(uDatePtr == 2)
					{
						uColPrev = 79;
						uColNext = 40;
						uLinePrev = 4;
						uLineNext = 5;
					}
					/* If it's a time */
					else
					{
						uColPrev = 40 + ((uDatePtr - 3) * 18);
						uColNext = 40 + ((uDatePtr - 2) * 18);
						uLineNext = uLinePrev = 5;
					}

					/* Update display */
					DISP_CharWrite(uLinePrev, uColPrev, (time[uDatePtr] / 10) + '0');
					DISP_CharWrite(uLinePrev, uColPrev + 6, (time[uDatePtr] % 10) + '0');
					uDatePtr++;
					DISP_CharWriteInvert(uLineNext, uColNext, (time[uDatePtr] / 10) + '0');
					DISP_CharWriteInvert(uLineNext, uColNext + 6, (time[uDatePtr] % 10) + '0');
				}
			}
			uIdleCntr = 0;
		}
		else
		{
			if(++uIdleCntr >= MENU_EXIT_COUNTER)
			{
				uIdleCntr = 0;
				bInitNeed = 1;
				DISP_Clear();

				/* Change back to main screen task */
				vTaskResume(xTaskHandleMainScreen);
				vTaskSuspend(NULL );
			}
		}
	}
}

/* This task sets the display's brightness */
void vTaskSetBrightness(void *pvParamters)
{
	/* Local variables */
	int8_t sReceivedValue;
	uint16_t uIdleCntr = 0;
	uint8_t uBrightness = 0;
	uint8_t i;

	/* Main loop for this task */
	for(;;)
	{
		/* Update screen */
		DISP_StringWrite(4, 31, "Fenyero:");
		uBrightness = DISP_GetBacklight();
		for(i = 0; i < 8; i++)
		{
			if(i <= uBrightness)
				DISP_BlockWrite(4, 79 + i, ~(0xff >> i));
			else
				DISP_BlockWrite(4, 79 + i, 0);
		}

		/* Wait for rotary action */
		if(xQueueReceive(xQueueMenu, &sReceivedValue, MENU_UPDATE_FREQUENCY))
		{
			/* Rotation received */
			if(sReceivedValue != 0)
			{
				DISP_SetBacklight(prvLimit(DISP_GetBacklight() + sReceivedValue, DISP_MAX_DUTY));
				uBrightness = DISP_GetBacklight();
				for(i = 0; i < 8; i++)
				{
					if(i <= uBrightness)
						DISP_BlockWrite(4, 79 + i, ~(0xff >> i));
					else
						DISP_BlockWrite(4, 79 + i, 0);
				}
			}
			/* Push button received */
			else
			{
				/* Clear display */
				DISP_Clear();

				/* Return back to main screen task */
				vTaskResume(xTaskHandleMainScreen);
				vTaskSuspend(NULL );
			}
			uIdleCntr = 0;
		}
		else
		{
			if(++uIdleCntr >= MENU_EXIT_COUNTER)
			{
				uIdleCntr = 0;
				DISP_Clear();

				/* Change back to main screen task */
				vTaskResume(xTaskHandleMainScreen);
				vTaskSuspend(NULL );
			}
		}
	}
}

/* This task sets the rf module */
void vTaskSetRFModule(void *pvParamters)
{
	/* Local variables */
	int8_t sReceivedValue;
	uint16_t uIdleCntr = 0;
	uint8_t bInitNeed = 1;
	uint8_t bRfStatus = 0;
	uint8_t bRfNewStatus = 0;

	/* Main loop for this task */
	for(;;)
	{
		/* Do some initialization */
		if(bInitNeed)
		{
			DISP_StringWrite(4, 35, "Radios modul:");
			DISP_BlockWrite(5, 57, 0xff);
			if(bRfStatus)
				DISP_StringWriteInvert(5, 58, "Be");
			else
				DISP_StringWriteInvert(5, 58, "Ki");
			bInitNeed = 0;
		}

		/* Wait for rotary action */
		if(xQueueReceive(xQueueMenu, &sReceivedValue, MENU_UPDATE_FREQUENCY))
		{
			if(sReceivedValue != 0)
			{
				/* Rotation received */
				bRfNewStatus = (bRfNewStatus + sReceivedValue) & 0x01;
				if(bRfNewStatus)
					DISP_StringWriteInvert(5, 58, "Be");
				else
					DISP_StringWriteInvert(5, 58, "Ki");
			}
			else
			{
				/* Clear display */
				DISP_Clear();
				bInitNeed = 1;

				/* Push button received */
				if(bRfNewStatus && !bRfStatus)
				{
					/* Turn on */
					if(xSemaphoreTake(xMutexSPIUse, (portTickType) 20))
					{
						RFM_SetSPI();
						rfm70_mode_receive();
						RFM_ITCmd(ENABLE);
						vTaskResume(xTaskHandleRFMRead);
						xSemaphoreGive(xMutexSPIUse);

						if(xSemaphoreTake(xMutexBattVolt, 10 * portTICK_RATE_MS))
						{
							bSensPresent = 1;
							xSemaphoreGive(xMutexBattVolt);
						}
					}
				}
				else if(!bRfNewStatus && bRfStatus)
				{
					/* Turn off */
					if(xSemaphoreTake(xMutexSPIUse, (portTickType) 20))
					{
						RFM_SetSPI();
						rfm70_mode_powerdown();
						RFM_ITCmd(DISABLE);
						vTaskSuspend(xTaskHandleRFMRead);
						xSemaphoreGive(xMutexSPIUse);

						if(xSemaphoreTake(xMutexBattVolt, 10 * portTICK_RATE_MS))
						{
							bSensPresent = 0;
							xSemaphoreGive(xMutexBattVolt);
						}
					}
				}
				bRfStatus = bRfNewStatus;

				/* Return back to main screen task */
				vTaskResume(xTaskHandleMainScreen);
				vTaskSuspend(NULL );
			}
			uIdleCntr = 0;
		}
		else
		{
			if(++uIdleCntr >= MENU_EXIT_COUNTER)
			{
				uIdleCntr = 0;
				bInitNeed = 1;
				DISP_Clear();

				/* Change back to main screen task */
				vTaskResume(xTaskHandleMainScreen);
				vTaskSuspend(NULL );
			}
		}
	}
}

void vTaskTurnOff(void *pvParameters)
{
	PWR_WakeUpPinCmd(ENABLE);
	PWR_EnterSTANDBYMode();
}

/*-----------------------------------------------------------*/
void prvTempStoreTask(void *pvParameters)
{
	portTickType xNextWakeTime;
	int32_t uCurrentTemp = 0;
	uint8_t bIsHeatOn = 0, i;
	int32_t uAvgTemp = 0;
	uint8_t temp = 0;

	/* Take the memory protector mutex */
	xSemaphoreTake(xMutexTempMemory, portMAX_DELAY);

	/* Allocate the memory for temp measure log, pointers should be global and protected */
	psStartOfBuffer = pvPortMalloc(SIZE_OF_BUFFER);
	uBuffCntr = 0x00;

	/* First measure */
	if(xSemaphoreTake(xMutexSPIUse, (portTickType) 50))
	{
		EADC_SetSPI();
		uCurrentTemp = EADC_GetTemperature();
		xSemaphoreGive(xMutexSPIUse);
	}
	for(i = 0; i < 5; i++)
	{
		psStartOfBuffer[uBuffCntr] = uCurrentTemp;
		uBuffCntr = (uBuffCntr + 1) & 0x7f;
	}
	uAvgTemp = uCurrentTemp;

	/* Give back the mutex */
	xSemaphoreGive(xMutexTempMemory);

	/* Start the display */
	DISP_Clear();
	vTaskResume(xTaskHandleMainScreen);

	/* Initialize xNextWakeTime - this only needs to be done once. */
	//xNextWakeTime = xTaskGetTickCount();

	EADC_ITCmd(ENABLE);
	for(;;)
	{
		//vTaskDelayUntil(&xNextWakeTime, MEASURE_TEMP_FREQ);
		xSemaphoreTake(xSemaphoreTempReady, portMAX_DELAY);

		/* Get the current ADC result */
		if(xSemaphoreTake(xMutexSPIUse, (portTickType) 20))
		{
			EADC_SetSPI();
			uCurrentTemp = EADC_GetTemperature();
			xSemaphoreGive(xMutexSPIUse);
		}
		else
		{
			/* Skip this measure */
			uCurrentTemp = 0;
		}

		/* If the value is seems corrupted, leave it */
		if(abs(uCurrentTemp - uAvgTemp) < TEMP_ERROR_DIFFERENCE)
		{
			/* Get the global buffer */
			if(xSemaphoreTake(xMutexTempMemory, (portTickType) 5) == pdTRUE)
			{

				/* Store the current temperature */
				psStartOfBuffer[uBuffCntr] = uCurrentTemp;
				uBuffCntr = (uBuffCntr + 1) & 0x7f;

				/* Summarize the latest 5 data */
				uAvgTemp = 0;
				for(i = 0; i < 5; i++)
					uAvgTemp += psStartOfBuffer[(uBuffCntr - 5 + i) & 0x7f];

				/* Our work is done here, give back the mutex */
				xSemaphoreGive(xMutexTempMemory);

				/* Compute average */
				uAvgTemp /= 5;

				/* Check if heating is  */
				if(uAvgTemp < (sTempLimit - (bIsHeatOn ? 0 : HYSTERESIS)))
				{
					if(bIsHeatOn == 0)
					{
						RELAY_Heat(RELAY_FanSpeed_OFF);
						bIsHeatOn = 1;
					}
				}
				else
				{
					if(bIsHeatOn == 1)
					{
						RELAY_OFF();
						bIsHeatOn = 0;
					}
				}
			}
		}
	}
}

void vTaskRFMRead(void *pvParameters)
{
	uint8_t aReceiveBuff[32];
	uint8_t uPipe;
	uint8_t uLength;

	uint16_t bat;
	uint16_t temp;

	xSemaphoreTake(xMutexSPIUse, portMAX_DELAY);
	rfm70_init();
	rfm70_mode_powerdown();
	xSemaphoreGive(xMutexSPIUse);
	RFM_ITCmd(DISABLE);
	vTaskSuspend(NULL );
	for(;;)
	{
		xSemaphoreTake(xSemaphoreRFInt, portMAX_DELAY);

		uPipe = 6;
		uLength = 0;
		if(xSemaphoreTake(xMutexSPIUse, (portTickType) 20))
		{
			RFM_SetSPI();

			if(rfm70_register_read(RFM70_REG_STATUS) & 0x40)
			{
				if(rfm70_receive(&uPipe, aReceiveBuff, &uLength))
				{
					rfm70_register_write(RFM70_CMD_FLUSH_RX, 0);
					if(uLength == 6)
					{
						bat = aReceiveBuff[0] | (aReceiveBuff[1] << 8);
						temp = aReceiveBuff[2] | (aReceiveBuff[3] << 8);
					}

				}
				rfm70_register_write(RFM70_REG_STATUS, 0x70);
			}
			xSemaphoreGive(xMutexSPIUse);
			if(bat)
			{
				if(xSemaphoreTake(xMutexBattVolt, (portTickType) 20))
				{
					uSensBattVoltage = bat;
					uSensExtTemp = temp;
					xSemaphoreGive(xMutexBattVolt);
				}
				bat = 0;
			}

		}
	}
}

void vTaskUSB(void *pvParameters)
{
	portTickType xNextWakeTime;

	/* Initialize xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	for(;;)
	{
		vTaskDelayUntil(&xNextWakeTime, 20);
		USBH_Process(&USB_OTG_Core, &USB_Host);
	}
}
/*-----------------------------------------------------------*/

/*
 * @brief	This function returns the limited value of Value
 * @param	Value: This value will be limited
 * @param	Limit: This value it the limit, it could be negative too
 * @retval	The limited value
 */
int16_t prvLimit(int16_t Value, int16_t Limit)
{
	if(Limit < 0)
	{
		if(Value > 0)
			return 0;
		else if(Value < Limit)
			return Limit;
	}
	else
	{
		if(Value < 0)
			return 0;
		else if(Value > Limit)
			return Limit;
	}
	return Value;
}

float prvLimitInterval(float Value, float Min, float Max)
{
	if(Value < Min)
		return Min;
	if(Value > Max)
		return Max;
	else
		return Value;
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	 free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	 internally by FreeRTOS API functions that create tasks, queues, software
	 timers, and semaphores.  The size of the FreeRTOS heap is set by the
	 configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for(;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
{
	(void) pcTaskName;
	(void) pxTask;

	/* Run time stack overflow checking is performed if
	 configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	 function is called if a stack overflow is detected. */
	for(;;)
		;
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void)
{
	volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	 does nothing useful, other than report the amout of FreeRTOS heap that
	 remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if(xFreeStackSpace > 100)
	{
		/* By now, the kernel has allocated everything it is going to, so
		 if there is a lot of heap remaining unallocated then
		 the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		 reduced accordingly. */
	}
}

/*
 * Minimal __assert_func used by the assert() macro
 * */
void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
	while(1)
	{
	}
}

/*
 * Minimal __assert() uses __assert__func()
 * */
void __assert(const char *file, int line, const char *failedexpr)
{
	__assert_func(file, line, NULL, failedexpr);
}

