/**
* msgproc.c -- 上行通信命令处理
* 
* 
* 创建时间: 2010-5-18
* 最后修改时间: 2010-5-18
*/

#include <stdio.h>
#include <string.h>

#include "include/version.h"
#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/reset.h"
#include "include/sys/mutex.h"
#include "include/sys/timeal.h"
#include "include/lib/bcd.h"
#include "include/param/term.h"
#include "include/param/unique.h"
#include "include/param/operation.h"
#include "include/monitor/runstate.h"
#include "include/monitor/alarm.h"
#include "include/cenmet/qrycurve.h"
#include "include/cenmet/qrydata.h"
#include "include/cenmet/forward.h"
#include "include/plcmet/plc_forward.h"
#include "include/debug/svrcomm_shell.h"
#include "uplink_pkt.h"
#include "uplink_dl.h"
#include "terminfo.h"
#include "svrcomm.h"
#include "cascade.h"
//#include "plmdb.h"
#include "include/plcmet/pltask.h"
#include "include/plcmet/plmdb.h"
#include "include/lib/dbtime.h"
#include "include/param/capconf.h"



typedef struct st_qrycurve_buffer_unlock {
	//in
	struct st_qrycurve_buffer_unlock *next;
	int maxlen;
	unsigned char * buffer;

	//out
	int datalen;
} qrycurve_buffer_unlock_t;

typedef union {
	qrycurve_buffer_unlock_t unblock;
	qrycurve_buffer_t block;
} qrycurve_buffer_union_t;

#define QUERY_CACHE_NUM		16
#define QUERY_CACHE_LEN		1152
static unsigned int QueryCache[QUERY_CACHE_NUM][QUERY_CACHE_LEN/4];
static sys_mutex_t QueryCacheMutex;
static qrycurve_buffer_union_t QueryCacheList[QUERY_CACHE_NUM];
extern unsigned short meter_total_cnt;
/**
* @brief 获得查询缓存区组
* @param 每个缓存区的最大长度
* @return 缓存区组指针
*/
qrycurve_buffer_t *GetQueryCache(int maxlen)
{
	int i;

	if(maxlen <= 0 || maxlen > (QUERY_CACHE_LEN-MINLEN_UPLINKPKT)) {
		ErrorLog("invalid maxlen(%d)\n", maxlen);
		maxlen = QUERY_CACHE_LEN-MINLEN_UPLINKPKT;
	}

	for(i=0; i<(QUERY_CACHE_NUM-1); i++) {
		QueryCacheList[i].unblock.next = &QueryCacheList[i+1].unblock;
		QueryCacheList[i].unblock.maxlen = maxlen;
		QueryCacheList[i].unblock.buffer = (unsigned char *)(QueryCache[i]) + OFFSET_UPDATA;
		QueryCacheList[i].unblock.datalen = 0;
	}
	QueryCacheList[i].unblock.next = NULL;
	QueryCacheList[i].unblock.maxlen = maxlen;
	QueryCacheList[i].unblock.buffer = (unsigned char *)(QueryCache[i]) + OFFSET_UPDATA;
	QueryCacheList[i].unblock.datalen = 0;

	return &(QueryCacheList[0].block);
}

/**
* @brief 缓存区清零
* @param buffer 缓存区指针
*/
void ClearQueryCache(qrycurve_buffer_t *buffer)
{
	for(; NULL != buffer; buffer = buffer->next) buffer->datalen = 0;
}

/**
* @brief 锁定缓存区组操作
*/
void QueryCacheLock(void)
{
	SysLockMutex(&QueryCacheMutex);
}

/**
* @brief 解锁缓存区组操作
*/
void QueryCacheUnlock(void)
{
	SysUnlockMutex(&QueryCacheMutex);
}

static uplink_timetag_t TimeTag[UPLINKITF_NUM];
static unsigned char TpvFlag[UPLINKITF_NUM] = {0};
static unsigned char SelfCmdFlag[UPLINKITF_NUM] = {0};

/**
* @brief 发送命令回应帧
* @param itf 接口编号
* @param pkt 发送帧指针
* @return 成功0, 否则失败
*/
int SvrEchoSend(unsigned char itf, uplink_pkt_t *pkt)
{
	unsigned char *paux;
	int rtn;
	unsigned int delay = 0;

	if(SelfCmdFlag[itf]) //如果为自定义命令
	{
		unsigned char *psrc, *pdst;
		unsigned short cpylen = UPLINKAPP_LEN(pkt);
		unsigned short cpyi;

		psrc = pkt->data + cpylen - 1;
		pdst = psrc + 5;

		for(cpyi=0; cpyi<cpylen; cpyi++) *pdst-- = *psrc--;

		pdst = pkt->data;
		pdst[0] = pdst[1] = 0;  //p0
		pdst[2] = 0x01;  //F9
		pdst[3] = 0x01;
		pdst[4] = pkt->afn;

		pkt->afn = UPAFN_TRANFILE;
		cpylen += 5;
		UPLINKAPP_LEN(pkt) = cpylen;
	}

	paux = pkt->data;
	paux += UPLINKAPP_LEN(pkt);

	if(RunState.flag_acd) {
		pkt->ctrl &= 0xef;
		pkt->ctrl |= 0x20;
		GetAlarmEc(paux);
		paux += 2;
		UPLINKAPP_LEN(pkt) += 2;
	}

	if(TpvFlag[itf]) //如果带tpv
	{
		pkt->seq |= UPSEQ_TPV;
		paux[0] = TimeTag[itf].pfc;
		paux[1] = TimeTag[itf].time[0];
		paux[2] = TimeTag[itf].time[1];
		paux[3] = TimeTag[itf].time[2];
		paux[4] = TimeTag[itf].time[3];
		paux[5] = TimeTag[itf].dly;
		UPLINKAPP_LEN(pkt) += LEN_UPLINK_TIMETAG;
	}

	if(0 == (pkt->seq & UPSEQ_FIN)) {
		delay = (int)UPLINKAPP_LEN(pkt)&0xffff;
		delay += 16;
		delay >>= 7;
		delay *= UPLINK_TIMEOUT(itf);
		delay += UPLINK_TIMEOUT(itf)*3;
		/*delay = UPLINKAPP_LEN(pkt);
		delay &= 0xffff;
		delay >>= 2;
		delay += 250;*/
	}

	rtn = UplinkSendPkt(itf, pkt);
	if(!rtn) {
		if(delay) Sleep(delay);
	}

	return(rtn);
}

