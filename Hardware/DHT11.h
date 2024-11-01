#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"                  // Device header
#include "delay.h"

#define DHT11       GPIO_Pin_12
#define DHT11_PROT  GPIOB

#define OUT 1
#define INT 0

#define DHT11_Low GPIO_ResetBits(DHT11_PROT,DHT11)
#define DHT11_High GPIO_SetBits(DHT11_PROT,DHT11)

void DHT11_Init(void);
void DHT11_Read_Data(u8 *humi, u8 *temp);

#endif
