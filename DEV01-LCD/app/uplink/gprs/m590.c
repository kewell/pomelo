/**
* m590.c -- MC37模块管理
* 
* 
* 创建时间: 2010-5-19
* 最后修改时间: 2010-5-19
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/reset.h"
#include "include/debug/statistic.h"
#include "gprs_dev.h"
#include "gprs_hardware.h"
#include "gprs_at.h"
#include "uplink/ppp/ppp_api.h"
#include "uplink/uplink_pkt.h"
#include "uplink/svrcomm.h"
#include "include/param/term.h"
#include "include/param/unique.h"
#include "include/sys/gpio.h"

//#define m590_uart_fd		UartGetFid(1)
#define m590_uart_fd		UartGetFid(0)
//#define m590_uart_close		UartClose(1)
#define m590_uart_close		UartClose(0)

#define m590_uart_open		UartOpen(0)
//#define m590_uart_open		UartOpen(0)

static int m590_sock = -1;
#define CLOSE_SOCKET(sock)   { \
	if((sock) >= 0) { \
		close(sock); \
		sock = -1; \
	}}

static char m590_smscentre[32];
static char m590_apn[32];

static const char m590_atcmd_sig[] = "AT+CSQ\r\n";
static const char m590_atcmd_pdphead[] = "AT+CGDCONT=1,\"IP\",\"";
static const char m590_atcmd_pdptail[] = "\",\"0.0.0.0\",0,0\r\n";
static const char m590_atcmd_chkpdp[] = "AT+CGDCONT?\r\n";
static const char m590_atcmd_dial[] = "ATD*99***1#\r\n";

static const char m590_atecho_OK[] = "OK";
static const char m590_atecho_ERR[] = "ERROR";
static const char m590_atecho_CNT[] = "CONNECT";
static const char m590_atecho_NOCAR[] = "NO CARRIER";

#define MC37ATIDX_ATE0		0
#define MC37ATIDX_SMSO		1
static const atlist_t m590_atlist[] = {
	{"ATE0\r\n", 2, {m590_atecho_OK, m590_atecho_ERR, NULL, NULL}},
	{"AT^SMSO\r\n", 1, {m590_atecho_OK, NULL, NULL, NULL}}
};
#define MC37AT_LIST(a)    ((atlist_t *)(&(m590_atlist[a])))
#define mc37_uart_fd		UartGetFid(0)
#define mc37_uart_close		UartClose(0)
#define mc37_uart_open		UartOpen(0)

DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_devpwroff, 0);
DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_devpwroffnormal, 0);
DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_devreset, 0);
DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_devresetok, 0);
DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_dail, 0);
DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_dailok, 0);
DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_connect, 0);
DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_connectok, 0);
DECLARE_STATISTIC(STATISGROUP_UPLINK, gprs_sigquality, 0);



#define GPRS_PORT	0x00


int get_m590_sock(void)
{
	return m590_sock;
}

#if 1
int CheckPBReady()
{
	int timeout;  // 超时次数变量；
	char c;
	unsigned char rcv_buf_tmp[256];
	unsigned char *pdata;
	
	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	pdata = rcv_buf_tmp;
	do
	{   
    		timeout++;
		while(UartRecv(GPRS_PORT,&c, 1))
		{
			*pdata++ = c;
		}
		
		if(strstr((char *)rcv_buf_tmp,"+PBREADY")>0)
		{
			printf("rcv +PBREADY\n");
			printf("rcv +PBREADY\n");
			printf("rcv +PBREADY\n");
			printf("rcv +PBREADY\n");
			printf("rcv +PBREADY\n");
			break;// 说明收到+PBREADY，退出；
		}
		Sleep(30);// 延时300ms
	}while(timeout<50);	  
	
	if( timeout>=50 )
	{	
		return 1;
	}

	return 0; 
}
#endif


void RecieveFromUart(unsigned char *buf) 
{
	int timeout;  // 超时次数变量；
	char c;
	unsigned char *pdata;
	
	pdata = buf;
	timeout = 0;
	do
	{   
    		timeout++;
		while(UartRecv(GPRS_PORT,&c, 1))
		{
			*pdata++ = c;
		}
		Sleep(30);// 延时300ms
	}while(timeout<50);	  
}

void RecieveFromUart1(unsigned char *buf) 
{
	int timeout;  // 超时次数变量；
	char c;
	unsigned char *pdata;
	int len = 0;
	
	pdata = buf;
	timeout = 0;
	do
	{   
    		timeout++;
		while(UartRecv(GPRS_PORT,&c, 1))
		{
			*pdata++ = c;
			len++;
			if(len>2000)
				break;
		}
	//}while(timeout<5000);	  
		Sleep(10);
	}while(timeout<3);	  
}




/****************************************************************************************
*函数名：CheckAT

*描  述：检查信号强度；

*****************************************************************************************/ 
int CheckAT()
{
	int timeout;  // 超时次数变量；
	unsigned char rcv_buf_tmp[256];
	
	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	do
	{   
		timeout++;
		UartSend(GPRS_PORT,"at\r",3);
		//Sleep(10);// 延时100ms
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		
		if( strstr((char *)rcv_buf_tmp,"OK")>0 )// 说明串口通，退出；
		{
			printf("CheckAT_OK\n");
			break;
		}
	//}while(timeout<50);	 
	}while(timeout<3);	 
	
	if( timeout>=2 )
	{	
		return 1;
	}

	return 0; 
}

