/*
 * main.h
 *
 *  Created on: 2012.10.24.
 *      Author: Gábor
 */

#ifndef MAIN_H_
#define MAIN_H_

/* Includes */
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

/* STM32 include */
#include "stm32f2xx.h"

typedef enum
{
	Queue_Mode_Rotary = 0x00,
	Queue_Mode_Update = 0x01
} QueueMode_TypeDef;

typedef struct
{
	signed short Data;
	QueueMode_TypeDef Mode;
} xQueueElement;

const xQueueElement xUpdate =
{ .Mode = Queue_Mode_Update, .Data = 0x00 };

#define UpdateMenu()	xQueueSend(xQueueMenu, &xUpdate, 5)

#define StepNextStage(NextStage)			\
					do 							\
					{ 								\
						DISP_Clear(); 			\
						ucMode = (NextStage);\
						UpdateMenu();			\
					} while(0)

#define rotaryQUEUE_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define tempMEASURE_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )

#define MAIN_SCREEN_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define MENU_SELECT_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define SET_TIMEDATE_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define SET_BRIGHTNESS_PRIORITY			( tskIDLE_PRIORITY + 2 )
#define SLEEP_TASK_PRIORITY				( tskIDLE_PRIORITY + 2 )

#define ROTARY_CHK_FREQUENCY				( 100 / portTICK_RATE_MS )
#define ROTARY_PB_DENY						( 200 / portTICK_RATE_MS )
#define MENU_UPDATE_FREQUENCY				( 1000 / portTICK_RATE_MS )
#define MEASURE_TEMPERATURE_FREQUENCY	( 500 / portTICK_RATE_MS )

#define MENU_EXIT_COUNTER					( 15 )
#define DISPLAY_OFF_TIME					( 30 )


#define HYSTERESIS							0x22A // Kb. 0,5 fok
#define TEMP_ERROR_DIFFERENCE				5000/*2223*/

#define menuQUEUE_LENGTH					( 10 )
#define SIZE_OF_BUFFER						( 128 )

/*-----------------------------------------------------------*/

/* Tasks */
static void prvMenuTask(void *pvParameters);
static void prvRotaryChkTask(void *pvParameters);
static void prvTempStoreTask(void *pvParameters);

static void vTaskMainScreen(void *pvParameters);
static void vTaskMenuSelect(void *pvParameters);
static void vTaskSetTimeDate(void *pvParameters);
static void vTaskSetBrightness(void *pvParameters);
static void vTaskSleep(void *pvParameters);

/* Timer Callback functions */
static void vAllowRotary(xTimerHandle pxTimer);
static void vSendUpdate(xTimerHandle pxTimer);
static void vStartMeasure(xTimerHandle pxTimer);

/* Other functions */
static int16_t prvLimit(int16_t Value, int16_t Limit);
static float prvLimitInterval(float Value, float Min, float Max);
#endif /* MAIN_H_ */
