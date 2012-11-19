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

/* Semaphore Handles */
xSemaphoreHandle xSemaphoreTempReady = NULL;

xTaskHandle xTaskHandleMainScreen = NULL;
xTaskHandle xTaskHandleMenuSelect = NULL;
xTaskHandle xTaskHandleSetTimeDate = NULL;
xTaskHandle xTaskHandleSetBrightness = NULL;
xTaskHandle xTaskHandleSleep = NULL;

xTaskHandle * aMenuTaskPtrs[MENU_MAX + 1] = {
		&xTaskHandleSetTimeDate, &xTaskHandleMainScreen, &xTaskHandleSetBrightness,
		&xTaskHandleMainScreen, &xTaskHandleMainScreen, &xTaskHandleMainScreen };

/* Global variables for temperature measuring */
int32_t *psStartOfBuffer = NULL;
uint8_t uBuffCntr = 0;
int32_t sTempLimit = 0x4e8f0;
float fTempLimit = 20.0;

/* Global variable used by Rotary */
extern int16_t DeltaValue;

/* Main Function, Program entry point */
int main(void)
{
	/* Ensure that all 4 interrupt priority bits are used as the pre-emption priority. */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2 ); // TODO Megnézni jó-e így

	/* Initialize the components */
	Backlight_Init();
	DISP_Init();
	RELAY_Init();
	ROT_Init(); //TODO visszatérés, hogy ha be kell állítani akkor azzal kezdjen
	RTCInit();
	EADC_Init();

	/* Create Queues */
	xQueueMenu = xQueueCreate( menuQUEUE_LENGTH, sizeof( int8_t ) );

	/* Create Mutexes */
	xMutexTempMemory = xSemaphoreCreateMutex();
	xMutexTempLimit = xSemaphoreCreateMutex();
	xMutexSPIUse = xSemaphoreCreateMutex();

	/* Create Semaphores */
	vSemaphoreCreateBinary(xSemaphoreTempReady);

	/* Create Tasks */
	xTaskCreate( vTaskMainScreen, ( signed char * ) "MainScreen", 70, NULL, MAIN_SCREEN_PRIORITY,
			&xTaskHandleMainScreen);
	xTaskCreate( vTaskMenuSelect, ( signed char * ) "MenuSelect", 70, NULL, MENU_SELECT_PRIORITY,
			&xTaskHandleMenuSelect);
	xTaskCreate( vTaskSetTimeDate, ( signed char * ) "SetTimeDate", 100, NULL, SET_TIMEDATE_PRIORITY,
			&xTaskHandleSetTimeDate);
	xTaskCreate( vTaskSetBrightness, ( signed char * ) "SetBrightness", 70, NULL,
			SET_BRIGHTNESS_PRIORITY, &xTaskHandleSetBrightness);
	xTaskCreate( vTaskSleep, ( signed char * ) "Sleep", 70, NULL, SLEEP_TASK_PRIORITY,
			&xTaskHandleSleep);

	xTaskCreate( prvRotaryChkTask, ( signed char * )"Rotary", 70, NULL,
			rotaryQUEUE_TASK_PRIORITY, NULL);
	xTaskCreate( prvTempStoreTask, ( signed char * ) "Temp", 70, NULL, tempMEASURE_TASK_PRIORITY,
			NULL);

	xTaskCreate(vTaskRFMRead, (signed char *)"rfm", 70, NULL, RFM_TASK_PRIORITY, NULL);

	/* Create Timers */
	xTimerRotaryAllow = xTimerCreate((signed char *) "Rotary", ROTARY_PB_DENY, pdFALSE, (void *) 0,
			vAllowRotary);
	xTimerTempMeasure = xTimerCreate((signed char *) "TempM", MEASURE_TEMPERATURE_FREQUENCY, pdTRUE,
			NULL, vStartMeasure);

	/* Start Temp Measure Timer */
	xTimerStart(xTimerTempMeasure, 0);

	/* Start with new measure */
	xSemaphoreGive(xSemaphoreTempReady);

	vTaskSuspend(xTaskHandleMainScreen);
	vTaskSuspend(xTaskHandleMenuSelect);
	vTaskSuspend(xTaskHandleSetTimeDate);
	vTaskSuspend(xTaskHandleSetBrightness);
	vTaskSuspend(xTaskHandleSleep);

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
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;

	NVIC_Init(&NVIC_InitStruct);
}

/*
 * Tasks
 */

/* This task retrieves the value of the rotary switch and send it to the queue */
static void prvRotaryChkTask(void *pvParameters)
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
		}
	}
}

