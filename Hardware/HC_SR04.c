#include "hc_sr04.h"

u32 msHCcount = 0;

uint16_t ultrasonicData[5];

HC_SR04_Delay HC_SR04_DelayStructure;

void HC_SR04_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin   = TRIG;	//触发测距引脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(HC_PROT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin   = ECHO;	//信号回响引脚
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;//下拉
	GPIO_Init(HC_PROT,&GPIO_InitStructure);
	
	TIM_DeInit(TIM4);
	TIM_InitStructure.TIM_Period = 1000-1;//1MS
	TIM_InitStructure.TIM_Prescaler = 72-1;
	TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStructure.TIM_RepetitionCounter = DISABLE;
	TIM_TimeBaseInit(TIM4,&TIM_InitStructure);
	
	TIM_ClearFlag(TIM4,TIM_FLAG_Update);
	TIM_ITConfig(TIM4,TIM_IT_Update|TIM_IT_Trigger,ENABLE);
	TIM_Cmd(TIM4,DISABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}
void TIM4_IRQHandler(void){
	
	if(TIM_GetITStatus(TIM4,TIM_IT_Update) != RESET){ //1MS
			
		TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
		msHCcount++;
	}
}

void HC_SR04_Ranging(u16 *data)
{
	
	u32 Count = 0;
	static u32 i = 0;
	while(GPIO_ReadInputDataBit(HC_PROT,ECHO) == 1);
	TRIG_HIGH;
	HC_SR04_DelayStructure.delay20msStartFlag = 1;
	if(HC_SR04_DelayStructure.delay20msFinishFlag)
	{
		i++;
		HC_SR04_DelayStructure.delay20msFinishFlag = 0;
		TRIG_LOW;							

		while(GPIO_ReadInputDataBit(HC_PROT,ECHO) == 0);
		TIM_SetCounter(TIM4,0);
		msHCcount = 0;
		TIM_Cmd(TIM4,ENABLE);	
		while(GPIO_ReadInputDataBit(HC_PROT,ECHO) == 1);
		TIM_Cmd(TIM4,DISABLE);
		
		Count = msHCcount*1000;//us
		Count = Count + TIM_GetCounter(TIM4);
		
		*data = Count * 0.017;//340/1000000/2/100
	}
}

void HC_SR04_Delay100msRanging(u16 *data)
{
	HC_SR04_DelayStructure.delay100msStartFlag = 1;
	if (HC_SR04_DelayStructure.delay100msFinishFlag)
	{
		HC_SR04_DelayStructure.delay100msFinishFlag = 0;
		HC_SR04_Ranging(data);
	}
}

void HC_SR04_Deboanle(uint8_t *distanceData)
{
	uint16_t dataA = 0;
	u32 dataB = 0;
	HC_SR04_Delay100msRanging(&dataA);
	for(u8 i=0;i<5;i++)
	{		
		dataB = dataB + dataA;//data += dataA
	}
	*distanceData = dataB / 5;
}


