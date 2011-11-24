/**
* svrcomm.c -- 服务器通信
* 
* 
* 创建时间: 2010-5-19
* 最后修改时间: 2010-5-19
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/debug.h"
#include "include/startarg.h"
#include "include/sys/event.h"
#include "include/sys/schedule.h"
#include "include/sys/task.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/param/unique.h"
#include "include/param/mix.h"
#include "include/param/datatask.h"
#include "include/monitor/runstate.h"
#include "include/uplink/svrnote.h"
#include "include/cenmet/qrycurve.h"
#include "include/cenmet/qrydata.h"
#include "include/cenmet/sndtime.h"
#include "include/screen/scr_show.h"
#include "uplink_pkt.h"
#include "uplink_dl.h"
#include "keepalive.h"
#include "svrcomm.h"
#include "cascade.h"

static unsigned int SvrCommNoteId = 0;
static unsigned int SvrCommNoteIdTaskCls2 = 0;
static sys_event_t SvrCommEvent;
unsigned char SvrCommInterface = UPLINKITF_GPRS;
int SvrCommLineState = 0;//已登陆主站
/**
* @brief 接收上行通信事件
* @param waitmask 接收掩码
* @param pevent 返回事件掩码指针
*/
void SvrCommPeekEvent(unsigned long waitmask, unsigned long *pevent)
{
	SysWaitEvent(&SvrCommEvent, 0, waitmask, pevent);
}

/**
* @brief 生成上行通信事件
* @param id 事件编号
*/
void SvrCommNote(int id)
{
	unsigned int mask;

	AssertLogReturnVoid(id<=0 || id > 64, "invalid note id(%d)\n", id);

	id--;

	if(id < 32) {
		mask = 1<<id;

		SvrCommNoteId |= mask;
	}
	else {
		mask = 1<<(id-32);
		SvrCommNoteIdTaskCls2 |= mask;
	}

	SysSendEvent(&SvrCommEvent, SVREV_NOTE);
}

static int TimerIdTaskCls1[MAX_DTASK_CLS1];
static int TimerIdTaskCls2[MAX_DTASK_CLS2];

/**
* @brief 1类数据任务上传定时器
* @param arg 任务号
* @param utime 当前时间
*/
static void RTimerDataCls1(unsigned long arg, utime_t utime)
{
	PrintLog(LOGTYPE_SHORT, "start task class one %d ...\n", arg);

	SvrCommNote(SVRNOTEID_TASKCLS1(arg));
}

/**
* @brief 1类数据任务上传初始化
* @param tid 任务号
*/
void DataTaskCls1ReInit(int tid)
{
	rtimer_conf_t conf;

	AssertLogReturnVoid(tid < 0 || tid >= MAX_DTASK_CLS1, "invalid taskid(%d)\n", tid);

	if(0 == ParaTaskCls1[tid].valid) {
		if(TimerIdTaskCls1[tid] >= 0) SysStopRTimer(TimerIdTaskCls1[tid]);
		TimerIdTaskCls1[tid] = -1;
		return;
	}

	conf.basetime.year = ParaTaskCls1[tid].base_year;
	conf.basetime.month = ParaTaskCls1[tid].base_month;
	conf.basetime.day = ParaTaskCls1[tid].base_day;
	conf.basetime.hour = ParaTaskCls1[tid].base_hour;
	conf.basetime.minute = ParaTaskCls1[tid].base_minute;
	conf.basetime.second = ParaTaskCls1[tid].base_second;
	conf.bonce = 0;
	conf.tdev = ParaTaskCls1[tid].dev_snd;
	conf.tmod = ParaTaskCls1[tid].mod_snd;
	conf.curtime = UTimeReadCurrent();

	if(TimerIdTaskCls1[tid] >= 0) SysStopRTimer(TimerIdTaskCls1[tid]);
	TimerIdTaskCls1[tid] = SysAddRTimer(&conf, RTimerDataCls1, tid);
}

