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
#include "STM32vldiscovery.h"
#include "usart.h"
#include "usart3.h"
#include "delay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "SIM800.h"


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

//���ͺ���ʹ�õ�һ�����
u8 result_Send_Data = 0xAA;
u8 count_Send_Data = 0x00;
u8 current_cmd=0;
#define TOTAL_SEND_DATA 3
#define TOTAL_WAIT_ECO  3        
u8 Total_Wait_Echo  =  0;

int main(void)
{
	//u8 temp = 0x00;
	u32 Count_Heart = 0;
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
	
	//����USART3_RX_BUF��USART3_RX_STA
	//��ʹ�ô���3֮ǰ�����㣬�ų�һЩ�������
	Clear_Usart3();
	Flag_Comm_OK = 0;	
	Flag_Need_Reset = 0;
	
	//����
	//����2Gģ���ԴоƬ�����ʹ�ܺ�2Gģ���PWRKEY��ʹ��
	//SIM800_POWER_ON��������Ϊ��ͣʹ��
	SIM800_POWER_ON();  
	SIM800_PWRKEY_ON();
	BSP_Printf("SIM800C�������\r\n");
	
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
	TIM6_Int_Init(499,2399);						     //50ms�ж�
	TIM_SetCounter(TIM6,0); 
	TIM_Cmd(TIM6,ENABLE);
	
	while(1)
	{
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
				//���ݵ�ǰ�豸״̬���п���(GPIO)���Ѿ����˵ľͲ�������
				//��ʵ��
				//�����豸�����ؼ�ʱ
				//if(Flag_Device_Run == 0x01)
				{
					//Enable_Device(1);
					Flag_Local_Time_Dev_01 = 0xAA;    //�豸���Կ�ʼ��ʱ
				}	
				
				//������ǰû�д���������Ϣʱ���Żᷢ��Enable ����
				if(current_cmd == CMD_NONE)
				{
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
						Clear_Usart3();
					}					
				}
			}
			
			if(Flag_Local_Time_Dev_01_OK == 0xAA)
			{
				Flag_Local_Time_Dev_01_OK = 0;
				//�ر��豸(GPIO)
				//��ʵ��
				
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
						Clear_Usart3();
					}					
				}
			}
			
			//����յ��豸���н�������Ļ���
			if(Flag_Receive_Device_OK == 0xAA)
			{
				current_cmd = CMD_NONE;	
				Flag_Receive_Device_OK = 0;
				//��ʱû��Ҫ������߼�
			}
		
			//����յ������������������ģ������ٴη����������Ķ�ʱ
			if(Flag_Receive_Heart == 0xAA)
			{
				Flag_Receive_Heart = 0;
				//�յ����ľͿ����ٴη����������Ķ�ʱ
				Flag_Send_Heart = 0xAA;
				
				if(current_cmd == CMD_HB)       //��ǰ�����ڵȴ���������(�Ƿ���̫�ϸ���)
				{
					current_cmd = CMD_NONE;
				}
				Clear_Usart3();
			}

			//�����������Ķ�ʱʱ�䵽�����봦����������ʱ��������������־
			if(Flag_Send_Heart_OK == 0xAA)
			{
				//��ֻ���ڵ�ǰû��ִ���κ�����ʱ���ŷ�����������Ϣ���������ѭ��	
				if(current_cmd == CMD_NONE)
				{
					//�������Ķ�ʱʱ���������һ��Ҫ������˲��ż���������־
					//�����ǰ���������������У��ȴ��˲�����ɺ�������������
					Flag_Send_Heart_OK = 0;				
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
						Clear_Usart3();
					}
				}
			}		

			//���յ������ڷ��������ط������Ҫ�ж�Ҫ�ط���������ָ��
			//�ڱ���ܳ����У��Ե�¼��Ϣ���ط�Ϊ��������ʾ������
			if(Flag_Receive_Resend == 0xAA)
			{
				Flag_Receive_Resend = 0;
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
						Clear_Usart3();
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
						Clear_Usart3();
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
						Clear_Usart3();
					}
				}	
			}
			
			if(Flag_Check_error == 0xAA)
			{
				//У�����Ҫ��������ط�
				Flag_Check_error = 0;
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
					Clear_Usart3();
				}
			}

			//�豸������Ϣ����������ȴ����ģ��и���ȴ�ʱ�䣬�����ʱ����û���յ��������Ļ���,��Ҫ�������ɴε����·���
			if(Flag_Time_Out_Comm == 0xAA)
			{
				BSP_Printf("1���ӵĵȴ����������ĳ�ʱ\r\n");
				Flag_Time_Out_Comm = 0;
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
						Clear_Usart3();
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
						Clear_Usart3();
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
						Clear_Usart3();
					}
				}			
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
				Flag_Receive_Login = 0;			
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
						Clear_Usart3();
					}	
				}
			}		
			
			if(Flag_Receive_Resend == 0xAA)
			{
				Flag_Receive_Resend = 0;
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
						Clear_Usart3();
					}
				}
			}
			
			if(Flag_Check_error == 0xAA)
			{
				//У�����Ҫ��������ط�
				Flag_Check_error = 0;
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
					Clear_Usart3();
				}
			}			

			if(Flag_Time_Out_Comm == 0xAA)
			{
				BSP_Printf("1���ӵĵȴ����������ĳ�ʱ\r\n");
				Flag_Time_Out_Comm = 0;
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
						Clear_Usart3();
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
						Clear_Usart3();
					}
				}
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
			//������Ϊ����û����
			Flag_Comm_OK = 0;
			//����ط��ȴ�
			Total_Wait_Echo = 0;
			Flag_ACK_Echo = 0xFF;
			//�����������־
			Flag_Send_Heart = 0;
			Flag_Send_Heart_OK = 0;
			//������豸���м�ʱ��־����Ϊ�����豸�Լ���״̬����SIM800C �޹�
			
			SIM800_Powerkey_Restart(); //��������ֱ��pwr/pwrkey?
			if(SIM800_Link_Server() == CMD_ACK_OK)
				if(Send_Login_Data_To_Server() == CMD_ACK_OK)
				{
					//���ִ�в�������ͻ��ظ�reset
					Flag_Need_Reset = 0;
				}	
		}

		//���յ���SIM800C����Ϣ
		if(0x01 == Receive_Data_From_USART())	
		{
			/*��ʱ��������*/
			//���㴮��3�Ľ��ձ�־����
			
			BSP_Printf("Main USART3_RX_BUF_SIM800C:%s\r\n",USART3_RX_BUF);

			if(strstr((const char*)USART3_RX_BUF,"CLOSED")!=NULL)
				Flag_Received_SIM800C_CLOSED = 0xAA;
			if(strstr((const char*)USART3_RX_BUF,"+PDP: DEACT")!=NULL)
				Flag_Received_SIM800C_DEACT = 0xAA;
			Clear_Usart3();

		}
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