/* This task displays the main screen, sets the temperature limit */
static void vTaskMainScreen(void *pvParamters)
{
	int8_t sReceivedValue;
	uint16_t uIdleCntr = 0;
	uint8_t bUpdateNeed = 1;
	float prvfTempLimit;
	float fRt, fT;
	int32_t prvsTempLimit, prvTemp;
	char time_string[15];
	uint8_t i;

	/* Main loop for this task */
	for(;;)
	{
		/* Do some update */
		/* Increase the idle counter, perform an update */
		if(bUpdateNeed)
		{
			/* Update the time and date */
			RTC_TimeToString(time_string, RTC_ShowSeconds_No);
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
					fRt = 27.E3 * (prvTemp) / (0x7fffff - prvTemp);
					fT = (fRt - 1000) / 3.85;
					DISP_2LineNumWrite(1, 79, ((uint8_t) floor(fT / 10.0)) + '0');
					DISP_2LineNumWrite(1, 90, ((uint8_t) fT % (uint8_t) 10.0) + '0');
					DISP_2LineNumWrite(1, 101, '.');
					fT -= (float) (uint8_t) fT;
					DISP_2LineNumWrite(1, 105, ((uint8_t) floor(fT * 10.0)) + '0');
					DISP_CharWrite(0, 115, 133);
					DISP_CharWrite(0, 121, 'C');

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

				vTaskResume(xTaskHandleSleep);
				vTaskSuspend(NULL );
			}
		}
	}
}

/* This task displays the main menu */
static void vTaskMenuSelect(void *pvParamters)
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
static void vTaskSetTimeDate(void *pvParamters)
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
static void vTaskSetBrightness(void *pvParamters)
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
			/* Push button recieved */
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

/* This task goes to sleep mode */
static void vTaskSleep(void *pvParamters)
{
	int8_t sReceivedValue;

	/* Main loop for this task */
	for(;;)
	{
		/* Block until rotary action */
		if(xQueueReceive(xQueueMenu, &sReceivedValue, portMAX_DELAY))
		{
			/* Send back action to the queue, comment out if needed */
			//xQueueSendToFront(xQueueMenu, &xReceivedValue, 0);
			/* Turn back the display */
			DISP_SetOn();

			/* Switch back to main screen */
			vTaskResume(xTaskHandleMainScreen);
			vTaskSuspend(NULL );
		}
	}

}

/*-----------------------------------------------------------*/
static void prvTempStoreTask(void *pvParameters)
{
	int32_t uCurrentTemp = 0;
	uint8_t bIsHeatOn = 0, i, uTempCntr = 0;
	int32_t uAvgTemp = 0;

	/* Allocate the memory for temp measure log, pointers should be global and protected */
	psStartOfBuffer = pvPortMalloc(SIZE_OF_BUFFER);
	uBuffCntr = 0x00;

	for(;;)
	{
		/* Block until new measure is ready */
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
			uCurrentTemp = 0;
		}


		/* If the value is seems corrupted, leave it */
		if(abs(uCurrentTemp - uAvgTemp) < TEMP_ERROR_DIFFERENCE || !uTempCntr)
		{
			/* Get the global buffer */
			if(xSemaphoreTake(xMutexTempMemory, (portTickType) 5) == pdTRUE)
			{

				/* Store the current temperature */
				psStartOfBuffer[uBuffCntr] = uCurrentTemp;
				uBuffCntr = (uBuffCntr + 1) & 0x7f;

				/* Clone the first measure five times */
				if(!uTempCntr)
				{
					for(i = 0; i < 4; i++)
					{
						psStartOfBuffer[uBuffCntr] = uCurrentTemp;
						uBuffCntr = (uBuffCntr + 1) & 0x7f;
					}
				}

				/* Summarize the latest 5 data */
				uAvgTemp = 0;
				for(i = 0; i < 5; i++)
					uAvgTemp += psStartOfBuffer[(uBuffCntr - 5 + i) & 0x7f];

				/* Our work is done here, give back the mutex */
				xSemaphoreGive(xMutexTempMemory);

				/* Compute avarage */
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
		if(!uTempCntr)
		{
			uTempCntr = 1;
			vTaskResume(xTaskHandleMainScreen);
		}
	}
}

static void vTaskRFMRead(void *pvParameters)
{
	portTickType xNextWakeTime;

	/* Initialize xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	xSemaphoreTake(xMutexSPIUse, portMAX_DELAY);
	rfm70_init();
	xSemaphoreGive(xMutexSPIUse);

	for(;;)
	{
		vTaskDelayUntil(&xNextWakeTime, RFM_READ_FREQ);

		if(xSemaphoreTake(xMutexSPIUse, (portTickType) 20))
		{
			RFM_SetSPI();
			if(0x07 != rfm70_receive_next_pipe())
				DISP_CharWrite(6, 64, '1');
			else
				DISP_CharWrite(6, 64, '0');
			xSemaphoreGive(xMutexSPIUse);
		}

	}
}
/*-----------------------------------------------------------*/

static void vStartMeasure(xTimerHandle pxTimer)
{
	//TODO Do SPI communication to start measure
	//Now just set the semaphore

	xSemaphoreGive(xSemaphoreTempReady);
}

/*
 * @brief	This function returns the limited value of Value
 * @param	Value: This value will be limited
 * @param	Limit: This value it the limit, it could be negative too
 * @retval	The limited value
 */
static int16_t prvLimit(int16_t Value, int16_t Limit)
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

static float prvLimitInterval(float Value, float Min, float Max)
{
	if(Value < Min)
		return Min;
	if(Value > Max)
		return Max;
	else
		return Value;
}

/* TODO float to string */

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

