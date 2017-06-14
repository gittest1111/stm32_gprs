#include "SIM800.h"	 	 	
#include "string.h"  
#include "stdio.h"	
#include "usart.h" 
#include "usart3.h" 
#include "timer.h"
#include "delay.h"
#include <stdlib.h>
#include "device.h"

extern u8 result_Send_Data;
extern u8 count_Send_Data;
extern u8 current_cmd;
#define TOTAL_SEND_DATA 3
#define COUNT_AT 10

u8 mode = 0;				//0,TCP����;1,UDP����
const char *modetbl[2] = {"TCP","UDP"};//����ģʽ

//const char  *ipaddr = "tuzihog.oicp.net";
//const char  *port = "28106";

//const char  *ipaddr = "42.159.117.91";
const char  *ipaddr = "116.62.187.167";
//const char  *ipaddr = "42.159.107.250";
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

char atcmd_ack[LENGTH_ATCMD_ACK] = {0};
bool need_ack_check = FALSE;
bool ack_ok = FALSE;

u8  Flag_SIM800C_In_Reset = 0; 

//SIM800���������,�����յ���Ӧ��
//str:�ڴ���Ӧ����
//����ֵ:0,û�еõ��ڴ���Ӧ����
//����,�ڴ�Ӧ������λ��(str��λ��)
u8 SIM800_Check_Cmd(u8 *str)
{
	u8 ret = CMD_ACK_NONE;
	if(USART3_RX_STA&0X8000)		//���յ�һ��������
	{ 
		ret = CMD_ACK_NOK;
		USART3_RX_BUF[USART3_RX_STA&0X7FFF] = 0;//��ӽ�����
		BSP_Printf("recv: %s\r\n", USART3_RX_BUF);
		if(strstr((const char*)USART3_RX_BUF,(const char*)str)!=NULL)   //�������Ƚ����Ƿ�����ȷ�����ж�
			ret = CMD_ACK_OK;
		else if((strstr((const char*)USART3_RX_BUF,"CLOSED")!=NULL) || (strstr((const char*)USART3_RX_BUF,"+PDP: DEACT")!=NULL))
			ret = CMD_ACK_DISCONN;
	} 
	return ret;
}

//SIM800��������
//cmd:���͵������ַ���(����Ҫ��ӻس���),��cmd<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//ack:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//waittime:�ȴ�ʱ��(��λ:10ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       1,����ʧ��
u8 SIM800_Send_Cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res = CMD_ACK_NONE; 
	Clear_Usart3();
	
	if(ack!=NULL)
	{
		ack_ok = FALSE;
		memset(atcmd_ack, 0, sizeof(atcmd_ack));
		strcpy(atcmd_ack, (char *)ack);
	}	
	
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
		while(waittime!=0)	//�ȴ�����ʱ
		{ 
			delay_ms(10);				
			if(USART3_RX_STA&0X8000)//���յ��ڴ���Ӧ����
			{
				//�õ�����Ч�Ļ�����Ϣ��
				res = SIM800_Check_Cmd(ack);
				if((res == CMD_ACK_OK) || (res == CMD_ACK_DISCONN))   //�յ������Ļ��ģ����߶��߶�����Ҫ��������
				{
					//����ĺ�������Ҫ����õ��Ļ��ģ��������ﲻ�ܵ��ú���Clear_Usart3
					break;
				} 
				//û�еõ���Ч�Ļ�����Ϣ�����㴮��3�������´εĽ���
				else  //res == CMD_ACK_NOK
				{
					if(need_ack_check && ack_ok)
						break;
					//�����п�����շ���������?	
					Clear_Usart3();
				}
			} 
			waittime--;
			//�Ӹ����Ƕȱ���ȴ�������յ��� ack ��ֱ������
			//��Ҫ������ack = "SEND OK"
			if(need_ack_check && ack_ok)
				break;			
		}

		//�����TIM6�﷢����at����
		if(need_ack_check && ack_ok)
			res = CMD_ACK_OK;
		
		if(waittime == 0)
		{
			//ATָ���Ѿ�������ֻ���ڵȴ�ʱ���ڣ�USART3_RX_STAû����λ���ߵõ������ݲ�����ack
			//���ص���CMD_ACK_NONE ����CMD_ACK_NOK
			Clear_Usart3();
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
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT","OK",100);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))   //ֻ����������������ظ�������
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))  //������AT ����������ϲ����ܷ���"CLOSED" �� ������������
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
	
}

//�رջ���
u8 Disable_Echo(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("ATE0","OK",200);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
	
}

