#include "led.h"

uint8_t ledDutyRatio = 0;				//存储当前的LED亮度数值

void LED_Init(void)
{
	//开启GPIOB时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	//配置LED引脚为模拟输出模式
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = LED;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_PROT, &GPIO_InitStructure);
	GPIO_ResetBits(LED_PROT, LED);
}

void LED_PWM_KEY(void)
{
	if (KeyNum == 2)
	{
		KeyNum = 0;
		ledDutyRatio ++;
		if (ledDutyRatio >= LED_PWM_UPPER_LIMIT)
		{
			ledDutyRatio = 0;
		}
		currentDataPoint.valueLamp_brightness = ledDutyRatio;
	}
	if (KeyNum == 3)
	{
		KeyNum = 0;
		ledDutyRatio --;	
		if (ledDutyRatio >= LED_PWM_UPPER_LIMIT)
		{
			ledDutyRatio = LED_PWM_UPPER_LIMIT;
		}
		currentDataPoint.valueLamp_brightness = ledDutyRatio;
	}
	if (KeyNum == 4)
	{
		KeyNum = 0;
		ledDutyRatio = 0;
		currentDataPoint.valueLamp_brightness = ledDutyRatio;
	}
	TIM_SetCompare1(TIM1, ledDutyRatio);

}


void LED_APP_Command(void)
{
	if (!systemModel)
	{
		if (currentDataPoint.valueLamp_brightness < ledDutyRatio)
		{
			ledDutyRatio--;
		}
		else if (currentDataPoint.valueLamp_brightness > ledDutyRatio)
		{
			ledDutyRatio++;
		}
		TIM_SetCompare1(TIM1, ledDutyRatio);
	}
	else if (systemModel && sensorData.people)
	{
		uint16_t tempData;
		if (currentDataPoint.valueIllumination_threshold > sensorData.lux)
		{
			tempData = currentDataPoint.valueIllumination_threshold - sensorData.lux;
			if (tempData > ledDutyRatio)
			{
				ledDutyRatio++;	
				if (ledDutyRatio >= LED_PWM_UPPER_LIMIT)
				{
					ledDutyRatio = 100;
				}	
			}
			else if (tempData < ledDutyRatio)
			{
				ledDutyRatio--;	
				if (ledDutyRatio >= LED_PWM_UPPER_LIMIT)
				{
					ledDutyRatio = 0;
				}				
			}
		}
		else if (currentDataPoint.valueIllumination_threshold < sensorData.lux) 	
		{
			ledDutyRatio--;	
			if (ledDutyRatio >= LED_PWM_UPPER_LIMIT)
			{
				ledDutyRatio = 0;
			}		
		}
		currentDataPoint.valueLamp_brightness = ledDutyRatio;
		TIM_SetCompare1(TIM1, ledDutyRatio);
	}
	else if (systemModel && sensorData.people == 0)
	{
		ledDutyRatio = 0;
		currentDataPoint.valueLamp_brightness = ledDutyRatio;
		TIM_SetCompare1(TIM1, ledDutyRatio);
	}
		
}


