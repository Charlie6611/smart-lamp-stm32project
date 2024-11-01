#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"
/*“˝Ω≈≈‰÷√*/

#define OLED_SCL			GPIO_Pin_11
#define OLED_SDA			GPIO_Pin_10
#define OLED_PROT  			GPIOB

#define OLED_W_SCL(x)		GPIO_WriteBit(OLED_PROT, OLED_SCL, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(OLED_PROT, OLED_SDA, (BitAction)(x))


void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t num);
#endif
