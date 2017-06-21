#include "timer.h"
#include "usart.h"
#include "usart3.h"
#include "SIM800.h"
#include "string.h"  
#include "stdlib.h"  
#include "device.h"

extern void Reset_Device_Status(u8 status);

//��ʱ��6�жϷ������		    
void TIM6_DAC_IRQHandler(void)
{
	u8 index;
	if(TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)					  //�Ǹ����ж�
	{	
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);  					//���TIM6�����жϱ�־

		//BSP_Printf("TIM6_S Dev Status: %d, Msg expect: %d, Msg recv: %d\r\n", dev.status, dev.msg_expect, dev.msg_recv);
		//BSP_Printf("TIM6_S HB: %d, HB TIMER: %d, Msg TIMEOUT: %d\r\n", dev.hb_count, dev.hb_timer, dev.msg_timeout);

	
		//�ٴη����������Ķ�ʱ����
		if(dev.hb_timer >= HB_1_MIN)
		{
			if(dev.status == CMD_IDLE)
			{
				BSP_Printf("TIM6: HB Ready\r\n");
				Reset_Device_Status(CMD_HB);
			}
		}
		else
			dev.hb_timer++;

		for(index=DEVICE_01; index<DEVICEn; index++)
		{
			switch(g_device_status[index].power)
			{
				case ON:
					if(g_device_status[index].total==0)
						g_device_status[index].power = UNKNOWN;
					else
					{
						if(g_device_status[index].passed >= g_device_status[index].total)
						{
							g_device_status[index].passed=g_device_status[index].total=0;
							g_device_status[index].power=OFF;
							Device_OFF(index);
							if(dev.status == CMD_IDLE)
							{
								BSP_Printf("TIM6: �����豸״̬ΪCLOSE_DEVICE\r\n");
								Reset_Device_Status(CMD_CLOSE_DEVICE);
							}
						}
						else
							g_device_status[index].passed++;
					}
					break;
					
				case OFF:
					if(g_device_status[index].total!=0)
						g_device_status[index].power = UNKNOWN;
					break;
					
				case UNKNOWN:
				default:
				break;
			}
		}
	
		switch(dev.status)
		{
			case CMD_LOGIN:
				if(dev.msg_expect & MSG_DEV_LOGIN)
				{
					if(dev.reply_timeout >= HB_1_MIN)
					{
						dev.msg_expect &= ~MSG_DEV_LOGIN;
						dev.reply_timeout = 0;
						dev.msg_timeout++;
					}
					dev.reply_timeout++;
				}
			break;
			case CMD_HB:
				if(dev.msg_expect & MSG_DEV_HB)
				{
					if(dev.reply_timeout >= HB_1_MIN)
					{
						dev.msg_expect &= ~MSG_DEV_HB;
						dev.reply_timeout = 0;
						dev.msg_timeout++;
					}
					dev.reply_timeout++;
				}				
			break;
			case CMD_CLOSE_DEVICE:
				if(dev.msg_expect & MSG_DEV_CLOSE)
				{
					if(dev.reply_timeout >= HB_1_MIN)
					{
						dev.msg_expect &= ~MSG_DEV_CLOSE;
						dev.reply_timeout = 0;
						dev.msg_timeout++;
					}
					dev.reply_timeout++;
				}				
			break;
			default:
			break;
		}

		if(dev.msg_timeout >= NUMBER_MSG_MAX_RETRY)
			dev.need_reset = TRUE;
		//BSP_Printf("TIM6_E Dev Status: %d, Msg expect: %d, Msg recv: %d\r\n", dev.status, dev.msg_expect, dev.msg_recv);
		//BSP_Printf("TIM6_E HB: %d, HB TIMER: %d, Msg TIMEOUT: %d\r\n", dev.hb_count, dev.hb_timer, dev.msg_timeout);
		
		TIM_SetCounter(TIM6,0); 
	}
}