/**
* @brief 2类数据任务上传定时器
* @param arg 任务号
* @param utime 当前时间
*/
static void RTimerDataCls2(unsigned long arg, utime_t utime)
{
	PrintLog(LOGTYPE_SHORT, "start task class two %d ...\n", arg);

	SvrCommNote(SVRNOTEID_TASKCLS2(arg));
}

/**
* @brief 2类数据任务上传初始化
* @param tid 任务号
*/
void DataTaskCls2ReInit(int tid)
{
	rtimer_conf_t conf;

	AssertLogReturnVoid(tid < 0 || tid >= MAX_DTASK_CLS2, "invalid taskid(%d)\n", tid);

	if(0 == ParaTaskCls2[tid].valid) {
		if(TimerIdTaskCls2[tid] >= 0) SysStopRTimer(TimerIdTaskCls2[tid]);
		TimerIdTaskCls2[tid] = -1;
		return;
	}

	conf.basetime.year = ParaTaskCls2[tid].base_year;
	conf.basetime.month = ParaTaskCls2[tid].base_month;
	conf.basetime.day = ParaTaskCls2[tid].base_day;
	conf.basetime.hour = ParaTaskCls2[tid].base_hour;
	conf.basetime.minute = ParaTaskCls2[tid].base_minute;
	conf.basetime.second = ParaTaskCls2[tid].base_second;
	conf.bonce = 0;
	conf.tdev = ParaTaskCls2[tid].dev_snd;
	conf.tmod = ParaTaskCls2[tid].mod_snd;
	conf.curtime = UTimeReadCurrent();

	if(TimerIdTaskCls2[tid] >= 0) SysStopRTimer(TimerIdTaskCls2[tid]);
	TimerIdTaskCls2[tid] = SysAddRTimer(&conf, RTimerDataCls2, tid);
}


#if 0
/**
* @brief 数据任务上传初始化
*/
static void DataTaskInit(void)
{
	int tid;

	for(tid=0; tid<MAX_DTASK_CLS1; tid++) {
		TimerIdTaskCls1[tid] = -1;
		DataTaskCls1ReInit(tid);
	}

	for(tid=0; tid<MAX_DTASK_CLS2; tid++) {
		TimerIdTaskCls2[tid] = -1;
		DataTaskCls2ReInit(tid);
	}
}
#endif

/**
* @brief 任务数据主动发送
*/
static inline int TaskActSend(unsigned char itf, uplink_pkt_t *pkt)
{
	if(CASCADE_CLIENT == CascadeMode) {
		return UplinkActiveSend(itf, 1, pkt);
	}
	else {
		UplinkActiveSend(itf, 0, pkt);
		return UplinkDelay(itf, pkt);
	}
}

/**
* @brief 1类数据任务处理
* @param itf 接口编号
* @param tid 任务编号
* @return 继续返回0, 否则中止
*/
static int DataTaskCls1Proc(unsigned char itf, int tid)
{
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	int rcvlen, sendlen, actlen, rtn, sendmax;
	const unsigned char *pdatarcv;
	unsigned char *pdatasnd;

	if(0 == ParaTaskCls1[tid].valid || 0 == ParaTaskCls1[tid].num) return 0;

	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_QRYCLS1;
	psnd->seq = UPSEQ_SPKT;

	pdatarcv = (const unsigned char *)ParaTaskCls1[tid].duid;
	pdatasnd = psnd->data;
	rcvlen = (int)ParaTaskCls1[tid].num & 0xff;
	if(rcvlen > MAX_TASK_DUID) return 0;
	rcvlen *= 4;
	sendlen = 0;
	sendmax = UPLINK_SNDMAX(itf);

	while(rcvlen >= 4) {
		if(pdatarcv[1] > 1) rtn = ReadPlMdb(pdatarcv, pdatasnd, sendmax, &actlen);//前8个为多功能表
		else rtn = ReadMdb(pdatarcv, pdatasnd, sendmax, &actlen);

		rcvlen -= 4;
		pdatarcv += 4;

		if(!rtn) {
			sendmax -= actlen;
			if(sendmax < 0) {
				ErrorLog("send max error(%d,%d)\n", sendmax,actlen);
				return 0;
			}
			pdatasnd += actlen;
			sendlen += actlen;
		}
	}

	if(0 == sendlen) return 0;

	UPLINKAPP_LEN(psnd) = sendlen;
	rtn = TaskActSend(itf, psnd);
	if((UPRTN_OKRCV == rtn) || (UPRTN_FAILRCV == rtn)) {
		SvrMessageProc(itf);
		return 1;
	}
	else if(rtn) return 1;

	return 0;
}

