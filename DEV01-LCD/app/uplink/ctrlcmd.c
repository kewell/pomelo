/**
* ctrlcmd.c -- 控制命令
* 
* 
* 创建时间: 2010-5-29
* 最后修改时间: 2010-5-30
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFINE_PARAMIX
#define DEFINE_PARAROUTE
#define DEFINE_PARAMETER

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/gpio.h"
#include "include/lib/bcd.h"
#include "include/param/operation.h"
#include "param/operation_inner.h"
#include "include/monitor/runstate.h"
#include "include/param/mix.h"
#include "include/param/route.h"
#include "include/param/meter.h"
#include "uplink/gprs/gprs_hardware.h"
#include "uplink/gprs/gprs_dev.h"
#include "uplink/uplink_pkt.h"
#include "uplink/svrcomm.h"
#include "include/plcmet/pltask.h"

extern save_mix_t ParaMixSave;
extern cfg_route_met_t ParaRoute[MAX_METER];
extern para_meter_t ParaMeter[MAX_METER];
unsigned char enable_comm_with_station_flag = 0;

/**
* @brief 较时命令处理
* @param metpid 测量点号
* @param buf 缓存区指针
* @param len 缓存区长度
* @param actlen 有效数据长度(由函数返回)
* @return 0成功, 返回-1表示缓存区溢出,返回-2表示失败, 
* @note 以下同类参数和返回值相同, 不做重复注释
*/
static int CtrlCmdTime(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	sysclock_t clock;
	extclock_t extclock;
	utime_t utimecur, utime;
	int i;

	*actlen = 6;

	if(len < 6) return -1;
	if(0 != metpid) return -2;

	clock.week = buf[4]>>5;
	if(clock.week == 7 || clock.week == 0) clock.week = 0;
	else clock.week -= 1;

	clock.second = buf[0];
	clock.minute = buf[1];
	clock.hour = buf[2];
	clock.day = buf[3];
	clock.month = buf[4] & 0x1f;
	clock.year = buf[5];
	BcdToHex(&clock.year, 6);

	utimecur = UTimeReadCurrent();
	utime = SysClockToUTime(&clock);

	SysClockSet(&clock);
	SysClockRead(&clock);

	extclock.century = 0;
	extclock.year = clock.year;
	extclock.month = clock.month;
	extclock.day = clock.day;
	extclock.hour = clock.hour;
	extclock.minute = clock.minute;
	extclock.second = clock.second;
	extclock.week = clock.week;

	ExtClockWrite(&extclock);

	if(utimecur > utime) i = utimecur - utime;
	else i = utime - utimecur;

	if(i > 300) SysRecalAllRTimer();

	return 0;
}

/**
* @brief 终端剔除投入
*/
static int CtrlCmdEnOutGrp(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	if(0 != metpid) return -2;

	RunStateModify()->outgrp = 1;

	SaveRunState();

	return 0;
}

/**
* @brief 终端剔除解除
*/
static int CtrlCmdDisOutGrp(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	if(0 != metpid) return -2;

	RunStateModify()->outgrp = 0;

	SaveRunState();

	return 0;
}

/**
* @brief 允许主动发送
*/
static int CtrlCmdEnActSnd(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	unsigned char old = ParaMixSave.mix.bactsend;

	if(0 != metpid) return -2;

	ParaMixSave.mix.bactsend = 0;

	if(old != ParaMixSave.mix.bactsend) SetSaveParamFlag(SAVEFLAG_MIX);

	return 0;
}

/**
* @brief 禁止主动发送
*/
static int CtrlCmdDisActSnd(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	unsigned char old = ParaMixSave.mix.bactsend;

	if(0 != metpid) return -2;

	ParaMixSave.mix.bactsend = 1;

	if(old != ParaMixSave.mix.bactsend) SetSaveParamFlag(SAVEFLAG_MIX);

	return 0;
}





/**
* @brief 允许与主站通话F27
*/
static int CtrlCmdEnActSComm(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	unsigned char old = ParaMixSave.mix.bactcomm;

	if(0 != metpid) return -2;

	ParaMixSave.mix.bactcomm = 0;

	if(old != ParaMixSave.mix.bactcomm) SetSaveParamFlag(SAVEFLAG_MIX);
	enable_comm_with_station_flag = 1;

	return 0;
}