//�鿴SIM�Ƿ���ȷ��⵽
u8 Check_SIM_Card(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CPIN?","OK",200);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}

u8 Check_OPS(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+COPS?","CHINA",500);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}


//�鿴��������
u8 Check_CSQ(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	u8 *p1 = NULL; 
	u8 *p2 = NULL;
	u8 p[50] = {0}; 
  	u8 signal=0;

	while(signal < 10)
	{
		delay_ms(2000);
		while(count != 0)
		{
			ret = SIM800_Send_Cmd("AT+CSQ","+CSQ:",200);
			if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
			{
				delay_ms(2000);
			}
			else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
				break;
			
			count--;
		}		
		
		if(ret == CMD_ACK_OK)
		{
			//ATָ���Ѿ�ָ����ɣ�����Է���ֵ���д���
			p1=(u8*)strstr((const char*)(USART3_RX_BUF),":");
			p2=(u8*)strstr((const char*)(p1),",");
			p2[0]=0;//���������
			signal = atoi((const char *)(p1+2));
			//sprintf((char*)p,"�ź�����:%s",p1+2);
			sprintf((char*)p,"�ź�����:%d",signal);
			BSP_Printf("%s\r\n",p);
		}
		//ATָ��Ļ����Ѿ�������ɣ�����
		Clear_Usart3();
	}	
	return ret;
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
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CCID","OK",200);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}

	if(ret == CMD_ACK_OK)	
	{
		//ATָ���Ѿ�ָ����ɣ�����Է���ֵ���д���
		//BSP_Printf("CCID:%s\r\n",USART3_RX_BUF);

		p_temp = USART3_RX_BUF;
		//��ȡICCID��Ϣ��ȫ�ֱ���ICCID_BUF
		for(index = 0;index < LENGTH_ICCID_BUF;index++)
		{
			ICCID_BUF[index] = *(p_temp+OFFSET_ICCID+index);
		}
		//BSP_Printf("ICCID_BUF:%s\r\n",ICCID_BUF);
	}
	//ATָ��Ļ����Ѿ�������ɣ�����
	Clear_Usart3();
	return ret;

}

u8 SIM800_GPRS_ON(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPSTART=\"TCP\",\"116.62.187.167\",\"8090\"","CONNECT OK",700);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;

}

//�ر�GPRS������
u8 SIM800_GPRS_OFF(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPCLOSE=1","CLOSE OK",500);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}

//����GPRS
u8 SIM800_GPRS_Adhere(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CGATT=1","OK",300);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}

//����ΪGPRS����ģʽ
u8 SIM800_GPRS_Set(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPCSGP=1,\"CMNET\"","OK",300);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}

//���ý���������ʾIPͷ(�����ж�������Դ)	
u8 SIM800_GPRS_Dispaly_IP(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPHEAD=1","OK",300);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}

//�ر��ƶ����� 
u8 SIM800_GPRS_CIPSHUT(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CIPSHUT","SHUT OK",500);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}

//����GPRS�ƶ�̨���ΪB,֧�ְ����������ݽ��� 
u8 SIM800_GPRS_CGCLASS(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CGCLASS=\"B\"","OK",300);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}


	//����PDP������,��������Э��,��������Ϣ
u8 SIM800_GPRS_CGDCONT(void)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Send_Cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",300);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
	
	Clear_Usart3();	
	return ret;
}

u8 Link_Server_AT(u8 mode,const char* ipaddr,const char *port)
{
	u8 count = COUNT_AT;
	u8 ret = CMD_ACK_NONE;
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


	while(count != 0)
	{
		ret = SIM800_Send_Cmd(p,"CONNECT",1000);
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			delay_ms(2000);
		}
		else if((ret == CMD_ACK_OK) || (ret == CMD_ACK_DISCONN))
			break;
		
		count--;
	}
		
	//ATָ���Ѿ�ָ����ɣ�����Է���ֵ���д���
	if(ret == CMD_ACK_OK)
	{
		for(index = 0; index < 256; index++)
		{
			p[index]= USART3_RX_BUF[index];
		}
		temp1 = strstr((const char*)p,"CONNECT OK");
		temp2 = strstr((const char*)p,"ALREADY CONNECT");
		temp3 = strstr((const char*)p,"CONNECT FAIL");
					
		if(temp3 != NULL)
		{
			ret = CMD_ACK_NOK;
		}
	}
	Clear_Usart3();
	return ret;
}

u8 Send_Data_To_Server(char* data)
{
	u8 ret = CMD_ACK_NONE;
	BSP_Printf("׼����ʼ��������\r\n");
	if((Flag_Received_SIM800C_CLOSED == 0xAA) || (Flag_Received_SIM800C_DEACT == 0xAA))
		ret = CMD_ACK_DISCONN;
	else
		ret = SIM800_Send_Cmd("AT+CIPSEND",">",500);
	
	if(ret==CMD_ACK_OK)		//��������
	{ 
		Clear_Usart3();   //��������൱��һ��������send
		u3_printf("%s",data);
		delay_ms(100);
		need_ack_check = TRUE;
		ret=SIM800_Send_Cmd((u8*)0x1A,"SEND OK",3000);
		if(ret==CMD_ACK_OK)
		{
			//���ڵȴ�ʱ��̫�����յ�Send OK��ͬʱ�����յ����������صĻ���
			//��˲������������
			if(strstr((const char*)USART3_RX_BUF,"TRVBP")==NULL)
				Clear_Usart3();	
		}
		need_ack_check = FALSE;		
	  //delay_ms(1000); 
	}
	BSP_Printf("�����һ�η���: %d\r\n", ret);
	return ret;
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
 for(i = 0; i < 10; i++)
 {
	 delay_ms(1000);	
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
 for(i = 0; i < 10; i++)
 {
	 delay_ms(1000);	
 }

}

void SIM800_GPRS_Restart(void)
{
	u8 temp = 0;
	SIM800_GPRS_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	SIM800_GPRS_ON();

}

void SIM800_Powerkey_Restart(void)
{
	u8 temp = 0;
	Flag_SIM800C_In_Reset = 0xAA;
	BSP_Printf("Powerkey Restart\r\n");
	SIM800_PWRKEY_OFF();
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	SIM800_PWRKEY_ON();
	Flag_SIM800C_In_Reset = 0;
}

void SIM800_Power_Restart(void)
{
	u8 temp = 0;
	SIM800_PWRKEY_OFF();
	SIM800_POWER_OFF();
	
	for(temp = 0; temp < 30; temp++)
	{
		delay_ms(1000);
	}
	SIM800_POWER_ON();
	SIM800_PWRKEY_ON();

}

//����1   ĳ��ATָ��ִ�д���
//����0   �ɹ������Ϸ�����
u8 SIM800_Link_Server_AT(void)
{
	u8 ret = CMD_ACK_NONE;
	//����ATָ�������������
	if((ret = Check_Module()) == CMD_ACK_OK)
		if((ret = Disable_Echo()) == CMD_ACK_OK)
			if((ret = Check_SIM_Card()) == CMD_ACK_OK)
				//if((ret = Check_CSQ()) == CMD_ACK_OK)
					if((ret = Get_ICCID()) == CMD_ACK_OK)
						//if((ret = Check_OPS()) == CMD_ACK_OK)
							//if((ret = SIM800_GPRS_OFF()) == CMD_ACK_OK)
								if((ret = SIM800_GPRS_CIPSHUT()) == CMD_ACK_OK)
									if((ret = SIM800_GPRS_CGCLASS()) == CMD_ACK_OK)
										if((ret = SIM800_GPRS_CGDCONT()) == CMD_ACK_OK)
											//if((ret = SIM800_GPRS_Adhere()) == CMD_ACK_OK)
												if((ret = SIM800_GPRS_Set()) == CMD_ACK_OK)
													//if((ret = SIM800_GPRS_Dispaly_IP()) == CMD_ACK_OK)
														if((ret = Link_Server_AT(0, ipaddr, port)) == CMD_ACK_OK)											
																;

	return ret;
}

u8 SIM800_Link_Server_Powerkey(void)
{
	u8 count = 5;
	u8 ret = CMD_ACK_NONE;	
	while(count != 0)
	{
		ret = SIM800_Link_Server_AT();
		if(ret != CMD_ACK_OK)
		{
			SIM800_Powerkey_Restart();
		}
		else
			break;
		count--;
	}

	return ret;

}
u8 SIM800_Link_Server(void)
{
	u8 count = 5;
	u8 ret = CMD_ACK_NONE;
	while(count != 0)
	{
		ret = SIM800_Link_Server_Powerkey();
		if(ret != CMD_ACK_OK)
		{
			SIM800_Power_Restart();
		}
		else
			break;
		count--;
	}
	
	return ret;

}


void Get_Login_Data(void)
{
	char temp_Login_Buffer[LENGTH_LOGIN] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP00,059,000,SIM800_";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	char temp05[] = "0000,";       //�豸����״̬
	//char temp06[] = "0000000000000000,";   //�豸����ʱ��

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
	
	//���ڶ�ȡ����GPIO �ߵͣ�������豸ʵʱ״̬
	for(i = 0; i < strlen(temp05); i++)
	{
		//BSP_Printf("Device[%d]: %d\n", i, Device_Power_Status(i));	
		temp05[i] = (ON==Device_Power_Status(i))?'1':temp05[i];		
	}
	
	//strcat(Login_Buffer,temp_Login_Buffer);
	//Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	strncat(Login_Buffer,temp05,strlen(temp05));
	
	//����豸����ʱ��
	Device_Timer_Status(temp_Login_Buffer);

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


//���͵�½��Ϣ��������
u8 Send_Login_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	Get_Login_Data();
	BSP_Printf("Login_Buffer:%s\r\n",Login_Buffer);
	ret = Send_Data_To_Server(Login_Buffer);
	//���ͳɹ�
	if(ret == CMD_ACK_OK)
	{
		Flag_ACK_Resend = 0x01;
		Flag_ACK_Echo = 0x01;
		//�����ȴ����������ĵĳ�ʱ����
		Flag_Wait_Echo = 0xAA;
		Count_Wait_Echo = 0;
	}

	return ret;

}

u8 Send_Login_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Login_Data();
		//Clear_Usart3();
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count -= 1;
	}
	
	return ret;
	
}