int CheckCCID()
{
	int timeout;  // 超时次数变量；
	unsigned char rcv_buf_tmp[256];

	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	do
	{   
		timeout++;

		UartSend(GPRS_PORT,"at+ccid\r",8); // 通过串口发送AT+CSQ回车
		//Sleep(20);// 延时200ms
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		
		if( strstr((char *)rcv_buf_tmp,"ERROR")==0 )// 
		{
			printf("CheckCCID_OK\n");
			break;
		}
	}while(timeout<3);	 
	
	if( timeout>=2 )
	{	
		return 1;
	}

	return 0; 
}

int CheckCSQ()
{
	int timeout;  // 超时次数变量；
	unsigned char rcv_buf_tmp[256];

	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	do
	{   
		timeout++;

		UartSend(GPRS_PORT,"at+csq\r",7); 
		//Sleep(10);// 延时100ms
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		printf("rcv_buf_tmp = %s\n",(char *)rcv_buf_tmp);
		if( strstr((char *)rcv_buf_tmp,"+CSQ:99,99")==0 )
		{
			printf("CheckCSQ_OK\n");
			break;
		}
	}while(timeout<3);	 
	
	if( timeout>=2 )
	{	
		return 1;
	}

	return 0; 
}



int CheckCSQ1()
{
	int timeout;  // 超时次数变量；
	unsigned char rcv_buf_tmp[256];
	char term_CSQ[2];
	unsigned char term_CSQ1;

	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	/*
	do
	{   
		timeout++;

		UartSend(GPRS_PORT,"ATE0\r",5); 
		//Sleep(10);// 延时100ms
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		printf("rcv_buf_tmp = %s\n",(char *)rcv_buf_tmp);
		if( strstr((char *)rcv_buf_tmp,"+CSQ:99,99")==0 )
		{
			printf("CheckCSQ_OK\n");
			break;
		}
	}while(timeout<3);	 
	*/
/*
	do
	{   
		timeout++;
		UartSend(GPRS_PORT,"at\r",3);
		//Sleep(10);// 延时100ms
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		
		if( strstr((char *)rcv_buf_tmp,"OK")>0 )// 说明串口通，退出；
		{
			printf("CheckAT_OK\n");
			break;
		}
	//}while(timeout<50);	 
	}while(timeout<3);	
	
	UartSend(GPRS_PORT,"ATE1\r",5); 
	Sleep(10);// 延时100ms
*/


	
	do
	{   
		timeout++;

		UartSend(GPRS_PORT,"at+csq\r",7); 
		//Sleep(10);// 延时100ms
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		printf("rcv_buf_tmp = %s\n",(char *)rcv_buf_tmp);
		if( strstr((char *)rcv_buf_tmp,"+CSQ:99,99")==0 )
		{
			printf("CheckCSQ_OK\n");
			break;
		}
	}while(timeout<3);	 
	
	if( timeout>=2 )
	{	
		return 1;
	}
	memcpy(term_CSQ,&rcv_buf_tmp[5],2);
	term_CSQ1 = atoi((char *)term_CSQ);
	if(term_CSQ1<10 && term_CSQ1>4)
		term_CSQ1 = 1;
	else if(term_CSQ1<16 && term_CSQ1>10)
		term_CSQ1 = 2;
	else if(term_CSQ1<22 && term_CSQ1>16)
		term_CSQ1 = 3;	
	else if(term_CSQ1<28 && term_CSQ1>22)
		term_CSQ1 = 4;		
	else if(term_CSQ1<99 && term_CSQ1>28)
		term_CSQ1 = 5;		
	else if(term_CSQ1>=99)
		term_CSQ1 = 0;	
	PrintLog(0, "term_CSQ1=%d\n",term_CSQ1);
	return term_CSQ1; 
}







