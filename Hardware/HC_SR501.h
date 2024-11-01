#ifndef	_HC_SR051_H_
#define	_HC_SR051_H_

#include "stm32f10x.h"                  // Device header
#include "delay.h"

#define HC_SR501			GPIO_Pin_9
#define HC_SR501_PROT		GPIOB

#define HC_SR051_SMOOTHING	10

void HC_SR501_Init(void);
void HC_SR501_Input(uint8_t *dat);

#endif
