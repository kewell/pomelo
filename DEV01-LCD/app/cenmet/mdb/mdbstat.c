/**
* mdbstat.c -- 终端状态数据
* 
* 
* 创建时间: 2010-5-13
* 最后修改时间: 2010-5-13
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/lib/bcd.h"
#include "include/param/capconf.h"
#include "include/param/mix.h"
#include "include/monitor/alarm.h"
#include "include/monitor/runstate.h"
#include "mdbstic.h"
#include "include/plcmet/pltask.h"
/**
* @brief 读取当前时间
* @param buf 缓存区指针
* @param plen 返回的缓存区长度
* @return 成功0, 否则失败(参数和返回值以下同类函数相同)
*/
static int MdbStatTime(unsigned char *buf, int *plen)
{
	sysclock_t clk;

	*plen = 6;

	SysClockReadCurrent(&clk);

	buf[0] = clk.second;
	buf[1] = clk.minute;
	buf[2] = clk.hour;
	buf[3] = clk.day;
	buf[4] = clk.month;
	buf[5] = clk.year;
	HexToBcd(buf, 6);

	buf[4] &= 0x1f;
	if(clk.week == 0) clk.week = 7;
	buf[4] |= (clk.week<<5)&0xe0;
	return 0;
}

static const unsigned char FlagParaValid[31] = {
	0xfd, 0xef, 0x10, 0x7f, 0x7f, 0x00, 0x00, 0x0c, 
	0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};
/**
* @brief 读取参数状态
*/
static int MdbStatDataValid(unsigned char *buf, int *plen)
{
	*plen = 31;
	smallcpy(buf, (unsigned char *)FlagParaValid, 31);
	return 0;
}

/**
* @brief 读取上行通信状态
*/
static int MdbStatComm(unsigned char *buf, int *plen)
{
	*plen = 1;

	*buf = 0x08;

	if(ParaMix.bactsend) *buf |= 0x02;
	else *buf |= 0x01;

	return 0;
}

/**
* @brief 读取事件计数器当前值
*/
static int MdbStatAlarmEc(unsigned char *buf, int *plen)
{
	*plen = 2;

	GetAlarmEc(buf);

	return 0;
}

/**
* @brief 读取事件状态
*/
static int MdbStatAlarmState(unsigned char *buf, int *plen)
{
	*plen = 8;

	smallcpy(buf, RunState.alarm.stat, 8);
	memset(RunStateModify()->alarm.stat, 0, 8);
	return 0;
}

/**
* @brief 读取输入状态量
*/
static int MdbStatIsig(unsigned char *buf, int *plen)
{
	*plen = 2;
	buf[0] = RunState.isig_stat;
	buf[1] = RunState.isig_chg;
	RunStateModify()->isig_chg = 0;
	return 0;
}

static int MdbStatCommBytes(unsigned char *buf, int *plen)
{
	*plen = 8;
	smallcpy(buf, &MdbStic.term_day.comm_bytes, 4);
	smallcpy(buf+4, &MdbStic.term_mon.comm_bytes, 4);
	return 0;
};
//extern unsigned char read_meter_finish;
//extern unsigned short meter_total_cnt;
//extern unsigned short meter_read_succ_cnt;

//extern sysclock_t read_meter_start_time;
//extern sysclock_t read_meter_finish_time;
//extern unsigned short imp_meter_read_succ_cnt;