u8 Send_Login_Data_To_Server(void)
{
	u8 count = 5;
	u8 ret = CMD_ACK_NONE;
	current_cmd = CMD_LOGIN;
	Flag_Send_Heart = 0x00;    //�ѵ�ǰ������ʱ��ͣ������ѭ�����յ���¼���ĺ�����������
	while(count != 0)
	{
		ret = Send_Login_Data_Normal();
		if(ret != CMD_ACK_OK)
		{
			SIM800_GPRS_Restart();
		}
		else
			break;
		count--;	
	}

	return ret;

}


void Get_Heart_Data(void)
{
	char temp_Heart_Buffer[LENGTH_HEART] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP01,031,000,";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	char temp05[] = "0000,";
	//char temp06[] = "1122334455667788,";
	

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

	//���ڶ�ȡ����GPIO �ߵͣ�������豸ʵʱ״̬
	for(i = 0; i < strlen(temp05); i++)
	{
		//BSP_Printf("Device[%d]: %d\n", i, Device_Power_Status(i));	
		temp05[i] = (ON==Device_Power_Status(i))?'1':temp05[i];		
	}
	
	//strcat(Login_Buffer,temp_Login_Buffer);
	//Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	strncat(Heart_Buffer,temp05,strlen(temp05));
	
	//����豸����ʱ��
	Device_Timer_Status(temp_Heart_Buffer);
	
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

//������������������
u8 Send_Heart_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	Get_Heart_Data();
	ret = Send_Data_To_Server(Heart_Buffer);
	//���ͳɹ�
	if(ret == CMD_ACK_OK)
	{
		Flag_ACK_Resend = 0x02;
		Flag_ACK_Echo = 0x02;
		//�����ȴ����������ĵĳ�ʱ����
		Flag_Wait_Echo = 0xAA;
		Count_Wait_Echo = 0;
	}

	return ret;

}