static int SndTimeFlag;

/**
* @brief 2类数据任务处理
* @param itf 接口编号
* @param tid 任务编号
* @return 继续返回0, 否则中止
*/
static int DataTaskCls2Proc(unsigned char itf, int tid)
{
	uplink_pkt_t *psnd;
	qrycurve_buffer_t *cache;
	unsigned char pngrp;
	int itemlen, rtn;
	unsigned char duid[4];
	//printf("DataTaskCls2Proc1...\n");
	duid[0] = 0x01;
	duid[1] = 0x00;
	duid[2] = 0x01;
	duid[3] = 0x14;
	//if(0 == ParaTaskCls2[tid].valid || 0 == ParaTaskCls2[tid].num) return 0;

	itemlen = (int)ParaTaskCls2[tid].num & 0xff;
	if(itemlen > MAX_TASK_DUID) return 0;
	itemlen *= 4;

	cache = GetQueryCache(UPLINK_SNDNOR(itf));
	//pngrp = ParaTaskCls2[tid].duid[0].da[1];
	pngrp = duid[1];
	//printf("DataTaskCls2Proc2...\n");
	//if(pngrp < 2) ActiveSendCurve((unsigned char *)ParaTaskCls2[tid].duid, itemlen, cache, ParaTaskCls2[tid].freq);
	//else ActiveSendPlCurve((unsigned char *)ParaTaskCls2[tid].duid, itemlen, cache, ParaTaskCls2[tid].freq);
	ActiveSendPlCurve(duid, itemlen, cache, ParaTaskCls2[tid].freq);
	//printf("DataTaskCls2Proc3...\n");
	for(; NULL!=cache; cache=cache->next) {
		if(cache->datalen <= 0) break;

		psnd = (uplink_pkt_t *)(cache->buffer - OFFSET_UPDATA);
		psnd->ctrl = UPECHO_USRDATA;
		psnd->afn = UPAFN_QRYCLS2;
		psnd->seq = UPSEQ_SPKT;
		UPLINKAPP_LEN(psnd) = cache->datalen;

		rtn = TaskActSend(itf, psnd);
		if((UPRTN_OKRCV == rtn) || (UPRTN_FAILRCV == rtn)) {
			if(UPRTN_OKRCV == rtn) SaveSndTime();
			QueryCacheUnlock();
			SvrMessageProc(itf);
			QueryCacheLock();
			return 1;
		}
		else if(rtn) return 1;

		SaveSndTime();
		SndTimeFlag = 1;
	}
	//printf("DataTaskCls2Proc4...\n");
	return 0;
}

