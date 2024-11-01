#ifndef	__SENSORMODULES_H_
#define __SENSORMODULES_H_

#include "stm32f10x.h"                  // Device header


typedef struct
{
	uint8_t humi;
	uint8_t temp;
	uint8_t distance;
	uint8_t people;
	uint16_t lux;	

	
}SensorModules;

typedef struct
{
	uint16_t Distance_threshold;
	uint16_t Illumination_threshold;	
	
}SensorThresholdValue;

extern SensorModules sensorData;			//声明传感器模块的结构体变量
extern SensorThresholdValue Sensorthreshold;	//声明传感器阈值结构体变量

#endif