/**
* @brief 发送回应无数据帧
* @param itf 接口编号
* @param pkt 发送帧缓存区指针
*/
void SvrEchoNoData(unsigned char itf, uplink_pkt_t *pkt)
{
	uplink_ack_t *pack = (uplink_ack_t *)pkt->data;

	pkt->ctrl = UPECHO_DENY;
	pkt->afn = UPAFN_ECHO;
	pack->da[0] = pack->da[1] = 0;
	pack->dt[0] = 0x02;  //F2 全部否认
	pack->dt[1] = 0;
	UPLINKAPP_LEN(pkt) = 4;

	SvrEchoSend(itf, pkt);
}

#if 0
/**
* @brief 计算密码校验
* @param pwd 密钥
* @param pdata 数据缓存区指针
* @param len 数据缓存区长度
* @return 密码校验数
*/
static unsigned short CalculatePwd(unsigned short pwd, const unsigned char *pdata, int len)
{
	unsigned short crc;
	int i;

	crc = 0;
	if(len <= 0) return 0;

	while(len-- != 0) {
		crc ^= (unsigned short)(*pdata++)&0x00ff;

		for(i=0; i<8; i++) {
			if(crc&0x0001) crc = (crc>>1)^pwd;
			else crc >>= 1;
		}
	}

	return(crc);
}

/**
* @brief 校验密码的正确性
* @param 密码
* @pdata 数据缓存区指针
* @return 正确返回0, 错误返回1
*/
static int CheckPassword(const unsigned char *pass, const unsigned char *pdata)
{
	unsigned short crc, pwd;

	pwd = ((unsigned short)ParaTerm.pwd.pwd[1]<<8) + (unsigned short)ParaTerm.pwd.pwd[0];
	crc = CalculatePwd(pwd, pdata, 12);
	pwd = ((unsigned short)pass[1]<<8) + (unsigned short)pass[0];
	if(crc == pwd) return 0;
	else return 1;
}
#endif

/**
* @brief 设置软件更新标志
* @param 修改标志
*/
void SetSoftChange(unsigned char flag)
{
	runstate_t *pstat = RunStateModify();

	pstat->softchg.flag[0] = flag;
	pstat->softchg.flag[1] = 0x31;
	pstat->softchg.flag[2] = 0xfe;
	pstat->softchg.flag[3] = 0xa5;

	pstat->softchg.ver[0] = '0'+VERSION_MAJOR;
	pstat->softchg.ver[1] = '0'+VERSION_MINOR/10;
	pstat->softchg.ver[2] = '0'+VERSION_MINOR%10;
	pstat->softchg.ver[3] = '0'+VERSION_PROJECT;
}

/**
* @brief 检查软件修改,如果修改则生成事件
*/
void CheckSoftChange(void)
{
	alarm_t buf;

	if((0 == RunState.softchg.flag[0]) || (0x31 != RunState.softchg.flag[1]) || 
		(0xfe != RunState.softchg.flag[2]) || (0xa5 != RunState.softchg.flag[3]))
		return;

	memset((unsigned char *)&buf, 0, sizeof(alarm_t));
	buf.erc = 1;
	buf.len = 9;

	buf.data[0] = RunState.softchg.flag[0];
	memcpy(&buf.data[1], RunState.softchg.ver, 4);
	buf.data[5] = '0'+VERSION_MAJOR;
	buf.data[6] = '0'+VERSION_MINOR/10;
	buf.data[7] = '0'+VERSION_MINOR%10;
	buf.data[8] = '0'+VERSION_PROJECT;

	memset(RunStateModify()->softchg.ver, 0, 8);
	MakeAlarm(ALMFLAG_ABNOR, &buf);
}

/**
* @brief 复位命令处理
* @param itf 接口编号
*/
static void SvrReset(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	unsigned char *pecho = psnd->data;
	unsigned char *pcmd = prcv->data;
	unsigned resetcmd;

	if(UPLINKAPP_LEN(prcv) != 4) return;
	psnd->ctrl = UPECHO_CONF;
	psnd->afn = UPAFN_ECHO;

	UPLINK_PID0(pecho);

	if(0 != pcmd[3]) return;
	resetcmd = pcmd[2];
	UPLINK_FID(1, pecho);
	UPLINKAPP_LEN(psnd) = 4;
	SvrEchoSend(itf, psnd);
	Sleep(50);

	if(1 == resetcmd) //硬件初始化
	{ 
		if(UPLINKITF_GPRS == itf) Sleep(300);
	}
	else if(2 == resetcmd) //数据区初始化
	{ 
		ClearAllData();

		SetSoftChange(SOFTCHG_DATAINIT);
		CheckSoftChange();

		return;
	}
	else if(4 == resetcmd)  //参数及数据区初始化
	{
		ClearAllParam();
		ClearAllData();

		SetSoftChange(SOFTCHG_DATAINIT);
	}

	SysRestart();

	return;
}

/**
* @brief 设置参数命令处理
* @param itf 接口编号
*/
static void SvrSetParam(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	uplink_ack_t *pack = (uplink_ack_t *)psnd->data;
	int ballok = 1;
	int rcvlen, sendlen, actlen, rtn;
	unsigned char *pdatarcv, *pdatasnd;

	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_ECHO;
	pack->da[0] = pack->da[1] = 0;
	pack->dt[1] = 0;
	pack->afn = prcv->afn;

	pdatarcv = prcv->data;
	pdatasnd = pack->data;
	rcvlen = UPLINKAPP_LEN(prcv);
	sendlen = 5;

	if(rcvlen <= 4) goto mark_fail;

	//@change later: 加入参数改变告警
	ClearSaveParamFlag();

	while(rcvlen > 4) 
	{
		actlen = 4;
		rtn = WriteParam(pdatarcv, rcvlen, &actlen);
		if(rtn == POERR_FATAL) goto mark_fail;

		*pdatasnd++ = pdatarcv[0];//Pn，Fn
		*pdatasnd++ = pdatarcv[1];
		*pdatasnd++ = pdatarcv[2];
		*pdatasnd++ = pdatarcv[3];
		if(rtn == POERR_OK) *pdatasnd++ = 0;
		else {
			*pdatasnd++ = 1;
			ballok = 0;
		}

		rcvlen -= actlen;
		pdatarcv += actlen;
		sendlen += 5;
	}

	SaveParam();//保存设置的参数

	if(ballok) {
		pack->dt[0] = 0x01;  //F1 全部确认
		UPLINKAPP_LEN(psnd) = 4;
	}
	else {
		pack->dt[0] = 0x04; //F3 部分确认
		UPLINKAPP_LEN(psnd) = sendlen;
	}

	SvrEchoSend(itf, psnd);
	return;

mark_fail:
	pack->dt[0] = 0x02;  //F2 全部否认
	UPLINKAPP_LEN(psnd) = 4;
	SvrEchoSend(itf, psnd);

	return;
}

