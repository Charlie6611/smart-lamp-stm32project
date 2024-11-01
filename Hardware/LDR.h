#ifndef _LDR_H_
#define _LDR_H_

#include "stm32f10x.h"                  // Device header
#include "adcx.h"
#include "Delay.h"
#include <math.h>

#define LDR_READ_TIMES	10
#define LDR				GPIO_Pin_1
#define LDR_PROT		GPIOB

void LDR_Init(void);
uint16_t LDR_Average_Data(void);
void LDR_LuxData(uint16_t *Lux);

#endif