int CheckCreg()
{
	int timeout;  // 超时次数变量；
	unsigned char rcv_buf_tmp[256];

	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	do
	{   
		timeout++;

		UartSend(GPRS_PORT,"at+creg?\r",9); 
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		
		if( strstr((char *)rcv_buf_tmp,"+CREG: 0,1")>0 ||  strstr((char *)rcv_buf_tmp,"+CREG: 0,5")>0 )
		{
			printf("CheckCreg_OK\n");
			break;
		}
	}while(timeout<3);	 
	
	if( timeout>=2 )
	{	
		return 1;
	}

	return 0; 
}


int send_dail_at_cmd()
{
	int timeout;
	unsigned char rcv_buf_tmp[256];
	char atcmd_apn[70];
	const char atcmd_head[] = "at+cgdcont=1,IP,";
	
	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	memset(atcmd_apn,'\0',sizeof(atcmd_apn));
	strcpy(atcmd_apn,atcmd_head);
	strcat(atcmd_apn,ParaTerm.svrip.apn);
	strcat(atcmd_apn,"\r");
	
	UartSend(GPRS_PORT,"at+tcpclose=0\r",14);// 发送AT+TCPCLOSE=0回车
	RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
	printf("rcv_buf_tmp = %s\n",(char *)rcv_buf_tmp);

	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	UartSend(GPRS_PORT,"at+tcpclose=1\r",14);// 发送AT+TCPCLOSE=0回车
	RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
	printf("rcv_buf_tmp = %s\n",(char *)rcv_buf_tmp);

	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	do
	{   
		timeout++;
		//UartSend(GPRS_PORT,"at+cgdcont=1,IP,CMNET\r",22);
		//UartSend(GPRS_PORT,"at+cgdcont=1,IP,CMNET\r",22);
		//UartSend(GPRS_PORT,"at+cgdcont=1,IP,xggdgs.hb\r",26);
		UartSend(GPRS_PORT,atcmd_apn,strlen(atcmd_apn));
		//Sleep(20);// 延时100ms
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		printf("rcv_buf_tmp = %s\n",(char *)rcv_buf_tmp);
		if( strstr((char *)rcv_buf_tmp,"OK")>0 )
		{
			printf("at+cgdcont=1,IP,CMNET_OK\n");
			break;
		}
	}while(timeout<5);	

	char buffer[30];
	memset(buffer,0,sizeof(buffer));
	strcpy(buffer,"AT+XGAUTH=1,2, gprs , gprs ");
	buffer[14] = '"';
	buffer[19] = '"';
	buffer[21] = '"';
	buffer[26] = '"';
	buffer[27] = 0x0d;
	UartSend(GPRS_PORT,buffer,28);
	Sleep(20);// 延时200ms
	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
	printf("AT+XGAUTH=1,2, gprs , gprs    = %s\n",(char *)rcv_buf_tmp);

	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	UartSend(GPRS_PORT,"AT+CGATT?\r",10);

	RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
	printf("AT+CGATT = %s\n",(char *)rcv_buf_tmp);

	timeout = 0;
	memset(rcv_buf_tmp,0x00,sizeof(rcv_buf_tmp));
	//UartSend(GPRS_PORT,"ATD*99***1#\r\n",13);
	UartSend(GPRS_PORT,"ATDT*99***1#\r\n",14);
	
	do
	{   
		timeout++;
		RecieveFromUart1(rcv_buf_tmp); //获得返回值信息
		printf("rcv_buf_tmp = %s\n",(char *)rcv_buf_tmp);
		if( strstr((char *)rcv_buf_tmp,"CONNECT")>0 )
		{
			printf("ATD*99#OK\r");
			break;
		}
	}while(timeout<5);	
	return 0;
}



/**
* @brief 检测MC37模块信号强度
* @return 信号强度, 99为未知
*/
static int M590SigAmp(void)
{
	int i;
	unsigned char *p = GPRS_CMDBUFFER;
	unsigned char stat = 0;

	GPRSAT_CLRRCV;

	GprsLineSend((unsigned char *)m590_atcmd_sig, strlen((char *)m590_atcmd_sig));

	for(i=0; i<8; i++) {
		Sleep(10);

		while(GprsLineRecv(p, 1) > 0) {
			//DebugPrint(0, "sig recv=%c\r\n", *p);
			switch(stat) {
			case 0:
				if(':' == *p) stat = 1;
				break;
			case 1:
				if(',' == *p) {
					*p = 0;

					p = GPRS_CMDBUFFER;
					i = atoi((char *)p);

					if((i < 0) || (i > 31)) i = 99;
					else if (i<3) i= 0;
					else i = (i/8)+1;

					Sleep(10);
					GPRSAT_CLRRCV;
					//Debug(0, "debug sigamp =%d\r\n", i);
					return i;
				}
				else p++;
				break;
			}
		}
	}

	return(99);
}

