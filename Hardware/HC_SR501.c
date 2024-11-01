#include "hc_sr501.h"

uint8_t HC_SR501_Dataset[HC_SR051_SMOOTHING];


void HC_SR501_Init(void)
{
	//开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	//配置引脚为下拉输入模式
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = HC_SR501;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(HC_SR501_PROT, &GPIO_InitStructure);
	GPIO_ResetBits(HC_SR501_PROT, HC_SR501);
}

void HC_SR501_Input(uint8_t *dat)
{
	*dat = GPIO_ReadInputDataBit(HC_SR501_PROT, HC_SR501);


}