extern int CtrlCommand(unsigned char *buf, int len, int *pactlen);
/**
* @brief 控制命令处理
* @param itf 接口编号
*/
static void SvrCtrlCmd(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	uplink_ack_t *pack = (uplink_ack_t *)psnd->data;
	int ballok = 1;
	int rcvlen, sendlen, actlen, rtn;
	unsigned char *pdatarcv, *pdatasnd;

	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_ECHO;
	pack->da[0] = pack->da[1] = 0;
	pack->dt[1] = 0;
	pack->afn = prcv->afn;

	pdatarcv = prcv->data;
	pdatasnd = pack->data;
	rcvlen = UPLINKAPP_LEN(prcv);
	sendlen = 5;

	if(rcvlen < 4) goto mark_fail;

	//@change later: 加入参数改变告警
	ClearSaveParamFlag();

	while(rcvlen >= 4) {
		actlen = 4;
		rtn = CtrlCommand(pdatarcv, rcvlen, &actlen);
		if(rtn == -1) goto mark_fail;

		*pdatasnd++ = pdatarcv[0];
		*pdatasnd++ = pdatarcv[1];
		*pdatasnd++ = pdatarcv[2];
		*pdatasnd++ = pdatarcv[3];
		if(rtn == 0) *pdatasnd++ = 0;
		else {
			*pdatasnd++ = 1;
			ballok = 0;
		}

		rcvlen -= actlen;
		pdatarcv += actlen;
		sendlen += 5;
	}

	SaveParam();

	if(ballok) {
		pack->dt[0] = 0x01;  //F1
		UPLINKAPP_LEN(psnd) = 4;
	}
	else {
		pack->dt[0] = 0x04; //F3
		UPLINKAPP_LEN(psnd) = sendlen;
	}

	SvrEchoSend(itf, psnd);
	return;

mark_fail:
	pack->dt[0] = 0x02;  //F2
	UPLINKAPP_LEN(psnd) = 4;
	SvrEchoSend(itf, psnd);

	return;
}

/**
* @brief 查询终端配置命令处理
* @param itf 接口编号
*/
static void SvrReqConfig(unsigned char itf)
{
	
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	int rcvlen, sendlen, rtn, sendmax;
	unsigned char *pdatarcv, *pdatasnd;

	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_REQCFG;

	pdatarcv = prcv->data;//指向数据段，即从pnfn开始
	pdatasnd = psnd->data;
	rcvlen = UPLINKAPP_LEN(prcv);
	sendlen = 0;
	sendmax = UPLINK_SNDMAX(itf);

	while(rcvlen >= 4) 
	{
		rtn = ReadTermInfo(pdatarcv, pdatasnd, sendmax);
		if(rtn > 0) 
		{
			sendmax -= rtn;
			if(sendmax < 0) 
			{
				ErrorLog("send max error(%d,%d)\n", sendmax, rtn);
				goto mark_end;
			}

			sendlen += rtn;
			pdatasnd += rtn;
		}

		rcvlen -= 4;
		pdatarcv += 4;
	}

	if(0 == sendlen) goto mark_end;

	UPLINKAPP_LEN(psnd) = sendlen;
	SvrEchoSend(itf, psnd);
	return;

mark_end:
	if(0 == sendlen) {
		SvrEchoNoData(itf, psnd);
		return;
	}

	UPLINKAPP_LEN(psnd) = sendlen;
	SvrEchoSend(itf, psnd);
}

/**
* @brief 查询参数命令处理
* @param itf 接口编号
*/
static void SvrQueryParam(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	int rcvlen, sendlen, actlen, rtn, sendmax;
	unsigned char *pdatarcv, *pdatasnd;
	para_readinfo_t readinfo;

	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_QRYPARA;

	pdatarcv = prcv->data;
	pdatasnd = psnd->data;
	rcvlen = UPLINKAPP_LEN(prcv);
	sendlen = 0;
	sendmax = UPLINK_SNDMAX(itf);//发送缓冲区最大长度

	while(rcvlen >= 4) {
		readinfo.actlen = 4;
		readinfo.buf = pdatarcv;
		readinfo.len = rcvlen;

		rtn = ReadParam(pdatasnd, sendmax, &actlen, &readinfo);
		if(rtn == POERR_FATAL) goto mark_end;
		else if(rtn == POERR_OK) {
			sendmax -= actlen;
			if(sendmax < 0) {
				ErrorLog("send max error(%d,%d)\n", sendmax,actlen);
				goto mark_end;
			}
			sendlen += actlen;
			pdatasnd += actlen;

			rcvlen -= readinfo.actlen;
			pdatarcv += readinfo.actlen;
		}
		else {
			rcvlen -= readinfo.actlen;
			pdatarcv += readinfo.actlen;
		}
	}

	if(0 == sendlen) goto mark_end;

	UPLINKAPP_LEN(psnd) = sendlen;//数据单元长度
	SvrEchoSend(itf, psnd);
	return;

mark_end:
	if(0 == sendlen) {//全部否认
		SvrEchoNoData(itf, psnd);
		return;
	}

	UPLINKAPP_LEN(psnd) = sendlen;
	SvrEchoSend(itf, psnd);
}

static inline void ReadTimeToClock(const sysclock_t *fileclock, unsigned short readtime, int flag, sysclock_t *clock)
{
	clock->second = 0;
	clock->minute = readtime%60;
	readtime /= 60;
	clock->month = fileclock->month;
	clock->year = fileclock->year;

	if(flag) {
		clock->hour = readtime%24;
		clock->day = readtime/24 + 1;
	}
	else {
		clock->hour = readtime;
		clock->day = fileclock->day;
	}
}