/**
* @brief 检测MC37模块的AT命令响应情况
* @return 成功0, 否则失败
*/
int M590CheckDev(void)
{
	int i;

	for(i=0; i<5; i++) 
	{
		//if(0 == GprsSendAt(MC37AT_LIST(MC37ATIDX_ATE0), 1)) 
		if(0 == CheckAT())
		return 0;
		Sleep(100);
	}

	return 1;
}

/**
* @brief 给MC37模块发送掉电命令
*/
static void M590SwitchOff(void)
{
	int i;

	PrintLog(LOGTYPE_ALARM, "MC37 switch off...");
	//gprsline_devoff;
	//gprsline_dtroff;
	for(i=0; i<3; i++) {
		if(0 == GprsSendAt(MC37AT_LIST(MC37ATIDX_SMSO), 3)) {
			for(i=0; i<200; i++) {  // 20s
				if(!gprsline_alive) break;
				Sleep(10);
			}
			if(i < 200) {
				ADDONE_STATISTIC(gprs_devpwroffnormal);
				PrintLog(LOGTYPE_ALARM, "ok\n");
			}
			else PrintLog(LOGTYPE_ALARM, "error\n");
			Sleep(10);
			return;
		}

		Sleep(100);
	}
	PrintLog(LOGTYPE_ALARM, "timeout\n");
}

/**
* @brief 给MC37模块掉电
* @param flag 上电标志, 0-异常上电, 1-正常上电
*/

static void M590PowerOff(int flag)
{
	PrintLog(LOGTYPE_ALARM, "MC37 Power off...\n");
	if(flag && gprsline_alive) M590SwitchOff();
	gprsline_pwroff;

	ADDONE_STATISTIC(gprs_devpwroff);
}

/**
* @brief 给MC37模块上电
* @param flag 上电标志, 0-异常上电, 1-正常上电
*/
static void M590PowerOn(int flag)
{
	static int nofirst = 0;

	if(nofirst) {
		M590PowerOff(flag);
		Sleep(1000);
	}
	else nofirst = 1;

	PrintLog(LOGTYPE_ALARM, "MC37 Power on\n");
	gprsline_pwron;
	Sleep(20);
}

/**
* @brief 起始复位MC37模块
* @return 成功0, 否则失败
*/
static int M590OnOff(void)
{
	int i, retry;

	PrintLog(LOGTYPE_ALARM, "MC37 restart... \n");

	for(retry=0; retry<3; retry++) {
		ADDONE_STATISTIC(gprs_devreset);
		gprsline_devoff;

		for(i=0; i<50; i++) {
			Sleep(10);
			if(gprsline_alive) {
				Sleep(200);
				gprsline_devon;
				PrintLog(LOGTYPE_ALARM, "MC37 restart ok.\n");
				GprsDevModuleState = GSTAT_DEV_RESOK;
				Sleep(300);
				return 0;
			}
		}

		PrintLog(LOGTYPE_ALARM, "***\n");
		M590PowerOn(0);
	}

	PrintLog(LOGTYPE_ALARM, "MC37 restart fail\n");
	M590PowerOff(0);
	GprsDevModuleState = GSTAT_DEV_DEVERR;
	return 1;
}

/**
* @brief 检查APN一致
* @param 参数APN
* @return 一致返回0, 不一致返回1, 未响应返回2
*/
static int M590CheckApn(const char *apn)
{
	char *p = (char *)GPRS_CMDBUFFER;
	int i, j, bend;
	char c;
	char cmpstr[10];

	p[0] = 0;
	strcpy(p, m590_atcmd_chkpdp);
	GprsLineSend((unsigned char *)p, strlen(p));

	strcpy(cmpstr, "1,\"IP\",\"");

	p = (char *)GPRS_CMDBUFFER;
	*p = 0;
	j = 0;
	bend = 0;

	for(i=0; i<10; i++) {
		Sleep(10);
		while(GprsLineRecv((unsigned char *)&c, 1) > 0) {
			//DebugPrint(0, "%c", c);
			if(j < 8) {
				if(c == cmpstr[j]) j++;
				else j= 0;
			}
			else {
				if('\"' == c) {
					*p = 0;
					bend = 1;
					break;
				}
				else {
					*p++ = c;
				}
			}
		}

		if(bend) break;
	}

	if(!bend) return 2;

	p = (char *)GPRS_CMDBUFFER;
	PrintLog(LOGTYPE_ALARM, "APN=%s\n", p);

	if(0 != strcmp(p, apn)) return 1;

	return 0;
}