//��ʱ��7�жϷ������		    
void TIM7_IRQHandler(void)
{ 	
	u16 length = 0; 
	u8 result = 0;
	u8 result_temp = 0;
	u8 index = 0;
	char *p, *p1;
	char *p_temp = NULL;
	u8 temp_array[3] = {0};
	u8 offset = 0;

	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)//�Ǹ����ж�
	{	 		
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);  //���TIM7�����жϱ�־    
		USART3_RX_STA|=1<<15;	//��ǽ������
		TIM_Cmd(TIM7, DISABLE);  //�ر�TIM7
		
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;	//��ӽ����� 

		memset(dev.usart_data, 0, sizeof(dev.usart_data));
		strcpy(dev.usart_data, (const char *)USART3_RX_BUF);

		BSP_Printf("USART BUF:%s\r\n",USART3_RX_BUF);

		BSP_Printf("TIM7_S Dev Status: %d, Msg expect: %d, Msg recv: %d\r\n", dev.status, dev.msg_expect, dev.msg_recv);
		BSP_Printf("TIM7_S HB: %d, HB TIMER: %d, Msg TIMEOUT: %d\r\n", dev.hb_count, dev.hb_timer, dev.msg_timeout);
		
		if((strstr((const char*)USART3_RX_BUF,"CLOSED")!=NULL) || (strstr((const char*)USART3_RX_BUF,"+PDP: DEACT")!=NULL))
		{
			dev.msg_recv |= MSG_DEV_RESET;
			dev.need_reset = TRUE;
		}
		else
		{	
			//CMD_NONE: ����֮ǰ��״̬
			if( (dev.status == CMD_NONE) || (dev.status == CMD_LOGIN) || (dev.status == CMD_HB) || (dev.status == CMD_CLOSE_DEVICE)
				 || (dev.status == CMD_OPEN_DEVICE))
			{
				if(dev.msg_expect & MSG_DEV_ACK)
				{
					if(strstr((const char*)USART3_RX_BUF, dev.atcmd_ack) != NULL)
					{
						dev.msg_recv |= MSG_DEV_ACK;
						dev.msg_expect &= ~MSG_DEV_ACK;
						//�������յ�Send Ok ���ĺ��������շ���������
						if(strstr(dev.atcmd_ack, "SEND OK")!=NULL) 
						{
							switch(dev.status)
							{
								case CMD_LOGIN:
									dev.msg_expect |= MSG_DEV_LOGIN;
								break;
								case CMD_HB:
									dev.msg_expect |= MSG_DEV_HB;
								break;
								case CMD_CLOSE_DEVICE:
									dev.msg_expect |= MSG_DEV_CLOSE;
								break;
								default:
									BSP_Printf("Wrong Status: %d\r\n", dev.status);
								break;
							}
						}	
					}
				}
			}	

			if((p=strstr((const char*)USART3_RX_BUF,"TRVBP"))!=NULL)
			{
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
						if(strstr((const char*)p,"TRVBP00")!=NULL)
						{
							BSP_Printf("�յ��豸��½��Ϣ�Ļ���\r\n");
							if((dev.status == CMD_LOGIN) && (dev.msg_expect & MSG_DEV_LOGIN))
							{
								Reset_Device_Status(CMD_IDLE);
							}
						}
						else
						{
							//�յ���������
							if(strstr((const char*)p,"TRVBP01")!=NULL)
							{
								BSP_Printf("�յ���������������\r\n");
								if((dev.status == CMD_HB) && (dev.msg_expect & MSG_DEV_HB))
								{
									Reset_Device_Status(CMD_IDLE);
								}
							}
							else
							{
								//�յ������豸ָ��
								if(strstr((const char*)p,"TRVBP03")!=NULL)
								{
									BSP_Printf("�յ������������豸ָ��\r\n");

									if(dev.status == CMD_IDLE)
									{
										Reset_Device_Status(CMD_OPEN_DEVICE);
										strncpy(dev.device_on_cmd_string, p, p1 - p +1);
									}
								}
								else
								{
									//�յ����н�������
									if(strstr((const char*)p,"TRVBP05")!=NULL)
									{
										BSP_Printf("�յ����������н�������\r\n");
										if((dev.status == CMD_CLOSE_DEVICE) && (dev.msg_expect & MSG_DEV_CLOSE))
										{
											Reset_Device_Status(CMD_IDLE);
										}
									}
									else
									{
										BSP_Printf("�����ڵķ�����ָ��\r\n");
									}
								}
							}
						}
					}   //������ȷ			
				}
			}		
			//��������Ϣ����շ���������
			Clear_Usart3();	
		}
		
		BSP_Printf("TIM7_E Dev Status: %d, Msg expect: %d, Msg recv: %d\r\n", dev.status, dev.msg_expect, dev.msg_recv);
		BSP_Printf("TIM7_E HB: %d, HB TIMER: %d, Msg TIMEOUT: %d\r\n", dev.hb_count, dev.hb_timer, dev.msg_timeout);
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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2 ;//��ռ���ȼ�0
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