u8 Send_Heart_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Heart_Data();
		//Clear_Usart3();
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count -= 1;
	}
	
	return ret;

}

u8 Send_Heart_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	current_cmd = CMD_HB;	
	ret = Send_Heart_Data_Normal();
	return ret;
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



//�յ�����ϢУ�����Ҫ����������·���
u8 Send_Resend_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	Get_Resend_Data();
	ret = Send_Data_To_Server(Resend_Buffer);
	//���ͳɹ�
	if(ret == CMD_ACK_OK)
	{
		Flag_ACK_Resend = 0x03;
		Flag_ACK_Echo = 0x03;
		//�����ȴ����������ĵĳ�ʱ����
		Flag_Wait_Echo = 0xAA;
		Count_Wait_Echo = 0;
	}

	return ret;

}

u8 Send_Resend_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Resend_Data();
		//Clear_Usart3();
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count --;
	}

	return ret;

}

u8 Send_Resend_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	//current_cmd =  ֮ǰ�����̣����Բ���Ҫ�ı�;		
	ret = Send_Resend_Data_Normal();
	return ret;
}



void Get_Enable_Data(void)
{
	char temp_Enable_Buffer[LENGTH_ENABLE] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP03,027,";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	char temp05[] = "0000,";
	//char temp06[] = "1122334455667788,";
	

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
	
	//���ڶ�ȡ����GPIO �ߵͣ�������豸ʵʱ״̬
	for(i = 0; i < strlen(temp05); i++)
	{
		//BSP_Printf("Device[%d]: %d\n", i, Device_Power_Status(i));	
		temp05[i] = (ON==Device_Power_Status(i))?'1':temp05[i];		
	}
	
	//strcat(Login_Buffer,temp_Login_Buffer);
	//Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	strncat(Enbale_Buffer,temp05,strlen(temp05));
	
	//����豸����ʱ��
	Device_Timer_Status(temp_Enable_Buffer);
	
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
	u8 ret = CMD_ACK_NONE;
	Get_Enable_Data();
	ret = Send_Data_To_Server(Enbale_Buffer);
	//���ͳɹ�
	if(ret == CMD_ACK_OK)
	{
		Flag_ACK_Resend = 0x04;
		//Flag_ACK_Echo = 0x04;    //��ʵ��������Ϣ����Ҫ��ʱ�ط�
		//�����ȴ����������ĵĳ�ʱ����
		//Flag_Wait_Echo = 0xAA;
		//Count_Wait_Echo = 0;
	}

	return ret;

}