/**
* @brief 设置初始的AT命令
* @param apn APN
* @param sms 短信中心号码
* @return 成功0, 否则失败
*/
static int M590InitAtCommand(const char *apn, const char *sms)
{
	atlist_t *plist = (atlist_t *)GPRS_CMDBUFFER;
	char *p;
	int i, rtn;

	if(M590CheckDev()) return 1;

	for(i=0; i<3; i++) {
		rtn  = M590CheckApn(apn);
		if(rtn < 2) {
			Sleep(20);
			GPRSAT_CLRRCV;
			break;
		}

		Sleep(100);
	}

	if(0 != rtn) {
		p = (char *)GPRS_CMDBUFFER;
		p += sizeof(atlist_t);

		plist->cmd = p;
		plist->keynum = 2;
		plist->key[0] = m590_atecho_OK;
		plist->key[1] = m590_atecho_ERR;

		strcpy(p, m590_atcmd_pdphead);
		strcat(p, apn);
		strcat(p, m590_atcmd_pdptail);

		for(i=0; i< 3; i++) {
			PrintLog(LOGTYPE_ALARM, "%s", p);
			if(0 == GprsSendAt(plist, 2)) {
				Sleep(10);
				GPRSAT_CLRRCV;
				break;
			}

			Sleep(100);
		}

		if(i >= 3) return 1;

		PrintLog(LOGTYPE_ALARM, "OK.\r\n");
	}

	/*for(i=0; i<3; i++)
	{
		if(0 == gprs_sendat(MC37AT_LIST(MC37ATIDX_SMSNOTE), 2)) break;
		Sleep(50);
	}
	if(i >= 3) return 1;

	for(i=0; i<3; i++)
	{
		if(0 == gprs_sendat(MC37AT_LIST(MC37ATIDX_MSGSTORE), 2)) break;
		Sleep(50);
	}
	if(i >= 3) return 1;*/

	return 0;
}

/**
* @brief 模块复位
* @return 成功0, 否则失败
*/
int M590Restart(const char *apn, const char *sms, const char *usrname, const char *pwd)
{
	int amp, count, i, dectcnt, continuouscnt;
	int statsig;

	if(m590_uart_fd >= 0) m590_uart_close;
	m590_uart_open;
	GprsLineSetBaud(57600);

	GprsDevDailState = GSTAT_DIAL_OFF;
	GprsDevLineState = GSTAT_LINE_OFF;
	GprsDevModuleState = GSTAT_DEV_DEVERR;
	GprsDevSigState = GSTAT_SIG_UNKNOWN;

	CLOSE_SOCKET(m590_sock);

	strcpy(m590_smscentre, sms);
	strcpy(m590_apn, apn);

	count = 1;

	while(1) {
		M590PowerOn(1);
		if(M590OnOff()) return 1;

		if(count > 1) Sleep((count-1)*200);

		//if(0 == M590InitAtCommand(apn, sms)) {
		if(0 == M590CheckDev()) {
			dectcnt = 60+count*5;
			continuouscnt = 0;
			for(i=0; i<dectcnt; i++) {
				amp = M590SigAmp();
				PrintLog(LOGTYPE_SHORT, "signal quality = %d\n", amp);
				GprsDevSigState = (unsigned char)amp;
				if(GprsDevSigState > 4) GprsDevSigState = 0;

				if((amp > 0) && (amp < 5)) {
					GprsDevModuleState = GSTAT_DEV_OK;
					continuouscnt++;
					if(continuouscnt > 3) {
						statsig = GET_STATISTIC(gprs_sigquality);
						if(0 == statsig) SET_STATISTIC(gprs_sigquality, amp*100);
						else {
							statsig += amp*100;
							statsig /= 2;
							SET_STATISTIC(gprs_sigquality, statsig);
						}

						if(0 == M590InitAtCommand(apn, sms)) {
							ADDONE_STATISTIC(gprs_devresetok);
							return 0;
						}
						else break;
					}
				}
				else if(0 == amp) {
					continuouscnt = 0;
					GprsDevModuleState = GSTAT_DEV_NOSIG;
				}
				else {
					continuouscnt = 0;
					GprsDevModuleState = GSTAT_DEV_SIMERR;
				}

				Sleep(200);
			}

			//m590_dev_initat(apn, sms);
		}

		/*if(0 == (count%3)) {
			M590PowerOn(1);
		}*/
		count++;

		if(count > 10) {
			M590PowerOff(1);
			return 1;
		}
	}

	return 1;
}

