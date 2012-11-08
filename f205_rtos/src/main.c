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

/* The queue used by both tasks. */
xQueueHandle xQueueMenu = NULL;

xTimerHandle xTimerRotaryAllow = NULL;
xTimerHandle xTimerUpdateDisplay = NULL;
xTimerHandle xTimerTempMeasure = NULL;

xSemaphoreHandle xSemaphoreTempReady = NULL;
xSemaphoreHandle xMutexTempMemory = NULL;

/* Variables for temperature measuring */
#define SIZE_OF_BUFFER 128
int32_t *psStartOfBuffer = NULL;
uint8_t uBuffCntr = 0;

extern int16_t DeltaValue;

int main(void)
{
	/* Hardware functions, GPIO, RCC, etc... */
	prvSetupHardware();

	/* Create the menu queue. */
	xQueueMenu = xQueueCreate( menuQUEUE_LENGTH, sizeof( xQueueElement ) );

	vSemaphoreCreateBinary(xSemaphoreTempReady);
	xMutexTempMemory = xSemaphoreCreateMutex();

	if(xQueueMenu != NULL )
	{
		/* Create tasks */
		xTaskCreate( prvMenuTask, ( signed char * ) "Rx", 255, NULL, menuQUEUE_TASK_PRIORITY, NULL);
		xTaskCreate( prvRotaryChkTask, ( signed char * ) "Tx", configMINIMAL_STACK_SIZE, NULL,
				rotaryQUEUE_TASK_PRIORITY, NULL);
		xTaskCreate( prvTempStoreTask, ( signed char * ) "Temp", configMINIMAL_STACK_SIZE, NULL,
				tempMEASURE_TASK_PRIORITY, NULL);

		/* Create timers */
		xTimerRotaryAllow = xTimerCreate("Rotary", ROTARY_PB_DENY, pdFALSE, (void *) 0, vAllowRotary);
		xTimerUpdateDisplay = xTimerCreate("Updater", MENU_UPDATE_FREQUENCY, pdTRUE, (void *) 0,
				vSendUpdate);
		xTimerTempMeasure = xTimerCreate("TempM", MEASURE_TEMPERATURE_FREQUENCY, pdTRUE, NULL,
				vStartMeasure);

		xTimerStart(xTimerTempMeasure, 0);

		/* Start the tasks and timer running. */
		vTaskStartScheduler();
	}

	/* Never should reach here */
	for(;;)
		;
}

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

void vSendUpdate(xTimerHandle pxTimer)
{
	xQueueSend(xQueueMenu, &xUpdate, 5);
}
/*-----------------------------------------------------------*/

