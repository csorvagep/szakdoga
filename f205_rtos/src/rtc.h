/*
 * rtc.h
 *
 *  Created on: 2012.10.18.
 *      Author: Gábor
 */

#ifndef RTC_H_
#define RTC_H_

typedef enum {
	RTC_ShowSeconds_Yes = 0x01,
	RTC_ShowSeconds_No = 0x00
}RTCShowSeconds_TypeDef;

#define BKP_TEMP_LIMIT1_OFFSET	0
#define BKP_TEMP_LIMIT2_OFFSET	4
#define BKP_BACKLIGHT_OFFSET		8

void RTCInit(void);
void RTC_Config(void);
void RTC_TimeToString(char* String, RTCShowSeconds_TypeDef Show);
void RTC_DateToString(char* String);

#endif /* RTC_H_ */
