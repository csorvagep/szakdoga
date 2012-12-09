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

#define DISPLAY_HANDLE_PRIORITY			( tskIDLE_PRIORITY + 2 )

#define USB_PRIORITY							( tskIDLE_PRIORITY + 3 )

#define ROTARY_CHK_FREQUENCY				( 100 / portTICK_RATE_MS )
#define ROTARY_PB_DENY						( 200 / portTICK_RATE_MS )
#define MENU_UPDATE_FREQUENCY				( 2000 / portTICK_RATE_MS )

#define RFM_READ_FREQ						( 1000 / portTICK_RATE_MS )

#define MENU_EXIT_COUNTER					( 15 )
#define DISPLAY_OFF_TIME					( (30 * 1000) / (MENU_UPDATE_FREQUENCY * portTICK_RATE_MS) )

#define DISP_TASKS_COUNT 6

#define HYSTERESIS							0x115//0x22A // Kb. 0,5 fok
#define TEMP_ERROR_DIFFERENCE				15000//5538 // Kb. 5 fok
#define menuQUEUE_LENGTH					( 10 )
#define SIZE_OF_BUFFER						( 128 )

/*-----------------------------------------------------------*/

/* Tasks */
void prvRotaryChkTask(void *pvParameters);
void prvTempStoreTask(void *pvParameters);

void vTaskMainScreen(void *pvParameters);
void vTaskMenuSelect(void *pvParameters);
void vTaskSetTimeDate(void *pvParameters);
void vTaskSetBrightness(void *pvParameters);
void vTaskSetRFModule(void *pvParameters);
void vTaskTurnOff(void *pvParameters);
void vTaskSetupProgram(void *pvParameters);

void vTaskUSB(void *pvParameters);

void vTaskRFMRead(void *pvParameters);

void vTaskRotaryIT(void *pvParameters);

/* Timer Callback functions */
void vAllowRotary(xTimerHandle pxTimer);

/* Other functions */
int16_t prvLimit(int16_t Value, int16_t Min, int16_t Max);
float prvLimitInterval(float Value, float Min, float Max);
#endif /* MAIN_H_ */