static void prvRotaryChkTask(void *pvParameters)
{
	portTickType xNextWakeTime;
	signed short CurrentVal = 0;
	xQueueElement xValueHolder;

	/* Initialize xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();

	xValueHolder.Mode = Queue_Mode_Rotary;
	xValueHolder.Data = 0;

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
			xValueHolder.Data = CurrentVal;
			xQueueSend( xQueueMenu, &xValueHolder, 0);
		}
	}
}
/*-----------------------------------------------------------*/

static void prvMenuTask(void *pvParameters)
{
	xQueueElement xReceivedValue;
	RTC_TimeTypeDef RTC_TimeStructure;
	RTC_DateTypeDef RTC_DateStructure;
	signed short sMenuPtr = 0, sLastMenuPtr = 0;
	unsigned char ucMode = 2;
	char time_string[15];
	char temp_str[15];
	uint8_t i;
	uint16_t time[6] = {
			0, 0, 0, 0, 0, 0 };
	uint8_t date_ptr = 0;
	uint8_t uLimit = 0;
	uint8_t uColNext = 0, uColPrev = 0;
	uint8_t uLineNext = 0, uLinePrev = 0;

	uint32_t temperature;
	float rt;
	float t,t2;

	UpdateMenu();

	for(;;)
	{
		/* Wait for item */
		xQueueReceive( xQueueMenu, &xReceivedValue, portMAX_DELAY);

		switch(ucMode)
		{
		/* Wait for PushButton, then write out the menu */
		case 0:
			if(xReceivedValue.Data == 0 && xReceivedValue.Mode == Queue_Mode_Rotary)
			{
				sMenuPtr = 0;
				StepNextStage(1);
			}
			else if(xReceivedValue.Mode == Queue_Mode_Update)
			{
				RTC_TimeToString(time_string, RTC_ShowSeconds_No);
				DISP_2LineStringWrite(1, 0, time_string);
				RTC_DateToString(time_string);
				DISP_StringWrite(2, 0, time_string);

				if(xSemaphoreTake(xMutexTempMemory, (portTickType) 5) == pdTRUE)
				{
					if(psStartOfBuffer)
					{
						temperature = psStartOfBuffer[uBuffCntr];
						xSemaphoreGive(xMutexTempMemory);

						rt = 27.E3 * (temperature) / (0x7fffff - temperature);
						t = (rt / 1000 - 1) / 3.9083E-3;
						DISP_2LineNumWrite(1, 90, ((uint8_t) floor(t / 10.0)) + '0');
						DISP_2LineNumWrite(1, 101, ((uint8_t) t % (uint8_t) 10.0) + '0');
						DISP_2LineNumWrite(1, 112, '.');
						t -= (float) (uint8_t) t;
						DISP_2LineNumWrite(1, 116, ((uint8_t) floor(t * 10.0)) + '0');
					}
				}
			}
			break;

		case 1:
			if(xReceivedValue.Data != 0 && xReceivedValue.Mode == Queue_Mode_Rotary)
			{
				sMenuPtr = (sMenuPtr + xReceivedValue.Data);
				if(sMenuPtr > MENU_MAX)
					sMenuPtr = MENU_MAX;
				else if(sMenuPtr < 0)
					sMenuPtr = 0;
			}
			else if(xReceivedValue.Data == 0 && xReceivedValue.Mode == Queue_Mode_Rotary)
			{
				StepNextStage(sMenuPtr + 10);
				break;
			}
			if(sMenuPtr != sLastMenuPtr || xReceivedValue.Mode == Queue_Mode_Update)
			{
				if(sMenuPtr == 0)
					DISP_CharWrite(0, 0, ' ');
				else
					DISP_CharWrite(0, 0, '<');

				if(sMenuPtr == MENU_MAX)
					DISP_CharWrite(0, 122, ' ');
				else
					DISP_CharWrite(0, 122, '>');

				for(i = 0; i < 118; i++)
					DISP_BlockWrite(0, 5 + i, 0x00);

				DISP_StringWrite(0, (uint8_t) (64 - (strlen(first_row[sMenuPtr]) * 6 / 2)),
						first_row[sMenuPtr]);
			}

			sLastMenuPtr = sMenuPtr;
			break;

		case 2:
			RTC_TimeToString(time_string, RTC_ShowSeconds_No);
			DISP_2LineStringWrite(1, 0, time_string);
			RTC_DateToString(time_string);
			DISP_StringWrite(2, 0, time_string);
			ucMode = 0;
			if(xTimerIsTimerActive(xTimerUpdateDisplay) == pdFALSE)
				xTimerStart(xTimerUpdateDisplay, 5);
			UpdateMenu();
			break;

		case 10:
			if(xReceivedValue.Mode == Queue_Mode_Update)
			{
				RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
				RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
				time[0] = RTC_DateStructure.RTC_Year;
				time[1] = RTC_DateStructure.RTC_Month;
				time[2] = RTC_DateStructure.RTC_Date;
				time[3] = RTC_TimeStructure.RTC_Hours;
				time[4] = RTC_TimeStructure.RTC_Minutes;
				time[5] = RTC_TimeStructure.RTC_Seconds;
				RTC_DateToString(time_string);
				strncpy(temp_str, time_string, 2);
				temp_str[2] = '\0';
				DISP_StringWrite(4, 31, temp_str);
				strncpy(temp_str, time_string + 2, 2);
				temp_str[2] = '\0';
				DISP_StringWriteInvert(4, 43, temp_str);
				strcpy(temp_str, time_string + 4);
				DISP_StringWrite(4, 55, temp_str);
				RTC_TimeToString(time_string, RTC_ShowSeconds_Yes);
				DISP_StringWrite(5, 40, time_string);
				date_ptr = 0;
				ucMode = 20;
				i = 0;
			}
			break;

		case 12:
			if(xReceivedValue.Mode == Queue_Mode_Update)
			{
				DISP_StringWrite(4, 31, "Fenyero: ");
				DISP_CharWriteInvert(4, 85, DISP_GetBacklight()/10+'0');
				DISP_CharWriteInvert(4, 91, DISP_GetBacklight()%10+'0');
			}
			else if(xReceivedValue.Mode == Queue_Mode_Rotary && xReceivedValue.Data == 0)
			{
				StepNextStage(2);
			}
			else
			{
				DISP_SetBacklight(prvLimit(DISP_GetBacklight() + xReceivedValue.Data, DISP_MAX_DUTY));
				DISP_CharWriteInvert(4, 85, DISP_GetBacklight()/10+'0');
				DISP_CharWriteInvert(4, 91, DISP_GetBacklight()%10+'0');
			}
			break;

		case 20:
			if(xReceivedValue.Mode == Queue_Mode_Update)
			{
				i++;
				if(i == MENU_EXIT_COUNTER)
				{
					StepNextStage(2);
				}
			}
			else if(xReceivedValue.Data == 0 && xReceivedValue.Mode == Queue_Mode_Rotary)
			{
				i = 0;

				if(date_ptr == 5)
				{
					RTC_DateStructure.RTC_Year = time[0];
					RTC_DateStructure.RTC_Month = time[1];
					RTC_DateStructure.RTC_Date = time[2];

					RTC_TimeStructure.RTC_Hours = time[3];
					RTC_TimeStructure.RTC_Minutes = time[4];
					RTC_TimeStructure.RTC_Seconds = time[5];

					RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);
					RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);

					StepNextStage(2);
				}
				else
				{
					if(date_ptr < 2)
					{
						uColPrev = 43 + (date_ptr * 18);
						uColNext = 43 + ((date_ptr + 1) * 18);
						uLineNext = uLinePrev = 4;
					}
					else if(date_ptr == 2)
					{
						uColPrev = 79;
						uColNext = 40;
						uLinePrev = 4;
						uLineNext = 5;
					}
					else
					{
						uColPrev = 40 + ((date_ptr - 3) * 18);
						uColNext = 40 + ((date_ptr - 2) * 18);
						uLineNext = uLinePrev = 5;
					}
					DISP_CharWrite(uLinePrev, uColPrev, (time[date_ptr] / 10) + '0');
					DISP_CharWrite(uLinePrev, uColPrev + 6, (time[date_ptr] % 10) + '0');
					date_ptr++;
					DISP_CharWriteInvert(uLineNext, uColNext, (time[date_ptr] / 10) + '0');
					DISP_CharWriteInvert(uLineNext, uColNext + 6, (time[date_ptr] % 10) + '0');
				}
			}
			else
			{
				i = 0;
				switch(date_ptr)
				{
				case 0:
					uLimit = 99;
					break;
				case 1:
					uLimit = 12;
					break;
				case 2:
					uLimit = 31;
					break;
				case 3:
					uLimit = 23;
					break;
				case 4:
				case 5:
					uLimit = 59;
					break;
				}
				time[date_ptr] = (uint8_t) prvLimit(time[date_ptr] + xReceivedValue.Data, uLimit);

				if(date_ptr < 3)
				{
					DISP_CharWriteInvert(4, 43 + (date_ptr * 18), (time[date_ptr] / 10) + '0');
					DISP_CharWriteInvert(4, 49 + (date_ptr * 18), (time[date_ptr] % 10) + '0');
				}
				else
				{
					DISP_CharWriteInvert(5, 40 + ((date_ptr-3) * 18), (time[date_ptr] / 10) + '0');
					DISP_CharWriteInvert(5, 46 + ((date_ptr-3) * 18), (time[date_ptr] % 10) + '0');
				}
			}
			break;

		default:
			StepNextStage(2);
			break;
		}

	}
}
/*-----------------------------------------------------------*/
static void prvTempStoreTask(void *pvParameters)
{
	uint8_t ret[3];
	uint32_t temp;
	uint8_t i;

	/* Allocate the memory for temp measure log, pointers should be global and protected */
	psStartOfBuffer = pvPortMalloc(SIZE_OF_BUFFER);
	uBuffCntr = 0x7f;

	for(;;)
	{
		/* Block until new measure is ready */
		xSemaphoreTake(xSemaphoreTempReady, portMAX_DELAY);

		while(!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8 ))
			;

		EADC_SPI_NSS_ON();

		SPI_I2S_SendData(SPI3, EADC_COMMAND_WAKEUP);
		while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE ))
			;
		SPI_I2S_SendData(SPI3, EADC_COMMAND_SLEEP);
		while(RESET == SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE ))
			;

		EADC_SPI_NSS_OFF();

		for(i=15;i;i--);

		EADC_SPI_NSS_ON();

		while(!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8 ))
			i++;

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

		EADC_SPI_NSS_OFF();

		temp = 0x00000000 | (ret[0] << 16) | (ret[1] << 8) | ret[2];

		if(xSemaphoreTake(xMutexTempMemory, (portTickType) 2) == pdTRUE)
		{
			uBuffCntr = (uBuffCntr + 1) & 0x7f;
			psStartOfBuffer[uBuffCntr] = temp;

			xSemaphoreGive(xMutexTempMemory);
		}
	}
}
/*-----------------------------------------------------------*/

static void prvSetupHardware(void)
{
	/* Ensure that all 4 interrupt priority bits are used as the pre-emption
	 priority. */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2 );

	/* Set up the display and the backlight */
	Backlight_Init();
	DISP_Init();

	RELAY_Init();

	ROT_Init();

	RTCInit();

	EADC_SPIInit();

}

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

#ifdef  USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *   where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif

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