void read_all_meter_class_one(unsigned char itf,uplink_pkt_t *rcv_pkt)
{
#define ONE_FRAME_METER_CNT		15
	unsigned short meter_num;
	unsigned char meter_prd;
	unsigned char read_time[5];
	unsigned char meter_data[ONE_FRAME_METER_CNT*14];
	unsigned char *pdata;
	int i;
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	int sendlen;
	unsigned char *pdatasnd;
	int frame_cnt;
	int last_frame_meter_cnt;
	sysclock_t clock,clock2;
	unsigned char  seq_flag = 0;
	static unsigned char fseq = 0;
	unsigned short plmet_cnt = 0;

	meter_total_cnt = get_meter_total_cnt();
	cen_meter_cnt = get_cen_meter_total_cnt();
	plmet_cnt = meter_total_cnt - cen_meter_cnt;
	if(!(meter_total_cnt - cen_meter_cnt))	return;
	fseq = rcv_pkt->seq&0x0f;
	frame_cnt = (plmet_cnt / ONE_FRAME_METER_CNT);
	last_frame_meter_cnt = (plmet_cnt % ONE_FRAME_METER_CNT);
	printf("frame_cnt = %d last_frame_meter_cnt = %d",frame_cnt,last_frame_meter_cnt);

	pdatasnd = psnd->data;
	
	pdata = meter_data;
	meter_prd = 0;
	meter_num = 2;
	memset(meter_data,0x00,ONE_FRAME_METER_CNT*14);
	sendlen =  10 + ONE_FRAME_METER_CNT * 14;
	seq_flag = 1;

	while(frame_cnt)
	{
		for(i=0;i<ONE_FRAME_METER_CNT;i++,meter_num++)
		{
			if(meter_ene_buffer[meter_num].read_stat == HAVE_READ)
			{
				*pdata++ = (1<<((meter_num - 1)&0x07));
				*pdata++ = (meter_num-1)/8+1;
				memcpy(pdata,meter_ene_buffer[meter_num].meter_addr,6);
				pdata += 6;
				*pdata++ = meter_prd;
				*pdata++ = 0;
				memcpy(pdata,meter_ene_buffer[meter_num].meter_ene,4);
				pdata += 4;
			}
			else
			{
				*pdata++ = (1<<((meter_num - 1)&0x07));
				*pdata++ = (meter_num-1)/8+1;
				memset(pdata,0xEE,6);
				pdata += 6;
				*pdata++ = meter_prd;
				memset(pdata,0xEE,5);
				pdata += 5;
			}
		}
		SysClockReadCurrent(&clock);
		ReadTimeToClock(&clock, meter_ene_buffer[meter_num].readtime, 0, &clock2);
		pdatasnd[0] = 0xFF;
		pdatasnd[1] = 0xFF;
		pdatasnd[2] = 0x01;
		pdatasnd[3] = 0x64;
		pdatasnd += 4;
		read_time[0] = clock2.minute;
		read_time[1] = clock2.hour;
		read_time[2] = clock2.day;
		read_time[3] = clock2.month;
		read_time[4] = clock2.year;
		HexToBcd(read_time, 5);
		memcpy(pdatasnd,read_time,5);
		pdatasnd += 5;
		*pdatasnd++ = ONE_FRAME_METER_CNT;
		memcpy(pdatasnd,meter_data,ONE_FRAME_METER_CNT*14);
		UPLINKAPP_LEN(psnd) = sendlen;//数据单元长度
		psnd->seq = fseq&0x0f;
		fseq++;
		if(seq_flag)
		{
			seq_flag = 0;
			psnd->seq |= UPSEQ_FIR;
			psnd->seq &= (~UPSEQ_FIN);
		}
		else
		{
			psnd->seq &= (~UPSEQ_FIR);
			psnd->seq &= (~UPSEQ_FIN);
		}
		SvrEchoSend(itf, psnd);
		frame_cnt--;
		pdatasnd = psnd->data;
		pdata = meter_data;
	}

	for(i=0;i<last_frame_meter_cnt;i++,meter_num++)
	{
		if(meter_ene_buffer[meter_num].read_stat == HAVE_READ)
		{
			*pdata++ = (1<<((meter_num - 1)&0x07));
			*pdata++ = (meter_num-1)/8+1;
			memcpy(pdata,meter_ene_buffer[meter_num].meter_addr,6);
			pdata += 6;
			*pdata++ = meter_prd;
			*pdata++ = 0;
			memcpy(pdata,meter_ene_buffer[meter_num].meter_ene,4);
			pdata += 4;
		}
		else
		{
			*pdata++ = (1<<((meter_num - 1)&0x07));
			*pdata++ = (meter_num-1)/8+1;
			memset(pdata,0xEE,6);
			pdata += 6;
			*pdata++ = meter_prd;
			memset(pdata,0xEE,5);
			pdata += 5;
		}
	}

	SysClockReadCurrent(&clock);
	ReadTimeToClock(&clock, meter_ene_buffer[meter_num - 1].readtime, 0, &clock2);
	pdatasnd[0] = 0xFF;
	pdatasnd[1] = 0xFF;
	pdatasnd[2] = 0x01;
	pdatasnd[3] = 0x64;
	pdatasnd += 4;
	read_time[0] = clock2.minute;
	read_time[1] = clock2.hour;
	read_time[2] = clock2.day;
	read_time[3] = clock2.month;
	read_time[4] = clock2.year;
	HexToBcd(read_time, 5);
	memcpy(pdatasnd,read_time,5);
	pdatasnd += 5;

	*pdatasnd++ = last_frame_meter_cnt;
	memcpy(pdatasnd,meter_data,last_frame_meter_cnt*14);
	//sendlen =  6 + last_frame_meter_cnt * 14;
	sendlen =  10 + last_frame_meter_cnt * 14;
	UPLINKAPP_LEN(psnd) = sendlen;//数据单元长度
	psnd->seq = fseq&0x0f;
	fseq++;
	psnd->seq |= UPSEQ_FIN;
	psnd->seq &= (~UPSEQ_FIR);
	SvrEchoSend(itf, psnd);
}



int get_one_day_meter_data(unsigned char *ScrDay,plmdb_day_t *ScrPlMdbDay)
{
	sysclock_t clock;
	//dbtime_t ScrPlQueryDbTime;
	dbtime_t dbtime;
	int rtn;

	clock.day = ScrDay[0];
	clock.month = ScrDay[1];
	clock.year = ScrDay[2];

	BcdToHex(&clock.day, 1);
	BcdToHex(&clock.month, 1);
	BcdToHex(&clock.year, 1);
	SYSCLOCK_DBTIME(&clock, dbtime);//根据冻结时间得到冻结文件
	rtn = ReadPlMdbDay(ScrPlMdbDay, dbtime);
	if(rtn > 0) 
	{
		printf("load %d mets plmdb day data\n", rtn);
	}
	else
	{
		return 1;
	}
	return 0;
}


