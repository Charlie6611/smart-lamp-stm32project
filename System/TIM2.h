#ifndef __TIM2_H__
#define __TIM2_H__

#include "stm32f10x.h"                  // Device header
#include "key.h"   
#include "hc_sr04.h"
#include "led.h"
#include "buzzer.h"
#include "myrtc.h"

void Timer2_Init(u16 Prescaler, u16 Period);


extern uint8_t systemModel;				//系统模式
extern uint8_t menu;					//显示菜单变量
#endif
