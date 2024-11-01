#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"                  // Device header

#define Buzzer			GPIO_Pin_1
#define Buzzer_PROT		GPIOA
extern u8 Buzzer_Flag;
void Buzzer_Init(void);
void Buzzer_ON(void);
void Buzzer_OFF(void);

#endif
