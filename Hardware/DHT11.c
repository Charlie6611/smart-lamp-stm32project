#include "dht11.h"

u8 Time = 0;

void DHT11_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
}

void DHT11_Mode(u8 mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	if(mode)
	{
		GPIO_InitStructure.GPIO_Pin = DHT11;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	}
	else
	{
		GPIO_InitStructure.GPIO_Pin =  DHT11;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	}
	GPIO_Init(DHT11_PROT, &GPIO_InitStructure);
}

void DHT11_Sends_Start(void)
{
	DHT11_Mode(OUT);
	
	DHT11_Low;
	Delay_ms(18);	//18ms
	DHT11_High;
	Delay_us(40);	//40us
}

void DHT11_Sends_Response(void)
{
	DHT11_Mode(INT);
	
	while(GPIO_ReadInputDataBit(DHT11_PROT,DHT11) == 0 && (Time < 100))	//Low 80us
	{
		Time++;
		Delay_us(1);	//100us DHT11=0 80us
	}
	Time = 0;
	
	while(GPIO_ReadInputDataBit(DHT11_PROT,DHT11) == 1 && (Time < 100))	//High 80us
	{
		Time++;
		Delay_us(1);	//100us DHT11=0 80us
	}	
	Time = 0;
}

u8 DHT11_Read_Byte(void)
{
	u8 data = 0;
	
	for(u8 i = 0; i < 8; i++)
	{
		DHT11_Mode(INT);
		
		while(GPIO_ReadInputDataBit(DHT11_PROT,DHT11) == 0 && (Time < 100))	
		{
			Time++;
			Delay_us(1);	//100us DHT11=0 80us
		}	
		Time = 0;
		
		data <<= 1;
	
		Delay_us(40);	//get hight 70us
		
		if(GPIO_ReadInputDataBit(DHT11_PROT, DHT11) == 1)
		{
			data |= 0x01;
		
		}
		
		while(GPIO_ReadInputDataBit(DHT11_PROT,DHT11) == 1 && (Time < 100))	
		{
			Time++;
			Delay_us(1);	//100us DHT11=0 80us
		}	
		Time = 0;
	}
	return data;
}

void DHT11_Read_Data(u8 *humi, u8 *temp)
{
	u8 DATA[5] = {0,0,0,0,0,};
	
	DHT11_Sends_Start();
	DHT11_Sends_Response();
	
	for(u8 i = 0; i < 5; i++)
	{
		DATA[i] = DHT11_Read_Byte();
	}
	Delay_ms(1);	//1ms 50us
	
	if(DATA[0] + DATA[1] + DATA[2] + DATA[3] == DATA[4])
	{
		*humi = DATA[0];
		*temp = DATA[2];
	}
	else
	{
		for(u8 i = 0; i < 5; i++)
		{
			DATA[i] = 0;
		}
	
	}
}
