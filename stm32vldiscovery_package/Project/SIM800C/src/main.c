/**
  ******************************************************************************
  * @file    Demo/src/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    09/13/2010
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32F10x.h"
#include "usart.h"
#include "usart3.h"
#include "delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "SIM800.h"
#include "device.h"


////////////////////////ST�ٷ���������Ҫ�ı����ͺ���///////////////////////////////////////////

/**********************�����ж����ȼ�����******************************************  
                PreemptionPriority        SubPriority
USART3                 0											0

DMA1                   1											0

TIM7									 2											0	

TIM6									 3											0

TIM4                   3                      1

************************************************************************************/

////////////////////////�û������Զ���ı����ͺ���///////////////////////////////////////////

#define TOTAL_WAIT_ECO  3        
u8 Total_Wait_Echo  =  0;

void Reset_Device_Status(u8 status)
{
	dev.status = status;
	dev.hb_timer = 0;
	dev.reply_timeout = 0;
	dev.msg_timeout = 0;
	dev.msg_recv = 0;
	dev.msg_expect = 0;
	memset(dev.atcmd_ack, 0, sizeof(dev.atcmd_ack));
	memset(dev.device_on_cmd_string, 0, sizeof(dev.device_on_cmd_string));	
}

int main(void)
{
	u8 i;
	//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
	/* Enable GPIOx Clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	delay_init();

#ifdef LOG_ENABLE	
	//ע�⴮��1�����������ͣ����յĺ�û��ʹ��
	//#define EN_USART1_RX 			   0		//ʹ�ܣ�1��/��ֹ��0������1����
	usart1_init(115200);                            //����1,Log
#endif

	usart3_init(115200);                            //����3,�Խ�SIM800
	Reset_Device_Status(CMD_NONE);
	//����USART3_RX_BUF��USART3_RX_STA
	//��ʹ�ô���3֮ǰ�����㣬�ų�һЩ�������
	Clear_Usart3();
	
	//����
	//����2Gģ���ԴоƬ�����ʹ�ܺ�2Gģ���PWRKEY��ʹ��
	//SIM800_POWER_ON��������Ϊ��ͣʹ��
	SIM800_POWER_ON();  
	SIM800_PWRKEY_ON();
	BSP_Printf("SIM800C�������\r\n");
	
	Device_Init();
	//Device_ON(DEVICE_01);
	//Device_ON(DEVICE_04);
	for(i=DEVICE_01; i<DEVICEn; i++)
	{
		BSP_Printf("Power[%d]: %d\n", i, Device_Power_Status(i));
	}
	
	//���ӷ�����ʧ�ܣ���˸һ��LED��Ϊ���ѣ������ö������滻
	if(SIM800_Link_Server() != CMD_ACK_OK)
	{
		BSP_Printf("SIM800C���ӷ�����ʧ�ܣ�����\r\n");
		//while(1){��˸LED}
	}
	
	BSP_Printf("SIM800C���ӷ��������\r\n");
	
	//�����������ˣ��Ѿ��ɹ��������Ϸ����������淢�͵�¼��Ϣ��������
		//���͵�¼��Ϣ��������ʧ�ܣ���˸һ��LED��Ϊ���ѣ������ö������滻
	if(Send_Login_Data_To_Server() != CMD_ACK_OK)
	{
		BSP_Printf("SIM800C���͵�¼��Ϣ��������ʧ�ܣ�����\r\n");
		//while(1){��˸LED}
	}
	BSP_Printf("SIM800C���͵�¼��Ϣ�����������\r\n");

	//�����������ˣ��Ѿ��ɹ����͵�¼��Ϣ�������������濪����ʱ�����ȴ��������Ļ�����Ϣ
	//���㴮��3�����ݣ��ȴ����շ�����������
	Clear_Usart3();
	
	//������ʱ����ÿ50ms��ѯһ�η������Ƿ����·�����
	//��ѭ��TIMER
	TIM6_Int_Init(9999,2399);						     // 1s�ж�
	TIM_SetCounter(TIM6,0); 
	TIM_Cmd(TIM6,ENABLE);
	
	while(1)
	{

		//BSP_Printf("Main_S Dev Status: %d, Msg expect: %d, Msg recv: %d\r\n", dev.status, dev.msg_expect, dev.msg_recv);
		//BSP_Printf("Main_S HB: %d, HB TIMER: %d, Msg TIMEOUT: %d\r\n", dev.hb_count, dev.hb_timer, dev.msg_timeout);
	
		if(dev.need_reset)
		{
			BSP_Printf("��ʼ����\r\n");	
			TIM_Cmd(TIM7, DISABLE);
			Reset_Device_Status(CMD_NONE);
			dev.need_reset = FALSE;
			SIM800_Powerkey_Restart(); 
			Clear_Usart3();
			TIM_Cmd(TIM7, ENABLE);
			if(SIM800_Link_Server() != CMD_ACK_OK)
			{
				BSP_Printf("�������ӷ�����ʧ��\r\n");
				dev.need_reset = TRUE;
			}
		}
		else
		{
			switch(dev.status)
			{
				case CMD_LOGIN:
					if((dev.msg_expect == 0) && (dev.status == CMD_LOGIN))
					{
						if(Send_Login_Data_To_Server() != CMD_ACK_OK)
						{
							BSP_Printf("SIM800C���͵�¼��Ϣ��������ʧ��\r\n");
							dev.need_reset = TRUE;
						}
					}
				break;
				
				case CMD_HB:					
					if((dev.msg_expect == 0) && (dev.status == CMD_HB))
					{
						BSP_Printf("��  %d  �η�������\r\n", dev.hb_count++);
						if(Send_Heart_Data_To_Server() != CMD_ACK_OK)
						{
							BSP_Printf("SIM800C����������Ϣ��������ʧ��\r\n");
							dev.need_reset = TRUE;
						}
					}					
				break;
				
				case CMD_CLOSE_DEVICE:
					if((dev.msg_expect == 0) && (dev.status == CMD_CLOSE_DEVICE))
					{
						if(Send_Close_Device_Data_To_Server() != CMD_ACK_OK)
						{
							BSP_Printf("SIM800C�����豸�رո�������ʧ��\r\n");
							dev.need_reset = TRUE;
						}
					}					
				break;
				
				case CMD_OPEN_DEVICE:
				{
					char *msg_id, *device, *interfaces, *periods;
					bool interface_on[DEVICEn]={FALSE};
					int period_on[DEVICEn]={0};
					//���ݵ�ǰ�豸״̬���п���(GPIO)���Ѿ����˵ľͲ�������
					//��ʵ��
					//�����豸�����ؼ�ʱ
					msg_id = strtok(dev.device_on_cmd_string, ",");
					if(msg_id)
						BSP_Printf("msg_id: %s\n", msg_id);

					device = strtok(NULL, ",");
					if(device)
						BSP_Printf("device: %s\n", device);	
					
					interfaces = strtok(NULL, ",");
					if(interfaces)
						BSP_Printf("ports: %s\n", interfaces);

					for(i=DEVICE_01; i<DEVICEn; i++)
						interface_on[i]=(interfaces[i]=='1')?TRUE:FALSE;
						
					periods = strtok(NULL, ",");
					if(periods)
						BSP_Printf("periods: %s\n", periods);	

					sscanf(periods, "%02d%02d%02d%02d,", &period_on[DEVICE_01], 
						&period_on[DEVICE_02], &period_on[DEVICE_03], &period_on[DEVICE_04]);

					for(i=DEVICE_01; i<DEVICEn; i++)
					{
						if(interface_on[i] && (g_device_status[i].power == OFF))
						{
							g_device_status[i].total = period_on[i] * NUMBER_TIMER_1_MINUTE;
							g_device_status[i].passed = 0;
							g_device_status[i].power = ON;		
							Device_ON(i);
							BSP_Printf("Port[%d]: %d\n", i, g_device_status[i].total);	
						}			
					}
					memset(dev.device_on_cmd_string, 0, sizeof(dev.device_on_cmd_string));
					
					//���ͻ��ĸ�������������Ҫ����·�豸��״̬���������豸��������ʱ�䣺Count_Local_Time��50���룩
					if(Send_Open_Device_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("���ʹ��豸����ʧ��\r\n");
						dev.need_reset = TRUE;			
					}
					else
					{
						BSP_Printf("SIM800C����Enable ���ĸ����������\r\n");
					}				
				}
								
				break;
				
				default:
				break;
			}
		}
		
		//BSP_Printf("Main_E Dev Status: %d, Msg expect: %d, Msg recv: %d\r\n", dev.status, dev.msg_expect, dev.msg_recv);
		//BSP_Printf("Main_E HB: %d, HB TIMER: %d, Msg TIMEOUT: %d\r\n", dev.hb_count, dev.hb_timer, dev.msg_timeout);
		
#if 0		
		if((Flag_Received_SIM800C_CLOSED == 0xAA) || (Flag_Received_SIM800C_DEACT == 0xAA))
		{
			Clear_Usart3();
			Flag_Received_SIM800C_CLOSED=0;
			Flag_Received_SIM800C_DEACT=0;
			Flag_Need_Reset =0xAA;
			BSP_Printf("���ӶϿ���׼������\r\n");
			goto reset;
		}
		
		//����յ���½��Ϣ���ģ��ͽ��뷢������������
		if(Flag_Comm_OK == 0xAA)//���������һ����Ϣ���Է��͵Ļ��������ǲ�Ӱ�춨ʱ��
		{			
			//����յ������豸��ָ��,������ȷ���յĻ��ĸ���������������ҵ��ָ����豸
			if(Flag_Receive_Enable == 0xAA)
			{
				//����ǰû�д���������Ϣʱ�����ܲ����豸�����ͻ���
				if(current_cmd == CMD_NONE)
				{
					char *msg_id, *device, *interfaces, *periods;
					bool interface_on[DEVICEn]={FALSE};
					int period_on[DEVICEn]={0};
					//���ݵ�ǰ�豸״̬���п���(GPIO)���Ѿ����˵ľͲ�������
					//��ʵ��
					//�����豸�����ؼ�ʱ
					msg_id = strtok(device_on_cmd_string, ",");
					if(msg_id)
						BSP_Printf("msg_id: %s\n", msg_id);

					device = strtok(NULL, ",");
					if(device)
						BSP_Printf("device: %s\n", device);	
					
					interfaces = strtok(NULL, ",");
					if(interfaces)
						BSP_Printf("ports: %s\n", interfaces);

					for(i=DEVICE_01; i<DEVICEn; i++)
						interface_on[i]=(interfaces[i]=='1')?TRUE:FALSE;
						
					periods = strtok(NULL, ",");
					if(periods)
						BSP_Printf("periods: %s\n", periods);	

					sscanf(periods, "%02d%02d%02d%02d,", &period_on[DEVICE_01], 
						&period_on[DEVICE_02], &period_on[DEVICE_03], &period_on[DEVICE_04]);

					for(i=DEVICE_01; i<DEVICEn; i++)					
						if(interface_on[i] && (g_device_status[i].power == OFF))
						{
							g_device_status[i].total = period_on[i] * NUMBER_TIMER_1_MINUTE;
							g_device_status[i].passed = 0;
							g_device_status[i].power = ON;		
							g_device_status[i].need_notif = FALSE;
							Device_ON(i);
							BSP_Printf("Port[%d]: %d\n", i, g_device_status[i].total);	
							//Flag_Local_Time_Dev_01 = 0xAA;    //�豸���Կ�ʼ��ʱ
						}			
					
					//���ͻ��ĸ�������������Ҫ����·�豸��״̬���������豸��������ʱ�䣺Count_Local_Time��50���룩
					if(Send_Enable_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("�����豸����״̬��Ϣʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;						
						//while(1){��˸LED}
					}
					else
					{				
						BSP_Printf("SIM800C����Enable ���ĸ����������\r\n");
						//Clear_Usart3();
					}					
				}
				memset(device_on_cmd_string, 0, sizeof(device_on_cmd_string));
				Flag_Receive_Enable = 0;				
			}

			//�豸�ڶ�ʱ�����Ѿ��رգ�������Ҫ�ϱ��ر���Ϣ
			for(i=DEVICE_01; i<DEVICEn; i++)
			{
				if(g_device_status[i].need_notif)  //if(Flag_Local_Time_Dev_01_OK == 0xAA)
					need_notif = TRUE;

				g_device_status[i].need_notif = FALSE;
			}
			if(need_notif)
			{	
				if(current_cmd == CMD_NONE)
				{
					//�����豸���н��������������
					if(Send_Device_OK_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("�����豸���н�����Ϣʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;							
						//while(1){��˸LED}
					}	
					else
					{				
						BSP_Printf("SIM800C�����豸���н��������������\r\n");
						//Clear_Usart3();
					}					
				}
				need_notif = FALSE;
			}				
			
			//����յ��豸���н�������Ļ���
			if(Flag_Receive_Device_OK == 0xAA) 
			{	
				if(current_cmd == CMD_CLOSE_DEVICE)
				{
					//�رյȴ����������ĵĳ�ʱ����			
					Flag_ACK_Resend = 0xFF;
					Flag_ACK_Echo = 0xFF;
					Flag_Wait_Echo = 0;
					Count_Wait_Echo = 0;	
					Total_Wait_Echo = 0;
					
					current_cmd = CMD_NONE;	
					//��ʱû��Ҫ������߼�
				}
				Flag_Receive_Device_OK = 0;					
			}
		
			//����յ������������������ģ������ٴη����������Ķ�ʱ
			//
			//if((Flag_Receive_Heart == 0xAA) && (current_cmd == CMD_HB))
			if(Flag_Receive_Heart == 0xAA)
			{				
				if(current_cmd == CMD_HB)       //��ǰ�����ڵȴ���������(�Ƿ���̫�ϸ���)
				{
					//�رյȴ����������ĵĳ�ʱ����			
					Flag_ACK_Resend = 0xFF;
					Flag_ACK_Echo = 0xFF;
					Flag_Wait_Echo = 0;
					Count_Wait_Echo = 0;	
					Total_Wait_Echo = 0;				
					
					current_cmd = CMD_NONE;
				}
				Flag_Receive_Heart = 0;
				//�յ����ľͿ����ٴη����������Ķ�ʱ
				Flag_Send_Heart = 0xAA;
				Count_Send_Heart = 0;				
				Clear_Usart3();
			}

			//�����������Ķ�ʱʱ�䵽�����봦����������ʱ��������������־
			if(Flag_Send_Heart_OK == 0xAA)
			{
				//��ֻ���ڵ�ǰû��ִ���κ�����ʱ���ŷ�����������Ϣ���������ѭ��	
				if(current_cmd == CMD_NONE)
				{
					//�������Ķ�ʱʱ���������һ��ִ�У���˲��ż���������־
					//�����ǰ���������������У��ȴ��˲�����ɺ�������������	
					Count_Heart += 1;
					BSP_Printf("��%ld�η���������\r\n",Count_Heart);
					BSP_Printf("һ���ӵ������������,�ٴη���������\r\n");				
					if(Send_Heart_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("SIM800C������������������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;
						//while(1){��˸LED}
					}
					else
					{				
						BSP_Printf("SIM800C���������������������\r\n");
						//Clear_Usart3();
					}
					Flag_Send_Heart_OK = 0;			
				}
			}		

			//���յ����Է��������ط������Ҫ�ж�Ҫ�ط���������ָ��
			//�ڱ���ܳ����У��Ե�¼��Ϣ���ط�Ϊ��������ʾ������
			if(Flag_Receive_Resend == 0xAA)
			{
				//����Flag_ACK_Resend��ֵ��ȷ��Ҫ�ط�������������
				if(Flag_ACK_Resend == 0x02)
				{
					Flag_ACK_Resend = 0xFF;
					if(Send_Heart_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("SIM800C������������������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;
						//while(1){��˸LED}
					}
					else
					{
						BSP_Printf("SIM800C���������������������\r\n");
						//Clear_Usart3();
					}
				}
				
				if(Flag_ACK_Resend == 0x04)
				{
					Flag_ACK_Resend = 0xFF;
					if(Send_Enable_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("SIM800C�����豸���п��������������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;
						//while(1){��˸LED}
					}
					else
					{					
						BSP_Printf("SIM800C�����豸���п�����������������\r\n");
						//Clear_Usart3();
					}
				}
				
				if(Flag_ACK_Resend == 0x05)
				{
					Flag_ACK_Resend = 0xFF;
					if(Send_Device_OK_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("SIM800C�����豸���н��������������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;						
						//while(1){��˸LED}
					}
					else
					{
						BSP_Printf("SIM800C�����豸���н�����������������\r\n");
						//Clear_Usart3();
					}
				}	
				Flag_Receive_Resend = 0;				
			}
			
			if(Flag_Check_error == 0xAA)
			{
				//У�����Ҫ��������ط�
				if(Send_Resend_Data_To_Server() != CMD_ACK_OK)
				{
					BSP_Printf("SIM800C�����ط������������ʧ�ܣ�����\r\n");
					current_cmd = CMD_NONE;
					Flag_Need_Reset = 0xAA;
					goto reset;						
					//while(1){��˸LED}
				}
				else
				{
					BSP_Printf("SIM800C�����ط���������������\r\n");
					//Clear_Usart3();
				}
				Flag_Check_error = 0;				
			}

			//�豸������Ϣ����������ȴ����ģ��и���ȴ�ʱ�䣬�����ʱ����û���յ��������Ļ���,��Ҫ�������ɴε����·���
			if(Flag_Time_Out_Comm == 0xAA)
			{
				BSP_Printf("1���ӵĵȴ����������ĳ�ʱ\r\n");
				Total_Wait_Echo	+= 1;	
				//���ݱ���Flag_ACK_Echo��ֵ���ж�Ҫ���·��͵�����
				if(Flag_ACK_Echo == 0x02)
				{
					Flag_ACK_Echo = 0xFF;
					BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�����·���������\r\n");
					if(Send_Heart_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ������������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;
						//while(1){��˸LED}
					}
					else
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�������������ɹ�\r\n");				
						//Clear_Usart3();
					}

				}
				if(Flag_ACK_Echo == 0x03)
				{
					Flag_ACK_Echo = 0xFF;
					BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�����·����豸���н�������\r\n");			
					if(Send_Resend_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�������豸���н�������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;
						//while(1){��˸LED}
					}
					else
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�������豸���н�������ɹ�\r\n");
						//Clear_Usart3();
					}
				}
				if(Flag_ACK_Echo == 0x05)
				{
					Flag_ACK_Echo = 0xFF;
					BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�����·����豸���н�������\r\n");			
					if(Send_Device_OK_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�������豸���н�������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;
						//while(1){��˸LED}
					}
					else
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�������豸���н�������ɹ�\r\n");
						//Clear_Usart3();
					}
				}		
				Flag_Time_Out_Comm = 0;				
			}
			
			//���ɴ��ط�����Ȼ�ղ������������ģ�����GPRS		
			if(Total_Wait_Echo == TOTAL_WAIT_ECO)
			{
				Total_Wait_Echo = 0;
				Flag_ACK_Echo = 0xFF;
				Flag_Need_Reset = 0xAA;
				Clear_Usart3();
				goto reset;
			}			
			
		}
		//�������ӣ���Ҫ����login�����������ȴ�login����
		//��û���յ�����������login����ʱ����ȫ������������Ϣ
		else  
		{
			if(Flag_Receive_Login == 0xAA)  //�յ�login ����
			{
				if(current_cmd == CMD_LOGIN)   //��ǰ�豸�����ڵȴ�login����״̬
				{
					//�յ���Ҫ�ȴ��Ļ��ģ���Ҫ���resend/echo ��־�˰�?
					Flag_ACK_Resend = 0xFF;
					Flag_ACK_Echo = 0xFF;		
					
					BSP_Printf("SIM800C��ȷ���յ�¼��Ϣ�Ļ���\r\n");
					//�������������ݸ�������
					if(Send_Heart_Data_To_Server() != CMD_ACK_OK)  //�����Ӻ�ĵ�һ������
					{
						BSP_Printf("SIM800C������������������ʧ�ܣ����µ�¼������\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;
						//while(1){��˸LED}
					}
					else
					{
						BSP_Printf("SIM800C��������������������ɣ��ȴ�����������\r\n");
						Flag_Comm_OK = 0xAA;     //�����������������������Ч
						//Clear_Usart3();
					}	
				}
				Flag_Receive_Login = 0;							
			}		
			
			if(Flag_Receive_Resend == 0xAA)
			{
				//δ����ʱ����login��Ϣ���ط�����
				//����û�п��Ƿ�����Ҫ���ط�Resend ��Ϣ�������о�ûɶ��Ҫ
				if(Flag_ACK_Resend == 0x01)
				{
					Flag_ACK_Resend = 0xFF;
					//���͵�¼��Ϣ��������ʧ�ܣ���˸һ��LED��Ϊ���ѣ������ö������滻
					if(Send_Login_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("SIM800C���͵�¼��Ϣ��������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;
						goto reset;
						//while(1){��˸LED}
					}
					else
					{
						BSP_Printf("SIM800C���͵�¼��Ϣ�����������\r\n");
						//Clear_Usart3();
					}
				}
				Flag_Receive_Resend = 0;				
			}
			
			if(Flag_Check_error == 0xAA)
			{
				//У�����Ҫ��������ط�
				if(Send_Resend_Data_To_Server() != CMD_ACK_OK)
				{
					BSP_Printf("SIM800C�����ط������������ʧ�ܣ�����\r\n");
					current_cmd = CMD_NONE;
					Flag_Need_Reset = 0xAA;
					goto reset;
					//while(1){��˸LED}
				}
				else
				{
					BSP_Printf("SIM800C�����ط���������������\r\n");
					//Clear_Usart3();
				}
				Flag_Check_error = 0;				
			}			

			if(Flag_Time_Out_Comm == 0xAA)
			{
				BSP_Printf("1���ӵĵȴ����������ĳ�ʱ\r\n");
				Total_Wait_Echo	+= 1;	
				//���ݱ���Flag_ACK_Echo��ֵ���ж�Ҫ���·��͵�����
				if(Flag_ACK_Echo == 0x01)
				{
					Flag_ACK_Echo = 0xFF;
					BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�����·��͵�¼��Ϣ\r\n");

					//���͵�¼��Ϣ��������ʧ�ܣ���˸һ��LED��Ϊ���ѣ������ö������滻
					if(Send_Login_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�����͵�¼��Ϣʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;	
						goto reset;
						//while(1){��˸LED}
					}
					else
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�����͵�¼��Ϣ�ɹ�\r\n");
						//Clear_Usart3();
					}
					
				}
				//����Resend ��Ϣ����������ʱ���������о�û��Ҫ���е����ˡ�����
				if(Flag_ACK_Echo == 0x03)
				{
					Flag_ACK_Echo = 0xFF;
					BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�����·����豸���н�������\r\n");			
					if(Send_Resend_Data_To_Server() != CMD_ACK_OK)
					{
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�������豸���н�������ʧ�ܣ�����\r\n");
						current_cmd = CMD_NONE;
						Flag_Need_Reset = 0xAA;	
						goto reset;						
						//while(1){��˸LED}
					}
					else
					{					
						BSP_Printf("1���ӵĵȴ����������ĳ�ʱ�������豸���н�������ɹ�\r\n");
						//Clear_Usart3();
					}
				}
				Flag_Time_Out_Comm = 0;				
			}
			
			//���ɴ��ط�����Ȼ�ղ������������ģ�����GPRS		
			if(Total_Wait_Echo == TOTAL_WAIT_ECO)
			{
				BSP_Printf("TOTAL_WAIT_ECO�εȴ����������ĳ�ʱ�����¿���GPRS\r\n");
				Total_Wait_Echo = 0;
				Flag_ACK_Echo = 0xFF;
				Flag_Need_Reset = 0xAA;
				Clear_Usart3();
				goto reset;
			}			
		}

	reset:
		if(Flag_Need_Reset == 0xAA)
		{
			BSP_Printf("��ʼ����\r\n");		
	
			Reset_Event();
			//������豸���м�ʱ��־����Ϊ�����豸�Լ���״̬����SIM800C �޹�
			
			SIM800_Powerkey_Restart(); //��������ֱ��pwr/pwrkey?
			if(SIM800_Link_Server() == CMD_ACK_OK)
				if(Send_Login_Data_To_Server() == CMD_ACK_OK)
				{
					//���ִ�в�������ͻ��ظ�reset
					Flag_Need_Reset = 0;
				}	
		}
#endif		
	}
}


#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
