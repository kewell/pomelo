/**
* gprs_task.c GPRS上行通信任务
* 
* 
* 创建时间: 2008-5-20
* 最后修改时间: 2010-5-20
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/reset.h"
#include "include/param/term.h"
#include "include/uplink/svrnote.h"
#include "uplink/uplink_pkt.h"
#include "uplink/uplink_dl.h"
#include "uplink/svrcomm.h"
#include "uplink/keepalive.h"
#include "gprs_hardware.h"
#include "gprs_dev.h"

//@change later 连接备用服务器
int GprsActiveTask_watchdog;

static int GprsLogon(void)
{
	printf("GprsLogon1...");
	SetKeepAlive(KEEPALIVE_FLAG_LOGONFAIL);

	SvrCommLineState = LINESTAT_OFF;
	//svr_lineled(0);

	if(UplinkLogon(UPLINKITF_GPRS)) {
		GprsDisConnect();
		GprsDevLineState = GSTAT_LINE_OFF;
		return 1;
	}
	printf("GprsLogon2...");
	SvrCommLineState = LINESTAT_ON;
	SetKeepAlive(KEEPALIVE_FLAG_LOGONOK);
	//svr_lineled(1);
	return 0;
}

static void GprsLogOff(void)
{
	GprsDisConnect();
	GprsDailOff();
	GprsDevDailState = GSTAT_DIAL_OFF;
	GprsDevLineState = GSTAT_LINE_OFF;
	SvrCommLineState = LINESTAT_OFF;
	//svr_lineled(0);
}

static int GprsKeepAlive(void)
{
	static int cnt_connecterr = 0;
	static int server_sel = 0;
	static int cnt_logonerr = 0;
	static int cnt_dailerr = 0;
	int i;							
	

	int rtn;

	PrintLog(LOGTYPE_SHORT, "gprs keep alive...\n");

	if(LINESTAT_ON == SvrCommLineState) 
	{
		i = 8;
		do
		{
			rtn = UplinkLinkTest(UPLINKITF_GPRS);
			if(UPRTN_OK == rtn) 
			{
				return 0;
			}
			else 
			{
				SvrMessageProc(UPLINKITF_GPRS);
			}	
			Sleep(1500);
			i--;
		}while(i);
		SvrCommLineState = LINESTAT_OFF;
		GprsDisConnect();
		GprsDailOff();
	}

	//UplinkLinkTest(UPLINKITF_GPRS);	保持数据发送
	

	//if(GSTAT_LINE_ON == GprsDevLineState) 
	//{
	//	if(!GprsLogon()) 
	//	{
	//		cnt_logonerr = 0;
	//		return 0;
	//	}
	//}

	if(GSTAT_DIAL_ON != GprsDevDailState) 
	{
		if(GprsDail()) 
		{
			cnt_dailerr++;
			goto mark_fail;
		}
		else
		{
			cnt_dailerr = 0;
			GprsDevDailState = GSTAT_DIAL_ON;
		}
	}

	for(i=0;i<3;i++)
	{
		if(GSTAT_DIAL_ON != GprsDevLineState) 
		{
			if(!GprsConnect(server_sel)) 
			{
				cnt_connecterr = 0;
				if(LINESTAT_ON != SvrCommLineState) 
				{
					if(!GprsLogon()) 
					{
						cnt_logonerr = 0;
						return 0;
					}
					else
					{
						cnt_logonerr++;
					}
				}
			}
			else
			{
				cnt_connecterr++;
			}
			Sleep(300);
		}
	}

	if(cnt_connecterr>(1 * 60 * 1))	////1个小时连接不上,则重新拨号
	{
		cnt_connecterr = 0;
		GprsDisConnect();
		GprsDailOff();
		GprsDevDailState = GSTAT_DIAL_OFF;
	}
	return 1;	

mark_fail:
	if(cnt_dailerr > 80)
	{
		cnt_dailerr = 0;
		GprsDevDailState = GSTAT_DIAL_OFF;
		GprsDevLineState = GSTAT_LINE_OFF;
		SvrCommLineState = LINESTAT_OFF;
		Sleep(100 * 30);
	}
	GprsDevDailState = GSTAT_DIAL_OFF;
	GprsDevLineState = GSTAT_LINE_OFF;
	SvrCommLineState = LINESTAT_OFF;
	return 1;
}

extern int EthSvrInit(void);
	
static void GprsActiveTask(void)
{
	unsigned long ev;
	int bactive;

	UplinkClearState(UPLINKITF_GPRS);
	SvrCommLineState = LINESTAT_OFF;
	GprsActiveTask_watchdog = 0;
	GprsDevDailState = GSTAT_DIAL_OFF;
	GprsDevLineState = GSTAT_LINE_OFF;
	SvrCommLineState = LINESTAT_OFF;

	GprsDevInit();

	/*while(1) {
		if(!GprsRestart()) {
			GprsKeepAlive();
			GprsDailOff();
		}
	}*/

	//bactive = KeepAliveInPeriod();
	bactive = 0;
	//if(!GprsRestart() && bactive) GprsKeepAlive();
	//GprsDevLineState = GSTAT_LINE_ON;
	//if(1) GprsKeepAlive();
	GprsKeepAlive();
	//system( "ifconfig eth0 down");
	//EthSvrInit();
	//system( "ifconfig eth0 up");
	while(1) 
	{
		SvrCommPeekEvent(SVREV_NOTE, &ev);
		if(ev&SVREV_NOTE) 
		{
			if(!RefreshKeepAlive()) GprsKeepAlive();
			if(LINESTAT_ON == SvrCommLineState) SvrNoteProc(UPLINKITF_GPRS);
		}

		GprsLineState();
		if(LINESTAT_ON == SvrCommLineState) 
		{
			if(!UplinkRecvPkt(UPLINKITF_GPRS))
				SvrMessageProc(UPLINKITF_GPRS);
		}

		//GprsRing();
		if(GprsDevRingFlag) 
		{
			GprsDevRingFlag = 0;
			if(LINESTAT_OFF == SvrCommLineState) //掉线
			{
				ClearKeepAlive();
				GprsKeepAlive();//激活在线
			}
		}

		//if(KeepAliveInPeriod()) {
		if(1) {	
			if(!bactive) {
				bactive = 1;
				GprsKeepAlive();
			}
			else if(!KeepAliveProc()) GprsKeepAlive();
		}
		else {
			bactive = 0;
			if(GSTAT_DIAL_ON == GprsDevDailState) {
				ClearKeepAlive();
				printf("before ClearKeepAlive GprsLogOff.....");
				GprsLogOff();
			}
		}
		if(GprsActiveTask_watchdog>30)
		{
			GprsActiveTask_watchdog = 0;
		}
		Sleep(10);
	}
}