int read_all_meter_class_two(unsigned char itf,uplink_pkt_t *rcv_pkt)
{
#define ONE_FRAME_METER_CNT		15
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	//dbtime_t dbtime;
	dbtime_t dbtime, dbtimecur;
	plmdb_day_t PlMdbDayTmp[MAX_PLCMET];
	unsigned char meter_data[ONE_FRAME_METER_CNT*14];
	unsigned char *pdata;
	int i;
	unsigned char *pdatasnd;
	int frame_cnt;
	int last_frame_meter_cnt;
	sysclock_t clock,clock2;
	unsigned char  seq_flag = 0;
	static unsigned char fseq = 0;
	unsigned short meter_num;
	unsigned char meter_prd;
	unsigned char read_time[5];
	int sendlen,rtn;
	unsigned short plmet_cnt = 0;
	dbtime_t PlQueryDbTime1;

	PlQueryDbTime1.s.day = rcv_pkt->data[4];
	PlQueryDbTime1.s.month = rcv_pkt->data[5];
	PlQueryDbTime1.s.year = rcv_pkt->data[6];
	BcdToHex(&PlQueryDbTime1.s.day, 3);
	PlQueryDbTime1.s.tick = 0;

	dbtime.u = PlQueryDbTime1.u;
	dbtime.s.tick = 0;
	{
		sysclock_t clock;
		SysClockReadCurrent(&clock);
		SYSCLOCK_DBTIME(&clock, dbtimecur);
		dbtimecur.s.tick = 0;
	}
	if(dbtime.u >= dbtimecur.u) return 1;

	//dbtime.s.tick = 0;
	//{
	//	sysclock_t clock1;
	//	SysClockReadCurrent(&clock1);
	//	SYSCLOCK_DBTIME(&clock1, dbtimecur);
	//}
	//DbTimeAddOneDay(&dbtime);
	//printf("dbtime.u = %d dbtimecur.u = %d \n",dbtime.u,dbtimecur.u);
	//if(dbtime.u > dbtimecur.u) return 1;

	meter_total_cnt = get_meter_total_cnt();
	cen_meter_cnt = get_cen_meter_total_cnt();
	plmet_cnt = meter_total_cnt - cen_meter_cnt;
	if(!(meter_total_cnt - cen_meter_cnt))	return 1;

	printf("read_all_meter_class_two...\n");
	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_QRYCLS2;
	fseq = rcv_pkt->seq&0x0f;
	frame_cnt = (plmet_cnt / ONE_FRAME_METER_CNT);
	last_frame_meter_cnt = (plmet_cnt % ONE_FRAME_METER_CNT);
	printf("frame_cnt = %d last_frame_meter_cnt = %d",frame_cnt,last_frame_meter_cnt);

	pdatasnd = psnd->data;
	
	pdata = meter_data;
	meter_prd = 0;
	meter_num = 0;
	//meter_num = 3;
	
	memset(meter_data,0x00,ONE_FRAME_METER_CNT*14);
	sendlen =  13 + ONE_FRAME_METER_CNT * 14;
	seq_flag = 1;
	memset(PlMdbDayTmp, 0xEE, sizeof(PlMdbDayTmp));

	SysClockRead(&clock);
	clock.day = rcv_pkt->data[4];
	clock.month = rcv_pkt->data[5];
	clock.year = rcv_pkt->data[6];
	BcdToHex(&clock.day, 1);
	BcdToHex(&clock.month, 1);
	BcdToHex(&clock.year, 1);
	SYSCLOCK_DBTIME(&clock, dbtime);//根据冻结时间得到冻结文件
	rtn = ReadPlMdbDay(PlMdbDayTmp, dbtime);
	if(rtn > 0) 
	{
		printf("load %d mets plmdb day data\n", rtn);
	}
	else
	{
		//SvrEchoNoData(itf, psnd);
		//return;
		return 1;
	}
	//printf("read_all_meter_class_two...2\n");
	while(frame_cnt)
	{
		for(i=0;i<ONE_FRAME_METER_CNT;i++,meter_num++)
		{
			printf("PlMdbDayTmp[%d].meter_addr[0] = %x\n", meter_num,PlMdbDayTmp[meter_num].meter_addr[0]);
			//if(PlMdbDayTmp[meter_num].meter_addr[0] == 0xEE)	continue;
			if((PlMdbDayTmp[meter_num].read_stat & 0xFF) == HAVE_READ)
			{
				//printf("read_all_meter_class...1\n");
				*pdata++ = (1<<((PlMdbDayTmp[meter_num].meter_num - 1)&0x07));
				*pdata++ = (PlMdbDayTmp[meter_num].meter_num-1)/8+1;
				//printf("read_all_meter_class...2\n");
				memcpy(pdata,PlMdbDayTmp[meter_num].meter_addr,6);
				pdata += 6;
				*pdata++ = meter_prd;
				*pdata++ = 0;
				memcpy(pdata,PlMdbDayTmp[meter_num].meter_ene,4);
				//printf("read_all_meter_class...3\n");
				pdata += 4;
				//printf("read_all_meter_class_two...3\n");
			}
			else
			{
				*pdata++ = (1<<((PlMdbDayTmp[meter_num].meter_num - 1)&0x07));
				*pdata++ = (PlMdbDayTmp[meter_num].meter_num-1)/8+1;
				memset(pdata,0xEE,6);
				pdata += 6;
				*pdata++ = meter_prd;
				memset(pdata,0xEE,5);
				pdata += 5;
				//printf("read_all_meter_class_two...4\n");
			}
		}
		//printf("read_all_meter_class_two...5\n");
		SysClockReadCurrent(&clock);
		ReadTimeToClock(&clock, PlMdbDayTmp[meter_num].readtime, 0, &clock2);
		pdatasnd[0] = 0xFF;
		pdatasnd[1] = 0xFF;
		pdatasnd[2] = 0x01;
		pdatasnd[3] = 0x64;
		pdatasnd += 4;
		
		memcpy(pdatasnd,(unsigned char *)&rcv_pkt->data[4],3);
		pdatasnd += 3;
		
		read_time[0] = clock2.minute;
		read_time[1] = clock2.hour;
		read_time[2] = clock2.day;
		read_time[3] = clock2.month;
		read_time[4] = clock2.year;
		HexToBcd(read_time, 5);
		memcpy(pdatasnd,read_time,5);
		pdatasnd += 5;
		*pdatasnd++ = ONE_FRAME_METER_CNT;
		memcpy(pdatasnd,meter_data,ONE_FRAME_METER_CNT*14);
		UPLINKAPP_LEN(psnd) = sendlen;//数据单元长度
		psnd->seq = fseq&0x0f;
		fseq++;
		if(seq_flag)
		{
			seq_flag = 0;
			psnd->seq |= UPSEQ_FIR;
			psnd->seq &= (~UPSEQ_FIN);
		}
		else
		{
			psnd->seq &= (~UPSEQ_FIR);
			psnd->seq &= (~UPSEQ_FIN);
		}
		SvrEchoSend(itf, psnd);
		frame_cnt--;
		pdatasnd = psnd->data;
		pdata = meter_data;
	}

	//printf("read_all_meter_class_two...6\n");
	for(i=0;i<last_frame_meter_cnt;i++,meter_num++)
	{
		printf("PlMdbDayTmp[%d].meter_addr[0] = %x\n", meter_num,PlMdbDayTmp[meter_num].meter_addr[0]);
		//if(PlMdbDayTmp[meter_num].meter_addr[0] == 0xEE)	continue;
		if((PlMdbDayTmp[meter_num].read_stat & 0xFF) == HAVE_READ)
		{
			*pdata++ = (1<<((PlMdbDayTmp[meter_num].meter_num - 1)&0x07));
			*pdata++ = (PlMdbDayTmp[meter_num].meter_num-1)/8+1;
			memcpy(pdata,PlMdbDayTmp[meter_num].meter_addr,6);
			pdata += 6;
			*pdata++ = meter_prd;
			*pdata++ = 0;
			memcpy(pdata,PlMdbDayTmp[meter_num].meter_ene,4);
			pdata += 4;
		}
		else
		{
			*pdata++ = (1<<((PlMdbDayTmp[meter_num].meter_num - 1)&0x07));
			*pdata++ = (PlMdbDayTmp[meter_num].meter_num-1)/8+1;
			//*pdata++ = (1<<((meter_num - 1)&0x07));
			//*pdata++ = (meter_num-1)/8+1;
			memset(pdata,0xEE,6);
			pdata += 6;
			*pdata++ = meter_prd;
			memset(pdata,0xEE,5);
			pdata += 5;
		}
	}
	//printf("read_all_meter_class_two...7\n");
	SysClockReadCurrent(&clock);
	ReadTimeToClock(&clock, PlMdbDayTmp[meter_num - 1].readtime, 0, &clock2);
	pdatasnd[0] = 0xFF;
	pdatasnd[1] = 0xFF;
	pdatasnd[2] = 0x01;
	pdatasnd[3] = 0x64;
	pdatasnd += 4;

	memcpy(pdatasnd,(unsigned char *)&rcv_pkt->data[4],3);
	pdatasnd += 3;
	
	read_time[0] = clock2.minute;
	read_time[1] = clock2.hour;
	read_time[2] = clock2.day;
	read_time[3] = clock2.month;
	read_time[4] = clock2.year;
	HexToBcd(read_time, 5);
	memcpy(pdatasnd,read_time,5);
	pdatasnd += 5;

	*pdatasnd++ = last_frame_meter_cnt;
	memcpy(pdatasnd,meter_data,last_frame_meter_cnt*14);
	sendlen =  13 + last_frame_meter_cnt * 14;
	UPLINKAPP_LEN(psnd) = sendlen;//数据单元长度
	psnd->seq = fseq&0x0f;
	fseq++;
	psnd->seq |= UPSEQ_FIN;
	psnd->seq &= (~UPSEQ_FIR);
	SvrEchoSend(itf, psnd);
	return 0;
}





