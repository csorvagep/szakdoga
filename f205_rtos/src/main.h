/*
 * main.h
 *
 *  Created on: 2012.10.24.
 *      Author: Gábor
 */

#ifndef MAIN_H_
#define MAIN_H_

/* Includes */
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

/* Priorities at which the tasks are created. */
#define menuQUEUE_TASK_PRIORITY			( tskIDLE_PRIORITY + 2 )
#define rotaryQUEUE_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define tempMEASURE_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )

/* The rate at which data is sent to the queue, specified in milliseconds, and
 converted to ticks using the portTICK_RATE_MS constant. */
#define ROTARY_CHK_FREQUENCY				( 100 / portTICK_RATE_MS )
#define ROTARY_PB_DENY						( 200 / portTICK_RATE_MS )
#define MENU_UPDATE_FREQUENCY				( 5000 / portTICK_RATE_MS )
#define MEASURE_TEMPERATURE_FREQUENCY	( 1000 / portTICK_RATE_MS )

#define MENU_EXIT_COUNTER					( 6 )
#define DISPLAY_OFF_TIME					( 30000 / ( MENU_UPDATE_FREQUENCY * portTICK_RATE_MS ) )

/* The number of items the queue can hold.  This is 1 as the receive task
 will remove items as they are added, meaning the send task should always find
 the queue empty. */
#define menuQUEUE_LENGTH					( 10 )

/*-----------------------------------------------------------*/

/* Setup the NVIC, LED outputs, and button inputs. */
static void prvSetupHardware(void);

/* Tasks */
static void prvMenuTask(void *pvParameters);
static void prvRotaryChkTask(void *pvParameters);
static void prvTempStoreTask(void *pvParameters);

/* Timer Callback functions */
static void vAllowRotary(xTimerHandle pxTimer);
static void vSendUpdate(xTimerHandle pxTimer);
static void vStartMeasure(xTimerHandle pxTimer);

/* Other functions */
int16_t prvLimit(int16_t Value, int16_t Limit);
#endif /* MAIN_H_ */
