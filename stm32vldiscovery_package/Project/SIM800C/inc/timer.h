#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"
	

//���������豸�ȴ��������Ļ��ĵ��ʱ�䣨��λ�Ƿ��ӣ�
#define TIME_ECHO 1
#define NUMBER_TIME_ECHO_50_MS (TIME_ECHO*60*1000/50)  	    //1MIN

//���������������ķ��ͼ������λ�Ƿ��ӣ�
//����������������1���ӵ�10����
#define TIME_HEART 1
#define NUMBER_TIME_HEART_50_MS (TIME_HEART*60*1000/50)  	  //1MIN
#define HB_1_MIN (TIME_HEART*60)  	  //1MIN

#define NUMBER_TIMER_1_MINUTE       1200  //60*1000/50 

#define OFFSET_Connec 30                      	//TRVBP00,000,UBICOM_AUTH_INFOR,0001,070,X,# 
#define OFFSET_TDS    35                      	//TRVBP00,000,UBICOM_AUTH_INFOR,0001,070,X,# 


void TIM6_Int_Init(u16 arr,u16 psc);
void TIM7_Int_Init(u16 arr,u16 psc);

#endif
