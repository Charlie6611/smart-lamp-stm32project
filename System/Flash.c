#include "flash.h"


////FLASHд������
void FLASH_W(u32 add,u16 dat,u16 dat2)	//����1��32λFLASH��ַ������2��16λ����
{ 
//	 RCC_HSICmd(ENABLE); //��HSIʱ��
	 FLASH_Unlock();  //����FLASH��̲���������
     FLASH_ClearFlag(FLASH_FLAG_BSY|FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);//�����־λ
     FLASH_ErasePage(add);     //����ָ����ַҳ
     FLASH_ProgramHalfWord(add,dat); //��ָ��ҳ��addr��ַ��ʼд
	 FLASH_ProgramHalfWord(add+2,dat2); //��ָ��ҳ��addr��ַ+2��ʼд
     FLASH_ClearFlag(FLASH_FLAG_BSY|FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);//�����־λ
     FLASH_Lock();    //����FLASH��̲���������
}

//FLASH��������
u16 FLASH_R(u32 add)	 //����1��32λ����FLASH��ַ������ֵ��16λ����
{
	u16 a;
    a = *(u16*)(add);//��ָ��ҳ��addr��ַ��ʼ��
return a;
}