/**
* @brief 查询一类数据命令处理
* @param itf 接口编号
*/
static void SvrQueryClassOne(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	int rcvlen, sendlen, actlen, rtn, sendmax;
	unsigned char *pdatarcv, *pdatasnd;

	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_QRYCLS1;

	pdatarcv = prcv->data;
	pdatasnd = psnd->data;
	rcvlen = UPLINKAPP_LEN(prcv);//得到接收数据的长度
	sendlen = 0;
	sendmax = UPLINK_SNDMAX(itf);//发送数据最大长度


	if(prcv->data[2]  == 0x01 && prcv->data[3]  == 0x64)
	{
		read_all_meter_class_one(itf,prcv);
		return;
	}
	else
	{
		while(rcvlen >= 4) 
		{
			actlen = 0;
			//if(pdatarcv[1] > 1) 
			//{
			//	rtn = ReadPlMdb(pdatarcv, pdatasnd, sendmax, &actlen);
			//}
			//else 
			//{
			//	rtn = ReadMdb(pdatarcv, pdatasnd, sendmax, &actlen);//p0集中器和p1到p8 台变多功能表
			//}
			if(pdatarcv[0]<=0x02 && pdatarcv[1]<=0x01)
			{
				rtn = ReadMdb(pdatarcv, pdatasnd, sendmax, &actlen);//p0集中器和p1到p8 台变多功能表
			}
			else
			{
				rtn = ReadPlMdb(pdatarcv, pdatasnd, sendmax, &actlen);
			}


			rcvlen -= 4;
			pdatarcv += 4;

			if(!rtn) 
			{
				sendmax -= actlen;
				if(sendmax < 0) 
				{
					ErrorLog("send max error(%d,%d)\n", sendmax,actlen);
					goto mark_end;
				}
				pdatasnd += actlen;
				sendlen += actlen;
			}
		}
	}

	if(0 == sendlen) goto mark_end;

	UPLINKAPP_LEN(psnd) = sendlen;//数据单元长度
	SvrEchoSend(itf, psnd);
	return;

mark_end:
	if(0 == sendlen) 
	{
		SvrEchoNoData(itf, psnd);
		return;
	}

	UPLINKAPP_LEN(psnd) = sendlen;
	SvrEchoSend(itf, psnd);
}

/**
* @brief 查询二类数据命令处理
* @param itf 接口编号
*/
static void SvrQueryClassTwo(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	qrycurve_buffer_t *cache;//定义一个指向链表的临时指针
	int first;
	unsigned char seq = 0;
	unsigned char *pn = prcv->data;

	printf("SvrQueryClassTwo...\n");
	if(UPLINKAPP_LEN(prcv) < 6) return;
	QueryCacheLock();//
	if((prcv->data[2]  == 0x01) && (prcv->data[3]  == 0x64))
	{
		if(read_all_meter_class_two(itf,prcv))
		{
			SvrEchoNoData(itf, psnd);
		}
		QueryCacheUnlock();
		return;
	}
	else
	{
		printf("SvrQueryClassTwo1...\n");
		cache = GetQueryCache(UPLINK_RCVMAX(itf));
		//if(pn[1] < 2) QueryCurve(prcv->data,UPLINKAPP_LEN(prcv), cache);//p0~p8 台变多功能表
		//else QueryPlCurve(prcv->data,UPLINKAPP_LEN(prcv), cache);//查询载波表

		if((pn[0]<=0x02) && (pn[1]<=0x01)) QueryCurve(prcv->data,UPLINKAPP_LEN(prcv), cache);//p0~p8 台变多功能表
		else QueryPlCurve(prcv->data,UPLINKAPP_LEN(prcv), cache);//查询载波表
		
		if(0 == cache->datalen) 
		{
			SvrEchoNoData(itf, psnd);
			QueryCacheUnlock();
			return;
		}
		first = 1;
		for(; NULL!=cache; cache=cache->next) 
		{
			DebugPrint(0, "cache=%08XH, buff=%08XH, next=%08XH, max=%d, datalen=%d\n",
				cache, cache->buffer, cache->next, cache->maxlen, cache->datalen);
			if(cache->datalen <= 0) break;
			psnd = (uplink_pkt_t *)(cache->buffer - OFFSET_UPDATA);//链表缓冲区
			psnd->ctrl = UPECHO_USRDATA;
			psnd->afn = UPAFN_QRYCLS2;//二类数据
			psnd->grp = prcv->grp;
			psnd->grp &= 0xfe;
			if(first) 
			{
				first = 0;
				if(cache->next->datalen > 0) {  //muti packet
					//DebugPrint(0, "multi packet\n");
					seq = prcv->seq & 0x0f;
					psnd->seq = seq++;
					psnd->seq |= UPSEQ_FIR;
				}
				else { //single packet
					psnd->seq = prcv->seq & 0x0f;
					psnd->seq |= UPSEQ_SPKT;
				}
			}
			else //
			{
				psnd->seq = seq++ & 0x0f;
				if(NULL == cache->next || cache->next->datalen <= 0)
					psnd->seq |= UPSEQ_FIN;  //last
			}
			UPLINKAPP_LEN(psnd) = cache->datalen;//数据单元长度
			SvrEchoSend(itf, psnd);
		}
	}

	QueryCacheUnlock();
}

