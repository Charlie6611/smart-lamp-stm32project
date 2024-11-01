#ifndef	__PWM_H
#define	__PWM_H

#include "stm32f10x.h"                  // Device header

void PWM_Init(u16 TIM_Period,u16 Prescaler);
void TIM1_Configuration(void); 

#endif
