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



#define OFFSET_Connec 30                      	//TRVBP00,000,UBICOM_AUTH_INFOR,0001,070,X,# 
#define OFFSET_TDS    35                      	//TRVBP00,000,UBICOM_AUTH_INFOR,0001,070,X,# 


extern u8 Flag_Time_Out_Comm;                       //ͨ�ŵĳ�ʱ������Чʱ��λ�ñ���
extern u8 Flag_Time_Out_Reconnec_Normal;            //��������ʱ�������ӷ������ĳ�ʱ��Чʱ��λ�ñ���
extern u8 Flag_Time_Out_AT;                         //ģ�����ӷ����������г����쳣��Ķ�ʱʱ�䵽����λ�ñ���
extern u8 Flag_Auth_Infor;  												//�յ������֤����ʱ��λ�ñ���
extern u8 Flag_Login;  												      //�յ���½��Ϣ����ʱ��λ�ñ���
extern u8 Flag_Reported_Data;   										//�յ��ϱ���������ʱ��λ�ñ������ֽ׶����ϱ�TDSֵ���¶�ֵ��
extern u8 Flag_Resend; 														  //���յ��ط�����ʱ��λ�ñ���
extern u8 Flag_Check_error;                         //�Խ��յ�����Ϣ����У�飬���У�鲻ͨ������λ�ñ���
extern u8 Flag_ACK_Resend;              					  //�豸�˳������øñ������ж�Ҫ�ط�������������
extern u8 Flag_Comm_OK;                             //����ͨ�Ź���˳������ʱ��λ�ñ���
extern u8 Flag_Need_Reset;

////////////////////////////////
extern u32 Count_Wait_Echo;         //�ȴ����������ĵļ���ֵ��һ����λ����50ms
extern u8  Flag_Wait_Echo;          //��Ҫ�ȴ�����������ʱ����λ�������

extern u32 Count_Local_Time_Dev_01;        //�����豸1�����ؼ�ʱ��ֵ��һ����λ����50ms
extern u32 Count_Local_Time_Dev_02;        //�����豸2�����ؼ�ʱ��ֵ��һ����λ����50ms
extern u32 Count_Local_Time_Dev_03;        //�����豸3�����ؼ�ʱ��ֵ��һ����λ����50ms
extern u32 Count_Local_Time_Dev_04;        //�����豸4�����ؼ�ʱ��ֵ��һ����λ����50ms
extern u8  Flag_Local_Time_Dev_01;         //�豸1��Ҫ���ؼ�ʱ����λ�������
extern u8  Flag_Local_Time_Dev_02;         //�豸2��Ҫ���ؼ�ʱ����λ�������
extern u8  Flag_Local_Time_Dev_03;         //�豸3��Ҫ���ؼ�ʱ����λ�������
extern u8  Flag_Local_Time_Dev_04;         //�豸4��Ҫ���ؼ�ʱ����λ�������
extern u8  Flag_Local_Time_Dev_01_OK;      //�豸1���ؼ�ʱ��ɣ���λ�������
extern u8  Flag_Local_Time_Dev_02_OK;      //�豸2���ؼ�ʱ��ɣ���λ�������
extern u8  Flag_Local_Time_Dev_03_OK;      //�豸3���ؼ�ʱ��ɣ���λ�������
extern u8  Flag_Local_Time_Dev_04_OK;      //�豸4���ؼ�ʱ��ɣ���λ�������

extern u8  Flag_Send_Heart_OK;      //�ٴη����������ļ�ʱ������λ�������

extern u8  Flag_Receive_Heart;      //�յ�������������������ʱ����λ�������
extern u8  Flag_Receive_Login;  										 //�յ���½��Ϣ����ʱ��λ�ñ���

extern u32 Count_Send_Heart;        //�ٴη����������ļ���ֵ��һ����λ����50ms
extern u8  Flag_Send_Heart;         //��Ҫ�ȴ��ٴη���������ʱ����λ�������

extern u8  Flag_ACK_Echo;           //�豸�˳������øñ������ж��ȴ����ĳ�ʱ��������ָ��

extern u8  Flag_Receive_Resend; 		//���յ��ط�����ʱ��λ�ñ���

extern u8  Flag_Receive_Enable; 		//���յ������豸����ʱ��λ�ñ���

extern u8  Flag_Receive_Device_OK;  //���յ��豸���н�������ʱ��λ�ñ���

extern u8 Flag_Received_SIM800C_CLOSED;
extern u8 Flag_Received_SIM800C_DEACT;

void TIM6_Int_Init(u16 arr,u16 psc);
void TIM7_Int_Init(u16 arr,u16 psc);




#endif
