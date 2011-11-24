/**
* cascade.c -- 级联通信
* 
* 
* 创建时间: 2010-6-14
* 最后修改时间: 2010-6-14
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFINE_CASCADE

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/task.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/rs485.h"
#include "include/param/mix.h"
#include "include/param/commport.h"
#include "include/monitor/runstate.h"
#include "uplink_pkt.h"
#include "uplink_dl.h"
#include "include/uplink/svrnote.h"
#include "keepalive.h"
#include "svrcomm.h"
#include "cascade.h"

int CascadeMode = 0;
int CascadeNoted = 0;

static const int CsBaudList[8] = {300, 600, 1200, 2400, 4800, 7200, 9600, 19200};

/**
* @brief 设置级联端口
* @param frame 端口字节格式配置
*/
static void SetCascadePort(unsigned char frame)
{
	int baud, databits, stopbits;
	unsigned char uc;
	char parity;

	uc = (frame>>5)&0x07;
	baud = CsBaudList[uc];

	databits = (int)(frame&0x03) + 5;
	if(frame & 0x08) {
		if(frame & 0x04) parity = 'O';
		else parity = 'E';
	}
	else parity = 'N';
	if(frame & 0x10) stopbits = 2;
	else stopbits = 1;

	Rs485Set(COMMPORT_CASCADE, baud, databits, stopbits, parity);
}

/**
* @brief 级联客户端任务
*/
void CascadeClientTask(void)
{
	unsigned long ev;

	UplinkClearState(UPLINKITF_CASCADE);
	SvrCommLineState = LINESTAT_OFF;

	SetCascadePort(ParaMix.cascade.frame);

	while(1) {
		SvrCommPeekEvent(SVREV_NOTE, &ev);

		if(ev&SVREV_NOTE) CascadeNoted = 1;

		if(!UplinkRecvPkt(UPLINKITF_CASCADE)) {
			SvrMessageProc(UPLINKITF_CASCADE);
		}

		Sleep(10);
	}
}

/**
* @brief 发送请求主动上报报文
* @param psnd 帧缓存区指针
* @param clientid 客户端序号(0~2)
*/
static void SendReqPkt(uplink_pkt_t *psnd, unsigned char clientid)
{
	static unsigned char seq = 0;

	psnd->ctrl = UPCMD_LINKTEST | UPCTRL_PRM;
	psnd->afn = UPAFN_CASCADE;
	psnd->area[0] = ParaMix.cascade.addr[clientid*4];
	psnd->area[1] = ParaMix.cascade.addr[clientid*4+1];
	psnd->sn[0] = ParaMix.cascade.addr[clientid*4+2];
	psnd->sn[1] = ParaMix.cascade.addr[clientid*4+3];

	psnd->seq = seq++ & 0x0f;
	psnd->seq |= UPSEQ_SPKT;

	psnd->data[0] = 0;
	psnd->data[1] = 0;
	psnd->data[2] = 0;
	psnd->data[3] = 0;

	UPLINKAPP_LEN(psnd) = 4;
	CascadeSvrSendPkt(UPLINKITF_CASCADE, psnd);
}

#define CSTAT_IDLE			0
#define CSTAT_STARTPOLL		1
#define CSTAT_WAITDATA		2
#define CSTAT_DELAY			3

#define DEFAULT_POLL_CYCLE	3000   // 5min
#define WAITDATA_CYCLE		600  // 1min