/**
* @brief 查询三类数据命令处理
* @param itf 接口编号
*/
static void SvrQueryClassThree(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	qrycurve_buffer_t *cache;
	int first;
	unsigned char seq = 0;

	if(UPLINKAPP_LEN(prcv) < 6) return;

	QueryCacheLock();

	cache = GetQueryCache(UPLINK_RCVMAX(itf));

	QueryAlarm(prcv->data, cache);

	if(0 == cache->datalen) {
		QueryCacheUnlock();
		SvrEchoNoData(itf, psnd);
		return;
	}

	first = 1;
	for(; NULL!=cache; cache=cache->next) {
		if(cache->datalen <= 0) break;

		psnd = (uplink_pkt_t *)(cache->buffer - OFFSET_UPDATA);
		psnd->ctrl = UPECHO_USRDATA;
		psnd->afn = UPAFN_QRYCLS3;
		if(first) {
			first = 0;
			if(cache->next->datalen > 0) {  //muti packet
				seq = prcv->seq & 0x0f;
				psnd->seq = seq++;
				psnd->seq |= UPSEQ_FIR;
			}
			else { //single packet
				psnd->seq = prcv->seq & 0x0f;
				psnd->seq |= UPSEQ_SPKT;
			}
		}
		else {
			psnd->seq = seq++ & 0x0f;
			if(NULL == cache->next || cache->next->datalen <= 0)
				psnd->seq |= UPSEQ_FIN;  //last
		}

		UPLINKAPP_LEN(psnd) = cache->datalen;
		SvrEchoSend(itf, psnd);
	}

	QueryCacheUnlock();
}

/**
* @brief 级联命令处理
* @param itf 接口编号
*/
static void SvrCascadeRequest(unsigned char itf)
{
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);

	if(CASCADE_CLIENT != CascadeMode) return;

	if(!CascadeNoted) {
		SvrEchoNoData(itf, psnd);
		return;
	}

	if(!SvrNoteProc(itf)) {
		CascadeNoted = 0;
		SvrEchoNoData(itf, psnd);
	}
}

/**
* @brief 自定义命令处理
* @param itf 接口编号
*/
static void SvrSelfCmd(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	unsigned char *puc;
	unsigned short len;

	len = UPLINKAPP_LEN(prcv);

	if(len < 9) return;

	puc = prcv->data + 4;
	prcv->afn = *puc++;
	len -= 5;

	//memcpy(prcv->data, puc, len);
	smallcpy(prcv->data, puc, len);
	UPLINKAPP_LEN(prcv) = len;

	SelfCmdFlag[itf] = 1;
	SvrMessageProc(itf);
	SelfCmdFlag[itf] = 0;
}

extern void SvrSelfTranFile(unsigned char itf);

/**
* @brief 文件传输命令处理
* @param itf 接口编号
*/

static void SvrTranFile(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	unsigned char *puc;

	if(UPLINKAPP_LEN(prcv) < 4) return;

	puc = prcv->data;
	if(0 != puc[0] || 0 != puc[1] || 1 != puc[3]) return;

	if(1 == puc[2]) SvrSelfCmd(itf);  //F9
	else if(2 == puc[2]) SvrSelfTranFile(itf);
	else if(4 == puc[2]) SvrShellProc(itf);

	return;
}

/**
* @brief 透明转发命令处理
* @param itf 接口编号
*/
static void SvrForward(unsigned char itf)
{
	//@change later: 需要添加载波表
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	unsigned char *puc = prcv->data;
	metfrwd_t *pcmd;
	metfrwd_echo_t *pecho;
	unsigned short len;
	int rtn;

	if(0 != puc[0] || 0 != puc[1]) return;//非P0

	if(1 == puc[3]) goto mark_plc;//对应规约组2

	if(1 != puc[2] || 0 != puc[3]) return; //p0, F1

	pcmd = (metfrwd_t *)&puc[4];

	puc = psnd->data;
	puc[0] = puc[1] = 0;
	puc[2] = 1;
	puc[3] = 0;
	pecho = (metfrwd_echo_t *)&puc[4];

	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_FORWARD;

	CenMetForward(pcmd, pecho);
	len = ((unsigned short)pecho->len[1]<<8) + (unsigned short)pecho->len[0];
	len += 7;
	UPLINKAPP_LEN(psnd) = len;

	SvrEchoSend(itf, psnd);
	return;

mark_plc:

	rtn = (int)UPLINKAPP_LEN(prcv)&0xffff;
	rtn = PlcForward(puc, rtn, psnd->data, UPLINK_SNDMAX(itf));
	if(rtn <= 0) {
		SvrEchoNoData(itf, psnd);
	}
	else {
		psnd->ctrl = UPECHO_USRDATA;
		psnd->afn = UPAFN_FORWARD;
		UPLINKAPP_LEN(psnd) = rtn;
		SvrEchoSend(itf, psnd);
	}
}

