#include "iwdg.h"

void IWDG_Init(void)
{
	//检测系统是由独立看门狗引起的复位还是正常复位
	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET)	
	{
		//独立看门狗引起的复位
		printf("IWDG RST ...\r\n");
		//清楚标志位
		RCC_ClearFlag();
	}
	else
	{
		//正常按键复位
		printf("Normal RST ...\r\n");
	}
	//使能 LSI 时钟 给看门狗提供时钟基准（不开启也行，系统会系统开启该时钟）
	RCC_LSICmd(ENABLE);
	//等待LSI 稳定
	while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
	//允许访问 IWDG_PR and IWDG_RLR寄存器
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	//设置分频值： LSI/32  40K/64 = 625HZ
	IWDG_SetPrescaler(IWDG_Prescaler_64);
	IWDG_SetReload(6250);	//10s看门狗 超过10s不喂狗 看门狗就会使单片机复位
	//使能重装载寄存器计数
	IWDG_ReloadCounter();    
	
	//使能看门狗
	IWDG_Enable();
}



