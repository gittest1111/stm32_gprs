#include "SIM800.h"	 	 	
#include "string.h"  
#include "stdio.h"	
#include "usart.h" 
#include "usart3.h" 
#include "timer.h"
#include "delay.h"


/* ���������ڵ��Խ׶��Ŵ� */
#define BSP_Printf		printf
//#define BSP_Printf(...)



extern u8 result_Send_Data;
extern u8 count_Send_Data;
#define TOTAL_SEND_DATA 3
#define COUNT_AT 10

u8 mode = 0;				//0,TCP����;1,UDP����
const char *modetbl[2] = {"TCP","UDP"};//����ģʽ

//const char  *ipaddr = "tuzihog.oicp.net";
//const char  *port = "28106";

const char  *ipaddr = "42.159.117.91";
const char  *port = "8090";



//ģ�����ӷ�����������,ATָ���쳣ʱ��λ�ñ���
u8 Flag_AT_Unusual = 0;

//�洢PCB_ID�����飨Ҳ����SIM����ICCID��
char ICCID_BUF[LENGTH_ICCID_BUF] = {0};

//�洢�豸�ط����������
char Resend_Buffer[LENGTH_RESEND] = {0};

//�洢�豸��½���������
char Login_Buffer[LENGTH_LOGIN] = {0};

//�洢������������
char Heart_Buffer[LENGTH_HEART] = {0};

//�洢ҵ��ָ����ĵ�����
char Enbale_Buffer[LENGTH_ENABLE] = {0};

//�洢ҵ��ִ�����ָ�������
char Device_OK_Buffer[LENGTH_DEVICE_OK] = {0};

//SIM800���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����
//����,�ڴ�Ӧ������λ��(str��λ��)
u8* SIM800_Check_Cmd(u8 *str)
{
	char *strx = 0;
	if(USART3_RX_STA&0X8000)		//���յ�һ��������
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF] = 0;//��ӽ�����
		strx = strstr((const char*)USART3_RX_BUF,(const char*)str);
	} 
	return (u8*)strx;
}

//SIM800��������
//cmd:���͵������ַ���(����Ҫ��ӻس���),��cmd<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 SIM800_Send_Cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res = 0; 
	Clear_Usart3();	
	if((u32)cmd <= 0XFF)
	{
		while((USART3->SR&0X40)==0);//�ȴ���һ�����ݷ������  
		USART3->DR = (u32)cmd;
	}
	else 
	{
		u3_printf("%s\r\n",cmd);//��������
	}

	if(ack&&waittime)		//��Ҫ�ȴ�Ӧ��
	{
		while(--waittime)	//�ȴ�����ʱ
		{ 
			delay_ms(10);	
			if(USART3_RX_STA&0X8000)//���յ��ڴ���Ӧ����
			{
				//�õ�����Ч�Ļ�����Ϣ��
				if(SIM800_Check_Cmd(ack))
				{
					//����ĺ�������Ҫ����õ��Ļ��ģ��������ﲻ�ܵ��ú���Clear_Usart3
					break;
				} 
				//û�еõ���Ч�Ļ�����Ϣ�����㴮��3�������´εĽ���
				else
				{
						Clear_buffer((char*)USART3_RX_BUF,USART3_MAX_RECV_LEN);	
						USART3_RX_STA = 0;		    	//����	
				}
			} 
		}
		
		if(waittime == 0)
		{
			//ATָ���Ѿ�������ֻ���ڵȴ�ʱ���ڣ�USART3_RX_STAû����λ����ʱ���ܻ������ݣ�����Ҫ����
			Clear_Usart3();
			res = 1; 
		}
	}
	
	else   //����Ҫ�ȴ�Ӧ��,������ʱ�������صĴ������
	{
		;
	
	}
	return res;
} 



u8 Check_Module(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT","OK",100))//����Ƿ�Ӧ��ATָ�� 
	{ 
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			//ATָ���Ѿ�������COUNT_AT�Σ���Ȼʧ��
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

	//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
	return 0;	
}

//�رջ���
u8 Disable_Echo(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("ATE0","OK",200))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

	//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
	return 0;	
}

//�鿴SIM�Ƿ���ȷ��⵽
u8 Check_SIM_Card(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CPIN?","OK",200))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

	//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
	return 0;	
}

//�鿴��������
u8 Check_CSQ(void)
{
	u8 count = 0;
	u8 *p1 = NULL; 
	u8 *p2 = NULL;
  u8 p[50] = {0}; 
	while(SIM800_Send_Cmd("AT+CSQ","+CSQ:",200))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

	//ATָ���Ѿ�ָ����ɣ�����Է���ֵ���д���
	p1=(u8*)strstr((const char*)(USART3_RX_BUF),":");
	p2=(u8*)strstr((const char*)(p1),",");
	p2[0]=0;//���������
	sprintf((char*)p,"�ź�����:%s",p1+2);
	BSP_Printf("%s\r\n",p);
	//ATָ��Ļ����Ѿ�������ɣ�����
	Clear_Usart3();
	return 0;
}

//��ȡSIM����ICCID
//SIM����ICCID,ȫ��Ψһ�ԣ���������PCB�����ID
//��ӡUSART3_RX_BUF�ă��� �{ԇ��;
		/*****  ע��+��ǰ���������ո�
  +CCID: 1,"898602B8191650216485"

		OK
		****/