/**
* @brief 禁止与主站通话	F35
*/
static int CtrlCmdDisActSComm(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	unsigned char old = ParaMixSave.mix.bactcomm;

	if(0 != metpid) return -2;

	ParaMixSave.mix.bactcomm = 1;

	if(old != ParaMixSave.mix.bactcomm) SetSaveParamFlag(SAVEFLAG_MIX);
	enable_comm_with_station_flag = 1;
	return 0;
}





/**
* @brief 激活连接主站
*/
static int CtrlCmdConnectSvr(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	if(0 != metpid) return -2;
	return 0;
}

/**
* @brief 与主站断开
*/
static int CtrlCmdDisconnectSvr(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	if(0 != metpid) return -2;
	SvrCommLineState = LINESTAT_OFF;
	GprsDisConnect();
	//svr_lineled(0);
	return 0;
}

/**
* @brief 指定端口暂停抄表
*/
static int CtrlCmdStopReadMet(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	unsigned char port;

	*actlen = 1;
	if(len < 1) return -1;
	if(0 != metpid) return -2;

	port = buf[0];
	if(port == PLC_PORT)
	{
		stop_point_read();
	}

	#if 0
	if(0 == port || port > MAX_COMMPORT) return -2;
	port -= 1;
	pcfg = &ParaMixSave.commport[port];
	old = pcfg->flag;

	pcfg->flag |= RDMETFLAG_ENABLE;
	if(old != pcfg->flag) SetSaveParamFlag(SAVEFLAG_MIX);
	#endif
	return 0;
}

/**
* @brief 指定端口恢复抄表
*/
static int CtrlCmdResumeReadMet(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	unsigned char port;

	*actlen = 1;
	if(len < 1) return -1;
	if(0 != metpid) return -2;

	port = buf[0];
	if(port == PLC_PORT)
	{
		go_to_point_read();
	}
	#if 0
	if(0 == port || port > MAX_COMMPORT) return -2;
	port -= 1;
	pcfg = &ParaMixSave.commport[port];
	old = pcfg->flag;

	pcfg->flag &= ~RDMETFLAG_ENABLE;
	if(old != pcfg->flag) SetSaveParamFlag(SAVEFLAG_MIX);
	#endif
	return 0;
}

/**
* @brief 指定端口重新抄表
*/
static int CtrlCmdResetReadMet(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	//para_commport_t *pcfg;
	unsigned port;

	*actlen = 1;
	if(len < 1) return -1;
	if(0 != metpid) return -2;

	port = buf[0];
	if(port == PLC_PORT)
	{
		clear_read_meter_stat();
	}
	#if 0
	if(0 == port || port > MAX_COMMPORT) return -2;
	port -= 1;
	pcfg = &ParaMixSave.commport[port];
	old = pcfg->flag;

	pcfg->flag &= ~RDMETFLAG_ENABLE;
	if(old != pcfg->flag) SetSaveParamFlag(SAVEFLAG_MIX);
	#endif
	return 0;
}



/**
* @brief 删除指定端口全部路由
*/
static int CtrlCmdInitRoute(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	//unsigned short metid;
	unsigned char port;

	*actlen = 1;
	if(len < 1) return -1;
	if(0 != metpid) return -2;

	port = buf[0];
	if(port == PLC_PORT)
	{
		go_to_active_register_init();
	}
	#if 0
	if(0 == port || port > MAX_COMMPORT) return -2;
	port -= 1;
	if(port != COMMPORT_PLC) return 0;

	for(metid=0; metid<MAX_METER; metid++) ParaRoute[metid].num = 0;

	SetSaveParamFlag(SAVEFLAG_ROUTE);
	#endif
	return 0;
}

/**
* @brief 删除指定端口全部表计
*/
static int CtrlCmdDelMet(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	unsigned short metid;
	unsigned char port;

	*actlen = 1;
	if(len < 1) return -1;
	if(0 != metpid) return -2;

	port = buf[0];
	if(0 == port || port > MAX_COMMPORT) return -2;

	for(metid=0; metid<MAX_METER; metid++) 
	{
		if(((ParaMeter[metid].portcfg & METPORT_MASK) == port) && ((ParaMeter[metid].portcfg & CENMETPORT_MASK) == port)) 
		{
			//ParaMeter[metid].metp_id = 0;
			memset((unsigned char *)&ParaMeter[metid].index,0x00,sizeof(para_meter_t));
		}
	}

	SetSaveParamFlag(SAVEFLAG_METER);

	return 0;
}