/**
* @brief 告警处理
* @param itf 接口编号
* @return 继续返回0, 否则中止
*/
static int AlarmProc(unsigned char itf)
{
	uplink_pkt_t *psnd;
	qrycurve_buffer_t *cache;
	int rtn;
	runstate_t *pstat = RunStateModify();

	cache = GetQueryCache(UPLINK_SNDNOR(itf));

	while(1) {
		ClearQueryCache(cache);
		ActiveSendAlarm(cache);//主动发送告警
		if(cache->datalen <= 4) break;

		psnd = (uplink_pkt_t *)(cache->buffer - OFFSET_UPDATA);
		psnd->ctrl = UPECHO_USRDATA;
		psnd->afn = UPAFN_QRYCLS3;
		psnd->seq = UPSEQ_SPKT;
		UPLINKAPP_LEN(psnd) = cache->datalen;

		rtn = UplinkActiveSend(itf, 1, psnd);
		if((UPRTN_OKRCV == rtn) || (UPRTN_FAILRCV == rtn)) {
			if(UPRTN_OKRCV == rtn) {
				pstat->cnt_snderr[0] = 0;
				pstat->alarm.snd[0]++;
			}
			QueryCacheUnlock();
			SvrMessageProc(itf);
			QueryCacheLock();
			return 1;
		}
		else if(UPRTN_TIMEOUT == rtn) {
			pstat->cnt_snderr[0]++;
			if(pstat->cnt_snderr[0] > 5) {  //skip this alarm
				pstat->cnt_snderr[0] = 0;
				pstat->alarm.snd[0]++;
			}

			return 1;
		}
		else if(rtn) return 1;

		pstat->alarm.snd[0]++;
	}

	SaveRunState();

	return 0;
}


/**
* @brief 上行通信事件处理
* @param itf 接口编号
* @return 完成返回0,未完成返回1
*/
int SvrNoteProc(unsigned char itf)
{
	int tid;
	unsigned int mask;

	//DebugPrint(LOGTYPE_SHORT, "actsend %08XH, %08XH, %d\n", SvrCommNoteId, SvrCommNoteIdTaskCls2, ParaMix.bactsend);
	printf("actsend %08XH, %08XH, %d\n", SvrCommNoteId, SvrCommNoteIdTaskCls2, ParaMix.bactsend);
	//if(ParaMix.bactsend) return 0;

	SndTimeFlag = 0;

	QueryCacheLock();

	if(AlarmProc(itf)) goto mark_end;//如果为告警事件
	SvrCommNoteId &= ~SVRNOTEMASK_ALARM;

	for(tid=0,mask=SVRNOTEMASK_TASKCLS2; tid<MAX_DTASK_CLS2; tid++,mask<<=1) 
	{
		//if(SvrCommNoteIdTaskCls2&mask) 
		//{
			//if(DataTaskCls2Proc(itf, tid)) 
			DataTaskCls2Proc(itf, tid);
				//goto mark_end;
			//SvrCommNoteIdTaskCls2 &= ~mask;
		//}
	}

	QueryCacheUnlock();

	if(SndTimeFlag) {
		SndTimeFlag = 0;
		SaveSndTimeFile();
	}

	for(tid=0,mask=SVRNOTEMASK_TASKCLS1; tid<MAX_DTASK_CLS1; tid++,mask<<=1) {
		if(SvrCommNoteId&mask) {
			if(DataTaskCls1Proc(itf, tid)) return 1;
			SvrCommNoteId &= ~mask;
		}
	}

	return 0;

mark_end:
	QueryCacheUnlock();
	if(SndTimeFlag) {
		SndTimeFlag = 0;
		SaveSndTimeFile();
	}
	return 1;
}

/**
* @brief 载入上行通信接口
*/
static inline void LoadSvrCommItf(void)
{
	SvrCommInterface = ParaUni.uplink & 0x0f;

	switch(SvrCommInterface) {
	case UPLINKITF_GPRS:
	case UPLINKITF_ETHER:
	case UPLINKITF_SMS:
	case UPLINKITF_SERIAL:
	case UPLINKITF_IR:
		break;
	default: 
		SvrCommInterface = UPLINKITF_GPRS;
		break;
	}
}

static const char *cons_svritf_name[] = {
	"Serial", "NetMtn", "GPRS/CDMA", "Ethernet", "SMS", "Ir", "Cascade",
};
static const char cons_svritf_sflag[] = {
	'U', 'N', 'G', 'E', 'S', 'I', 'R',
};

