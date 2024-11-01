#ifndef	_HC_SR04_H_
#define	_HC_SR04_H_

#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "oled.h"

#define TRIG GPIO_Pin_14
#define ECHO GPIO_Pin_15
#define HC_PROT GPIOB

#define TRIG_HIGH GPIO_SetBits(HC_PROT,TRIG)
#define TRIG_LOW GPIO_ResetBits(HC_PROT,TRIG)

typedef struct
{
	uint8_t delay20ms;
	uint8_t delay20msStartFlag;
	uint8_t delay20msFinishFlag;	
	uint8_t delay100ms;
	uint8_t delay100msStartFlag;
	uint8_t delay100msFinishFlag;
	
}HC_SR04_Delay;

extern HC_SR04_Delay HC_SR04_DelayStructure;

void HC_SR04_Init(void);
void HC_SR04_Deboanle(uint8_t *distanceData);
void HC_SR04_dataScan(void);

#endif