#if 0

void gprs_power_on()
{
	printf("gprs_power_on...\n");
	Sleep(100);
	set_io(AT91_PIN_PA5,1);
	Sleep(100);
	set_io(AT91_PIN_PA5,0);
	Sleep(100);
	set_io(AT91_PIN_PA5,1);
	Sleep(1500);
}



void gprs_power_off()
{
	printf("gprs_power_off...\n");
	Sleep(100);
	set_io(AT91_PIN_PB19,1);
	Sleep(100);
	set_io(AT91_PIN_PB19,0);
	Sleep(500);
	set_io(AT91_PIN_PB19,1);
	Sleep(1000);
}


void gprs_reset()
{
	printf("gprs_reset...\n");
	gprs_power_off();
	gprs_power_on();
}

#endif

#if 1
void gprs_power_on()
{
	printf("gprs_power_on...\n");
	Sleep(100);
	set_io(AT91_PIN_PB20,1);
	Sleep(100);
	set_io(AT91_PIN_PB20,0);
	Sleep(100);
	set_io(AT91_PIN_PB20,1);
	Sleep(1500);
}



void gprs_reset()
{
	printf("gprs_reset...\n");
	Sleep(100);
	set_io(AT91_PIN_PA5,1);
	Sleep(100);
	set_io(AT91_PIN_PA5,0);
	Sleep(100);
	set_io(AT91_PIN_PA5,1);
	Sleep(1500);
}


void gprs_power_off()
{
	printf("gprs_power_off...\n");
	Sleep(100);
	set_io(AT91_PIN_PB19,1);
	Sleep(100);
	set_io(AT91_PIN_PB19,0);
	Sleep(500);
	set_io(AT91_PIN_PB19,1);
	Sleep(1000);
}

void gprs_restart()
{
	printf("gprs_restart...\n");

	gprs_power_off();
	gprs_power_on();
	gprs_reset();
}
#endif

/*
void gprs_reset_2()
{
	printf("gprs_reset_2...\n");
	Sleep(100);
	set_io(AT91_PIN_PB20,1);
	Sleep(100);
	set_io(AT91_PIN_PB20,0);
	Sleep(500);
	set_io(AT91_PIN_PB20,1);
	Sleep(1000);
}

void gprs_reset_3()
{
	printf("gprs_reset_3...\n");
	Sleep(100);
	set_io(AT91_PIN_PA4,1);
	Sleep(100);
	set_io(AT91_PIN_PA4,0);
	Sleep(500);
	set_io(AT91_PIN_PA4,1);
	Sleep(1000);
}
*/




int M590Init(void)
{
	printf("M590Init1 ...\n");
	UartClose(GPRS_PORT);
	printf("M590Init2 ...\n");
	Sleep(10);
	UartOpen(GPRS_PORT);
	printf("M590Init3 ...\n");
	Sleep(10);
	//UartSet(GPRS_PORT, 115200, 8, 1, 'N');
	UartSet(GPRS_PORT, ParaUni.gprs_mod_baud, 8, 1, 'N');
	printf("M590Init4 ...\n");
	Sleep(10);
	gprs_restart();
	printf("M590Init5 ...\n");
	//在这里进行断电操作
	
	if(CheckCCID())
	{
		printf("CheckCCID fail ...\n");
		return 1;
	}
	//UartSend(GPRS_PORT,"ATE1\r",5); 
	//Sleep(10);// 延时100ms
	
	if(CheckCSQ())
	{
		printf("CheckCSQ fail ...\n");
		return 1;
	}
	if(CheckCreg())
	{
		printf("CheckCreg fail ...\n");
		return 1;
	}
	return 0;
}

