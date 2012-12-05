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

/* USB OTG Host */
#include "usbh_core.h"
#include "usbh_usr.h"
#include "usbh_msc_core.h"

#define rotaryQUEUE_TASK_PRIORITY		( tskIDLE_PRIORITY + 3 )
#define tempMEASURE_TASK_PRIORITY		( tskIDLE_PRIORITY + 4 )
#define RFM_TASK_PRIORITY					( tskIDLE_PRIORITY + 1 )

#define MAIN_SCREEN_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define MENU_SELECT_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define SET_TIMEDATE_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define SET_BRIGHTNESS_PRIORITY			( tskIDLE_PRIORITY + 2 )
#define SLEEP_TASK_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define RFMODULE_PRIORITY					( tskIDLE_PRIORITY + 2 )
#define TURNOFF_PRIORITY					( tskIDLE_PRIORITY + 2 )

#define USB_PRIORITY							( tskIDLE_PRIORITY + 3 )

#define ROTARY_CHK_FREQUENCY				( 100 / portTICK_RATE_MS )
#define ROTARY_PB_DENY						( 200 / portTICK_RATE_MS )
#define MENU_UPDATE_FREQUENCY				( 3000 / portTICK_RATE_MS )
#define MEASURE_TEMP_FREQ					( 1000 / portTICK_RATE_MS )

#define RFM_READ_FREQ						( 1000 / portTICK_RATE_MS )

#define MENU_EXIT_COUNTER					( 15 )
#define DISPLAY_OFF_TIME					( 30 )

#define DISP_TASKS_COUNT 6

#define HYSTERESIS							0x22A // Kb. 0,5 fok
#define TEMP_ERROR_DIFFERENCE				5538 // Kb. 5 fok

#define menuQUEUE_LENGTH					( 10 )
#define SIZE_OF_BUFFER						( 128 )

/*-----------------------------------------------------------*/

/* Tasks */
static void prvRotaryChkTask(void *pvParameters);
static void prvTempStoreTask(void *pvParameters);

static void vTaskMainScreen(void *pvParameters);
static void vTaskMenuSelect(void *pvParameters);
static void vTaskSetTimeDate(void *pvParameters);
static void vTaskSetBrightness(void *pvParameters);
static void vTaskSleep(void *pvParameters);
static void vTaskSetRFModule(void *pvParameters);
static void vTaskTurnOff(void *pvParameters);

static void vTaskUSB(void *pvParameters);

static void vTaskRFMRead(void *pvParameters);

/* Timer Callback functions */
static void vAllowRotary(xTimerHandle pxTimer);

/* Other functions */
static int16_t prvLimit(int16_t Value, int16_t Limit);
static float prvLimitInterval(float Value, float Min, float Max);
#endif /* MAIN_H_ */
