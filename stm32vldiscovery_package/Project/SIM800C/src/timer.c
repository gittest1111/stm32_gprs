#include "timer.h"
#include "usart.h"
#include "usart3.h"
#include "SIM800.h"
#include "string.h"  
#include "stdlib.h"  

//////////////////////////////////////////////

u8 Flag_Time_Out_Reconnec_Normal = 0;           //��������ʱ�������ӷ������ĳ�ʱ��Чʱ��λ�ñ���
u8 Flag_Time_Out_AT = 0;                        //ģ�����ӷ����������г����쳣��Ķ�ʱʱ�䵽����λ�ñ���
u8 Flag_Auth_Infor = 0;  												//�յ������֤����ʱ��λ�ñ���

u8 Flag_Reported_Data = 0;   										//�յ��ϱ���������ʱ��λ�ñ������ֽ׶����ϱ�TDSֵ���¶�ֵ��

u8 Flag_Comm_OK = 0;                            //�豸�Ѿ���������������ӣ����յ���login����
u8 Flag_Need_Reset = 0;

extern u8 Total_Wait_Echo;
extern u8 current_cmd;
//////////////////////////////////
u32 Count_Wait_Echo = 0;         //�ȴ����������ĵļ���ֵ��һ����λ����50ms
u8  Flag_Wait_Echo = 0;          //��Ҫ�ȴ�����������ʱ����λ�������

u32 Count_Local_Time_Dev_01 = 0;        //�����豸1�����ؼ�ʱ��ֵ��һ����λ����50ms
u32 Count_Local_Time_Dev_02 = 0;        //�����豸2�����ؼ�ʱ��ֵ��һ����λ����50ms
u32 Count_Local_Time_Dev_03 = 0;        //�����豸3�����ؼ�ʱ��ֵ��һ����λ����50ms
u32 Count_Local_Time_Dev_04 = 0;        //�����豸4�����ؼ�ʱ��ֵ��һ����λ����50ms
u8  Flag_Local_Time_Dev_01 = 0;         //�豸1��Ҫ���ؼ�ʱ����λ�������
u8  Flag_Local_Time_Dev_02 = 0;         //�豸2��Ҫ���ؼ�ʱ����λ�������
u8  Flag_Local_Time_Dev_03 = 0;         //�豸3��Ҫ���ؼ�ʱ����λ�������
u8  Flag_Local_Time_Dev_04 = 0;         //�豸4��Ҫ���ؼ�ʱ����λ�������

u8  Flag_Local_Time_Dev_01_OK = 0;         //�豸1���ؼ�ʱ��ɣ���λ�������
u8  Flag_Local_Time_Dev_02_OK = 0;         //�豸2���ؼ�ʱ��ɣ���λ�������
u8  Flag_Local_Time_Dev_03_OK = 0;         //�豸3���ؼ�ʱ��ɣ���λ�������
u8  Flag_Local_Time_Dev_04_OK = 0;         //�豸4���ؼ�ʱ��ɣ���λ�������


u32 Count_Send_Heart = 0;        //�ٴη����������ļ���ֵ��һ����λ����50ms
u8  Flag_Send_Heart = 0;         //��Ҫ�ȴ��ٴη���������ʱ����λ�������
u8  Flag_Send_Heart_OK = 0;      //�ٴη����������ļ�ʱ������λ�������

u8  Flag_Receive_Heart = 0;      //�յ�������������������ʱ����λ�������

u8 	Flag_Time_Out_Comm = 0;                      //ͨ�ų�ʱ������Чʱ��λ�ñ���
u8 	Flag_Receive_Login = 0;  										 //�յ���½��Ϣ����ʱ��λ�ñ���
u8 	Flag_ACK_Echo = 0xFF;                        //�豸�˳������øñ������ж��ȴ����ĳ�ʱ��������ָ��

u8 	Flag_Check_error = 0;                        //�Խ��յ�����Ϣ����У�飬���У�鲻ͨ������λ�ñ���
u8 	Flag_Receive_Resend = 0; 										 //���յ��ط�����ʱ��λ�ñ���
u8 	Flag_Receive_Enable = 0; 										 //���յ������豸����ʱ��λ�ñ���
u8 	Flag_Receive_Device_OK = 0; 								 //���յ��豸���н�������ʱ��λ�ñ���
u8 	Flag_ACK_Resend = 0xFF;                      //�豸�˳������øñ������ж�Ҫ�ط�������������

u8 Flag_Received_SIM800C_CLOSED = 0;
u8 Flag_Received_SIM800C_DEACT = 0;
/*
	50ms�Ķ�ʱɨ�裬Ŀ����������
	һ�Ǽ�ʱ��ȡ���������·����������·�����������豸�Ļ�����Ϣ�������·���ҵ��ָ�
	���ǻ�ȡSIM800C�ĸ��������Ϣ�����ֳ����������ϢҲ�����������У�ϵͳ���ȶ��Ե���Ҫ��������¹���
	50�����ɨ������Ӧ�����㹻���ٵģ������ĵĻ������Գ����޸�Ϊ20����������10����......
*/

//��ʱ��6�жϷ������		    
void TIM6_DAC_IRQHandler(void)
{
	u16 length = 0; 
	u8 result = 0;
	u8 result_temp = 0;
	u8 index = 0;
	char *p, *p1;
	char *p_temp = NULL;
	u8 temp_array[3] = {0};
	u8 offset = 0;
	u8 temp = 0xAA;

	if(TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)					  //�Ǹ����ж�
	{	
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);  					//���TIM6�����жϱ�־

		//�ٴη����������Ķ�ʱ����
		if(Flag_Send_Heart == 0xAA)
		{
			Count_Send_Heart += 1;
			if(Count_Send_Heart >= NUMBER_TIME_HEART_50_MS)
			{
				Flag_Send_Heart = 0;
				Count_Send_Heart = 0;
				Flag_Send_Heart_OK = 0xAA;
			}
		}
		
		if(Flag_Local_Time_Dev_01 == 0xAA)
		{
			Count_Local_Time_Dev_01 += 1;
			if(Count_Local_Time_Dev_01 >= 1000)  //�����1000Ҫ����ʵ����������޸�
			{
				Flag_Local_Time_Dev_01_OK = 0xAA;
				//�ر��豸
			
			}
		}
		if(Flag_Local_Time_Dev_02 == 0xAA)
		{
			Count_Local_Time_Dev_02 += 1;
		}
		if(Flag_Local_Time_Dev_03 == 0xAA)
		{
			Count_Local_Time_Dev_03 += 1;
		}
		if(Flag_Local_Time_Dev_04 == 0xAA)
		{
			Count_Local_Time_Dev_04 += 1;
		}
		
		if(Flag_Wait_Echo == 0xAA)
		{
			Count_Wait_Echo += 1;
			//TIME_ECHO�ĳ�ʱʱ�䵽,�ȴ����������ĵ��ʱ�䵽
			if(Count_Wait_Echo >= NUMBER_TIME_ECHO_50_MS)
			{
				BSP_Printf("��ʱ������Ч\r\n");
				BSP_Printf("Count_Wait_Echo:%ld\r\n",Count_Wait_Echo);
				
				Flag_Wait_Echo = 0;
				Count_Wait_Echo = 0;
				Flag_Time_Out_Comm	= 0xAA;
			}
		}
			


		temp = Receive_Data_From_USART();
		//���յ���SIM800C����Ϣ
		if(0x01 == temp)	
		{
			/*��ʱ��������*/
			//���㴮��3�Ľ��ձ�־����
			
			BSP_Printf("USART3_RX_BUF_SIM800C:%s\r\n",USART3_RX_BUF);

			if(strstr((const char*)USART3_RX_BUF,"CLOSED")!=NULL)
				Flag_Received_SIM800C_CLOSED = 0xAA;
			if(strstr((const char*)USART3_RX_BUF,"+PDP: DEACT")!=NULL)
				Flag_Received_SIM800C_DEACT = 0xAA;
			//Clear_Usart3();

		}
		//���յ��˷���������Ϣ
		if(0x00 == temp)	
		{
			BSP_Printf("USART3_RX_BUF_������:%s\r\n",USART3_RX_BUF);
			BSP_Printf("Count_Wait_Echo_0X00:%ld\r\n",Count_Wait_Echo);
			
			//�յ����ģ���������ر�ʶ��
			Flag_Wait_Echo = 0;
			Count_Wait_Echo = 0;
			Total_Wait_Echo = 0;
			
			BSP_Printf("need_ack_check: %d, ack: %s\r\n",need_ack_check, atcmd_ack);
			if(need_ack_check && strstr((const char*)USART3_RX_BUF, atcmd_ack))
				ack_ok = TRUE;
				
			p=strstr((const char*)USART3_RX_BUF,"TRVBP");
			if((p1=strstr((const char*)p,"#"))!=NULL)
			{
				//�������ͺ�����У�����	
				//length = strlen((const char *)(USART3_RX_BUF));
				length = p1 - p +1;
				//У������
				result = Check_Xor_Sum((char *)(p),length-5);
				BSP_Printf("result:%d\r\n",result);
				
				//ȡ�ַ����е�У��ֵ
				p_temp = (p+length-5);
				offset = 3;
				for(index = 0;index < offset;index++)
				{
					temp_array[index] = p_temp[index];
				
				}
				//У��ֵת��Ϊ���֣�����ӡ
				result_temp = atoi((const char *)(temp_array));	
				BSP_Printf("result_temp:%d\r\n",result_temp);			
				
				//������ȷ
				if(result == result_temp)
				{
					//�յ��豸��½��Ϣ�Ļ���
					if(strstr((const char*)p,"TRVBP00"))
					{
						BSP_Printf("�յ��豸��½��Ϣ�Ļ���\r\n");
						Flag_Receive_Login = 0xAA;					
					}
					else
					{
						//�յ��ط�����
						if(strstr((const char*)p,"TRVBP98"))
						{
							BSP_Printf("�յ��������ط�����\r\n");
							Flag_Receive_Resend = 0xAA;
						}
						else
						{
							//�յ���������
							if(strstr((const char*)p,"TRVBP01"))
							{
								BSP_Printf("�յ���������������\r\n");
								Flag_Receive_Heart = 0xAA;
							}
							else
							{
								//�յ������豸ָ��
								if(strstr((const char*)p,"TRVBP03"))
								{
									BSP_Printf("�յ������������豸ָ��\r\n");
									Flag_Receive_Enable = 0xAA;
									//������Ҫ�ж�Ҫ�򿪵��豸�ǲ����Ѿ��������ˣ�
									//����ǣ��ǾͲ�����������÷�����ͨ����һ�������ж�
									//������ǣ���Ҫ������Ϣ��һ���µ�buf��������ѭ������
								}
								else
								{
									//�յ����н�������
									if(strstr((const char*)p,"TRVBP05"))
									{
										BSP_Printf("�յ����������н�������\r\n");
										Flag_Receive_Device_OK = 0xAA;
									}
									else
									{
										BSP_Printf("�����ڵķ�����ָ��\r\n");
										Flag_Check_error = 0xAA;
									}
								}
							}
						}
					}
				}   //������ȷ
				//�����쳣	
				if(result != result_temp)
				{
					//�ô����ط���־
					//���ﲻ���ж������������ճ�����Ϊ�Դ�������ݵĽ����ж������岻���ɷ������������ж��ط�������
					//ͬ��Ƕ��ʽ�����ڽ��յ����������ط�����ʱ��ҲҪ�ж���Ҫ�ط��������
					BSP_Printf("������ָ��У���\r\n");
					Flag_Check_error = 0xAA;
					//Clear_Usart3();
				}			
			}
			else
			{
				BSP_Printf("������ָ�����\r\n");
				Flag_Check_error = 0xAA;
			}
			//��������Ϣ����շ���������
			Clear_Usart3();	
			BSP_Printf("current_cmd: %d\r\n", current_cmd);
		}
		TIM_SetCounter(TIM6,0); 
	}
}