/**
* @brief 级联服务器端任务
*/
static void *CascadeServerTask(void *arg)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(UPLINKITF_CASCADE);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(UPLINKITF_CASCADE);
	int state, count, rcvecho, retry;
	int poll_cyc, wait_timeout, bforward;
	unsigned char clientid, clientnum;

	Sleep(200);
	while(RunState.pwroff) Sleep(100);

	DebugPrint(LOGTYPE_ALARM, "cascade server task started\n");

	SetCascadePort(ParaMix.cascade.frame);

	UplinkClearState(UPLINKITF_CASCADE);
	poll_cyc = (int)ParaMix.cascade.cycle & 0xff;
	poll_cyc *= 600;
	if(0 == poll_cyc) poll_cyc = DEFAULT_POLL_CYCLE;

	state = 0;
	count = 0;
	rcvecho = 0;
	retry = clientid = wait_timeout = 0;

	while(1) {
		switch(state) {
		case CSTAT_IDLE:
			count++;
			if(count >= poll_cyc) {
				count = 0;
				poll_cyc = (int)ParaMix.cascade.cycle & 0xff;
				poll_cyc *= 600;
				if(0 == poll_cyc) poll_cyc = DEFAULT_POLL_CYCLE;
				if(!RunState.pwroff && LINESTAT_ON == SvrCommLineState) {
					state = CSTAT_STARTPOLL;
					clientid = 0;
				}
			}
			break;

		case CSTAT_STARTPOLL:
			clientnum = ParaMix.cascade.num;
			if(clientnum > MAX_CASCADE) clientnum = 0;
			if(clientid >= clientnum) {
				state = CSTAT_IDLE;
				count = 0;
				break;
			}
			
			SendReqPkt(psnd, clientid);
			state = CSTAT_WAITDATA;
			count = 0;
			rcvecho = 0;
			retry = 0;
			wait_timeout = (int)ParaMix.cascade.timeout & 0xff;
			if(wait_timeout < 10) wait_timeout = 20;
			break;

		case CSTAT_WAITDATA:
			if(rcvecho) {
				count = 0;
				state = CSTAT_DELAY;
				wait_timeout = 30;
				break;
			}

			count++;
			if(count > wait_timeout) {
				count = 0;
				retry++;
				if(retry >= (int)ParaMix.cascade.retry) {
					clientid++;
					state = CSTAT_STARTPOLL;
				}
				else SendReqPkt(psnd, clientid);
			}
			break;

		case CSTAT_DELAY:
			count++;
			if(count > wait_timeout) {
				count = 0;
				state = CSTAT_STARTPOLL;
			}
			break;

		default:
			state = CSTAT_IDLE;
			break;
		}

		if(UplinkRecvPkt(UPLINKITF_CASCADE)) {
			Sleep(10);
			continue;
		}

		bforward = 1;
		if(state == CSTAT_WAITDATA) {
			if(0 == prcv->afn) {
				if(2 == prcv->data[2]) {  //无数据
					clientid++;
					state = CSTAT_STARTPOLL;
					bforward = 0;
				}
			}
			else rcvecho = 1;
		}

		if(bforward && LINESTAT_ON == SvrCommLineState) {
			CascadeForwardPkt(SvrCommInterface, prcv);
		}

		Sleep(10);
	}

	return 0;
}

/**
* @brief 级联通信初始化
*/
DECLARE_INIT_FUNC(CascadeInit);
int CascadeInit(void)
{
	CascadeMode = CASCADE_NONE;
	CascadeNoted = 0; 

	//if(0 == ParaMix.cascade.port) goto mark_end;
	if(0 == ParaMix.cascade.num || ParaMix.cascade.num > MAX_CASCADE) goto mark_end;

	if(ParaMix.cascade.flag) CascadeMode = CASCADE_CLIENT;
	else CascadeMode = CASCADE_SERVER;

	if(CASCADE_SERVER == CascadeMode) SysCreateTask(CascadeServerTask, NULL);

mark_end:
	SET_INIT_FLAG(CascadeInit);
	return 0;
}

/**
* @brief 从级联通信接口读取一个字节
* @param buf 返回字符指针
* @return 成功0, 否则失败
*/
int CascadeGetChar(unsigned char *buf)
{
	if(Rs485Recv(COMMPORT_CASCADE, buf, 1) > 0) return 0;
	else return 1;
}

/**
* @brief 向级联口通信接口发送数据
* @param buf 发送缓存区指针
* @param len 缓存区长度
* @return 成功0, 否则失败
*/
int CascadeRawSend(const unsigned char *buf, int len)
{
	return(Rs485Send(COMMPORT_CASCADE, buf, len));
}

