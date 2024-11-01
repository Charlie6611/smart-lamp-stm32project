#include "ldr.h"

/**
  * @brief  LDR初始化程序
  * @param  
  * @retval 
  */
void LDR_Init(void)
{
	//开始GPIOB的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	//将LDR引脚设为模拟输入
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = LDR;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LDR_PROT, &GPIO_InitStructure);
}

uint16_t LDR_ADC_Read(void)
{
	//设置指定ADC的规则组通道，采样时间
	return ADC_GetValue(ADC_Channel_9, ADC_SampleTime_55Cycles5);
}

uint16_t LDR_Average_Data(void)
{
	uint32_t  tempData = 0;
	for (uint8_t i = 0; i < LDR_READ_TIMES; i++)
	{
		tempData += LDR_ADC_Read();
		Delay_ms(5);
	}

	tempData /= LDR_READ_TIMES;
	return (uint16_t)tempData;
}

void LDR_LuxData(uint16_t *Lux)
{
	float voltage = 0;	
	float R = 0;	
	voltage = LDR_Average_Data();
	voltage  = voltage / 4096 * 3.3f;
	
	R = voltage / (3.3f - voltage) * 10000;
		
	*Lux = 40000 * pow(R, -0.6021);
	
	if (*Lux > 999)
	{
		*Lux = 999;
	}
}