/**
* @brief 连接GPRS服务
* @return 成功0, 否则失败
*/
int M590Dail(void)
{
	static int cnterr = 0;

	int i, count;

	printf("GPRS Dial ...\n");
	GprsDevLineState = GSTAT_LINE_OFF;
	GprsDevDailState = GSTAT_DIAL_OFF;
	CLOSE_SOCKET(m590_sock);

	count = 0;
	//system( "ifconfig ppp0 down");
	//system( "ifconfig ppp1 down");
	//gprs_restart();
	//for(i=0; i<5; i++) 
	//for(i=0; i<3; i++) 	
	for(i=0; i<1; i++) 		
	{
		if(M590CheckDev()) 
		{
			DebugPrint(LOGTYPE_ALARM, "m590 no response...\r\n");
			M590Init();
			//if(M590Init())
			//{
			//	gprs_reset();
			//}
			//if(M590Restart(m590_apn, m590_smscentre, NULL, NULL)) return 1;
		}

		ADDONE_STATISTIC(gprs_dail);

		//GprsLineSend((unsigned char *)m590_atcmd_dial, strlen(m590_atcmd_dial));
		send_dail_at_cmd();

		//system( "ifconfig eth0 down");
		if(!ppp_start(mc37_uart_fd)) {
			printf("GPRS Dial OK...\n");
			//system( "ifconfig eth0 up");
			GprsDevDailState = GSTAT_DIAL_ON;
			cnterr = 0;
			return 0;
		}
		else
		{
			printf("GPRS Dial FAIL...\n");
		}
		ppp_end(mc37_uart_fd);
		//system( "ifconfig eth0 up");
		//system( "ifconfig eth0 down");
		cnterr++;
		if(cnterr > 600) 
		{
			PrintLog(LOGTYPE_ALARM, "m590 dail fail so much times..,reboot\n");
			SysRestart();
		}

		//count++;
		//if(0 == (count&0x01)) 
		//{
		//	if(M590Restart(m590_apn, m590_smscentre, NULL, NULL)) 
		//	{
		//		return 1;
		//	}
			//else cnterr = 0;
		//}

		//PrintLog(LOGTYPE_SHORT, "GPRS Dial Fail.\r\n");
		gprs_restart();	
		Sleep(2000);
		//Sleep(100);
	}

	return 1;
}

/**
* @brief 断开GPRS服务
* @return 成功0, 否则失败
*/
int M590DialOff(void)
{
	ppp_end(m590_uart_fd);
	return 0;
}