/**
* @brief 删除指定端口全部路由
*/
static int CtrlCmdSetRoute(unsigned short metpid, const unsigned char *buf, int len, int *actlen)
{
	//unsigned short metid;
	unsigned char port;

	*actlen = 1;
	if(len < 1) return -1;
	if(0 != metpid) return -2;

	port = buf[0];
	//if(port == PLC_PORT)
	//{
	go_to_set_ruter_work_mode();
	//}
	#if 0
	if(0 == port || port > MAX_COMMPORT) return -2;
	port -= 1;
	if(port != COMMPORT_PLC) return 0;

	for(metid=0; metid<MAX_METER; metid++) ParaRoute[metid].num = 0;

	SetSaveParamFlag(SAVEFLAG_ROUTE);
	#endif
	return 0;
}





struct ctrlcmd_list {
	unsigned short fnid;
	int (*pfunc)(unsigned short metpid, const unsigned char *buf, int len, int *actlen);
};
static const struct ctrlcmd_list CtrlCmdList[] = {
	{0x0304,CtrlCmdEnActSComm}, //F27
	{0x0308, CtrlCmdEnOutGrp}, //F28
	{0x0310, CtrlCmdEnActSnd}, //F29
	{0x0340, CtrlCmdTime},	//F31
	{0x0404, CtrlCmdDisActSComm}, //F35
	{0x0408, CtrlCmdDisOutGrp}, //F36
	{0x0410, CtrlCmdDisActSnd},	//F37
	{0x0420, CtrlCmdConnectSvr},//F38
	{0x0440, CtrlCmdDisconnectSvr},//F39
	{0x0601, CtrlCmdStopReadMet},	//F49
	{0x0602, CtrlCmdResumeReadMet},	//F50
	{0x0604, CtrlCmdResetReadMet},	//F51
	{0x0608, CtrlCmdInitRoute},	//F52
	{0x0610, CtrlCmdDelMet},	//F53
	{0x0620, CtrlCmdSetRoute},	//F54
};
#define NUM_LIST	(sizeof(CtrlCmdList)/sizeof(CtrlCmdList[0]))

/**
* @brief 控制命令
* @param buf 输入缓存区指针
* @param len 缓存区长度
* @param pactlen 有效数据长度(由函数返回)
* @return 0成功, 返回-1表示缓存区溢出,返回-2表示失败
*/
int CtrlCommand(unsigned char *buf, int len, int *pactlen)
{
	unsigned short metpid;
	unsigned char pns, pnmask, fns, fnmask, grpid;
	int actlen, berror, idx, rtn, itemlen;

	AssertLogReturn(len<4, -1, "invalid len(%d)\n", len);

	pns = buf[0];
	if(0 == buf[1]) metpid = 0;
	else metpid = ((unsigned short)(buf[1]-1)<<3) + 1;

	grpid = buf[3];
	fns = buf[2];

	buf += 4;
	actlen = 4;
	len -= 4;
	berror = 0;

	if(0 == metpid) pnmask = 0x80;
	else pnmask = 1;
	for(; pnmask!=0; pnmask<<=1,metpid++) {
		unsigned short fnid;
		const struct ctrlcmd_list *plist;

		if(metpid && (pns&pnmask) == 0) continue;

		for(fnmask=1; 0!=fnmask; fnmask<<=1) {
			if(0 == (fnmask&fns)) continue;

			fnid = ((unsigned short)grpid<<8) + (unsigned short)fnmask;

			plist = CtrlCmdList;
			for(idx=0; idx<NUM_LIST; idx++,plist++) {
				if(fnid == plist->fnid) break;
			}
			if(idx >= NUM_LIST) return -1;

			itemlen = 0;
			rtn = (*plist->pfunc)(metpid, buf, len, &itemlen);
			if(rtn == -1) return -1;
			if(rtn < 0) berror = 1;

			buf += itemlen;
			actlen += itemlen;
			len -= itemlen;
			if(len < 0) return -1;
		}
	}

	*pactlen = actlen;
	if(berror) return -2;
	else return 0;
}