#if 0
static void GprsInactiveTask(void)
{
#define TIME_RESET		1800  // 3minute
	//@change later 短信
	unsigned long ev;
	int onlinetime, onlinemax;
	int timereset = 0;

	UplinkClearState(UPLINKITF_GPRS);
	SvrCommLineState = LINESTAT_OFF;
	printf("GprsInactiveTask...\n");

	#if 1


/*
	if(GprsDail())
	{
		printf("GprsDailfail...\n");
	}
	else
	{
		printf("GprsDailfsucc...\n");
	}
	*/

	if(!GprsConnect(0)) 
	{
		printf("GprsConnectsucc...\n");
		if(!GprsLogon()) 
		{
			printf("GprsLogonsucc...\n");
		}
		UplinkLinkTest(UPLINKITF_GPRS);
	}
	#endif
	
	GprsDevInit();
	//GprsRestart();
	onlinemax = (int)ParaTerm.uplink.timedown&0xff;
	onlinemax *= 600;
	onlinetime = 0;

	while(1) {
		SvrCommPeekEvent(SVREV_NOTE, &ev);
		if(ev&SVREV_NOTE) {
			if(LINESTAT_OFF == SvrCommLineState) GprsKeepAlive();
			if(LINESTAT_ON == SvrCommLineState) SvrNoteProc(UPLINKITF_GPRS);
		}

		GprsLineState();
		if(LINESTAT_ON == SvrCommLineState) {
			onlinetime++;
			if(!UplinkRecvPkt(UPLINKITF_GPRS)) {
				onlinetime = 0;
				SvrMessageProc(UPLINKITF_GPRS);
			}

			if(onlinetime > onlinemax) GprsLogOff();
		}
		else if(GSTAT_DIAL_ON == GprsDevDailState) GprsLogOff();

		if(GSTAT_DEV_OK != GprsDevModuleState) {
			timereset++;
			if(timereset > TIME_RESET) {
				timereset = 0;
				GprsRestart();
			}
		}

		GprsRing();
		if(GprsDevRingFlag) {
			GprsDevRingFlag = 0;
			if(LINESTAT_OFF == SvrCommLineState) GprsKeepAlive();
		}

		Sleep(10);
	}
}
#endif

void GprsTask(void)
{
	//unsigned char dev;

	//dev = ReadRightModuleNo();
	//printf("Module No = %d\n", dev);

	//if(ParaTerm.uplink.clientmode == 1) GprsInactiveTask();
	//else GprsActiveTask();

	GprsActiveTask();
	
}