extern void CheckSoftChange(void);
extern void EtherTask(void);
extern void GprsTask(void);
extern void SerialActiveTask(void);
extern void IrActiveTask(void);

static int SerialPassiveStarted = 0;

/**
* @brief 上行通信处理任务
*/
static void *SvrCommTask(void *arg)
{
	/*osh_event_init(&svrcomm_event);
	keepalive_clear();
	load_svrcomm_itf();
	datatask_init();*/

	ClearKeepAlive();
	//DataTaskInit();//一类，二类任务定时主动上传初始化

	Sleep(100);
	//while(RunState.pwroff) Sleep(100);//
	printf("uplink channel is %s ...\r\n", cons_svritf_name[SvrCommInterface]);
	ScreenSetHeadFlag(SCRHEAD_FLAG_CHN, (unsigned int)cons_svritf_sflag[SvrCommInterface]);

	CheckSoftChange();//检查软件修改,如果修改则生成事件
	
	switch(SvrCommInterface) {//只能有一个上行通信信道生效
	case UPLINKITF_GPRS:
		GprsTask();
		break;
	case UPLINKITF_SERIAL:
		if(SerialPassiveStarted) break;
		SerialActiveTask();
		break;
	case UPLINKITF_ETHER:
		EtherTask();
		break;
	case UPLINKITF_IR:
		IrActiveTask();
		break;
	case UPLINKITF_CASCADE:
		CascadeClientTask();
		break;

	default: break;
	}

	while(1) Sleep(1000);
}

/**
* @brief 是否有主动发送事件
* @return 有返回1, 无返回0
*/
int SvrCommHaveTask(void)
{
	int balarm = 0;

	if(ParaMix.bactsend) return 0;

	if(SvrCommNoteId&SVRNOTEMASK_ALARM) balarm = 1;
	else if(SvrCommNoteIdTaskCls2) balarm = 1;

	if(balarm) return 1;
	else return 0;
}

extern int UplinkSerialStart(int mode, unsigned int baud);
extern int UplinkIrStart(int mode);
extern int SvrSelfTranFileInit(void);
extern int EthSvrInit(void);
extern int SvrMessgeInit(void);

/**
* @brief 服务器通信初始化
*/
DECLARE_INIT_FUNC(SvrCommInit);
int SvrCommInit(void)
{
	char buf[16];

	SysInitEvent(&SvrCommEvent);

	LoadSvrCommItf();

	SvrMessgeInit();
	SvrSelfTranFileInit();
	
	EthSvrInit();

	if(GetStartArg('b', NULL, 0) && GetStartArg('s', NULL, 0)) //前台运行, 未启动命令行界面
	{ 
		if(!GetStartArg('p', buf, 16)) //tty使用raw模式,并且启动串口接收任务
		{ 
			int baud;

			baud = atoi(buf);
			if(baud >= 1200 && baud <= 115200) 
			{
				UplinkSerialStart(1, baud);
				SerialPassiveStarted = 1;
			}
		}
		else if(!GetStartArg('r', buf, 16)) //tty使用raw模式(不能使用shell命令行)
		{ 
			int baud;

			baud = atoi(buf);
			if(baud >= 1200 && baud <= 115200) 
			{
				UplinkSerialStart(0, baud);
			}
		}
	}

	//UplinkSerialStart(1, 1200);
	UplinkSerialStart(1, 9600);
	
	CascadeInit();
	if(CASCADE_CLIENT == CascadeMode) SvrCommInterface = UPLINKITF_CASCADE;

	//if(SvrCommInterface == UPLINKITF_IR) UplinkIrStart(0);
	//else UplinkIrStart(1);

	SysCreateTask(SvrCommTask, NULL);

	SET_INIT_FLAG(SvrCommInit);
	
	return 0;
}