//��ʱ��7�жϷ������		    
void TIM7_IRQHandler(void)
{ 	
	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)//�Ǹ����ж�
	{	 		
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update  );  //���TIM7�����жϱ�־    
	  USART3_RX_STA|=1<<15;	//��ǽ������
		TIM_Cmd(TIM7, DISABLE);  //�ر�TIM7
	}	    
}


	 



//ͨ�ö�ʱ��6�жϳ�ʼ��
//����ѡ��ΪAPB1��1������APB1Ϊ24M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz 
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��	
void TIM6_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);				//TIM6ʱ��ʹ��    
	
	//��ʱ��TIM6��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr;                     //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc;                   //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);             //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE );                   //ʹ��ָ����TIM6�ж�,��������ж�
	
	//TIM_Cmd(TIM6,ENABLE);//������ʱ��6
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3 ;//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
}

//TIMER7�ĳ�ʼ�� ����USART3���Խ�SIM800�����жϽ��ճ���/////////
//ͨ�ö�ʱ��7�жϳ�ʼ��
//����ѡ��ΪAPB1��1������APB1Ϊ24M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��
//��ʱ�����ʱ����㷽��:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=��ʱ������Ƶ��,��λ:Mhz 
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��		 
void TIM7_Int_Init(u16 arr,u16 psc)
{	
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);//TIM7ʱ��ʹ��    
	
	//��ʱ��TIM7��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr;                     //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc;                   //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);             //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE );                   //ʹ��ָ����TIM7�ж�,��������ж�
	
	TIM_Cmd(TIM7,ENABLE);//������ʱ��7
	
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2 ;	//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		    	//�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			      	//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
}