static int MdbStatReadMeter(unsigned char *buf, int *plen)
{
	unsigned char read_meter_flag = 0;
	unsigned char *pdata = buf;
	unsigned short plmet_cnt = 0 ;
	//int i = 0;

	meter_total_cnt = get_meter_total_cnt();
	cen_meter_cnt = get_cen_meter_total_cnt();
	//imp_meter_cnt = get_imp_meter_total_cnt();
	imp_meter_read_succ_cnt = get_imp_meter_read_succ_cnt();
	plmet_cnt = meter_total_cnt - cen_meter_cnt;
	
	*pdata++ = 0x01;
	*pdata++ = PLC_PORT;
	smallcpy(pdata, &plmet_cnt, 2);
	pdata += 2;
	if(read_meter_finish)
	{
		read_meter_flag = 0x02;
	}
	else
	{
		read_meter_flag = 0x01;
	}
	*pdata++ = read_meter_flag;
	smallcpy(pdata, &meter_read_succ_cnt, 2);
	pdata += 2;
	*pdata++ = imp_meter_read_succ_cnt;//重点表
	*pdata++ = 	read_meter_start_time.second;
	*pdata++ = 	read_meter_start_time.minute;
	*pdata++ = 	read_meter_start_time.hour;
	*pdata++ = 	read_meter_start_time.day;
	*pdata++ = 	read_meter_start_time.month;
	*pdata++ = 	read_meter_start_time.year;	
	*pdata++ = 	read_meter_finish_time.second;
	*pdata++ = 	read_meter_finish_time.minute;
	*pdata++ = 	read_meter_finish_time.hour;
	*pdata++ = 	read_meter_finish_time.day;
	*pdata++ = 	read_meter_finish_time.month;
	*pdata++ = 	read_meter_finish_time.year;	
	HexToBcd(pdata-12, 12);
	*plen = 20;
	return 0;
}




//@change later
#if 0
extern int print_metproto_info(unsigned char *buf);
static int mdbstatf_metproto(unsigned char *buf, int *plen)
{
	*plen = print_metproto_info(buf);

	return 0;
}

static int mdbstatf_termstat(unsigned char *buf, int *plen)
{
	*plen = 7;

	if(run_state.pwroff) buf[0] = 1;
	else buf[0] = 0;
	if(run_state.battery) buf[1] = 1;
	else buf[1] = 0;
	if(run_state.isig_stat&0x80) buf[2] = 1;
	else buf[2] = 0;

	buf += 3;
	*buf++ = gprsdevstat_dial;
	*buf++ = svrcomm_linestat;
	*buf++ = gprsdevstat_dev;
	*buf = gprsdevstat_sig;

	return 0;
}

extern int read_malm_stat(unsigned char *buf);
static int mdbstatf_metalm(unsigned char *buf, int *plen)
{
	*plen = read_malm_stat(buf);
	return 0;
}

void imet_print(void);
static int mdbstatf_imet(unsigned char *buf, int *plen)
{
	*plen = 1;
	*buf = 0;

	imet_print();

	return 0;
}

#endif

typedef int (*mdbstat_pf)(unsigned char *buf, int *plen);

typedef struct {
	unsigned short id;
	mdbstat_pf pfunc;
} mdbfuncs_t;

static const mdbfuncs_t mdb_funcs[] = {
	{0x0002, MdbStatTime},//F2
	{0x0004, MdbStatDataValid},//F3
	{0x0008, MdbStatComm},//F4
	{0x0040, MdbStatAlarmEc},//F7
	{0x0080, MdbStatAlarmState},//F8

	{0x0101, MdbStatIsig},//F9
	{0x0102, MdbStatCommBytes},//F10
	{0x0104, MdbStatReadMeter},//F11

	/*
	{0x1402, mdbstatf_imet},
	{0x1404, mdbstatf_metproto},
	{0x1408, mdbstatf_metalm},*/

	{0, NULL},
};

/**
* @brief 读取当前状态数据
* @param metpid 测量点号, 1~MAX_CENMETP
* @param itemid 数据项编号
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度, 失败返回负数, 无此数据项返回-2, 缓存区溢出返回-1
*/
int ReadMdbState(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len)
{
	mdbfuncs_t *plist = (mdbfuncs_t *)mdb_funcs;
	int alen;

	if(0 != metpid) return 1;

	for(; 0 != plist->id; plist++) {
		if(itemid == plist->id) {
			if(NULL == plist->pfunc) return -2;

			alen = 0;
			if((*plist->pfunc)(buf, &alen)) return -2;

			if(alen > len) return -1;
			return alen;
		}
	}

	if(0 == plist->id) return -1;

	return -1;
}

