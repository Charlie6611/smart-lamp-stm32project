#include "buzzer.h"

u8 Buzzer_Flag;

void Buzzer_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = Buzzer;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(Buzzer_PROT, &GPIO_InitStructure);
	GPIO_ResetBits(Buzzer_PROT, Buzzer);
}

void Buzzer_ON(void)
{
	Buzzer_Flag = 1;
	GPIO_SetBits(Buzzer_PROT, Buzzer);

}

void Buzzer_OFF(void)
{
	Buzzer_Flag = 0;
	GPIO_ResetBits(Buzzer_PROT, Buzzer);	

}