struct srvmsg_func_t {
	unsigned char needpass;
	void (*pf)(unsigned char itf);
};
static const struct srvmsg_func_t srvmsg_func[] = {
	{0, NULL}, /*0x00, UPAFN_ECHO*/    
	{1, SvrReset}, /*0x01, UPAFN_RESET*/   
	//{0, SvrReset}, /*0x01, UPAFN_RESET*/   
	{0, NULL}, /*0x02, UPAFN_LINKTEST*/
	{0, NULL}, /*0x03, UPAFN_RELAY*/
	{1, SvrSetParam}, /*0x04, UPAFN_SETPARA*/
	//{0, SvrSetParam}, /*0x04, UPAFN_SETPARA*/
	{1, SvrCtrlCmd}, /*0x05, UPAFN_CTRL*/
	{0, NULL}, /*0x06*/
	{0, NULL}, /*0x07*/
	{0, SvrCascadeRequest}, /*0x08, UPAFN_CASCADE*/
	{0, SvrReqConfig}, /*0x09, UPAFN_REQCFG*/
	{0, SvrQueryParam}, /*0x0a, UPAFN_QRYPARA*/
	{0, NULL}, /*0x0b, UPAFN_QRYTASK*/
	{0, SvrQueryClassOne}, /*0x0c, UPAFN_QRYCLS1*/
	{0, SvrQueryClassTwo}, /*0x0d, UPAFN_QRYCLS2*/
	{0, SvrQueryClassThree}, /*0x0e, UPAFN_QRYCLS3*/
	{1, SvrTranFile}, /*0x0f, UPAFN_TRANFILE*/
	//{0, SvrTranFile}, /*0x0f, UPAFN_TRANFILE*/
	{1, SvrForward}, /*0x10, UPAFN_FORWARD*/
	{0, NULL}, /*0x11*/
	{0, NULL}, /*0x12*/
	{0, NULL}, /*0x13*/
};
#define SVRAFN_MAX		(sizeof(srvmsg_func)/sizeof(srvmsg_func[0]))

static unsigned char SvrAddrRecord = 0;   //记录主站参数变更事件用
//static unsigned char CountPwdError[UPLINKITF_NUM] = {0, 0, 0, 0};
#define MAXCNT_PWDERR    3

#if 0
/**
* @brief 生成密码错误告警
* @param pwd 错误的密码
*/
static void MakePwdErrAlarm(unsigned char *pwd)
{
	alarm_t buf;

	memset((unsigned char *)&buf, 0, sizeof(alarm_t));
	buf.erc = 20;
	buf.len = 17;
	buf.data[0] = pwd[0];
	buf.data[1] = pwd[1];
	buf.data[16] = SvrAddrRecord;

	MakeAlarm(ALMFLAG_ABNOR, &buf);
}
#endif

/**
* @brief 服务器命令处理
* @param itf 接口编号
*/
void SvrMessageProc(unsigned char itf)
{
	printf("SvrMessageProc...\n");
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	const struct srvmsg_func_t *plist;
	unsigned char *paux;

	//DebugPrint(LOGTYPE_UPLINK, "srv msg proc, len=%d, f=%02X\r\n", UPLINKAPP_LEN(prcv), prcv->data[0]);

	//if(UplinkCheckEchoPkt(prcv)) return;
	if(0 == (prcv->ctrl&UPCTRL_PRM)) return;
	if(prcv->afn >= SVRAFN_MAX) return;
	plist = &srvmsg_func[prcv->afn];
	if(NULL == plist->pf) return;

	TpvFlag[itf] = 0;
	if(prcv->seq&UPSEQ_TPV) 
	{
		TpvFlag[itf] = 1;
		if(UPLINKAPP_LEN(prcv) <= LEN_UPLINK_TIMETAG) return;

		paux = prcv->data;
		paux += UPLINKAPP_LEN(prcv);
		paux -= LEN_UPLINK_TIMETAG;
		TimeTag[itf].pfc = paux[0];
		TimeTag[itf].time[0] = paux[1];
		TimeTag[itf].time[1] = paux[2];
		TimeTag[itf].time[2] = paux[3];
		TimeTag[itf].time[3] = paux[4];
		TimeTag[itf].dly = paux[5];

		if(TimeTag[itf].dly) 
		{  //计算时间有效
			sysclock_t clk;
			utime_t time1, time2;
			int diff;

			SysClockReadCurrent(&clk);
			clk.day = TimeTag[itf].time[3];
			clk.hour = TimeTag[itf].time[2];
			clk.minute = TimeTag[itf].time[1];
			BcdToHex(&(clk.day), 3);
			time1 = SysClockToUTime(&clk);
			time2 = UTimeReadCurrent();
			if(time1 > time2) diff = time1 - time2;
			else diff = time2 - time1;
			diff /= 60;
			if(diff > ((int)TimeTag[itf].dly&0xff)) return;
		}

		UPLINKAPP_LEN(prcv) -= LEN_UPLINK_TIMETAG;
	}

	psnd->seq = prcv->seq & 0x0f;
	psnd->seq |= UPSEQ_SPKT;
	psnd->grp = prcv->grp;
	psnd->grp &= 0xfe;
	SvrAddrRecord = (prcv->grp)>>1;

	if(plist->needpass) 
	{
		/*
		if(UPLINKAPP_LEN(prcv) <= 2) return;

		paux = prcv->data;
		paux += UPLINKAPP_LEN(prcv);
		paux -= 2;

		if(CheckPassword(paux, &prcv->ctrl)) {
			if(CountPwdError[itf] < MAXCNT_PWDERR) {
				CountPwdError[itf]++;
				if(CountPwdError[itf] >= MAXCNT_PWDERR) {
					MakePwdErrAlarm(paux);
				}
			}
			return;
		}
		CountPwdError[itf] = 0;
		*/
		//UPLINKAPP_LEN(prcv) -= 2;
		UPLINKAPP_LEN(prcv) -= 16;
	}
	(*plist->pf)(itf);
	return;
}

/**
* @brief 检查AFN是否为终端会响应的命令
* @param afn AFN
* @return 会响应返回1, 否则返回0
*/
int IsSvrMsgAfn(unsigned char afn)
{
	if(afn >= SVRAFN_MAX) return 0;

	if(srvmsg_func[afn].pf != NULL) return 1;
	else return 0;
}

/**
* @brief 消息处理模块初始化
*/
DECLARE_INIT_FUNC(SvrMessgeInit);
int SvrMessgeInit(void)
{
	SysInitMutex(&QueryCacheMutex);

	SET_INIT_FLAG(SvrMessgeInit);
	return 0;
}

/*
int is_svrmsg_cmd(unsigned char afn)
{
	srvmsg_func_t *plist = (srvmsg_func_t *)srvmsg_func;

	while(0xff != plist->afn) {
		if(afn == plist->afn) return 1;

		plist++;
	}

	return 0;
}
*/