//���������û������ȷ��....
u8 Get_ICCID(void)
{
	u8 index = 0;
	u8 *p_temp = NULL;
	u8 count = 0;

	while(SIM800_Send_Cmd("AT+CCID","OK",200))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}
	
	//ATָ���Ѿ�ָ����ɣ�����Է���ֵ���д���
	//BSP_Printf("CCID:%s\r\n",USART3_RX_BUF);

	p_temp = USART3_RX_BUF;
	//��ȡICCID��Ϣ��ȫ�ֱ���ICCID_BUF
	for(index = 0;index < LENGTH_ICCID_BUF;index++)
	{
		ICCID_BUF[index] = *(p_temp+OFFSET_ICCID+index);
	}
	//BSP_Printf("ICCID_BUF:%s\r\n",ICCID_BUF);
	//ATָ��Ļ����Ѿ�������ɣ�����
	Clear_Usart3();
	return 0;

}

u8 SIM800_GPRS_ON(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CGCLASS=\"B\"","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}
		//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
	count = 0;
	
	while(SIM800_Send_Cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}
	Clear_Usart3();
	count = 0;
	
	while(SIM800_Send_Cmd("AT+CGATT=1","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}
	Clear_Usart3();
	count = 0;
	
	while(SIM800_Send_Cmd("AT+CIPCSGP=1,\"CMNET\"","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}
	Clear_Usart3();
	count = 0;

	while(SIM800_Send_Cmd("AT+CIPHEAD=1","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}
	Clear_Usart3();
	count = 0;
	
	while(Link_Server_AT(0,ipaddr,port))
	{
		if(count < COUNT_AT)
		{
		  count += 1;
			delay_ms(2000);
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;
		}
	}
	//ATָ��Ļ����Ѿ�������ɣ�����
	Clear_Usart3();
	return 0;		
}


//�ر�GPRS������
u8 SIM800_GPRS_OFF(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",500))//����Ƿ�Ӧ��ATָ�� 
	{
		
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

		//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
  return 0;

}

//����GPRS
u8 SIM800_GPRS_Adhere(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CGATT=1","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

		//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
  return 0;
}

//����ΪGPRS����ģʽ
u8 SIM800_GPRS_Set(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CIPCSGP=1,\"CMNET\"","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

		//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
  return 0;
}

//���ý���������ʾIPͷ(�����ж�������Դ)	
u8 SIM800_GPRS_Dispaly_IP(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CIPHEAD=1","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

		//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
  return 0;
}

//�ر��ƶ����� 
u8 SIM800_GPRS_CIPSHUT(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CIPSHUT","SHUT OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

		//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
  return 0;
}

//����GPRS�ƶ�̨���ΪB,֧�ְ����������ݽ��� 
u8 SIM800_GPRS_CGCLASS(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CGCLASS=\"B\"","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

		//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
  return 0;
}


	//����PDP������,��������Э��,��������Ϣ
u8 SIM800_GPRS_CGDCONT(void)
{
	u8 count = 0;
	while(SIM800_Send_Cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",300))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}

		//ATָ���Ѿ���ɣ����Ҳ���Ҫ�Է���ֵ���д�������ֱ������
	Clear_Usart3();
  return 0;
}


u8 Link_Server_Echo(void)
{
	u8 count = 0;
	while(Link_Server_AT(0,ipaddr,port))
	{
		if(count < COUNT_AT)
		{
		  count += 1;
			delay_ms(2000);
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;
		}
	}
	//ATָ��Ļ����Ѿ�������ɣ�����
	Clear_Usart3();
	return 0;
}


u8 Link_Server_AT(u8 mode,const char* ipaddr,const char *port)
{
	u8 count = 0;
	u8 res = 0xAA;
	char *temp1 = NULL;
	char *temp2 = NULL;
	char *temp3 = NULL;
	u8 p[256] = {0};
	u16 index = 0;
	
	if(mode)
	;
	else 
		;
		
  sprintf((char*)p,"AT+CIPSTART=\"%s\",\"%s\",\"%s\"",modetbl[mode],ipaddr,port);	

	//��������
	//AT+IPSTARTָ����ܵĻ����ǣ�CONNECT OK ��ALREADY CONNECT��CONNECT FAIL
	//������ȡ���ֿ��ܻ��ĵĹ�����������Ϊ�жϸ�ָ������ȷ���ĵ�����

	while(SIM800_Send_Cmd(p,"CONNECT",500))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
			Flag_AT_Unusual = 0xAA;
			return 1;		
		}
	}
	
	//ATָ���Ѿ�ָ����ɣ�����Է���ֵ���д���

	for(index = 0; index < 256; index++)
	{
		p[index]= USART3_RX_BUF[index];
	}
	temp1 = strstr((const char*)p,"CONNECT OK");
	temp2 = strstr((const char*)p,"ALREADY CONNECT");
	temp3 = strstr((const char*)p,"CONNECT FAIL");
	
	if(temp1 != NULL)
	{
		//ATָ��Ļ����Ѿ�������ɣ�����
		Clear_Usart3();
		res = 0;
	}
	if(temp2 != NULL)
	{
		//ATָ��Ļ����Ѿ�������ɣ�����
		Clear_Usart3();
		res = 0;
	}			
	if(temp3 != NULL)
	{
		res = 2;
	}
	return res;
}




u8 Send_Data_To_Server(char* data)
{
	u8 res = 0xBB;
	if(SIM800_Send_Cmd("AT+CIPSEND",">",500)==0)		//��������
	{ 
		Clear_Usart3();
		u3_printf("%s\r\n",data);
		delay_ms(100);

		if(SIM800_Send_Cmd((u8*)0x1A,"SEND OK",1000)==0)
		{
			Clear_Usart3();
			res = 0x00;
		}
		else 
		{
			res = 0x01;
		}
	  //delay_ms(1000); 
	}
	else 
	{
		res = 0x01;
	}
	
	return res;
}


u8 Receive_Data_From_USART(void)
{
	u8 res = 0xAA;
		
	if(USART3_RX_STA & 0X8000)		            //���յ�һ��������
	{ 
		USART3_RX_BUF[USART3_RX_STA&0X7FFF]=0;	//��ӽ����� 
		if(strstr((const char*)USART3_RX_BUF,"TRVBP"))
		{
			res = 0;
		}
		else
		{
			res = 1;
		}
	}
	
	return res;
}



#if 0
u8 Check_Link_Status(void)
{
	u8 count = 0;

	while(SIM800_Send_Cmd("AT+CMSTATE","CONNECTED",500))//����Ƿ�Ӧ��ATָ�� 
	{
		if(count < COUNT_AT)
		{
			count += 1;
			delay_ms(2000);			
		}
		else
		{
//ATָ���Ѿ�������COUNT_AT�Σ���Ȼʧ�ܣ��ϵ�SIM800������TIME_AT���ӵĶ�ʱ����ʱʱ�䵽���ٴ����ӷ�����
//Ŀǰ������û�е��ñ�������Ҳû�ж�Flag_TIM6_2_S���ж����룬������ʱ���ε�Flag_TIM6_2_S�ĸ�ֵ
			//Flag_TIM6_2_S = 0xAA;
			return 1;		
		}
	}	

		//ATָ��Ļ��Ĳ���Ҫ��������
	Clear_Usart3();
	return 0;

}
#endif

//����2Gģ��ĵ�ԴоƬ��������ͣ��ť��ʹ��
void SIM800_POWER_ON(void)
{
 u8 i= 0;
	
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;				 
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
 GPIO_Init(GPIOB, &GPIO_InitStructure);	

 GPIO_SetBits(GPIOB,GPIO_Pin_8);	

 for(i = 0; i < 5; i++)
 {
	 delay_ms(1000);	
 }
}

//�ر�2Gģ��ĵ�ԴоƬ��������ͣ��ť��ʹ��
void SIM800_POWER_OFF(void)
{
 u8 i= 0;

 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 ;				 
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
 GPIO_Init(GPIOB, &GPIO_InitStructure);	
	


//��ԴоƬ��ʧ��
 GPIO_ResetBits(GPIOB,GPIO_Pin_8);	

 for(i = 0; i < 5; i++)
 {
	 delay_ms(1000);	
 }

}


//ͨ��2Gģ���PWRKEY��ʵ�ֿ��ػ�
void SIM800_PWRKEY_ON(void)
{
 u8 i= 0;
	
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;				 
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
 GPIO_Init(GPIOB, &GPIO_InitStructure);	

 //PWRKEY��ʹ��
 GPIO_SetBits(GPIOB,GPIO_Pin_9);	

 for(i = 0; i < 5; i++)
 {
	 delay_ms(1000);	
 }
	//�������������ͷ�
 GPIO_ResetBits(GPIOB,GPIO_Pin_9);
 for(i = 0; i < 1; i++)
 {
	 delay_ms(500);	
 }

}

//ͨ��2Gģ���PWRKEY��ʵ�ֿ��ػ�
void SIM800_PWRKEY_OFF(void)
{
 u8 i= 0;
	
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 
	
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;				 
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
 GPIO_Init(GPIOB, &GPIO_InitStructure);	

 //PWRKEY��ʹ��
 GPIO_SetBits(GPIOB,GPIO_Pin_9);	

 for(i = 0; i < 5; i++)
 {
	 delay_ms(1000);	
 }
	//�������������ͷ�
 GPIO_ResetBits(GPIOB,GPIO_Pin_9);
 for(i = 0; i < 1; i++)
 {
	 delay_ms(500);	
 }

}




//����1		ĳ��ATָ��ִ�д���
//����0   �ɹ������Ϸ�����
u8 SIM800_Link_Server(void)
{
//����ATָ�������������
	Check_Module();
	if(Flag_AT_Unusual == 0xAA)
	{
		Flag_AT_Unusual = 0;
		return 1;
	}
	else
	{
		Disable_Echo();
		if(Flag_AT_Unusual == 0xAA)
		{
			Flag_AT_Unusual = 0;
			return 1;
		}
		else
		{
			Check_SIM_Card();
			if(Flag_AT_Unusual == 0xAA)
			{
				Flag_AT_Unusual = 0;
				return 1;
			}
				else
				{
					Check_CSQ();
					if(Flag_AT_Unusual == 0xAA)
					{
						Flag_AT_Unusual = 0;
						return 1;
					}
					else
					{
						Get_ICCID();
						if(Flag_AT_Unusual == 0xAA)
						{
							Flag_AT_Unusual = 0;
							return 1;
						}
//						else	
//						{
//								SIM800_GPRS_OFF();
//								if(Flag_AT_Unusual == 0xAA)
//								{
//									Flag_AT_Unusual = 0;
//									BSP_Printf("1\r\n");
//									return 1;
//								}
								else
								{
									SIM800_GPRS_CIPSHUT();
									if(Flag_AT_Unusual == 0xAA)
									{
										Flag_AT_Unusual = 0;
										BSP_Printf("2\r\n");
										return 1;
									}
									else	
									{
										SIM800_GPRS_CGCLASS();
										if(Flag_AT_Unusual == 0xAA)
										{
											Flag_AT_Unusual = 0;
											BSP_Printf("3\r\n");
											return 1;
										}
										else
										{
											SIM800_GPRS_CGDCONT();
											if(Flag_AT_Unusual == 0xAA)
											{
												Flag_AT_Unusual = 0;
												BSP_Printf("4\r\n");
												return 1;
											}
											else
											{
												SIM800_GPRS_Adhere();
												if(Flag_AT_Unusual == 0xAA)
											  {
													Flag_AT_Unusual = 0;
													BSP_Printf("5\r\n");
													return 1;
											  }
												else
												{
													SIM800_GPRS_Set();
													if(Flag_AT_Unusual == 0xAA)
													{
														Flag_AT_Unusual = 0;
														BSP_Printf("6\r\n");
														return 1;
													}			
//													else
//													{
//														SIM800_GPRS_Dispaly_IP();
//														if(Flag_AT_Unusual == 0xAA)
//														{
//															Flag_AT_Unusual = 0;
//															BSP_Printf("7\r\n");
//															return 1;
//														}	
														else		
														{
															Link_Server_Echo();
															if(Flag_AT_Unusual == 0xAA)
															{
																Flag_AT_Unusual = 0;
																BSP_Printf("8\r\n");
																return 1;
															}
															else
															{
																;
															}
														}															
													//}
												}	
											}
										}	
									}										
								}
							//}
						}												
					}						
				}
		}
 return 0;			
}


	


//���͵�½��Ϣ��������
u8 Send_Login_Data(void)
{
	Get_Login_Data();
	BSP_Printf("Login_Buffer:%s\r\n",Login_Buffer);
	count_Send_Data = TOTAL_SEND_DATA;
	while(count_Send_Data--)
	{
		result_Send_Data = Send_Data_To_Server(Login_Buffer);
		//��ȷ����
		if(result_Send_Data == 0x00)
		{
			break;
		}
		//��ʱ���ط�
		else
		{
			delay_ms(2000);
			delay_ms(2000);
		}
	}
	
  if(count_Send_Data == 0)
	{
		return 1;
	}
	
	result_Send_Data = 0xAA;
	Flag_ACK_Resend = 0x01;
	Flag_ACK_Echo = 0x01;
	//�����ȴ����������ĵĳ�ʱ����
	Flag_Wait_Echo = 0xAA;
	
  return 0;

}
void Send_Login_Data_To_Server(void)
{
	u8 temp = 0;
	while(1)
	{
		if(Send_Login_Data() == 0x01)
		{
			//������ͺ���ִ�����ɴ���Ȼʧ�ܣ�ֻ����ʱ������GPRS����
			BSP_Printf("SIM800C�ر�GPRS_Login\r\n");
			SIM800_GPRS_OFF();
			
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
			BSP_Printf("SIM800C��GPRS_Login\r\n");
			SIM800_GPRS_ON();
		}
		else
		{
			break;
		}
	}
}


//�����ط������������
u8 Send_Resend_Data(void)
{
	Get_Resend_Data();
	count_Send_Data = TOTAL_SEND_DATA;
	while(count_Send_Data--)
	{
		result_Send_Data = Send_Data_To_Server(Resend_Buffer);
		//��ȷ����
		if(result_Send_Data == 0x00)
		{
			break;
		}
		//��ʱ���ط�
		else
		{
			delay_ms(2000);
			delay_ms(2000);
		}
	}
	
  if(count_Send_Data == 0)
	{
		return 1;
	}
	
	result_Send_Data = 0xAA;
	//Flag_ACK_Resend = 0x03;
  return 0;

}
void Send_Resend_Data_To_Server(void)
{
	u8 temp = 0;
	while(1)
	{
		if(Send_Resend_Data() == 0x01)
		{
			//������ͺ���ִ�����ɴ���Ȼʧ�ܣ�ֻ����ʱ������GPRS����
			SIM800_GPRS_OFF();
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
			SIM800_GPRS_ON();
		}
		else
		{
			break;
		}
	}
}

void Get_Enable_Data(void)
{
	char temp_Enable_Buffer[LENGTH_ENABLE] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP03,019,";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	char temp05[] = "0000,";
	char temp06[] = "11223344,";
	

	//����Resend_Buffer
	for(i = 0; i < LENGTH_ENABLE; i++)
	{
		Enbale_Buffer[i] = 0;
	}
	
	//���TRVAP01,FF,000,
	
	for(i = 0; i < strlen(temp00); i++)
	{
		temp_Enable_Buffer[i] = temp00[i];
	}
	strcat(Enbale_Buffer,temp_Enable_Buffer);
	Clear_buffer(temp_Enable_Buffer,LENGTH_ENABLE);
	
		//����豸״̬����ʱ�̶�Ϊ0000,
	//temp05
	for(i = 0; i < strlen(temp05); i++)
	{
		temp_Enable_Buffer[i] = temp05[i];
	}
	
	strcat(Enbale_Buffer,temp_Enable_Buffer);
	Clear_buffer(temp_Enable_Buffer,LENGTH_ENABLE);
	
	//����豸����ʱ�䣬��ʱ�̶�Ϊ1234,
	//temp06
	for(i = 0; i < strlen(temp06); i++)
	{
		temp_Enable_Buffer[i] = temp06[i];
	}
	
	strcat(Enbale_Buffer,temp_Enable_Buffer);
	Clear_buffer(temp_Enable_Buffer,LENGTH_ENABLE);
	
	//���У���
	Result_Validation = Check_Xor_Sum(Enbale_Buffer, strlen(Enbale_Buffer));
	
	//У��ֵת��Ϊ�ַ���
  sprintf(temp_Result,"%d",Result_Validation);
	
	//���У��ֵ
	//�ж�У��ֵ�Ƿ��������ַ�
	//У��ֵ�������ַ���ֱ��ƴ��

	if(strlen(temp_Result) == 1)
	{
		//���У��ֵ
		for(i = 0; i < 2; i++)
		{
			temp_Enable_Buffer[i] = temp04[i];
		}
		temp_Enable_Buffer[2] = temp_Result[0];
		
		
		strcat(Enbale_Buffer,temp_Enable_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Enable_Buffer,LENGTH_ENABLE);
	}
	if(strlen(temp_Result) == 2)
	{
		//���У��ֵ
		temp_Enable_Buffer[0] = temp03[0];
		temp_Enable_Buffer[1] = temp_Result[0];
		temp_Enable_Buffer[2] = temp_Result[1];
		
		strcat(Enbale_Buffer,temp_Enable_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Enable_Buffer,LENGTH_ENABLE);
	}
	if(strlen(temp_Result) == 3)
	{
			//���У��ֵ
		for(i = 0; i < strlen(temp_Result); i++)
		{
			temp_Enable_Buffer[i] = temp_Result[i];
		}
		strcat(Enbale_Buffer,temp_Enable_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Enable_Buffer,LENGTH_ENABLE);
	}	
	
	//���,�ָ���
		for(i = 0; i < strlen(temp01); i++)
		{
			temp_Enable_Buffer[i] = temp01[i];
		}
		strcat(Enbale_Buffer,temp_Enable_Buffer);
		Clear_buffer(temp_Enable_Buffer,LENGTH_ENABLE);
	
	
	//��ӽ�����  #
	for(i = 0; i < strlen(temp02); i++)
	{
		temp_Enable_Buffer[i] = temp02[i];
	}
		strcat(Enbale_Buffer,temp_Enable_Buffer);
		Clear_buffer(temp_Enable_Buffer,LENGTH_ENABLE);


}

//���ͽ���ҵ��ָ����ɻ��ĸ�������
u8 Send_Enable_Data(void)
{
	Get_Enable_Data();
	count_Send_Data = TOTAL_SEND_DATA;
	while(count_Send_Data--)
	{
		result_Send_Data = Send_Data_To_Server(Heart_Buffer);
		//��ȷ����
		if(result_Send_Data == 0x00)
		{
			break;
		}
		//��ʱ���ط�
		else
		{
			delay_ms(2000);
			delay_ms(2000);
		}
	}
	
  if(count_Send_Data == 0)
	{
		return 1;
	}
	
	result_Send_Data = 0xAA;
	Flag_ACK_Resend = 0x02;
	Flag_ACK_Echo = 0x02;
	//�����ȴ����������ĵĳ�ʱ����
	Flag_Wait_Echo = 0xAA;
	
  return 0;


}

//������������������
u8 Send_Heart_Data(void)
{
	Get_Heart_Data();
	count_Send_Data = TOTAL_SEND_DATA;
	while(count_Send_Data--)
	{
		result_Send_Data = Send_Data_To_Server(Heart_Buffer);
		//��ȷ����
		if(result_Send_Data == 0x00)
		{
			break;
		}
		//��ʱ���ط�
		else
		{
			delay_ms(2000);
			delay_ms(2000);
		}
	}
	
  if(count_Send_Data == 0)
	{
		BSP_Printf("Send_Heart_Data����ִ�г���������ʧ�ܷ���\r\n");
		return 1;
	}
	
	result_Send_Data = 0xAA;
	Flag_ACK_Resend = 0x02;
	Flag_ACK_Echo = 0x02;
	//�����ȴ����������ĵĳ�ʱ����
	Flag_Wait_Echo = 0xAA;
	
  return 0;

}
void Send_Heart_Data_To_Server(void)
{
	u8 temp = 0;
	while(1)
	{
		if(Send_Heart_Data() == 0x01)
		{
			//������ͺ���ִ�����ɴ���Ȼʧ�ܣ�ֻ����ʱ������GPRS����
			BSP_Printf("SIM800C�ر�GPRS in Send_Heart_Data_To_Server\r\n");
			SIM800_GPRS_OFF();
			
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
			BSP_Printf("SIM800C��GPRS in Send_Heart_Data_To_Server\r\n");
			SIM800_GPRS_ON();
			
		}
		else
		{
			break;
		}
	}
}

//���ͽ���ҵ��ָ����������������
void Send_Enable_Data_To_Server(void)
{
	u8 temp = 0;
	while(1)
	{
		if(Send_Enable_Data() == 0x01)
		{
			//������ͺ���ִ�����ɴ���Ȼʧ�ܣ�ֻ����ʱ������GPRS����
			SIM800_GPRS_OFF();
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
			SIM800_GPRS_ON();
		}
		else
		{
			break;
		}
	}

}

void Enable_Device(u8 mode)
{
	;

}


//����ҵ��ִ�����ָ���������
u8 Send_Device_OK_Data(void)
{
	Get_Device_OK_Data();
	count_Send_Data = TOTAL_SEND_DATA;
	while(count_Send_Data--)
	{
		result_Send_Data = Send_Data_To_Server(Heart_Buffer);
		//��ȷ����
		if(result_Send_Data == 0x00)
		{
			break;
		}
		//��ʱ���ط�
		else
		{
			delay_ms(2000);
			delay_ms(2000);
		}
	}
	
  if(count_Send_Data == 0)
	{
		return 1;
	}
	
	result_Send_Data = 0xAA;
	Flag_ACK_Resend = 0x02;
	Flag_ACK_Echo = 0x02;
	//�����ȴ����������ĵĳ�ʱ����
	Flag_Wait_Echo = 0xAA;
	
  return 0;

}

//�����豸���н��������������
void Send_Device_OK_To_Server(void)
{
	u8 temp = 0;
	while(1)
	{
		if(Send_Device_OK_Data() == 0x01)
		{
			//������ͺ���ִ�����ɴ���Ȼʧ�ܣ�ֻ����ʱ������GPRS����
			SIM800_GPRS_OFF();
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
			SIM800_GPRS_ON();
		}
		else
		{
			break;
		}
	}

}



void Get_Device_OK_Data(void)
{
	char temp_Device_OK_Buffer[LENGTH_DEVICE_OK] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP05,019,";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	char temp05[] = "0000,";
	char temp06[] = "11223344,";
	

	//����Resend_Buffer
	for(i = 0; i < LENGTH_DEVICE_OK; i++)
	{
		Device_OK_Buffer[i] = 0;
	}
	
	//���TRVAP05,FF,
	
	for(i = 0; i < strlen(temp00); i++)
	{
		temp_Device_OK_Buffer[i] = temp00[i];
	}
	strcat(Device_OK_Buffer,temp_Device_OK_Buffer);
	Clear_buffer(temp_Device_OK_Buffer,LENGTH_DEVICE_OK);
	
		//����豸״̬����ʱ�̶�Ϊ0000,
	//temp05
	for(i = 0; i < strlen(temp05); i++)
	{
		temp_Device_OK_Buffer[i] = temp05[i];
	}
	
	strcat(Device_OK_Buffer,temp_Device_OK_Buffer);
	Clear_buffer(temp_Device_OK_Buffer,LENGTH_DEVICE_OK);
	
	//����豸����ʱ�䣬��ʱ�̶�Ϊ1234,
	//temp06
	for(i = 0; i < strlen(temp06); i++)
	{
		temp_Device_OK_Buffer[i] = temp06[i];
	}
	
	strcat(Device_OK_Buffer,temp_Device_OK_Buffer);
	Clear_buffer(temp_Device_OK_Buffer,LENGTH_DEVICE_OK);
	
	//���У���
	Result_Validation = Check_Xor_Sum(Device_OK_Buffer, strlen(Device_OK_Buffer));
	
	//У��ֵת��Ϊ�ַ���
  sprintf(temp_Result,"%d",Result_Validation);
	
	//���У��ֵ
	//�ж�У��ֵ�Ƿ��������ַ�
	//У��ֵ�������ַ���ֱ��ƴ��

	if(strlen(temp_Result) == 1)
	{
		//���У��ֵ
		for(i = 0; i < 2; i++)
		{
			temp_Device_OK_Buffer[i] = temp04[i];
		}
		temp_Device_OK_Buffer[2] = temp_Result[0];
		
		
		strcat(Device_OK_Buffer,temp_Device_OK_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Device_OK_Buffer,LENGTH_DEVICE_OK);
	}
	if(strlen(temp_Result) == 2)
	{
		//���У��ֵ
		temp_Device_OK_Buffer[0] = temp03[0];
		temp_Device_OK_Buffer[1] = temp_Result[0];
		temp_Device_OK_Buffer[2] = temp_Result[1];
		
		strcat(Device_OK_Buffer,temp_Device_OK_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Device_OK_Buffer,LENGTH_DEVICE_OK);
	}
	if(strlen(temp_Result) == 3)
	{
			//���У��ֵ
		for(i = 0; i < strlen(temp_Result); i++)
		{
			temp_Device_OK_Buffer[i] = temp_Result[i];
		}
		strcat(Device_OK_Buffer,temp_Device_OK_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Device_OK_Buffer,LENGTH_DEVICE_OK);
	}	
	
	//���,�ָ���
	for(i = 0; i < strlen(temp01); i++)
	{
		temp_Device_OK_Buffer[i] = temp01[i];
	}
		strcat(Device_OK_Buffer,temp_Device_OK_Buffer);
	  Clear_buffer(temp_Device_OK_Buffer,LENGTH_DEVICE_OK);
	
	
	//��ӽ�����  #
	for(i = 0; i < strlen(temp02); i++)
	{
		temp_Device_OK_Buffer[i] = temp02[i];
	}
		strcat(Device_OK_Buffer,temp_Device_OK_Buffer);
	  Clear_buffer(temp_Device_OK_Buffer,LENGTH_DEVICE_OK);


}
void Get_Resend_Data(void)
{
	char temp_Resend_Buffer[LENGTH_RESEND] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP98,009,000,";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	

	//����Resend_Buffer
	for(i = 0; i < LENGTH_RESEND; i++)
	{
		Resend_Buffer[i] = 0;
	}
	
	//���TRVAP98,FF,000,
	
	for(i = 0; i < strlen(temp00); i++)
	{
		temp_Resend_Buffer[i] = temp00[i];
	}
	strcat(Resend_Buffer,temp_Resend_Buffer);
	Clear_buffer(temp_Resend_Buffer,LENGTH_RESEND);
	
	//���У���
	Result_Validation = Check_Xor_Sum(Resend_Buffer, strlen(Resend_Buffer));
	
	//У��ֵת��Ϊ�ַ���
  sprintf(temp_Result,"%d",Result_Validation);
	
	//���У��ֵ
	//�ж�У��ֵ�Ƿ��������ַ�
	//У��ֵ�������ַ���ֱ��ƴ��

	if(strlen(temp_Result) == 1)
	{
		//���У��ֵ
		for(i = 0; i < 2; i++)
		{
			temp_Resend_Buffer[i] = temp04[i];
		}
		temp_Resend_Buffer[2] = temp_Result[0];
		
		strcat(Resend_Buffer,temp_Resend_Buffer);
		Clear_buffer(temp_Result,3);
		Clear_buffer(temp_Resend_Buffer,LENGTH_RESEND);
	}
	if(strlen(temp_Result) == 2)
	{
		//���У��ֵ
		temp_Resend_Buffer[0] = temp03[0];
		temp_Resend_Buffer[1] = temp_Result[0];
		temp_Resend_Buffer[2] = temp_Result[1];
		
		strcat(Resend_Buffer,temp_Resend_Buffer);
		Clear_buffer(temp_Result,3);
		Clear_buffer(temp_Resend_Buffer,LENGTH_RESEND);
	}
	if(strlen(temp_Result) == 3)
	{
			//���У��ֵ
		for(i = 0; i < strlen(temp_Result); i++)
		{
			temp_Resend_Buffer[i] = temp_Result[i];
		}
		strcat(Resend_Buffer,temp_Resend_Buffer);
		Clear_buffer(temp_Result,3);
		Clear_buffer(temp_Resend_Buffer,LENGTH_RESEND);
	}	
	
	//���,�ָ���
	for(i = 0; i < strlen(temp01); i++)
	{
		temp_Resend_Buffer[i] = temp01[i];
	}
	strcat(Resend_Buffer,temp_Resend_Buffer);
	Clear_buffer(temp_Resend_Buffer,LENGTH_RESEND);
	
	
	//��ӽ�����  #
	for(i = 0; i < strlen(temp02); i++)
	{
		temp_Resend_Buffer[i] = temp02[i];
	}
	strcat(Resend_Buffer,temp_Resend_Buffer);
	Clear_buffer(temp_Resend_Buffer,LENGTH_RESEND);


}

void Get_Heart_Data(void)
{
	char temp_Heart_Buffer[LENGTH_HEART] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP01,023,000,";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	char temp05[] = "0000,";
	char temp06[] = "11223344,";
	

	//����Resend_Buffer
	for(i = 0; i < LENGTH_HEART; i++)
	{
		Heart_Buffer[i] = 0;
	}
	
	//���TRVAP01,FF,000,
	
	for(i = 0; i < strlen(temp00); i++)
	{
		temp_Heart_Buffer[i] = temp00[i];
	}
	strcat(Heart_Buffer,temp_Heart_Buffer);
	Clear_buffer(temp_Heart_Buffer,LENGTH_HEART);
	
		//����豸״̬����ʱ�̶�Ϊ0000,
	//temp05
	for(i = 0; i < strlen(temp05); i++)
	{
		temp_Heart_Buffer[i] = temp05[i];
	}
	
	strcat(Heart_Buffer,temp_Heart_Buffer);
	Clear_buffer(temp_Heart_Buffer,LENGTH_HEART);
	
	//����豸����ʱ�䣬��ʱ�̶�Ϊ1234,
	//temp06
	for(i = 0; i < strlen(temp06); i++)
	{
		temp_Heart_Buffer[i] = temp06[i];
	}
	
	strcat(Heart_Buffer,temp_Heart_Buffer);
	Clear_buffer(temp_Heart_Buffer,LENGTH_HEART);
	
	//���У���
	Result_Validation = Check_Xor_Sum(Heart_Buffer, strlen(Heart_Buffer));
	
	//У��ֵת��Ϊ�ַ���
  sprintf(temp_Result,"%d",Result_Validation);
	
	//���У��ֵ
	//�ж�У��ֵ�Ƿ��������ַ�
	//У��ֵ�������ַ���ֱ��ƴ��

	if(strlen(temp_Result) == 1)
	{
		//���У��ֵ
		for(i = 0; i < 2; i++)
		{
			temp_Heart_Buffer[i] = temp04[i];
		}
		temp_Heart_Buffer[2] = temp_Result[0];
		
		
		strcat(Heart_Buffer,temp_Heart_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Heart_Buffer,LENGTH_HEART);
	}
	if(strlen(temp_Result) == 2)
	{
		//���У��ֵ
		temp_Heart_Buffer[0] = temp03[0];
		temp_Heart_Buffer[1] = temp_Result[0];
		temp_Heart_Buffer[2] = temp_Result[1];
		
		strcat(Heart_Buffer,temp_Heart_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Heart_Buffer,LENGTH_HEART);
	}
	if(strlen(temp_Result) == 3)
	{
			//���У��ֵ
		for(i = 0; i < strlen(temp_Result); i++)
		{
			temp_Heart_Buffer[i] = temp_Result[i];
		}
		strcat(Heart_Buffer,temp_Heart_Buffer);
		Clear_buffer(temp_Result,3);
	  Clear_buffer(temp_Heart_Buffer,LENGTH_HEART);
	}	
	
	//���,�ָ���
	for(i = 0; i < strlen(temp01); i++)
	{
		temp_Heart_Buffer[i] = temp01[i];
	}
	strcat(Heart_Buffer,temp_Heart_Buffer);
	Clear_buffer(temp_Heart_Buffer,LENGTH_HEART);
	
	
	//��ӽ�����  #
	for(i = 0; i < strlen(temp02); i++)
	{
		temp_Heart_Buffer[i] = temp02[i];
	}
	strcat(Heart_Buffer,temp_Heart_Buffer);
	Clear_buffer(temp_Heart_Buffer,LENGTH_HEART);


}



void Get_Login_Data(void)
{
	char temp_Login_Buffer[LENGTH_LOGIN] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP00,051,000,SIM800_";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	char temp05[] = "0000,";       //�豸����״̬
	char temp06[] = "11223344,";   //�豸����ʱ��

	//����Resend_Buffer
	for(i = 0; i < LENGTH_LOGIN; i++)
	{
		Login_Buffer[i] = 0;
	}
	
	//���TRVAP00,FF,000,SIM800_
	
	for(i = 0; i < strlen(temp00); i++)
	{
		temp_Login_Buffer[i] = temp00[i];
	}
	strcat(Login_Buffer,temp_Login_Buffer);
	Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	
	//���SIM����ICCID
	for(i = 0; i < LENGTH_ICCID_BUF; i++)
	{
		temp_Login_Buffer[i] = ICCID_BUF[i];
	}
	
	strcat(Login_Buffer,temp_Login_Buffer);
	Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	
		
	//���,�ָ���
	for(i = 0; i < strlen(temp01); i++)
	{
		temp_Login_Buffer[i] = temp01[i];
	}
	strcat(Login_Buffer,temp_Login_Buffer);
	Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	
	
	//����豸״̬����ʱ�̶�Ϊ0000,
	//temp05
	for(i = 0; i < strlen(temp05); i++)
	{
		temp_Login_Buffer[i] = temp05[i];
	}
	
	strcat(Login_Buffer,temp_Login_Buffer);
	Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	
	//����豸����ʱ�䣬��ʱ�̶�Ϊ11223344,
	//temp06
	for(i = 0; i < strlen(temp06); i++)
	{
		temp_Login_Buffer[i] = temp06[i];
	}
	
	strcat(Login_Buffer,temp_Login_Buffer);
	Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	
	//���У���
	Result_Validation = Check_Xor_Sum(Login_Buffer, strlen(Login_Buffer));
	
	//У��ֵת��Ϊ�ַ���
  sprintf(temp_Result,"%d",Result_Validation);
	
	//���У��ֵ
	//�ж�У��ֵ�Ƿ��������ַ�
	//У��ֵ�������ַ���ֱ��ƴ��

	if(strlen(temp_Result) == 1)
	{
		//���У��ֵ
		for(i = 0; i < 2; i++)
		{
			temp_Login_Buffer[i] = temp04[i];
		}
		temp_Login_Buffer[2] = temp_Result[0];
		
		strcat(Login_Buffer,temp_Login_Buffer);
		Clear_buffer(temp_Result,3);
		Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	}
	if(strlen(temp_Result) == 2)
	{
		//���У��ֵ
		temp_Login_Buffer[0] = temp03[0];
		temp_Login_Buffer[1] = temp_Result[0];
		temp_Login_Buffer[2] = temp_Result[1];
		
		strcat(Login_Buffer,temp_Login_Buffer);
		Clear_buffer(temp_Result,3);
		Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	}
	if(strlen(temp_Result) == 3)
	{
			//���У��ֵ
		for(i = 0; i < strlen(temp_Result); i++)
		{
			temp_Login_Buffer[i] = temp_Result[i];
		}
		strcat(Login_Buffer,temp_Login_Buffer);
		Clear_buffer(temp_Result,3);
		Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	}	
	
	//���,�ָ���
	for(i = 0; i < strlen(temp01); i++)
	{
		temp_Login_Buffer[i] = temp01[i];
	}
	strcat(Login_Buffer,temp_Login_Buffer);
	Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	
	
	//��ӽ�����  #
	for(i = 0; i < strlen(temp02); i++)
	{
		temp_Login_Buffer[i] = temp02[i];
	}
	strcat(Login_Buffer,temp_Login_Buffer);
	Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);


}






void Clear_buffer(char* buffer,u16 length)
{
	u16 i = 0;
	for(i = 0; i < length;i++)
	{
		buffer[i] = 0;
	}
}

//////////////���У��ͺ���///////
u8 Check_Xor_Sum(char* pBuf, u16 len)
{
  u8 Sum = 0;
  u8 i = 0;
	Sum = pBuf[0];
	
  for (i = 1; i < len; i++ )
  {
    Sum = (Sum ^ pBuf[i]);
  }
	
  return Sum;
}
