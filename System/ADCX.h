#ifndef _ADCX_H_
#define _ADCX_H_

#include "stm32f10x.h"                  // Device header

void ADCX_Init(void);
u16 ADC_GetValue(uint8_t ADC_Channel,uint8_t ADC_SampleTime);

#endif