u8 Send_Enable_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret= Send_Enable_Data();
		//Clear_Usart3();
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count -= 1;
	}
	
	return ret;

}

//���������ʵ��һ���豸���ģ��豸��CMD_NONE ״̬��ʱ��(��ǰû�д����κ���Ϣ)�Ż���������
//��ȻӲ���Ŀ������յ�������ָ��ʱ����Ҫ��ɣ���������������Ϣʲôʱ����    
u8 Send_Enable_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	current_cmd = CMD_ENABLE_DEVICE; 
	ret = Send_Enable_Data_Normal();
	current_cmd = CMD_NONE;    //�������������ˣ����ڴ��κη���		
	return ret;
}

void Get_Device_OK_Data(void)
{
	char temp_Device_OK_Buffer[LENGTH_DEVICE_OK] = {0}; 
	char temp_Result[3] = {0}; 
	u8 Result_Validation = 0;
	u8 i = 0;
	char temp00[] = "TRVAP05,027,";
	char temp01[] = ",";
	char temp02[] = "#";
	char temp03[] = "0";
	char temp04[] = "00";
	char temp05[] = "0000,";
	//char temp06[] = "1122334455667788,";
	

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
	
	//���ڶ�ȡ����GPIO �ߵͣ�������豸ʵʱ״̬
	for(i = 0; i < strlen(temp05); i++)
	{
		//BSP_Printf("Device[%d]: %d\n", i, Device_Power_Status(i));	
		temp05[i] = (ON==Device_Power_Status(i))?'1':temp05[i];		
	}
	
	//strcat(Login_Buffer,temp_Login_Buffer);
	//Clear_buffer(temp_Login_Buffer,LENGTH_LOGIN);
	strncat(Device_OK_Buffer,temp05,strlen(temp05));
	
	//����豸����ʱ��
	Device_Timer_Status(temp_Device_OK_Buffer);
	
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



//����ҵ��ִ�����ָ���������
u8 Send_Device_OK_Data(void)
{
	u8 ret = CMD_ACK_NONE;
	Get_Device_OK_Data();
	ret = Send_Data_To_Server(Device_OK_Buffer);
	//���ͳɹ�
	if(ret == CMD_ACK_OK)
	{
		Flag_ACK_Resend = 0x05;
		Flag_ACK_Echo = 0x05;
		//�����ȴ����������ĵĳ�ʱ����
		Flag_Wait_Echo = 0xAA;
		Count_Wait_Echo = 0;
	}

	return ret;

}

u8 Send_Device_OK_Data_Normal(void)
{
	u8 temp = 0;
	u8 ret = CMD_ACK_NONE;
	u8 count = 5;	//ִ��count�Σ������ɹ��Ļ���������GPRS
	while(count != 0)
	{
		//Clear_Usart3();
		ret = Send_Device_OK_Data();
		//Clear_Usart3();
		if((ret == CMD_ACK_NONE) || (ret == CMD_ACK_NOK))
		{
			//��������ʧ��
			for(temp = 0; temp < 30; temp++)
			{
				delay_ms(1000);
			}
		}
		else if((ret == CMD_ACK_OK) ||(ret == CMD_ACK_DISCONN))
			break;
		count --;		
	}

	return ret;

}

u8 Send_Device_OK_Data_To_Server(void)
{
	u8 ret = CMD_ACK_NONE;
	current_cmd = CMD_CLOSE_DEVICE;
	ret = Send_Device_OK_Data_Normal();
	return ret;
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
