#include "key.h"

u8 key1_long_flag,key2_long_flag,key3_long_flag,key4_long_flag;
u8 key1_lock_flag,key2_lock_flag,key3_lock_flag,key4_lock_flag;
u16 key1_cnt,key2_cnt,key3_cnt,key4_cnt;
u16 key1_cnt2,key2_cnt2,key3_cnt2,key4_cnt2;
u8 key1_lock_flag,key2_lock_flag,key3_lock_flag,key4_lock_flag;

u8 key1_short_flag = 0;


u8 KeyNum = 0;
/**
  * @brief  Key初始化
  * @param  无
  * @retval 无
  */
void Key_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = KEY1_GPIO_PIN | KEY2_GPIO_PIN | KEY3_GPIO_PIN | KEY4_GPIO_PIN ;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(KEY_PROT, &GPIO_InitStructure);
}

/**
  * @brief  Key键值获取
  * @param  无
  * @retval 返回触发按键的键值
  */
void Key_scan(void)
{
	if(KEY1) //如果没有按键按下
	{
		key1_lock_flag = 0; //清零自锁标志
		key1_cnt = 0; //清零计数标志
		if(key1_long_flag)
		{
			key1_long_flag = 0;	//清零长按标志位
			KeyNum = 11; //赋键值编码 长按键值编码
		}
		if(key1_short_flag)
		{
			key1_short_flag = 0;	//清零长按标志位
			KeyNum = 1; //赋键值编码 长按键值编码
		}		
	}
	else if(!key1_lock_flag)
	{
		key1_cnt++; //累计按键消抖延时次数
		if(key1_cnt > KEY_DELAY_TIME)
		{
			key1_cnt = 0; //清零计数标志
			key1_short_flag = 1; //赋键值编码 短按键值编码
			key1_lock_flag = 1; //自锁标志置1 防止多次触发
		}
	}
	else
	{
		key1_cnt++;	//累计按键长按功能延时时间

		if(key1_cnt > KEY1_LONG_TIME)
		{
			key1_short_flag = 0;
			key1_long_flag = 1;	//赋长按标志位
		}
	}
	
	if(KEY2) //如果没有按键按下
	{
		key2_lock_flag = 0; //清零自锁标志
		key2_cnt = 0; //清零计数标志
		if(key2_long_flag)
		{
			key2_long_flag = 0;	//清零长按标志位
			KeyNum = 22; //赋键值编码 长按键值编码
		}
	}
	else if(!key2_lock_flag)
	{
		key2_cnt++; //累计按键消抖延时次数
		if(key2_cnt > KEY_DELAY_TIME)
		{
			key2_cnt = 0; //清零计数标志
			KeyNum = 2; //赋键值编码 短按键值编码
			key2_lock_flag = 1; //自锁标志置1 防止多次触发
		}
	}
	else if(key2_cnt < KEY_Continue_TIME)
	{
		key2_cnt++;	//累计按键连击功能触发时间
	}
	else
	{
		key2_cnt++;	//累计按键长按功能延时时间
		key2_cnt2++;	//累计按键连击功能延迟时间
		if(key2_cnt2 > KEY_Continue_Trigger_TIME)
		{
			key2_cnt2 = 0;	//清零连击延迟时间
			KeyNum = 2;	//赋连击编码键值
		}
		if(key2_cnt > KEY_LONG_TIME)
		{
			key2_long_flag = 1;	//赋长按标志位
		}
	}
	
	if(KEY3) //如果没有按键按下
	{
		key3_lock_flag = 0; //清零自锁标志
		key3_cnt = 0; //清零计数标志
		if(key3_long_flag)
		{
			key3_long_flag = 0;	//清零长按标志位
			KeyNum = 33; //赋键值编码 长按键值编码
		}
	}
	else if(!key3_lock_flag)
	{
		key3_cnt++; //累计按键消抖延时次数
		if(key3_cnt > KEY_DELAY_TIME)
		{
			key3_cnt = 0; //清零计数标志
			KeyNum = 3; //赋键值编码 短按键值编码
			key3_lock_flag = 1; //自锁标志置1 防止多次触发
		}
	}
	else if(key3_cnt < KEY_Continue_TIME)
	{
		key3_cnt++;	//累计按键连击功能触发时间
	}
	else
	{
		key3_cnt++;	//累计按键长按功能延时时间
		key3_cnt2++;	//累计按键连击功能延迟时间
		if(key3_cnt2 > KEY_Continue_Trigger_TIME)
		{
			key3_cnt2 = 0;	//清零连击延迟时间
			KeyNum = 3;	//赋连击编码键值
		}
		if(key3_cnt > KEY_LONG_TIME)
		{
			key3_long_flag = 1;	//赋长按标志位
		}
	
	}
	
	if(KEY4) //如果没有按键按下
	{
		key4_lock_flag = 0; //清零自锁标志
		key4_cnt = 0; //清零计数标志
		if(key4_long_flag)
		{
			key4_long_flag = 0;	//清零长按标志位
			KeyNum = 44; //赋键值编码 长按键值编码
		}
	}
	else if(!key4_lock_flag)
	{
		key4_cnt++; //累计按键消抖延时次数
		if(key4_cnt > KEY_DELAY_TIME)
		{
			key4_cnt = 0; //清零计数标志
			KeyNum = 4; //赋键值编码 短按键值编码
			key4_lock_flag = 1; //自锁标志置1 防止多次触发
		}
	}
	else if(key4_cnt < KEY_Continue_TIME)
	{
		key4_cnt++;	//累计按键连击功能触发时间
	}
	else
	{
		key4_cnt++;	//累计按键长按功能延时时间
		key4_cnt2++;	//累计按键连击功能延迟时间
		if(key4_cnt2 > KEY_Continue_Trigger_TIME)
		{
			key4_cnt2 = 0;	//清零连击延迟时间
			KeyNum = 4;	//赋连击编码键值
		}
		if(key4_cnt > KEY_LONG_TIME)
		{
			key4_long_flag = 1;	//赋长按标志位
		}
	
	}
}



