/**
* @brief 连接到服务器
* @param ip 服务器IP
* @param port 服务器端口号
* @param proto 连接协议, 0-TCP, 1-UDP
* @return 成功0, 否则失败
*/
int M590Connect(unsigned long ip, unsigned short port, unsigned char proto)
{
	struct sockaddr_in addr;
	//int ctlflag;

	//system( "ifconfig eth0 down");
	//printf("ifconfig eth0 down.........\n");
	Sleep(100);
	GprsDevLineState = GSTAT_LINE_OFF;

	CLOSE_SOCKET(m590_sock);

	printf("M590Connect.........\n");
	if(proto) //UDP
		m590_sock = socket(AF_INET, SOCK_DGRAM, 0);
	else  //TCP
		m590_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(m590_sock < 0) {
		PrintLog(LOGTYPE_ALARM, "create socket errror.\n");
		return 1;
	}

	ADDONE_STATISTIC(gprs_connect);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(ip);

	//PrintLog(LOGTYPE_DEBUG, "connect to %d.%d.%d.%d, %d...\n",
			//(ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff, port);
	printf("connect to %d.%d.%d.%d, %d...\n",
			(unsigned char)(ip>>24)&0xff, (unsigned char)(ip>>16)&0xff, (unsigned char)(ip>>8)&0xff, (unsigned char)ip&0xff, port);

	if(connect(m590_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		printf("M590Connect  failed.\n");
		PrintLog(LOGTYPE_DEBUG, "connect failed.\n");
		CLOSE_SOCKET(m590_sock);
		//Sleep(100);
		//system( "ifconfig eth0 up");
		//printf("ifconfig eth0 up.........\n");
		//Sleep(100);
		return 1;
	}
	//Sleep(100);
	//system( "ifconfig eth0 up");
	//printf("ifconfig eth0 up.........\n");
	//Sleep(100);


	/*ctlflag = fcntl(m590_sock, F_GETFL);
	ctlflag |= O_NONBLOCK;
	fcntl(m590_sock, F_SETFL, ctlflag);*/

	GprsDevLineState = GSTAT_LINE_ON;
	PrintLog(LOGTYPE_DEBUG, "M590connect succeed.\n");
	printf("gprs connect succeed.\n");
	ADDONE_STATISTIC(gprs_connectok);
	return 0;
}

/**
* @brief 与服务器断开连接
*/
void M590Disconnect(void)
{
	printf("M590Disconnect...........\n");
	CLOSE_SOCKET(m590_sock);
	GprsDevLineState = GSTAT_LINE_OFF;
	PrintLog(LOGTYPE_SHORT, "disconnect server...OK\r\n");
	printf("disconnect server...OK...........\n");
}

/**
* @brief 检测在线状态
* @return 在线返回1, 掉线返回0
*/
int M590LineState(void)
{
	//@change later:需要测试一下DCD
	if(GprsDevDailState == GSTAT_DIAL_ON) {
		ppp_proc();
		if(!ppp_isup) 
		{
			if(GprsDevLineState == GSTAT_LINE_ON)
			{
				printf("M590LineState...........\n");
				printf("before M590Disconnect()...........\n");
				M590Disconnect();
			}
			GprsDevDailState = GSTAT_DIAL_OFF;
			return 0;
		}
	}

	return 1;
}

/**
* @brief 测试ring状态
*/
void M590Ring(void)
{
	static int cnt = 0;
	static unsigned char stat = 0;

	if(GSTAT_DEV_OK != GprsDevModuleState) {
		cnt = 0;
		stat = 0;
		GprsDevRingFlag = 0;
		return;
	}

	if(stat) {
		cnt++;
		if(cnt >= 3000) {
			cnt = 0;
			stat = 0;
		}
	}
	else {
		if(gprsline_ring) {
			if((cnt >= 10) && (cnt < 200)) {
				GprsDevRingFlag = 1;
				cnt = 0;
				stat = 1;
			}
			else cnt = 0;
		}
		else cnt++;
	}
}

extern void UpdateSticComm(unsigned int bytes);
//static unsigned char m590_recvbuf[2048];
//static int m590_recvlen = 0;
//static int m590_recvhead = 0;

/**
* @brief 从GPRS读取一个字节
* @param buf 返回字符指针
* @return 成功0, 否则失败
*/
int M590GetChar(unsigned char *buf)
{

	
	if(m590_sock < 0) return 1;

	/*
	if(m590_recvlen <= 0) 
	{
		m590_recvlen = recv(m590_sock, m590_recvbuf, 2048, MSG_DONTWAIT);
		//if(m590_recvlen>10)
		//{
		//	printf("m590_recvlen = %d\n",m590_recvlen);
		//}
		if(((m590_recvlen < 0) && (errno != EWOULDBLOCK)) ||(m590_recvlen == 0))
		{
			PrintLog(LOGTYPE_SHORT, "connection corrupted.\n");
			printf("M590GetChar..............\n");
			printf("connection corrupted..............\n");
			M590Disconnect();
			SvrCommLineState = LINESTAT_OFF;
			return 1;
		}
		else if(m590_recvlen < 0) 
		{
			return 1;
		}
		else 
		{
			m590_recvhead = 0;
			UpdateSticComm(m590_recvlen);
		}
	}

	*buf = m590_recvbuf[m590_recvhead++];
	m590_recvlen--;
	return 0;
	*/

	
	int rcv_len = -1;

	rcv_len = recv(m590_sock, buf, 1, MSG_DONTWAIT);
	if(rcv_len>0)
	{
		//printf("M590GetChar..............\n");
		return 0;
	}
	else
	{
		return 1;
	}

	
}

#if 0
int m590_dev_rawsend(unsigned char *buf, int len)
{
	if(m590_sock < 0) return 1;

	send(m590_sock, buf, len, MSG_NOSIGNAL);
	return 0;
}
#else
int M590RawSend(const unsigned char *buf, int len)
{
	//int i, buflen;

	if(m590_sock < 0) return 1;

	if(send(m590_sock, buf, len, MSG_NOSIGNAL) < 0) {
		DebugPrint(1, "send fail\n");
		goto mark_failend;
	}

	UpdateSticComm(len);
	return 0;

	//wait until send buffer empty
	/*for(i=0; i<450; i++) {
		Sleep(10);

		if(ioctl(m590_sock, SIOCOUTQ, &buflen)) goto mark_failend;

		//debug_print(1, "buflen = %d\r\n", buflen);
		if(0 == buflen) return 0;
	}*/

mark_failend:
	PrintLog(LOGTYPE_SHORT, "connection corrupted.\r\n");
	CLOSE_SOCKET(m590_sock);
	SvrCommLineState = LINESTAT_OFF;
	return 1;
}
#endif

void M590DetectLine(unsigned char uc)
{
	return;
}

