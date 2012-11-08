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

void RTCInit(void);
void RTC_Config(void);
void RTC_TimeToString(char* String, RTCShowSeconds_TypeDef Show);
void RTC_DateToString(char* String);

#endif /* RTC_H_ */
