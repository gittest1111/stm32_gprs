#ifndef __SIM800C_H__
#define __SIM800C_H__	 
#include "sys.h"


//存储PCB_ID的数组（也就是SIM卡的ICCID）
#define LENGTH_ICCID_BUF 20     								//ICCID的长度是20个字符
extern char ICCID_BUF[LENGTH_ICCID_BUF];
#define OFFSET_ICCID 2                          //

//存储设备重发命令的数组
#define LENGTH_RESEND 35
extern char Resend_Buffer[LENGTH_RESEND];

//存储设备登陆命令的数组
#define LENGTH_LOGIN 80
extern char Login_Buffer[LENGTH_LOGIN];

//存储心跳包的数组
#define LENGTH_HEART 50
extern char Heart_Buffer[LENGTH_HEART];

//存储业务指令回文的数组
#define LENGTH_ENABLE 50
extern char Enbale_Buffer[LENGTH_ENABLE];

//存储业务执行完成指令的数组
#define LENGTH_DEVICE_OK 50
extern char Device_OK_Buffer[LENGTH_DEVICE_OK];

#define LENGTH_ATCMD_ACK 50
#define LENGTH_DEVICE_OPEN_CMD        50
#define LENGTH_USART_DATA        100

/*********WJ*********/
u8 	SIM800_Send_Cmd(u8 *cmd,u8 *ack,u16 waittime);
void Clear_buffer(char* buffer,u16 length);
u8 Check_Xor_Sum(char* pBuf, u16 len);

u8 	Check_Module(void);
u8 	Disable_Echo(void);
u8 	Check_SIM_Card(void);
u8 	Check_CSQ(void);
u8 	Get_ICCID(void);


u8 	SIM800_GPRS_Adhere(void);
u8	SIM800_GPRS_Set(void);
u8 	SIM800_GPRS_Dispaly_IP(void);
u8 	SIM800_GPRS_CIPSHUT(void);
u8 	SIM800_GPRS_CGCLASS(void);
u8 	SIM800_GPRS_CGDCONT(void);

u8 	Link_Server_Echo(void);
u8 	Link_Server_AT(u8 mode,const char* ipaddr,const char *port);

u8 	Send_Data_To_Server(char* data);

u8 	SIM800_GPRS_ON(void);
u8	SIM800_GPRS_OFF(void);
void SIM800_POWER_ON(void);
void SIM800_POWER_OFF(void);
void SIM800_PWRKEY_ON(void);
void SIM800_PWRKEY_OFF(void);

void SIM800_GPRS_Restart(void);
void SIM800_Powerkey_Restart(void);
void SIM800_Power_Restart(void);


u8 SIM800_Link_Server(void);
u8 SIM800_Link_Server_AT(void);
u8 SIM800_Link_Server_Powerkey(void);

void Get_Login_Data(void);
u8 Send_Login_Data(void);
u8 Send_Login_Data_Normal(void);
u8 Send_Login_Data_To_Server(void);

void Get_Heart_Data(void);
u8 Send_Heart_Data(void);
u8 Send_Heart_Data_Normal(void);
u8 Send_Heart_Data_To_Server(void);

void Get_Resend_Data(void);
u8 Send_Resend_Data(void);
u8 Send_Resend_Data_Normal(void);
u8 Send_Resend_Data_To_Server(void);

void Get_Open_Device_Data(void);
u8 Send_Open_Device_Data(void);
u8 Send_Open_Device_Data_Normal(void);
u8 Send_Open_Device_Data_To_Server(void);

void Get_Close_Device_Data(void);
u8 Send_Close_Device_Data(void);
u8 Send_Close_Device_Data_Normal(void);
u8 Send_Close_Device_Data_To_Server(void);

typedef struct
{
	u8 status;
	u8 hb_timer;   //hb always running
	u8 reply_timeout;
	bool need_reset;
	u32 msg_recv;
	u32 msg_expect;
	char atcmd_ack[LENGTH_ATCMD_ACK];
	char device_on_cmd_string[LENGTH_DEVICE_OPEN_CMD];
	char usart_data[LENGTH_DEVICE_OPEN_CMD];
}t_DEV;

/*********WJ*********/
extern t_DEV dev;
enum
{
	CMD_NONE,   //before connect

	CMD_IDLE,
		
	CMD_LOGIN,
	CMD_HB,
	CMD_CLOSE_DEVICE,

	CMD_OPEN_DEVICE,	
	CMD_ERROR,
};

enum
{ 
	CMD_ACK_OK = 0,                 //USART3_RX_STA置位，返回的数据正确
	CMD_ACK_DISCONN = 1,       //USART3_RX_STA置位，返回的数据表明掉线
	CMD_ACK_NONE,                  //USART3_RX_STA没有置位
};

enum
{
	MSG_BIT_ACK = 0,                  //Dev->Server
	MSG_BIT_LOGIN,
	MSG_BIT_HB,                        
	MSG_BIT_CLOSE,	

	MSG_BIT_OPEN,               //Server->Dev

	MSG_BIT_RESEND,          //Both Dir
	
	MSG_BIT_RESET,             //Reset	
};

#define MSG_DEV_ACK          ((u32)(1<<MSG_BIT_ACK))
#define MSG_DEV_LOGIN     ((u32)(1<<MSG_BIT_LOGIN))
#define MSG_DEV_HB            ((u32)(1<<MSG_BIT_HB))
#define MSG_DEV_CLOSE      ((u32)(1<<MSG_BIT_CLOSE))

#define MSG_DEV_OPEN        ((u32)(1<<MSG_BIT_OPEN))

#define MSG_DEV_RESEND   ((u32)(1<<MSG_BIT_RESEND))

#define MSG_DEV_RESET      ((u32)(1<<MSG_BIT_RESET))


#endif

