/**
* pltask.c -- 载波表任务
* 
* 
* 创建时间: 2010-5-29
* 最后修改时间: 2010-5-29
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>



#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#include "include/sys/key_board.h"

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/event.h"
#include "include/sys/task.h"
#include "include/sys/bin.h"
#include "include/sys/schedule.h"
#include "include/lib/dbtime.h"
#include "include/lib/bcd.h"
#include "include/param/meter.h"
#include "include/param/commport.h"
#include "include/param/metp.h"
#include "include/param/mix.h"
#include "include/param/operation.h"
#include "include/param/unique.h"
#include "include/monitor/runstate.h"
#include "param/operation_inner.h"


#include "plcomm.h"
#include "plc_stat.h"
#include "include/debug/shellcmd.h"
#include "include/plcmet/pltask.h"
#include "include/plcmet/plmdb.h"
#include "include/sys/mutex.h"
#include "include/sys/timeal.h"
#include "include/sys/gpio.h"

#include "include/lib/align.h"
#include "plcmet/module/plstatic_dl.h"
#include "include/sys/uart.h"
#include "pltask.h"

static sys_mutex_t Plc_Mutex;
#define PLC_LOCK	SysLockMutex(&Plc_Mutex)
#define PLC_UNLOCK	SysUnlockMutex(&Plc_Mutex)
static sys_event_t plc_meter_event;
static unsigned char ruter_is_busy = 1;
static unsigned char get_ruter_stat = 0;
static unsigned char active_rgst_start = 0;
static unsigned char active_rgst_command = 0;
static unsigned char add_meter_start = 0;

unsigned short meter_total_cnt = 0;
unsigned short cen_meter_cnt = 0;
unsigned short imp_meter_cnt = 0;
unsigned short imp_meter_read_succ_cnt = 0;
unsigned short meter_read_succ_cnt = 0;
int PlcTask_watchdog = 0;
int PlFrezTask_watchdog = 0;
static unsigned char set_ruter_work_mode_start = 0;
static unsigned int set_ruter_work_mode_start_times = 0;
static unsigned int query_pl_node_time_out = 0; 
static unsigned int query_pl_node_finish = 0; 
static unsigned short query_pl_node_cnt = 0;
static unsigned char point_read_start = 0;
static unsigned char clear_read_meter_stat_flag = 0;
static unsigned char pl_shell_cmd_flag = 0;
static unsigned char stop_point_read_flag = 0;
unsigned int start_point_read_timer = 0;
unsigned int stop_point_read_timer = 0;
unsigned char read_meter_finish = 0;
unsigned char read_meter_finish_frez = 0;
unsigned char read_meter_major_frez = 0;
unsigned char read_meter_major_frez_flag = 0;
static unsigned short point_read_index = 0;
static unsigned short one_time_point_read_stat = 0;
static unsigned int point_read_cyc = 0;
static unsigned short meter_type = 0;

sysclock_t read_meter_start_time;
sysclock_t read_meter_finish_time;
sysclock_t cyc_read_meter_start_time;

meter_ene_buf_t meter_ene_buffer[MAX_METER_CNT];
pl_node_info_t pl_node_info[MAX_METER_CNT];
pl_read_meter_stic_t pl_read_meter_stic[31];
pl_cjq_info_t pl_cjq_info[1024];
static unsigned short add_meter_index = 0;
static unsigned short add_meter_fail_cnt = 0;
static unsigned short add_meter_succ_cnt = 0;
/*#define PLCEV_DAY		1
#define PLCEV_WAIT		PLCEV_DAY*/

static unsigned char PlReadStopped = 0;
static unsigned char PlReadStoppedFlag = 0;
//static unsigned char PlMonthReading = 0;
static unsigned char PlNeedFrez = 0;
static unsigned char query_pl_node_info = 0;
static unsigned char query_pl_node_info_start = 0;
static unsigned char PlFrezFinished = 0;
//static unsigned char PlStartCheckTime = 0;//启动定时器广播校时
//static unsigned short PlCurMetid = 0;
//static unsigned short PlCurImpMetid = 0;
//static sys_event_t PlcEvent;

static int PlCycCount = 1;
//static int PlCycStartFlag = 0;//轮次抄表开始标志
static int PlcTaskReseting = 0;//重新抄表标志
//static int PlReadMetEnd = 0;//最后一块表抄收结束标志




static inline unsigned short clock_to_read_time(const sysclock_t *clock, int flag)
{
	unsigned short us;

	us = (unsigned short)clock->hour * 60 + (unsigned short)clock->minute;
	if(flag) {
		unsigned short us2 = clock->day;

		if(us2 == 0 || us2 > 31) us2 = 1;
		us += (us2-1)*1440;
	}

	return us;
}





#if 0

/**
* @brief 检查抄日冻结数据是否完成
* @param metid 电表号
* @return 完成返回1, 未完返回0
*/
static inline int PlcMetDayFinished(unsigned short metid)
{
	if(PLTIME_EMTPY != PlMdbDay[metid-PLC_BASEMETP].readtime) return 1;

	return 0;
}

/**
* @brief 检查抄月冻结数据是否完成
* @param metid 电表号
* @return 完成返回1, 未完返回0
*/
static inline int PlcMetMonthFinished(unsigned short metid)
{
	unsigned mid = metid - PLC_BASEMETP;
	int i, fenum;

	fenum = (int)ParaMeter[metid].prdnum & 0xff;
	if(fenum > MAX_PLMET_FENUM) fenum = 0;

	fenum = (fenum+1)<<2;

	for(i=0; i<fenum; i++) {
		if(PLDATA_EMPTY == PlMdbMonth[mid].ene[i]) return 0;
	}

	return 1;
}

/**
* @brief 检查抄重点用户抄表是否完成
* @param kmid 重点用户号
* @return 完成返回1, 未完返回0
*/
static inline int PlcMetImpFinished(unsigned short kmid)
{
	sysclock_t clock;
	int offset;

	SysClockReadCurrent(&clock);
	offset = clock.hour;
	offset <<= 2;

	if(clock.minute > MAX_IMPORTANT_USER) return 1;
	if(PLDATA_EMPTY != PlMdbImp[kmid].ene[offset]) return 1;

	return 0;
}
/**
* @brief 读取并更新日冻结数据
* @param metid 电表号
*/
static void ReadPlcMetDay(unsigned short metid)
{
	plc_dest_t dest;
	unsigned char databuf[4];

	PrintLog(LOGTYPE_DOWNLINK, "read met%d day..\n", metid);

	PlCycState[0].rd_mets++;
	if(PlCycCount < MAX_PLCYCSTAT) PlCycState[PlCycCount].rd_mets++;

	MakePlcDest(metid, &dest);//根据测量点号得到测量点地址

	if(PlcRead(&dest, 0x9090, databuf, 5) > 0) 
	{
		DebugPrint(LOGTYPE_DOWNLINK, "9010=%02X%02X%02X.%02X\n", 
			databuf[3], databuf[2], databuf[1], databuf[0]);
		UpdatePlMdb(metid, 0x9210, databuf, 4);
		PlCycState[0].ok_mets++;
		if(PlCycCount < MAX_PLCYCSTAT) PlCycState[PlCycCount].ok_mets++;
	}
}

/**
* @brief 读取并更新月冻结数据
* @param metid 电表号
*/
static void ReadPlcMetMonth(unsigned short metid)
{
	plc_dest_t dest;
	int i, fenum, j;
	unsigned short itemid;
	unsigned char databuf[4];
	const unsigned char *pene;

	PrintLog(LOGTYPE_DOWNLINK, "read met%d month..\n", metid);

	MakePlcDest(metid, &dest);

	fenum = (int)ParaMeter[metid].prdnum & 0xff;
	if(fenum > MAX_PLMET_FENUM) {
		ErrorLog("invalid fenum(%d)\n", fenum);
		fenum = 0;
	}

	SetPlMdbFenum(metid, fenum);
	itemid = 0x9410;
	pene = PlMdbMonth[metid-PLC_BASEMETP].ene;
	for(i=0; i<=fenum; i++,itemid++,pene+=4) {
		for(j=0; j<4; j++) {
			if(PLDATA_EMPTY == pene[j]) break;
		}
		if(j >= 4) continue;

		//PlCycState[0].rd_mets++;
		if(PlcRead(&dest, itemid, databuf, 4) > 0) {
			DebugPrint(LOGTYPE_DOWNLINK, "%04X=%02X%02X%02X.%02X\n", 
				itemid, databuf[3], databuf[2], databuf[1], databuf[0]);
			UpdatePlMdb(metid, itemid, databuf, 4);
			//PlCycState[0].ok_mets++;
		}

		//Sleep(10);
	}
}

/**
* @brief 读取并更新重点用户数据
* @param metid 电表号
*/
static void ReadPlcMetImp(unsigned char kmid)
{
	plc_dest_t dest;
	unsigned char databuf[4];
	unsigned char impnum;
	unsigned short metid;
	
	impnum = ParaMix.impuser.num;
	if(0 == impnum) return;
	else if(impnum > MAX_IMPORTANT_USER) impnum = MAX_IMPORTANT_USER;

	metid = ParaMix.impuser.metid[kmid];
	MakePlcDest(metid, &dest);
	PrintLog(LOGTYPE_DOWNLINK, "read met%d imp..\n", metid);
	
	if(PlcRead(&dest, 0x9010, databuf, 4) > 0) {
		DebugPrint(LOGTYPE_DOWNLINK, "9010=%02X%02X%02X.%02X\n", 
			databuf[3], databuf[2], databuf[1], databuf[0]);
		UpdatePlMdb(kmid, 0xF001, databuf, 4);
	}
}

static void UpdatePlCycTime(unsigned char *pbcdtime)
{
	sysclock_t clock;

	SysClockReadCurrent(&clock);

	pbcdtime[0] = clock.second;
	pbcdtime[1] = clock.minute;
	pbcdtime[2] = clock.hour;
	pbcdtime[3] = clock.day;
	pbcdtime[4] = clock.month;
	pbcdtime[5] = clock.year;
	HexToBcd(pbcdtime, 6);
	pbcdtime[4] &= 0x1f;
}

/**
* @brief 载波较时定时器
* @param arg 定时器参数
* @param utime 当前时间
*/
static void RTimerPlCheckTime(unsigned long arg, utime_t utime)
{
	PlStartCheckTime = 1;
}

/**
* @brief 载波较时初始化
*/
static void PlCheckTimeInit(void)
{
	const para_commport_t *pcfg = GetParaCommPort(COMMPORT_PLC);
	rtimer_conf_t conf;

	PlStartCheckTime = 0;

	if(!(pcfg->flag & RDMETFLAG_CHECKTIME)) return;

	conf.curtime = UTimeReadCurrent();
	conf.bonce = 0;
	conf.tdev = 1;
	if(!pcfg->chktime_day) conf.tmod = UTIMEDEV_DAY;  // 1 day
	else conf.tmod = UTIMEDEV_MONTH;  // 1 month

	SysClockReadCurrent(&conf.basetime);
	conf.basetime.day = pcfg->chktime_day;
	conf.basetime.hour = pcfg->chktime_hour;
	conf.basetime.minute = pcfg->chktime_minute;

	SysAddRTimer(&conf, RTimerPlCheckTime, 0);
}

/**
* @brief 重新开始抄表
*/
static void ResetPlTask(void)
{
	memset(PlCycState, 0, sizeof(PlCycState));
	PlCycCount = 1;
	PlCurMetid = PLC_BASEMETP;
	PlCycStartFlag = 0;
	PlReadMetEnd = 0;
}

#endif



unsigned short check_meter_type(viod)
{
	int i = 0;
	int have_07_meter = 0;
	int have_97_meter = 0;

	for(i=0;i<MAX_METER;i++)
	{
		if(ParaMeter[i].proto == PROTO_DL1997_376_1)	
		{
			have_97_meter = 1;
			break;
		}
	}
	for(i=0;i<MAX_METER;i++)
	{
		if(ParaMeter[i].proto == PROTO_DL2007_376_1)	
		{
			have_07_meter = 1;
			break;
		}
	}

	if(have_97_meter == 1 && have_07_meter == 0)
	{
		return 1;
	}
	else if(have_97_meter == 0 && have_07_meter == 1)
	{
		return 2;
	}
	else if(have_97_meter == 1 && have_07_meter == 1)
	{
		return 3;
	}	

	return 0;
}

unsigned short get_meter_total_cnt(viod)
{
	int i = 0;
	unsigned short meter_total_cnt = 0;

	for(i=0;i<MAX_METER;i++)
	{
		if(ParaMeter[i].metp_id)	
		{
			meter_total_cnt++;
		}
	}

	return meter_total_cnt;
}





unsigned short get_cen_meter_total_cnt(viod)
{
	int i = 0;
	unsigned short cen_meter_total_cnt = 0;

	for(i=0;i<MAX_METER;i++)
	{
		if(ParaMeter[i].metp_id)	
		{
			if((ParaMeter[i].portcfg & 0x1F) == CEN_METER_PORT)	
			{
				cen_meter_total_cnt++;
			}
		}
	}

	return cen_meter_total_cnt;
}


unsigned short get_imp_meter_total_cnt(viod)
{
	return	ParaMix.impuser.num;
}


unsigned short get_read_meter_succ_cnt(viod)
{
	int i = 0;
	unsigned short read_meter_succ_cnt = 0;

	for(i=0;i<MAX_METER;i++)
	{
		if(meter_ene_buffer[i].read_stat == HAVE_READ)
		{
			read_meter_succ_cnt++;
		}
	}
	return read_meter_succ_cnt;
}

int is_imp_meter(unsigned short meter_num)
{
	unsigned char imp_meter_cnt_tmp = 0;
	int i;

	if((meter_num == 0) ||(meter_num >= MAX_METER))	return 1;
	imp_meter_cnt_tmp = ParaMix.impuser.num;
	for(i=0;i<imp_meter_cnt_tmp;i++)
	{
		if(ParaMix.impuser.metid[i] == meter_num)
		{
			return 0;
		}
	}

	return 1;
}


unsigned short get_imp_meter_read_succ_cnt(void)
{
	int i;
	int imp_meter_read_succ_cnt = 0;
	
	for(i=0;i<MAX_METER_CNT;i++)
	{
		if(meter_ene_buffer[i].read_stat == HAVE_READ)
		{
			if(!is_imp_meter(meter_ene_buffer[i].met_id))	
			{
				imp_meter_read_succ_cnt++;
			}
		}
	}
	return imp_meter_read_succ_cnt;
}

int check_buf(unsigned char *buf)
{
	int i = 0;
	
	for(i=0;i<4;i++)
	{
		if(buf[i] == 0xEE)
			return 1;
	}
	return 0;
}


int check_meter_succ(void)
{
	int i = 0;
	int j = 0;
	
	for(i=0;i<MAX_METER_CNT;i++)
	{
		if(!check_buf(meter_ene_buffer[i].meter_ene))
		{
			j++;
		}
	}
	return j;
}


int check_read_meter_succ_cnt(void)
{
	int i = 0,j = 0;
	
	for(i=0;i<MAX_METER_CNT;i++)
	{
		if(!check_buf(meter_ene_buffer[i].meter_ene) 
			&& (meter_ene_buffer[i].met_id>=2) 
			&&(meter_ene_buffer[i].portcfg&0x1F) == PLC_PORT)
		{
			j++;
		}
	}
	return j;
}

void clear_read_meter_stat(void)
{
	int i = 0;


	for(i=0;i<MAX_METER_CNT;i++)
	{
		meter_ene_buffer[i].read_stat = 0x00;
		memcpy((unsigned char *)&meter_ene_buffer[i].meter_index,(unsigned char *)&ParaMeter[i].index,4);//序号，测量点号
		memcpy((unsigned char *)&meter_ene_buffer[i].portcfg,(unsigned char *)&ParaMeter[i].portcfg,1);
		memcpy((unsigned char *)&meter_ene_buffer[i].meter_proto,(unsigned char *)&ParaMeter[i].proto,1);
		memcpy((unsigned char *)&meter_ene_buffer[i].meter_addr,(unsigned char *)&ParaMeter[i].addr,6);
		memset((unsigned char *)meter_ene_buffer[i].meter_ene,0xEE,20);
		memset((unsigned char *)&pl_node_info[i].node_type,0x00,sizeof(pl_node_info_t));
		memset((unsigned char *)&PlMdbDay[i].meter_num, PLDATA_EMPTY, sizeof(plmdb_day_t));
		memcpy((unsigned char *)&PlMdbDay[i].meter_num,(unsigned char *)&ParaMeter[i+2].metp_id,2);
		//memcpy((unsigned char *)&PlMdbDay[i].meter_addr,(unsigned char *)&ParaMeter[i+2].addr,6);
		//memset((unsigned char *)&PlMdbDay[i].readtime, PLDATA_EMPTY, 8);
	}
	for(i=0;i<1024;i++)
	{
		memset((unsigned char *)&pl_cjq_info,0x00,sizeof(pl_cjq_info_t));
	}

	ruter_is_busy = 1;
	active_rgst_start = 0;
	set_ruter_work_mode_start = 0;//不启动补组网
	point_read_index = 0;
	add_meter_index = 1;
	add_meter_fail_cnt = 0;
	add_meter_succ_cnt = 0;
	add_meter_start = 0;
	meter_total_cnt = get_meter_total_cnt();
	cen_meter_cnt = get_cen_meter_total_cnt();
	imp_meter_cnt = get_imp_meter_total_cnt();
	meter_read_succ_cnt = 0;
	read_meter_finish = 0;
	PlFrezFinished = 0;
	PlNeedFrez = 0;
	read_meter_finish_frez = 0;
	PlReadStopped = 0;
	PlReadStoppedFlag = 0;

	point_read_start = 1;
	one_time_point_read_stat = 0;
	set_ruter_work_mode_start_times = 0;
	query_pl_node_info = 0;
	query_pl_node_time_out = 0;
	query_pl_node_cnt = 0;
	query_pl_node_finish = 0;
	query_pl_node_info_start = 0;
	meter_type = check_meter_type();

	start_point_read_timer = 0;
	stop_point_read_timer = 0;
	PlcTask_watchdog = 0;
	PlFrezTask_watchdog = 0;
	stop_point_read_flag = 0;
	pl_shell_cmd_flag = 0;
	clear_read_meter_stat_flag = 0;
	read_meter_major_frez = 0;
	read_meter_major_frez_flag = 0;
	active_rgst_command = 0;
	point_read_cyc = 0;
	memset((unsigned char *)&read_meter_start_time,0x00,sizeof(read_meter_start_time));
	SysClockReadCurrent(&read_meter_start_time);
	memset((unsigned char *)&read_meter_finish_time,0x00,sizeof(read_meter_finish_time));
}

/*
void clear_read_meter_stat_2(void)
{
	int i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "clear_read_meter_stat_2...");
	ruter_is_busy = 1;
	active_rgst_start = 0;
	set_ruter_work_mode_start = 0;
	cyc_read_start_97 = 1;
	point_read_start = 0;
	point_read_index = 0;
	add_meter_index = 1;
	add_meter_fail_cnt = 0;
	add_meter_succ_cnt = 0;
	add_meter_start = 0;
	meter_total_cnt = get_meter_total_cnt();
	cen_meter_cnt = get_cen_meter_total_cnt();
	imp_meter_cnt = get_imp_meter_total_cnt();
	meter_read_succ_cnt = 0;
	read_meter_finish = 0;
	PlFrezFinished = 0;
	PlNeedFrez = 0;
	read_meter_finish_frez = 0;
	PlReadStopped = 0;
	PlReadStoppedFlag = 0;
	cyc_read_start = 1;
	cyc_read_start_97 = 0;
	cyc_read_start_07 = 0;

	memset((unsigned char *)&read_meter_start_time,0x00,sizeof(read_meter_start_time));
	memset((unsigned char *)&read_meter_finish_time,0x00,sizeof(read_meter_finish_time));
	for(i=0;i<MAX_METER_CNT;i++)
	{
		meter_ene_buffer[i].read_stat = 0x00;
		memset((unsigned char *)meter_ene_buffer[i].meter_ene,0xEE,20);
	}
}
*/








int make_read_645_pkt(const unsigned char *addr, unsigned int itemid, unsigned char *buf)
{
	unsigned char check, *puc;
	int i;


	if(buf == NULL || addr == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...addr...err\n");
		return -1;
	}
	puc = buf;
	*puc++ = 0x68;
	for(i=0; i<6; i++) *puc++ = addr[i];
	*puc++ = 0x68;
	switch(itemid)
	{
		case 0x9010:
		case 0x901F:
			*puc++ = 0x01;
			*puc++ = 0x02;
			*puc++ = (unsigned char)(itemid) + 0x33;
			*puc++ = (unsigned char)(itemid>>8) + 0x33;
			puc = buf;
			check = 0;
			for(i=0; i<12; i++) check += *puc++;
			*puc++ = check;
			*puc = 0x16;
			return 14;
		case 0x00010000:
		case 0x0001FF00:
		case 0x03300001:
		case 0x03300101:			
			*puc++ = 0x11;
			*puc++ = 0x04;
			*puc++ = (unsigned char)(itemid) + 0x33;
			*puc++ = (unsigned char)(itemid>>8) + 0x33;
			*puc++ = (unsigned char)(itemid>>16) + 0x33;	
			*puc++ = (unsigned char)(itemid>>24) + 0x33;			
			puc = buf;
			check = 0;
			for(i=0; i<14; i++) check += *puc++;
			*puc++ = check;
			*puc = 0x16;
			return 16;
	}
	return 0;
}



void make_init_param_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	//int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x0F;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x42;
	memset(plc_packet.mes,0x00,6);
	plc_packet.afn = 0x01;
	plc_packet.fn[0] = 0x02;
	//plc_packet.fn[0] = 0x04;
	plc_packet.fn[1] = 0x00;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	pbuf = buf + 3;
	for(i=0;i<10;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	PrintLog(LOGTYPE_DOWNLINK, "check_sum = %02x\n",check_sum);
	*len = plc_packet.len1;
}






void make_hard_ware_reset_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x0F;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x42;
	memset(plc_packet.mes,0x00,6);
	plc_packet.afn = 0x01;
	plc_packet.fn[0] = 0x01;
	plc_packet.fn[1] = 0x00;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	pbuf = buf + 3;
	for(i=0;i<10;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	PrintLog(LOGTYPE_DOWNLINK, "check_sum = %02x\n",check_sum);
	*len = plc_packet.len1;
}

int get_645_pak_lenth(unsigned int itemid)
{
	switch(itemid)
	{
		case 0x9010:
		case 0x901F:
			return 0x0E;
		case 0x00010000:
		case 0x0001FF00:
		case 0x03300001:
		case 0x03300101:		
			return 0x10;	
	}
	return 0;	
}

void make_cyc_read_pak(unsigned char *buf,unsigned char *len,unsigned int itemid)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	int len_645 = 0;
	unsigned int check_sum = 0;
	//const unsigned char src_addr[6] = {0x01,0x02,0x03,0x04,0x00,0x00};
	const unsigned char src_addr[6] = {0xBB,0xBB,0xBB,0xBB,0xBB,0xBB};
	const unsigned char broadcast_addr[6] = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};


	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	switch(itemid)
	{
		case 0x9010:
		case 0x901F:
			plc_packet.len1 = 0x2C;
			break;
		case 0x00010000:
		case 0x0001FF00:
			plc_packet.len1 = 0x2E;
			break;
	}
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x02;
	plc_packet.mes[0] = 0x04;
	memset(&plc_packet.mes[1],0x00,5);
	plc_packet.afn = 0x13;
	plc_packet.fn[0] = 0x01;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,10);
	pdata += 10;
	memcpy(pdata,src_addr,6);
	pdata += 6;
	memcpy(pdata,broadcast_addr,6);
	pdata += 6;
	memcpy(pdata,(unsigned char *)&plc_packet.afn,3);
	pdata += 3;
	*pdata++ = 0x00;
	*pdata++ = 0x00;
	len_645 = get_645_pak_lenth(itemid);
	*pdata++ = len_645;
	make_read_645_pkt(broadcast_addr, itemid, pdata);
	pdata += len_645;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = check_sum;
	*pdata++ = 0x16;

	*len = plc_packet.len1;
}


#if 1
void make_point_read_pak(unsigned char *buf,unsigned char *len,unsigned int itemid,unsigned char *meter_addr)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	int len_645 = 0;
	unsigned int check_sum = 0;
	unsigned char src_addr[6];
	const unsigned char broadcast_addr[6] = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
	
	src_addr[0] = 0xBB;
	src_addr[1] = 0xBB;
	src_addr[2] = ParaUni.addr_area[1];
	src_addr[3] = ParaUni.addr_area[0];
	src_addr[4] = ParaUni.addr_sn[1];
	src_addr[5] = ParaUni.addr_sn[0];

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	switch(itemid)
	{
		case 0x9010:
		case 0x901F:
			plc_packet.len1 = 0x2C;
			break;
		case 0x00010000:
		case 0x0001FF00:
			
		case 0x03300001:
		case 0x03300101:			
			plc_packet.len1 = 0x2E;
			break;
	}
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x02;
	//plc_packet.ctl = 0x42;
	plc_packet.mes[0] = 0x04;
	memset(&plc_packet.mes[1],0x00,5);
	plc_packet.afn = 0x13;
	plc_packet.fn[0] = 0x01;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,10);
	pdata += 10;
	memcpy(pdata,src_addr,6);
	pdata += 6;
	memcpy(pdata,broadcast_addr,6);
	pdata += 6;
	memcpy(pdata,(unsigned char *)&plc_packet.afn,3);
	pdata += 3;
	*pdata++ = 0x00;
	*pdata++ = 0x00;
	len_645 = get_645_pak_lenth(itemid);
	*pdata++ = len_645;
	make_read_645_pkt(meter_addr, itemid, pdata);
	pdata += len_645;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = check_sum;
	*pdata++ = 0x16;

	*len = plc_packet.len1;
}
#endif

#if 0
void make_point_read_pak(unsigned char *buf,unsigned char *len,unsigned int itemid,unsigned char *meter_addr)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	int len_645 = 0;
	unsigned int check_sum = 0;
	//const unsigned char src_addr[6] = {0x01,0x02,0x03,0x04,0x00,0x00};
	const unsigned char src_addr[6] = {0xBB,0xBB,0xBB,0xBB,0xBB,0xBB};
	const unsigned char broadcast_addr[6] = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};


	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	switch(itemid)
	{
		case 0x9010:
		case 0x901F:
			plc_packet.len1 = 0x2C;
			break;
		case 0x00010000:
		case 0x0001FF00:
			plc_packet.len1 = 0x2E;
			break;
	}
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x02;
	plc_packet.mes[0] = 0x04;
	memset(&plc_packet.mes[1],0x00,5);
	plc_packet.afn = 0x13;
	plc_packet.fn[0] = 0x01;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,10);
	pdata += 10;
	memcpy(pdata,src_addr,6);
	pdata += 6;
	memcpy(pdata,broadcast_addr,6);
	pdata += 6;
	memcpy(pdata,(unsigned char *)&plc_packet.afn,3);
	pdata += 3;
	*pdata++ = 0x00;
	*pdata++ = 0x00;
	len_645 = get_645_pak_lenth(itemid);
	*pdata++ = len_645;
	make_read_645_pkt(meter_addr, itemid, pdata);
	pdata += len_645;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = check_sum;
	*pdata++ = 0x16;

	*len = plc_packet.len1;
}
#endif













void make_add_one_meter_pak(unsigned char *buf,unsigned char *len,unsigned char *addr,unsigned short index,unsigned char proto)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x19;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x02;
	memset(plc_packet.mes,0x00,6);
	plc_packet.afn = 0x11;
	plc_packet.fn[0] = 0x01;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	*pdata++ = 0x01;
	memcpy(pdata,addr,6);
	pdata += 6;
	DEPART_SHORT(index, pdata);	
	pdata += 2;
	*pdata++ = proto;

	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	//PrintLog(LOGTYPE_DOWNLINK, "check_sum = %02x\n",check_sum);
	*len = plc_packet.len1;
}

int check_rcv_frame_crc(unsigned char *buf,unsigned char len)
{
	unsigned char crc = 0;
	unsigned char i = 0;
	unsigned char check_sum = 0;
	unsigned char *pbuf;

	if(buf == NULL || len <= 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return -1;
	}
	check_sum = buf[len - 2];
	pbuf = &buf[3];
	for(i=0;i<len-5;i++)
	{
		crc +=  pbuf[i];
	}
	if(check_sum != crc)	return -1;
	return 1;
}
 
int process_add_addr_frame(unsigned char *buf,unsigned char len,jzq_meter_t *jzq_meter)	
{
	plc_packet_t pak;
	int pak_len = 0,i = 0;
	unsigned char *pbuf;
	int add_meter_cnt = 0;
	jzq_meter_t *p_jzq_meter;

	p_jzq_meter = jzq_meter;
	pbuf = buf;
	if(buf == NULL || len <= 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return -1;
	}
	
	memset((unsigned char *)&pak.head,0x00,sizeof(pak));
	memcpy((unsigned char *)&pak.head,buf,len);
	
	if(pak.head != FRAME_HEAD)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.head = %d\n",pak.head);
		PrintLog(LOGTYPE_DOWNLINK, "FRAME_HEAD_err\n");
		return -1;
	}

	if((pak_len < MIN_FRAME_LENTH) ||(pak_len > MAX_FRAME_LENTH) ||(pak_len > len) )   
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak_len = %d\n",pak_len);
		PrintLog(LOGTYPE_DOWNLINK, "pak_len_err\n");
		return -1;
	}
	if((pak.ctl & COMM_MODE) != 0x01 && (pak.ctl & COMM_MODE) != 0x02)    
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & COMM_MODE = %d\n",pak.ctl & COMM_MODE);
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & COMM_MODE_err\n");
		return -1;
	}
	#if 0
	if(!(pak.ctl & DIR_CTL))	
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & DIR_CTL = %d\n",pak.ctl & DIR_CTL);
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & DIR_CTL_err\n");
		return -1;
	}
	if((pak.ctl & FRM_CTL))	
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & FRM_CTL = %d\n",pak.ctl & FRM_CTL);
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & FRM_CTL_err\n");
		return -1;
	}
	#endif
	
	if(check_rcv_frame_crc(buf,len) < 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "check_sun_err\n");
		return -1;
	}

	if(pak.afn != 0x11 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00)
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.afn_err...pak.fn_err...\n");
		return -1;
	}
	pbuf += 13;
	add_meter_cnt = *pbuf++;
	for(i=0;i<add_meter_cnt;i++)
	{
		memcpy(p_jzq_meter->meter_addr,pbuf,6);
		pbuf += 6;
		memcpy((unsigned char *)&p_jzq_meter->meter_index,pbuf,2);
		pbuf += 2;
		p_jzq_meter->meter_proto = *pbuf++;
	}
	PrintLog(LOGTYPE_DOWNLINK, "process_add_addr_frame_ok\n");
	return 1;
}


int process_data_fwd_frame(unsigned char *buf,unsigned char len,unsigned char *buf_645,unsigned char *proto_645,unsigned char *len_645,unsigned char *meter_addr_645)	
{
	plc_packet_t pak;
	int pak_len = 0;
	unsigned char *pbuf;
	unsigned char proto_type = 0;
	unsigned char pak_len_645 = 0;

	pbuf = buf;
	if(buf == NULL || len <= 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return -1;
	}
	
	memset((unsigned char *)&pak.head,0x00,sizeof(pak));
	memcpy((unsigned char *)&pak.head,buf,len);
	
	if(pak.head != FRAME_HEAD)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.head = %d\n",pak.head);
		PrintLog(LOGTYPE_DOWNLINK, "FRAME_HEAD_err\n");
		return -1;
	}

	if((pak_len < MIN_FRAME_LENTH) ||(pak_len > MAX_FRAME_LENTH) ||(pak_len > len) )   
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak_len = %d\n",pak_len);
		PrintLog(LOGTYPE_DOWNLINK, "pak_len_err\n");
		return -1;
	}

	if((pak.ctl & COMM_MODE) != 0x01 && (pak.ctl & COMM_MODE) != 0x02)    
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & COMM_MODE = %d\n",pak.ctl & COMM_MODE);
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & COMM_MODE_err\n");
		return -1;
	}
	if(check_rcv_frame_crc(buf,len) < 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "check_sum_err\n");
		return -1;
	}

	if(pak.afn != 0x02 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00)
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.afn_err...pak.fn_err...\n");
		return -1;
	}

	pbuf += 13;
	proto_type = *pbuf++;
	pak_len_645 = *pbuf++;
	if(*pbuf++ != FRAME_HEAD)
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak_645_head_err...\n");
		return -1;
	}
	if(proto_type != PROTO_DL1997 && proto_type != PROTO_DL2007)
	{
		PrintLog(LOGTYPE_DOWNLINK, "proto_type_err\n");
		return -1;
	}
	
	*proto_645 = proto_type;
	*len_645 = pak_len_645;
	memcpy(meter_addr_645,pbuf,6);
	PrintLog(LOGTYPE_DOWNLINK, "process_data_fwd_frame_ok\n");
	return 1;
}

void make_check_route_mod_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x0F;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x47;
	memset(plc_packet.mes,0x00,6);
	plc_packet.mes[2] = 0xFF;
	plc_packet.afn = 0x02;
	plc_packet.fn[0] = 0x10;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	*len = plc_packet.len1;
}


void make_query_mod_info_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x0F;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x41;
	memset(plc_packet.mes,0x00,6);
	plc_packet.mes[2] = 0x5F;
	plc_packet.afn = 0x03;
	plc_packet.fn[0] = 0x01;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	*len = plc_packet.len1;
}


void make_set_route_mod_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x10;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x47;
	memset(plc_packet.mes,0x00,6);
	plc_packet.mes[2] = 0x5F;
	plc_packet.afn = 0x01;
	plc_packet.fn[0] = 0x40;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	*pdata++ = 0x01;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	*len = plc_packet.len1;
}


void make_set_plnode_addr_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x15;
	plc_packet.len2 = 0x00;
	//plc_packet.ctl = 0x42;
	plc_packet.ctl = 0x41;
	memset(plc_packet.mes,0x00,6);
	plc_packet.mes[0] = 0x01;
	plc_packet.mes[2] = 0x5F;
	plc_packet.afn = 0x05;
	plc_packet.fn[0] = 0x01;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	*pdata++ = 0xBB;
	*pdata++ = 0xBB;
	*pdata++ = ParaUni.addr_area[1];
	*pdata++ = ParaUni.addr_area[0];
	*pdata++ = ParaUni.addr_sn[1];
	*pdata++ = ParaUni.addr_sn[0];
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	*len = plc_packet.len1;
}



void make_set_ruter_work_mode_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x12;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x42;
	memset(plc_packet.mes,0x00,6);
	plc_packet.afn = 0x11;
	plc_packet.fn[0] = 0x08;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	*pdata++ = 0x03;
	*pdata++ = 0x00;
	*pdata++ = 0x00;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	*len = plc_packet.len1;
}

void make_data_fwd_pak(unsigned char *buf,unsigned char *len,unsigned char proto)
{
	unsigned char *pdata;
	
	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}

	pdata = buf;
	*pdata++ = 0x68;
	//*pdata++ =
	//*pdata++ = 
	*pdata++ = 0x81;
	memset(pdata++,0x00,18);
	pdata += 18;
	*pdata++ = 0x02;
	*pdata++ = 0x01;
	*pdata++ = 0x00;
	*pdata++ = proto;
	




}

void make_confirm_ack_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x13;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x81;
	memset(plc_packet.mes,0x00,6);
	plc_packet.afn = 0x00;
	plc_packet.fn[0] = 0x01;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	*pdata++ = 0xFF;
	*pdata++ = 0xFF;
	*pdata++ = 0x00;
	*pdata++ = 0x00;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	*len = plc_packet.len1;
}




void make_enable_active_rgst_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}

	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x19;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x42;
	memset(plc_packet.mes,0x00,6);
	plc_packet.afn = 0x11;
	plc_packet.fn[0] = 0x10;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	memset(pdata,0x00,10);
	pdata += 10;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	*len = plc_packet.len1;
}


void make_get_ruter_run_stat_pak(unsigned char *buf,unsigned char *len)
{
	unsigned char *pdata,*pbuf;
	plc_packet_t plc_packet;
	int i = 0;
	int len1 = 0;
	unsigned int check_sum = 0;

	if(buf == NULL || len == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return;
	}
	
	pdata = buf;
	plc_packet.head = 0x68;
	plc_packet.len1 = 0x0F;
	plc_packet.len2 = 0x00;
	plc_packet.ctl = 0x42;
	memset(plc_packet.mes,0x00,6);
	plc_packet.afn = 0x10;
	plc_packet.fn[0] = 0x08;
	plc_packet.fn[1] = 0x00;
	len1 = plc_packet.len1 - 5;
	memcpy(pdata,(unsigned char *)&plc_packet.head,13);
	pdata += 13;
	pbuf = buf + 3;
	for(i=0;i<len1;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum &= 0xFF;
	*pdata++ = (unsigned char)check_sum;
	*pdata++ = 0x16;
	*len = plc_packet.len1;
}




int send_init_param_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "send_init_param_pak\n");
	make_init_param_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DOWNLINK, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DOWNLINK, "\n");
		return 0;
	}
	return 1;
}


int send_hard_ware_reset_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "send_hard_ware_reset_pak\n");
	make_hard_ware_reset_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DOWNLINK, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DOWNLINK, "\n");
		return 0;
	}
	return 1;
}


int send_set_ruter_work_mode_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "send_set_ruter_work_mode_pak\n");
	make_set_ruter_work_mode_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DOWNLINK, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DOWNLINK, "\n");
		return 0;
	}
	return 1;
}

int send_check_route_mod_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(0, "send_check_route_mod_pak\n");
	make_check_route_mod_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(0, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(0, "%02X ",send_buf[i]);
		PrintLog(0, "\n");
		return 0;
	}
	return 1;
}

int send_set_route_mod_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(0, "send_check_route_mod_pak\n");
	make_set_route_mod_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(0, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(0, "%02X ",send_buf[i]);
		PrintLog(0, "\n");
		return 0;
	}
	return 1;
}

int send_query_mod_info_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(0, "send_check_route_mod_pak\n");
	make_query_mod_info_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(0, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(0, "%02X ",send_buf[i]);
		PrintLog(0, "\n");
		return 0;
	}
	return 1;
}


int send_set_plnode_addr_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(0, "send_set_plnode_addr_pak\n");
	make_set_plnode_addr_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(0, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(0, "%02X ",send_buf[i]);
		PrintLog(0, "\n");
		return 0;
	}
	return 1;
}




int send_confirm_ack_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DATA, "send_confirm_ack_pak\n");
	make_confirm_ack_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(LOGTYPE_DATA, "send_buf = ");
		//for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DATA, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DATA, "\n");
		return 0;
	}
	return 1;
}



int send_enable_active_rgst_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "send_enable_active_rgst_pak\n");
	make_enable_active_rgst_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DOWNLINK, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DOWNLINK, "\n");
		return 0;
	}
	return 1;
}


int send_cyc_read_pak(unsigned int itemid)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "send_cyc_read_pak\n");
	memset(send_buf,0x00,sizeof(send_buf));
	make_cyc_read_pak(send_buf,&len,itemid);
	PrintLog(LOGTYPE_DOWNLINK, "send_cyc_read_pak len = %d\n",len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DOWNLINK, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DOWNLINK, "\n");
		return 0;
	}
	return 1;
}

int send_point_read_pak(unsigned int itemid,unsigned char *meter_addr)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DATA, "send_point_read_pak\n");
	memset(send_buf,0x00,sizeof(send_buf));
	make_point_read_pak(send_buf,&len,itemid,meter_addr);
	PrintLog(LOGTYPE_DATA, "send_point_read_pak len = %d\n",len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(LOGTYPE_DATA, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DATA, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DATA, "\n");
		return 0;
	}
	return 1;
}


const unsigned char query_plnode_info_buf[15] = {0x68, 0x0F, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x33, 0x16};
int send_query_plnode_info_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "send_query_plnode_info_pak\n");
	memset(send_buf,0x00,sizeof(send_buf));
	len = sizeof(query_plnode_info_buf);
	memcpy(send_buf,query_plnode_info_buf,len);
	PrintLog(LOGTYPE_DOWNLINK, "query_plnode_info len = %d\n",len);
	if(!UartSend(PLC_UART_PORT,query_plnode_info_buf,15))
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DOWNLINK, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DOWNLINK, "\n");
		return 0;
	}
	return 1;
}


int send_add_one_meter_pak(unsigned char *addr,unsigned short index,unsigned char proto)
{
	unsigned char send_buf[256];
	unsigned char len = 0;

	//PrintLog(LOGTYPE_DOWNLINK, "send_add_one_meter_pak\n");
	memset(send_buf,0x00,sizeof(send_buf));
	make_add_one_meter_pak(send_buf,&len,addr,index,proto);
	//PrintLog(LOGTYPE_DOWNLINK, "send_add_one_meter_pak_len = %d\n",len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		//PrintLog(LOGTYPE_DOWNLINK, "send_buf = ");
		//for(i=0;i<len;i++)
			//PrintLog(LOGTYPE_DOWNLINK, "%02X ",send_buf[i]);
		//PrintLog(LOGTYPE_DOWNLINK, "\n");
		return 0;
	}
	return 1;
}



int send_get_ruter_run_stat_pak(void)
{
	unsigned char send_buf[256];
	unsigned char len = 0;
	unsigned char i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "send_get_ruter_run_stat_pak\n");
	make_get_ruter_run_stat_pak(send_buf,&len);
	if(!UartSend(PLC_UART_PORT, send_buf,len))
	{
		PrintLog(LOGTYPE_DATA, "send_buf = ");
		for(i=0;i<len;i++)
			PrintLog(LOGTYPE_DATA, "%02X ",send_buf[i]);
		PrintLog(LOGTYPE_DATA, "\n");
		return 0;
	}
	return 1;
}




int check_plc_modern_run_stat(unsigned char *buf,unsigned char len,unsigned char *run_stat)
{
	AFN10_FN4_t	pak;
	int pak_len = 0;

	memset((unsigned char *)&pak.head,0x00,sizeof(pak));
	memcpy((unsigned char *)&pak.head,buf,len);

	if(buf == NULL || len <= 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return -1;
	}
	pak_len = pak.len1 + pak.len2 * 100;
	if(pak.head != FRAME_HEAD)	return -1;
	if((pak_len < MIN_FRAME_LENTH) ||(pak_len > MAX_FRAME_LENTH) )    return -1;
	if((pak.ctl & COMM_MODE) != 0x01 && (pak.ctl & COMM_MODE) != 0x02)    return -1;
	if(!(pak.ctl & DIR_CTL))	return -1;
	if((pak.ctl & FRM_CTL))		return -1;
	if(pak.afn != RUTER_CHECK) return -1;
	if(pak.fn[0] != 0x08 || pak.fn[1] != 0x00) return -1;
	if(check_rcv_frame_crc(buf,len) < 0)	return -1;

	*run_stat = pak.run_stat;
	
	return 1;
}


int check_rcv_frame(unsigned char *buf,int len)
{
	plc_packet_t pak;
	int pak_len = 0;

	PrintLog(LOGTYPE_DATA, "check_rcv_frame\n");
	if(buf == NULL || len < 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return -1;
	}
	memset((unsigned char *)&pak.head,0x00,sizeof(pak));
	memcpy((unsigned char *)&pak.head,buf,len);

	pak_len = pak.len1 + pak.len2 * 100;
	if(pak.head != FRAME_HEAD)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.head = %d\n",pak.head);
		PrintLog(LOGTYPE_DOWNLINK, "FRAME_HEAD_err\n");
		return -1;
	}

	if((pak_len < MIN_FRAME_LENTH) ||(pak_len > MAX_FRAME_LENTH) ||(pak_len > len) )   
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak_len = %d\n",pak_len);
		PrintLog(LOGTYPE_DOWNLINK, "pak_len_err\n");
		return -1;
	}

	if((pak.ctl & COMM_MODE) != 0x01 && (pak.ctl & COMM_MODE) != 0x02)    
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & COMM_MODE = %d\n",pak.ctl & COMM_MODE);
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & COMM_MODE_err\n");
		return -1;
	}


	if(!(pak.ctl & DIR_CTL))	
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & DIR_CTL = %d\n",pak.ctl & DIR_CTL);
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & DIR_CTL_err\n");
		return -1;
	}
	if((pak.ctl & FRM_CTL))	
	{
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & FRM_CTL = %d\n",pak.ctl & FRM_CTL);
		PrintLog(LOGTYPE_DOWNLINK, "pak.ctl & FRM_CTL_err\n");
		return -1;
	}
	if(check_rcv_frame_crc(buf,len) < 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "check_sun_err\n");
		return -1;
	}

	PrintLog(LOGTYPE_DATA, "check_rcv_frame_ok\n");
	//PrintLog(LOGTYPE_DATA, "check_rcv_frame_ok\n");
	return 1;
}


int init_param(void)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;
	
	memset(rcv_buf,0x00,sizeof(rcv_buf));

	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_init_param_pak())
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_init_param_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
			PrintLog(LOGTYPE_DOWNLINK, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(LOGTYPE_DOWNLINK, "rcv_init_param_pak_fail\n");
	}
	return -1;
}


int hard_ware_reset(void)
{
	#if 0
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;
	
	memset(rcv_buf,0x00,sizeof(rcv_buf));

	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_hard_ware_reset_pak())
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_hard_ware_reset_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
			PrintLog(LOGTYPE_DOWNLINK, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(LOGTYPE_DOWNLINK, "rcv_hard_ware_reset_pak_fail\n");
	}
	return -1;
	#endif
	return 1;
	
}


int set_ruter_work_mode(void)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_set_ruter_work_mode_pak())
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_set_ruter_work_mode_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
			PrintLog(LOGTYPE_DOWNLINK, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_set_ruter_work_mode_pak_fail\n");
	}
	return -1;
}

int check_route_mod(void)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_check_route_mod_pak())
		{
			PrintLog(0, "send_check_route_mod_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(0, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(0, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(0, "%02X ",rcv_buf[i]);
			PrintLog(0, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(0, "check_route_mod_pak_fail\n");
	}
	return -1;
}


int set_route_mod(void)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_set_route_mod_pak())
		{
			PrintLog(0, "send_check_route_mod_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(0, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(0, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(0, "%02X ",rcv_buf[i]);
			PrintLog(0, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(0, "check_route_mod_pak_fail\n");
	}
	return -1;
}


int query_mod_info(void)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_query_mod_info_pak())
		{
			PrintLog(0, "send_check_route_mod_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(0, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(0, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(0, "%02X ",rcv_buf[i]);
			PrintLog(0, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(0, "check_route_mod_pak_fail\n");
	}
	return -1;
}

int set_plnode_addr(void)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_set_plnode_addr_pak())
		{
			PrintLog(0, "send_set_plnode_addr_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(0, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(0, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(0, "%02X ",rcv_buf[i]);
			PrintLog(0, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(0, "send_set_ruter_work_mode_pak_fail\n");
	}
	return -1;
}


int confirm_ack(void)
{
	unsigned char rcv_buf[256];
	unsigned char j;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_confirm_ack_pak())
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_confirm_ack_pak\n");
		}
		PLC_UNLOCK;
		return 1;
	}
	if(j == 3)
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_confirm_ack_fail\n");
	}
	return -1;
}






int enable_active_rgst(void)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_enable_active_rgst_pak())
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_enable_active_rgst_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
			PrintLog(LOGTYPE_DOWNLINK, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_set_ruter_work_mode_pak_fail\n");
	}
	return -1;
}




int get_ruter_run_stat(unsigned char *run_stat,unsigned char *pl_node_cnt,unsigned char *pl_have_read_node_cnt,unsigned char *pl_fwd_read_node_cnt)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	AFN10_FN4_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_get_ruter_run_stat_pak())
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_get_ruter_run_stat_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(LOGTYPE_DATA, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(LOGTYPE_DATA, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(LOGTYPE_DATA, "%02X ",rcv_buf[i]);
			PrintLog(LOGTYPE_DATA, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != RUTER_CHECK) return -1;
				if(pak.fn[0] != 0x08 || pak.fn[1] != 0x00) return -1;
				*run_stat = pak.run_stat;
				pl_node_cnt[0] = pak.node_cnt[0];
				pl_node_cnt[1] = pak.node_cnt[1];
				pl_have_read_node_cnt[0] = pak.have_read_node_cnt[0];
				pl_have_read_node_cnt[1] = pak.have_read_node_cnt[1];
				pl_fwd_read_node_cnt[0] = pak.fwd_read_node_cnt[0];
				pl_fwd_read_node_cnt[1] = pak.fwd_read_node_cnt[1];

				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_get_ruter_run_stat_pak_fail\n");
	}
	return -1;
}


int add_one_meter(unsigned char *addr,unsigned short index,unsigned char proto)
{
	unsigned char rcv_buf[256];
	unsigned char j;
	int rcv_len = 0;
	unsigned char add_proto = 0;
	plc_packet_t pak;

	//PrintLog(LOGTYPE_DOWNLINK, "add_one_meter\n");
	memset(rcv_buf,0x00,sizeof(rcv_buf));

	if(proto == PROTO_DL2007_376_1)
	{
		add_proto = PROTO_DL2007;
	}
	else if(proto == PROTO_DL1997_376_1)
	{
		add_proto = PROTO_DL1997;
	}
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_add_one_meter_pak(addr,index,add_proto))
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_add_one_meter_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		//PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			//PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
			//for(i=0;i<rcv_len;i++)
				//PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
			//PrintLog(LOGTYPE_DOWNLINK, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) return -1;
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_add_one_meter_pak\n");
	}	
	return -1;
}


int cyc_read(unsigned int itemid)
{
	unsigned char rcv_buf[256];
	unsigned char i,j;
	int rcv_len = 0;
	plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	for(j=0;j<3;j++)
	{
		PLC_LOCK;
		if(send_cyc_read_pak(itemid))
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_cyc_read_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;
		PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
			PrintLog(LOGTYPE_DOWNLINK, "\n");
			if(check_rcv_frame(rcv_buf,rcv_len) == 1)
			{
				memset((unsigned char *)&pak.head,0x00,sizeof(pak));
				memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
				if(pak.afn != 0x00 || pak.fn[0] != 0x01 || pak.fn[1] != 0x00) 
				{
					return -1;
				}
				return 1;
			}
		}
	}
	if(j == 3)
	{
		PrintLog(LOGTYPE_DOWNLINK, "send_cyc_read_pak_fail\n");
	}
	return -1;
}



int point_read(unsigned int itemid,unsigned char *meter_addr,unsigned char *buf)
{
	unsigned char rcv_buf[256];
	unsigned char j;
	int rcv_len = 0;
	//plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	//for(j=0;j<3;j++)
	for(j=0;j<1;j++)	
	{
		PLC_LOCK;
		if(send_point_read_pak(itemid,meter_addr))
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_point_read_pak\n");
		}
		//rcv_len = PlcRecvPkt(rcv_buf,100);
		//rcv_len = PlcRecvPkt(rcv_buf,200);
		rcv_len = PlcRecvPkt(rcv_buf,150);
		PLC_UNLOCK;

		if(rcv_len>0)
		{
			memcpy(buf,rcv_buf,rcv_len);
			break;
		}
	}
	return rcv_len;
}


int query_plnode_info(unsigned char *buf)
{
	unsigned char rcv_buf[256];
	unsigned char j;
	int rcv_len = 0;
	//plc_packet_t pak;

	memset(rcv_buf,0x00,sizeof(rcv_buf));
	//for(j=0;j<3;j++)
	for(j=0;j<3;j++)	
	{
		PLC_LOCK;
		if(send_query_plnode_info_pak())
		{
			PrintLog(LOGTYPE_DOWNLINK, "send_cyc_point_pak_fail\n");
		}
		rcv_len = PlcRecvPkt(rcv_buf,100);
		PLC_UNLOCK;

		if(rcv_len>0)
		{
			memcpy(buf,rcv_buf,rcv_len);
			break;
		}
	}
	return rcv_len;
}


void add_meter_to_moderm(void)
{
	int i = 0;

	PrintLog(LOGTYPE_DOWNLINK, "add_meter_to_moderm...\n");

	add_meter_start = 1;
	for(i=0;i<MAX_METER;i++)
	{
		if(ParaMeter[i].metp_id && (ParaMeter[i].portcfg&0x1F) == PLC_PORT)	
		{
			//PrintLog(LOGTYPE_DATA, "ParaMeter[%d].proto = %d\n",i,ParaMeter[i].proto);
			//PrintLog(LOGTYPE_DATA, "ParaMeter[%d]index = %d\n",i,ParaMeter[i].index);
			if(add_one_meter((unsigned char *)&ParaMeter[i].addr[0],ParaMeter[i].index,ParaMeter[i].proto) != 1)
			{
				PrintLog(LOGTYPE_DOWNLINK, "add_one_meter_fail\n");
				add_meter_fail_cnt++;
				PrintLog(LOGTYPE_DOWNLINK, "add_meter_fail_cnt = %d\n",add_meter_fail_cnt);
			}
			else
			{
				PrintLog(LOGTYPE_DOWNLINK, "add_meter_index = %d\n",ParaMeter[i].index);
				add_meter_succ_cnt++;
			}
			Sleep(150);
		}
	}
	PrintLog(LOGTYPE_DOWNLINK, "add_meter_fail_cnt = %d\n",add_meter_fail_cnt);
	PrintLog(LOGTYPE_DOWNLINK, "add_meter_succ_cnt = %d\n",add_meter_succ_cnt);
	//add_meter_start = 0;
}








int find_meter_postion(const unsigned char *meter_addr)
{
	int i = 0;

	for(i=0;i<MAX_METER;i++)
	{
		if(meter_ene_buffer[i].met_id >= 2)
		{
			if(!HexComp(meter_addr,meter_ene_buffer[i].meter_addr,6))	
			{
				return i;
			}
		}
	}

	return -1;
}




int process_rcv_645_pak(unsigned char *buf,unsigned char len,unsigned char *meter_addr,unsigned char *ene)
{
	unsigned char i = 0;
	unsigned meter_postion = 0;
	unsigned char proto;

	if(buf == NULL || len <= 0)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...len...err\n");
		return -1;
	}
	if(len < 18)	return -1;
	memcpy(meter_addr,buf + 3,6);

	meter_postion = find_meter_postion(meter_addr);
	if(meter_postion<0)	return 	0;

	proto = meter_ene_buffer[meter_postion].meter_proto;

	if(proto == PROTO_DL1997_376_1)
	{
		memcpy(ene,buf + 14,4);
	}
	else if(proto == PROTO_DL2007_376_1)
	{
		memcpy(ene,buf + 16,4);
	}
	for(i=0;i<4;i++)
	{
		ene[i] -= 0x33;
	}
	return meter_postion;
}

#if 0
/**
* @brief 载波抄表任务
*/
static void *PlcTask(void *arg)
{
	unsigned char rcv_buf[256];
	int i = 0,rcv_len = 0;
	unsigned char run_stat = 0;
	unsigned long ev = 0;
	plc_packet_t pak;

	unsigned char active_receive_buf[256];
	unsigned char point_read_receive_buf[256];
	int active_receive_len = 0;
	int point_read_receive_len = 0;
	pak_645_t pak_645;
	unsigned char meter_addr_tmp[6];
	int meter_postion = 0;
	unsigned char ene_buf[20];
	sysclock_t read_meter_succ_time;
	sysclock_t clock;
	const para_commport_t *pcfg = GetParaCommPort(COMMPORT_PLC);

	
	//unsigned char *pbuf;

	//pbuf = active_receive_buf;
	active_receive_len = 0;
	memset(active_receive_buf,0x00,sizeof(active_receive_buf));
	point_read_receive_len = 0;
	memset(active_receive_buf,0x00,sizeof(point_read_receive_buf));

	if(hard_ware_reset() != 1)
	{
		PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_fail\n");
	}
	Sleep(1000);


	while(1) 
	{
		//SysWaitEvent(&plc_meter_event, 1, METEV_1MIN_CHECK_RUTER_RUN_STAT, &ev);
		//if(PlReadStopped)		continue;


		if(pcfg->flag&RDMETFLAG_ENABLE) 	continue;	//禁止自动抄表
		
		
		if(PlReadStopped && !PlReadStoppedFlag)//禁止自动抄表则不抄表		
		{
			for(i=0;i<10;i++)
			{
				if(hard_ware_reset() != 1)
				{
					PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_fail\n");
				}
				else
				{
					PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_succ\n");
					PlReadStoppedFlag = 1;
					Sleep(1000);
					break;
				}
				Sleep(1000);
			}
		}

		if(PlReadStopped && PlReadStoppedFlag)		continue;
			//PrintLog(0, "禁止自动抄表\n");
		//else 
			//PrintLog(0, "允许自动抄表\n");

		
		SysWaitEvent(&plc_meter_event, 0, PLC_EVENT, &ev);

		if(ev&METEV_1MIN_CHECK_RUTER_RUN_STAT) 
		{
			if(get_meter_param_change_flag())
			{
				clear_meter_param_change_flag();
				point_read_start = 0;//停止点抄
				for(i=0;i<10;i++)
				{
					if(hard_ware_reset() != 1)
					{
						PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_fail\n");
					}
					else
					{
						PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_succ\n");
						Sleep(1000);
						break;
					}
					Sleep(1000);
				}
				if(i>=10)		continue;

				for(i=0;i<10;i++)
				{
					if(init_param() != 1)
					{
						PrintLog(LOGTYPE_DOWNLINK, "init_param_fail\n");
					}
					else
					{
						PrintLog(LOGTYPE_DOWNLINK, "init_param_succ\n");
						Sleep(1000);
						break;
					}
					Sleep(1000);
				}
				if(i>=10)		continue;
				
				add_meter_to_moderm();
				Sleep(1000);
				for(i=0;i<10;i++)
				{
					if(hard_ware_reset() != 1)
					{
						PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_fail\n");
					}
					else
					{
						PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_succ\n");
						Sleep(1000);
						break;
					}
					Sleep(1000);
				}
				if(i>=10)		continue;

				clear_read_meter_stat();

				point_read_start = 0;
				for(i=0;i<10;i++)
				{
					if(enable_active_rgst() != 1)
					{
						PrintLog(LOGTYPE_DOWNLINK, "enable_active_rgst_fail\n");
					}
					else
					{
						active_rgst_start = 1;
						PrintLog(LOGTYPE_DOWNLINK, "enable_active_rgst_succ\n");
						break;
					}
					Sleep(1000);
				}
			}
			
			//控制点抄轮抄的顺序
			if(!add_meter_start && !PlNeedFrez && !point_read_start)
			{
				get_ruter_stat = 1;
				run_stat = 0x03;//设置为忙
				if(get_ruter_run_stat(&run_stat) != 1)//读取失败
				{
					run_stat = 0x03;//设置为忙
					ruter_is_busy = 1;
					PrintLog(LOGTYPE_DOWNLINK, "get_ruter_run_stat_fail\n");
				}
				else
				{
					get_ruter_stat = 0;
					PrintLog(LOGTYPE_DOWNLINK, "get_ruter_run_stat_succ\n");
				}

				if(run_stat & 0x02)
				{
					ruter_is_busy = 1;
				}
				else
				{
					ruter_is_busy = 0;
					
	 				if(cyc_read_start_07)
					{
						cyc_read_start_07 = 0;
						point_read_start = 1;
					}

					if(cyc_read_start_97)
					{
						cyc_read_start_97 = 0;
						cyc_read_start_07 = 1;
					}

					if(cyc_read_start)
					{
						cyc_read_start = 0;
						cyc_read_start_97 = 1;
					}	

					if(active_rgst_start)
					{
						active_rgst_start = 0;
						cyc_read_start = 1;
						point_read_start = 0;
					}
					if(set_ruter_work_mode_start)
					{
						set_ruter_work_mode_start = 0;
						cyc_read_start = 1;
						point_read_start = 0;
					}
					PrintLog(LOGTYPE_DOWNLINK, "cyc_read_start = %d\n",cyc_read_start);
					PrintLog(LOGTYPE_DOWNLINK, "cyc_read_start_97 = %d\n",cyc_read_start_97);
					PrintLog(LOGTYPE_DOWNLINK, "cyc_read_start_07 = %d\n",cyc_read_start_07);
					PrintLog(LOGTYPE_DOWNLINK, "point_read_start = %d\n",point_read_start);
					PrintLog(LOGTYPE_DOWNLINK, "active_rgst_start = %d\n",active_rgst_start);
					PrintLog(LOGTYPE_DOWNLINK, "set_ruter_work_mode_start = %d\n",set_ruter_work_mode_start);
					PrintLog(LOGTYPE_DOWNLINK, "add_meter_start = %d\n",add_meter_start);
				}
			}
		}

		//定时启动补组网
		if(ev&METEV_SET_RUTER_WORK_MOD	) 
		{
			//SvrNoteProc(2);
			#if 1
			PrintLog(LOGTYPE_DOWNLINK, "start_set_ruter_work_mode...\n");
			point_read_start = 0;
			cyc_read_start_97 = 0;
			for(i=0;i<10;i++)
			{
				if(hard_ware_reset() != 1)
				{
					PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_fail\n");
				}
				else
				{
					PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_succ\n");
					Sleep(1000);
					break;
				}
				Sleep(1000);
			}
			//if(i>=10)		continue;
			Sleep(2000);

			for(i=0;i<10;i++)
			{
				if(set_ruter_work_mode() != 1)
				{
					PrintLog(LOGTYPE_DOWNLINK, "set_ruter_work_mode_fail\n");
				}
				else
				{
					set_ruter_work_mode_start = 1;
					//ruter_is_busy = 1;
					Sleep(1000);
					break;
				}
				Sleep(1000);
			}
			//if(i>=10)		continue;
			Sleep(100);
			#endif
		}

		//处理主动注册报文
		if(active_rgst_start)
		{
			rcv_len = 0;
			rcv_len = PlcRecvPkt(rcv_buf,10);
			if(rcv_len>0)	
			{
				PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
				PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
				for(i=0;i<rcv_len;i++)
					PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
				PrintLog(LOGTYPE_DOWNLINK, "\n");
				if(check_rcv_frame(rcv_buf,rcv_len) == 1)
				{
					memset((unsigned char *)&pak.head,0x00,sizeof(pak));
					memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
					if(pak.afn == 0x06 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)
					{
						confirm_ack();
					}
				}
				rcv_len = 0;
			}
		}

		//处理补组网返回的报文
		if(set_ruter_work_mode_start)
		{
			rcv_len = 0;
			rcv_len = PlcRecvPkt(rcv_buf,10);
			if(rcv_len>0)	
			{
				PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
				PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
				for(i=0;i<rcv_len;i++)
					PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
				PrintLog(LOGTYPE_DOWNLINK, "\n");
				if(check_rcv_frame(rcv_buf,rcv_len) == 1)
				{
					memset((unsigned char *)&pak.head,0x00,sizeof(pak));
					memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
					if(pak.afn == 0x06 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)
					{
						confirm_ack();
					}
				}
				rcv_len = 0;
			}
		}
		//启动97表轮抄
		if(!ruter_is_busy && cyc_read_start_97 && !point_read_start)
		{
			//Sleep(5000);//等待点抄结束
			Sleep(10);//等待点抄结束
			//for(i=0;i<10;i++)
			//{
			//	if(hard_ware_reset() != 1)
			//	{
			//		PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_fail\n");
			//	}
			//	else
			//	{
			//		PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_succ\n");
			////		Sleep(1000);
			//		break;
			//	}
			//	Sleep(1000);
			//}
			//if(i>=10)		continue;

			for(i=0;i<10;i++)
			{
				if(cyc_read(0x9010) != 1)
				{
					PrintLog(LOGTYPE_DOWNLINK, "cyc_read_fail\n");
				}
				else
				{
					PrintLog(LOGTYPE_DOWNLINK, "cyc_read_succ\n");
					SysClockReadCurrent(&clock);
					if(clock.hour<1)
					{
						SysClockReadCurrent(&read_meter_start_time);
						SysClockReadCurrent(&cyc_read_meter_start_time);
					}
					ruter_is_busy = 1;
					break;
				}
				Sleep(1500);
			}
		}

		//启动07表轮抄
		if(!ruter_is_busy && cyc_read_start_07 && !point_read_start)
		{
			//Sleep(5000);//等待点抄结束
			Sleep(10);//等待点抄结束
			//for(i=0;i<10;i++)
			//{
			//	if(hard_ware_reset() != 1)
			//	{
			//		PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_fail\n");
			//	}
			//	else
			//	{
			//		PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_succ\n");
			////		Sleep(1000);
			//		break;
			//	}
			//	Sleep(1000);
			//}
			//if(i>=10)		continue;

			for(i=0;i<10;i++)
			{
				if(cyc_read(0x00010000) != 1)
				{
					PrintLog(LOGTYPE_DOWNLINK, "cyc_read_fail\n");
				}
				else
				{
					PrintLog(LOGTYPE_DOWNLINK, "cyc_read_succ\n");
					SysClockReadCurrent(&clock);
					if(clock.hour<1)
					{
						SysClockReadCurrent(&read_meter_start_time);
						SysClockReadCurrent(&cyc_read_meter_start_time);
					}
					ruter_is_busy = 1;
					break;
				}
				Sleep(1500);
			}
		}
		
		if(cyc_read_start_97 || cyc_read_start_07)
		{
			PLC_LOCK;
			active_receive_len = PlcRecvPkt(active_receive_buf,10);
			PLC_UNLOCK;
			if(active_receive_len>0)	
			{
				PrintLog(LOGTYPE_DOWNLINK, "active_receive_len = %d\n",active_receive_len);
				PrintLog(LOGTYPE_DOWNLINK, "active_receive_buf = ");
				for(i=0;i<active_receive_len;i++)
					PrintLog(LOGTYPE_DOWNLINK, "%02X ",active_receive_buf[i]);
				PrintLog(LOGTYPE_DOWNLINK, "\n");

				if(check_rcv_frame(active_receive_buf,active_receive_len) == 1)
				{
					PrintLog(LOGTYPE_DOWNLINK, "rcv_active_report_data...1\n");
					memset((unsigned char *)&pak.head,0x00,sizeof(pak));
					memset((unsigned char *)pak_645.meter_index,0x00,sizeof(pak_645));
					memcpy((unsigned char *)&pak.head,active_receive_buf,active_receive_len);
					if(pak.afn == 0x06 && pak.fn[0] == 0x02 && pak.fn[1] == 0x00)
					{
						PrintLog(LOGTYPE_DOWNLINK, "rcv_active_report_data...2\n");
						//memcpy((unsigned char *)pak_645.meter_index,(unsigned char *)pak.data,active_receive_len - 13);
						memset(meter_addr_tmp,0x00,6);
						memset(ene_buf,0x00,20);
						meter_postion = -1;
						//pak_645.proto = PROTO_DL1997;
						PrintLog(LOGTYPE_DOWNLINK, "rcv_active_report_data...3\n");
						meter_postion = process_rcv_645_pak((unsigned char *)&pak.data[2],pak.data[3],meter_addr_tmp,ene_buf);
						//if(meter_postion>=8)
						if(meter_postion>=2)	
						//if(process_rcv_645_pak((unsigned char *)&pak.data[2],pak.data[3],meter_addr_tmp,ene_buf) == 1)
						{
							PrintLog(LOGTYPE_DOWNLINK, "rcv_active_report_meter_addr = ");
							for(i=0;i<6;i++)
								PrintLog(LOGTYPE_DOWNLINK, "%02X ",meter_addr_tmp[i]);
							PrintLog(LOGTYPE_DOWNLINK, "\n");
							//meter_postion = find_meter_postion(meter_addr_tmp);
							PrintLog(LOGTYPE_DATA, "meter_postion = %d\n",meter_postion);
							//meter_ene_buffer[meter_postion].meter_index = meter_postion + 1;
							//if(meter_postion >= 0)
							//{
								confirm_ack();
								cyc_read_check_time_out = 0;
								if(meter_ene_buffer[meter_postion].read_stat != HAVE_READ)
								{
									memcpy((unsigned char *)&meter_ene_buffer[meter_postion].meter_ene[0],ene_buf,20);
									meter_ene_buffer[meter_postion].read_stat = HAVE_READ;
									meter_read_succ_cnt++;
									SysClockReadCurrent(&read_meter_succ_time);
									meter_ene_buffer[meter_postion].readtime = clock_to_read_time(&read_meter_succ_time, 0);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_ene[0],(unsigned char *)&meter_ene_buffer[meter_postion].meter_ene[0],4);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].readtime,(unsigned char *)&meter_ene_buffer[meter_postion].readtime,2);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_num,(unsigned char *)&meter_ene_buffer[meter_postion].met_id,2);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_addr[0],(unsigned char *)&meter_ene_buffer[meter_postion].meter_addr[0],6);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].read_stat,(unsigned char *)&meter_ene_buffer[meter_postion].read_stat,1);
								}
							//}
						}
					}
				}
			}
			active_receive_len = 0;
			memset(active_receive_buf,0x00,256);
		}
		//PrintLog(0, "meter_ene_buffer[point_read_index].portcfg = %d\n",meter_ene_buffer[point_read_index].portcfg&0x1F);
		//printf("meter_ene_buffer[point_read_index].portcfg = %d\n",meter_ene_buffer[point_read_index].portcfg&0x1F);
		if(point_read_start && (!add_meter_start) && (!cyc_read_start_97) && (!cyc_read_start_07) && (!set_ruter_work_mode_start) && (!active_rgst_start))
		{
			if(meter_ene_buffer[point_read_index].met_id && (meter_ene_buffer[point_read_index].portcfg&0x1F) == PLC_PORT)
			//PrintLog(0, "meter_ene_buffer[point_read_index].portcfg = %d\n",meter_ene_buffer[point_read_index].portcfg&0x1F);
			//printf("meter_ene_buffer[point_read_index].portcfg = %d\n",meter_ene_buffer[point_read_index].portcfg&0x1F);
			//if(meter_ene_buffer[point_read_index].met_id)
			{
				if(meter_ene_buffer[point_read_index].read_stat != HAVE_READ)
				{
					PrintLog(LOGTYPE_DOWNLINK, "point_read...\n");
					PrintLog(LOGTYPE_DOWNLINK, "point_read_index = %d\n",point_read_index);
					PrintLog(LOGTYPE_DOWNLINK, "meter_proto = %d\n",meter_ene_buffer[point_read_index].meter_proto);
					if(meter_ene_buffer[point_read_index].meter_proto == PROTO_DL1997_376_1)
					{
						point_read_receive_len = point_read(0x9010,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						pak_645.proto = PROTO_DL1997;
					}
					else if(meter_ene_buffer[point_read_index].meter_proto == PROTO_DL2007_376_1)
					{
						point_read_receive_len = point_read(0x00010000,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						pak_645.proto = PROTO_DL2007;
					}
					if(point_read_receive_len>0)	
					{
						PrintLog(LOGTYPE_DOWNLINK, "point_read_receive_len = %d\n",point_read_receive_len);
						PrintLog(LOGTYPE_DOWNLINK, "point_read_receive_buf = ");
						for(i=0;i<point_read_receive_len;i++)
							PrintLog(LOGTYPE_DOWNLINK, "%02X ",point_read_receive_buf[i]);
						PrintLog(LOGTYPE_DOWNLINK, "\n");

						if(check_rcv_frame(point_read_receive_buf,point_read_receive_len) == 1)
						{
							PrintLog(LOGTYPE_DOWNLINK, "rcv_point_read_report_data...1\n");
							memset((unsigned char *)&pak.head,0x00,sizeof(pak));
							memset((unsigned char *)pak_645.meter_index,0x00,sizeof(pak_645));
							memcpy((unsigned char *)&pak.head,point_read_receive_buf,point_read_receive_len);
							if(pak.afn == 0x13 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)	
							{
								PrintLog(LOGTYPE_DOWNLINK, "rcv_point_read_report_data...2\n");
								//memcpy((unsigned char *)pak_645.meter_index,(unsigned char *)pak.data,point_read_receive_len);
								memset(meter_addr_tmp,0x00,6);
								memset(ene_buf,0x00,20);
								meter_postion = -1;
								//pak_645.proto = PROTO_DL1997;
								//pak_645.proto = PROTO_DL2007;
								pak_645.len = pak.data[1];
								meter_postion = process_rcv_645_pak((unsigned char *)pak.data,pak_645.len,meter_addr_tmp,ene_buf);
								//if(meter_postion>=8)
								if(meter_postion>=2)	
								//if(process_rcv_645_pak((unsigned char *)pak.data,pak_645.len,pak_645.proto,meter_addr_tmp,ene_buf) == 1)
								{
									PrintLog(LOGTYPE_DOWNLINK, "rcv_point_read_report_meter_addr = ");
									for(i=0;i<6;i++)
										PrintLog(LOGTYPE_DOWNLINK, "%02X ",meter_addr_tmp[i]);
									PrintLog(LOGTYPE_DOWNLINK, "\n");
									//meter_postion = find_meter_postion(meter_addr_tmp);
									PrintLog(LOGTYPE_DOWNLINK, "meter_postion = %d\n",meter_postion);
									confirm_ack();
									memcpy(meter_ene_buffer[meter_postion].meter_ene,ene_buf,20);
									meter_ene_buffer[meter_postion].read_stat = HAVE_READ;
									meter_read_succ_cnt++;
									SysClockReadCurrent(&read_meter_succ_time);
									meter_ene_buffer[meter_postion].readtime = clock_to_read_time(&read_meter_succ_time, 0);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_ene[0],(unsigned char *)&meter_ene_buffer[meter_postion].meter_ene[0],4);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].readtime,(unsigned char *)&meter_ene_buffer[meter_postion].readtime,2);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_num,(unsigned char *)&meter_ene_buffer[meter_postion].met_id,2);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_addr[0],(unsigned char *)&meter_ene_buffer[meter_postion].meter_addr[0],6);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].read_stat,(unsigned char *)&meter_ene_buffer[meter_postion].read_stat,1);
								}
							}
						}
					}
					point_read_receive_len = 0;
					memset(point_read_receive_buf,0x00,sizeof(point_read_receive_buf));
					//Sleep(800);
				}
				//Sleep(800);
			}
			point_read_index++;
			if(ParaPlcMetp[point_read_index -1].stopped) 
			{
				point_read_index++;
			}
			if(point_read_index == MAX_METER_CNT)
			//if(point_read_index == meter_total_cnt)
			//if(meter_total_cnt == point_read_index)	
			{
				point_read_index = 0;
			}
		}
		//Sleep(10);
	}
}

#endif


/**
* @brief 载波抄表任务
*/
static void *PlcTask(void *arg)
{
	unsigned char rcv_buf[256];
	int i = 0;
	int rcv_len = 0;
	unsigned char run_stat = 0;
	unsigned long ev = 0;
	plc_packet_t pak;
	unsigned char active_receive_buf[256];
	unsigned char point_read_receive_buf[256];
	int active_receive_len = 0;
	int point_read_receive_len = 0;
	pak_645_t pak_645;
	unsigned char meter_addr_tmp[6];
	int meter_postion = 0;
	unsigned char ene_buf[20];
	sysclock_t read_meter_succ_time;
	const para_commport_t *pcfg = GetParaCommPort(COMMPORT_PLC);
	unsigned char query_pl_node_flag = 0;
	unsigned char pl_node_cnt[2],pl_have_read_node_cnt[2],pl_fwd_read_node_cnt[2];

	active_receive_len = 0;
	query_pl_node_flag = 0;
	memset(active_receive_buf,0x00,sizeof(active_receive_buf));
	point_read_receive_len = 0;
	memset(active_receive_buf,0x00,sizeof(point_read_receive_buf));
	rcv_len = 0;
	#if 0
	if(hard_ware_reset() != 1)
	{
		PrintLog(LOGTYPE_DOWNLINK, "hard_ware_reset_fail\n");
	}
	#endif
	Sleep(1000);


	while(1) 
	{
		//SysWaitEvent(&plc_meter_event, 1, METEV_1MIN_CHECK_RUTER_RUN_STAT, &ev);

		//if(!(meter_total_cnt - cen_meter_cnt))	continue;
		
		if(query_pl_node_info_start)	continue;
		if(query_pl_node_info)	continue;

		if(pcfg->flag&RDMETFLAG_ENABLE) 	//禁止自动抄表
		{
			Sleep(50);	
			continue;	
		}
		if(PlReadStopped)		
		{
			Sleep(50);	
			continue;
		}

		SysWaitEvent(&plc_meter_event, 0, PLC_EVENT, &ev);

		if((ev&METEV_1MIN_CHECK_RUTER_RUN_STAT)) 	
		{
			if(!add_meter_start)	
			{
				if(get_meter_param_change_flag())
				{
					clear_meter_param_change_flag();
					Sleep(6000*3);
					if(get_meter_param_change_flag())
					{
						clear_meter_param_change_flag();
					}
					ACTIVE_REGISTER:
					point_read_start = 0;//停止点抄
					add_meter_start = 1;
					set_ruter_work_mode_start = 0;
					meter_total_cnt = get_meter_total_cnt();
					cen_meter_cnt = get_cen_meter_total_cnt();
					imp_meter_cnt = get_imp_meter_total_cnt();
					imp_meter_read_succ_cnt = get_imp_meter_read_succ_cnt();

					for(i=0;i<10;i++)
					{
						if(init_param() != 1)
						{
							PrintLog(LOGTYPE_DOWNLINK, "init_param_fail\n");
						}
						else
						{
							PrintLog(LOGTYPE_DOWNLINK, "init_param_succ\n");
							Sleep(1000);
							break;
						}
						Sleep(1000);
					}
					Sleep(1000);
					add_meter_to_moderm();
					Sleep(1000);
					for(i=0;i<10;i++)
					{
						if(set_plnode_addr() != 1)
						{
							PrintLog(LOGTYPE_DOWNLINK, "set_plnode_addr_fail\n");
						}
						else
						{
							PrintLog(LOGTYPE_DOWNLINK, "set_plnode_addr_succ\n");
							Sleep(1000);
							break;
						}
						Sleep(1000);
					}
					for(i=0;i<10;i++)
					{
						if(enable_active_rgst() != 1)
						{
							PrintLog(LOGTYPE_DOWNLINK, "enable_active_rgst_fail\n");
						}
						else
						{
							active_rgst_start = 1;
							add_meter_start = 0;
							Sleep(1000);
							PrintLog(LOGTYPE_DOWNLINK, "enable_active_rgst_succ\n");
							break;
						}
						Sleep(1000);
					}

					for(i=0;i<MAX_METER_CNT;i++)
					{
						meter_ene_buffer[i].read_stat = 0x00;
						memcpy((unsigned char *)&meter_ene_buffer[i].meter_index,(unsigned char *)&ParaMeter[i].index,4);//序号，测量点号
						memcpy((unsigned char *)&meter_ene_buffer[i].portcfg,(unsigned char *)&ParaMeter[i].portcfg,1);
						memcpy((unsigned char *)&meter_ene_buffer[i].meter_proto,(unsigned char *)&ParaMeter[i].proto,1);
						memcpy((unsigned char *)&meter_ene_buffer[i].meter_addr,(unsigned char *)&ParaMeter[i].addr,6);
						memset((unsigned char *)meter_ene_buffer[i].meter_ene,0xEE,20);
						memset((unsigned char *)&pl_node_info,0x00,sizeof(pl_node_info_t));
					}
				}
			}
			
			if(!add_meter_start)	
			{
				get_ruter_stat = 1;
				run_stat = 0x03;//设置为忙
				if(get_ruter_run_stat(&run_stat,pl_node_cnt,pl_have_read_node_cnt,pl_fwd_read_node_cnt) != 1)
				{
					run_stat = 0x03;//设置为忙
					ruter_is_busy = 1;
					PrintLog(LOGTYPE_DOWNLINK, "get_ruter_run_stat_fail\n");
				}
				else
				{
					get_ruter_stat = 0;
					PrintLog(LOGTYPE_DOWNLINK, "get_ruter_run_stat_succ\n");
					if(run_stat & 0x02)
						PrintLog(LOGTYPE_DOWNLINK, "路由正在工作\n");
					if(!(run_stat & 0x02))
						PrintLog(LOGTYPE_DOWNLINK, "路由停止工作\n");
					if((run_stat & 0x01) == 1)
						PrintLog(LOGTYPE_DOWNLINK, "路由学习完成\n");
					if((run_stat & 0x01) == 0)
						PrintLog(LOGTYPE_DOWNLINK, "路由学习未完成\n");
					PrintLog(LOGTYPE_DOWNLINK, "从节点总数量= %d\n",MAKE_SHORT(pl_node_cnt));
					PrintLog(LOGTYPE_DOWNLINK, "已抄从节点数量= %d\n",MAKE_SHORT(pl_have_read_node_cnt));
					PrintLog(LOGTYPE_DOWNLINK, "中继数量= %d\n",MAKE_SHORT(pl_fwd_read_node_cnt));
					if(MAKE_SHORT(pl_have_read_node_cnt) == 0)
					{
						active_rgst_command++;
					}
					if(active_rgst_command >=80)
					{
						active_rgst_command = 0;
						goto ACTIVE_REGISTER;
					}
				}
				if(!(run_stat & 0x02) )
				//if(!(run_stat & 0x02) && ((run_stat & 0x01) == 0))	
				{
					if(active_rgst_start)
					{
						active_rgst_start = 0;
						PrintLog(LOGTYPE_DOWNLINK, "active_rgst_finished\n");
					}
					#if 1
					if(!point_read_start && (!add_meter_start) && ((run_stat & 0x01) == 0))
					{
						PrintLog(LOGTYPE_DOWNLINK, "set_ruter_work_mode_finished\n");
						PrintLog(LOGTYPE_DOWNLINK, "go_on_set_ruter_work_mode\n");
						for(i=0;i<10;i++)
						{
							if(set_ruter_work_mode() != 1)
							{
								PrintLog(LOGTYPE_DOWNLINK, "set_ruter_work_mode_fail\n");
							}
							else
							{
								PrintLog(LOGTYPE_DOWNLINK, "set_ruter_work_mode_succ\n");
								Sleep(1000);
								break;
							}
							Sleep(1000);
						}
					}
					#endif
				}
				PrintLog(LOGTYPE_DOWNLINK, "point_read_start = %d\n",point_read_start);
				PrintLog(LOGTYPE_DOWNLINK, "active_rgst_start = %d\n",active_rgst_start);
				PrintLog(LOGTYPE_DOWNLINK, "set_ruter_work_mode_start = %d\n",set_ruter_work_mode_start);
				PrintLog(LOGTYPE_DOWNLINK, "add_meter_start = %d\n",add_meter_start);
			}
		}

		if(active_rgst_start && (!point_read_start) && (!add_meter_start)&& (!pl_shell_cmd_flag))		//处理主动注册报文
		{
			rcv_len = 0;
			rcv_len = PlcRecvPkt(rcv_buf,10);
			if(rcv_len>0)	
			{
				PrintLog(LOGTYPE_DOWNLINK, "rcv_active_rgst_report...\n");
				PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
				PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
				for(i=0;i<rcv_len;i++)
					PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
				PrintLog(LOGTYPE_DOWNLINK, "\n");
				if(check_rcv_frame(rcv_buf,rcv_len) == 1)
				{
					memset((unsigned char *)&pak.head,0x00,sizeof(pak));
					memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
					if(pak.afn == 0x06 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)
					{
						confirm_ack();
					}
				}
				rcv_len = 0;
			}
		}
		if((!active_rgst_start) && (!point_read_start)&& (!add_meter_start)&& (!pl_shell_cmd_flag))		//处理补组网返回的报文
		{
			rcv_len = 0;
			rcv_len = PlcRecvPkt(rcv_buf,10);
			if(rcv_len>0)	
			{
				PrintLog(LOGTYPE_DOWNLINK, "rcv_set_ruter_work_mode_report...\n");
				PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
				PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
				for(i=0;i<rcv_len;i++)
					PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
				PrintLog(LOGTYPE_DOWNLINK, "\n");
				if(check_rcv_frame(rcv_buf,rcv_len) == 1)
				{
					memset((unsigned char *)&pak.head,0x00,sizeof(pak));
					memcpy((unsigned char *)&pak.head,rcv_buf,rcv_len);
					if(pak.afn == 0x06 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)
					{
						confirm_ack();
					}
				}
				rcv_len = 0;
			}
		}	
		if(!(meter_total_cnt - cen_meter_cnt))	continue;
		if(point_read_start && (!add_meter_start) && (!active_rgst_start) && (!stop_point_read_flag)&& (!pl_shell_cmd_flag))
		{
			if(meter_ene_buffer[point_read_index].met_id && (meter_ene_buffer[point_read_index].portcfg&0x1F) == PLC_PORT)
			{
				//if(meter_ene_buffer[point_read_index].read_stat != HAVE_READ)
				if(check_buf(meter_ene_buffer[point_read_index].meter_ene))
				{
					PrintLog(LOGTYPE_DOWNLINK, "read %d meter...\n",point_read_index + 1);
					#if 0
					if(meter_ene_buffer[point_read_index].meter_proto == PROTO_DL1997_376_1)
					{
						if((start_point_read_timer>0) && (point_read_cyc/2))
						{
							point_read_receive_len = point_read(0x901F,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						}
						else
						{
							point_read_receive_len = point_read(0x9010,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						}
						pak_645.proto = PROTO_DL1997;
					}
					else if(meter_ene_buffer[point_read_index].meter_proto == PROTO_DL2007_376_1)
					{
						if((start_point_read_timer>0) && (point_read_cyc/2))
						{
							point_read_receive_len = point_read(0x0001FF00,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						}
						else
						{
							point_read_receive_len = point_read(0x00010000,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						}
						pak_645.proto = PROTO_DL2007;
					}
					#endif
					if(meter_ene_buffer[point_read_index].meter_proto == PROTO_DL1997_376_1)
					{
						if(ParaMeter[point_read_index].userclass == 3)
						{
							point_read_receive_len = point_read(0x901F,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						}
						else
						{
							point_read_receive_len = point_read(0x9010,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						}
						pak_645.proto = PROTO_DL1997;
					}
					else if(meter_ene_buffer[point_read_index].meter_proto == PROTO_DL2007_376_1)
					{
						if(ParaMeter[point_read_index].userclass == 3)
						{
							point_read_receive_len = point_read(0x0001FF00,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);
						}
						else
						{
							point_read_receive_len = point_read(0x00010000,meter_ene_buffer[point_read_index].meter_addr,point_read_receive_buf);							
						}
						pak_645.proto = PROTO_DL2007;
					}

					if(point_read_receive_len>0)	
					{
						PrintLog(LOGTYPE_DATA, "point_read_receive_len = %d\n",point_read_receive_len);
						PrintLog(LOGTYPE_DATA, "point_read_receive_buf = ");
						for(i=0;i<point_read_receive_len;i++)
							PrintLog(LOGTYPE_DATA, "%02X ",point_read_receive_buf[i]);
						PrintLog(LOGTYPE_DATA, "\n");
						if(check_rcv_frame(point_read_receive_buf,point_read_receive_len) == 1)
						{
							PrintLog(LOGTYPE_DATA, "rcv_point_read_report_data...1\n");
							memset((unsigned char *)&pak.head,0x00,sizeof(pak));
							memset((unsigned char *)pak_645.meter_index,0x00,sizeof(pak_645));
							memcpy((unsigned char *)&pak.head,point_read_receive_buf,point_read_receive_len);
							if(pak.afn == 0x13 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)	
							{
								PrintLog(LOGTYPE_DATA, "rcv_point_read_report_data...2\n");
								memset(meter_addr_tmp,0x00,6);
								memset(ene_buf,0x00,20);
								meter_postion = -1;
								pak_645.len = pak.data[1];
								meter_postion = process_rcv_645_pak((unsigned char *)pak.data,pak_645.len,meter_addr_tmp,ene_buf);
								if(meter_postion>=2)	
								{
									#if 1
									PrintLog(LOGTYPE_DATA, "rcv_point_read_report_meter_addr = ");
									for(i=0;i<6;i++)
										PrintLog(LOGTYPE_DATA, "%02X ",meter_addr_tmp[i]);
									PrintLog(LOGTYPE_DATA, "\n");
									PrintLog(LOGTYPE_DATA, "meter_postion = %d\n",meter_postion);
									confirm_ack();
									memcpy(meter_ene_buffer[meter_postion].meter_ene,ene_buf,20);//得到抄收的电能量
									meter_ene_buffer[meter_postion].read_stat = HAVE_READ;
									SysClockReadCurrent(&read_meter_succ_time);
									meter_ene_buffer[meter_postion].readtime = clock_to_read_time(&read_meter_succ_time, 0);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_ene[0],(unsigned char *)&meter_ene_buffer[meter_postion].meter_ene[0],4);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].readtime,(unsigned char *)&meter_ene_buffer[meter_postion].readtime,2);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_num,(unsigned char *)&meter_ene_buffer[meter_postion].met_id,2);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_addr[0],(unsigned char *)&meter_ene_buffer[meter_postion].meter_addr[0],6);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].read_stat,(unsigned char *)&meter_ene_buffer[meter_postion].read_stat,1);
									PrintLog(LOGTYPE_DOWNLINK, "read %d meter succ\n\n",point_read_index + 1);
									#endif
									#if 0
									confirm_ack();
									memcpy(meter_ene_buffer[point_read_index].meter_ene,ene_buf,20);//得到抄收的电能量
									meter_ene_buffer[point_read_index].read_stat = HAVE_READ;
									meter_read_succ_cnt++;
									SysClockReadCurrent(&read_meter_succ_time);
									meter_ene_buffer[point_read_index].readtime = clock_to_read_time(&read_meter_succ_time, 0);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_ene[0],(unsigned char *)&meter_ene_buffer[point_read_index].meter_ene[0],4);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].readtime,(unsigned char *)&meter_ene_buffer[point_read_index].readtime,2);
									//memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_num,(unsigned char *)&meter_ene_buffer[meter_postion].met_id,2);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_num,(unsigned char *)&meter_ene_buffer[point_read_index].met_id,2);									
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_addr[0],(unsigned char *)&meter_ene_buffer[point_read_index].meter_addr[0],6);
									memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].read_stat,(unsigned char *)&meter_ene_buffer[point_read_index].read_stat,1);
									PrintLog(LOGTYPE_DOWNLINK, "read %d meter succ\n\n",point_read_index + 1);
									#endif
									#if 0
									confirm_ack();
									memcpy(meter_ene_buffer[point_read_index].meter_ene,ene_buf,20);//得到抄收的电能量
									meter_ene_buffer[point_read_index].read_stat = HAVE_READ;
									meter_read_succ_cnt++;
									SysClockReadCurrent(&read_meter_succ_time);
									meter_ene_buffer[point_read_index].readtime = clock_to_read_time(&read_meter_succ_time, 0);
									memcpy((unsigned char *)&PlMdbDay[point_read_index].meter_ene[0],(unsigned char *)&meter_ene_buffer[point_read_index].meter_ene[0],4);
									memcpy((unsigned char *)&PlMdbDay[point_read_index].readtime,(unsigned char *)&meter_ene_buffer[point_read_index].readtime,2);
									//memcpy((unsigned char *)&PlMdbDay[meter_postion - 2].meter_num,(unsigned char *)&meter_ene_buffer[meter_postion].met_id,2);
									memcpy((unsigned char *)&PlMdbDay[point_read_index].meter_num,(unsigned char *)&meter_ene_buffer[point_read_index].met_id,2);									
									memcpy((unsigned char *)&PlMdbDay[point_read_index].meter_addr[0],(unsigned char *)&meter_ene_buffer[point_read_index].meter_addr[0],6);
									memcpy((unsigned char *)&PlMdbDay[point_read_index].read_stat,(unsigned char *)&meter_ene_buffer[point_read_index].read_stat,1);
									PrintLog(LOGTYPE_DOWNLINK, "read %d meter succ\n\n",point_read_index + 1);
									#endif
								}
							}
						}
					}
					else
						PrintLog(LOGTYPE_DOWNLINK, "read %d meter fail\n\n",point_read_index + 1);
					point_read_receive_len = 0;
					memset(point_read_receive_buf,0x00,sizeof(point_read_receive_buf));
				}
			}
			point_read_index++;

			if(ParaPlcMetp[point_read_index -1].stopped) 
			{
				point_read_index++;
			}
			
			if(point_read_index == MAX_METER_CNT)
			{
				point_read_index = 0;
				point_read_cyc++;
			}
			//if(start_point_read_timer>0)
			//{
			//	Sleep(100*30);
				//Sleep(100*45);
			//}
		}
		if(PlcTask_watchdog>30)
		{
			PlcTask_watchdog = 0;
		}
	}
}


/**
* @brief 检查抄表时段
*/
static void PlcPeriodCheck(void)
{
	const para_commport_t *pcfg = GetParaCommPort(COMMPORT_PLC);
	sysclock_t clock;
	unsigned int i;
	unsigned int num;


	//printf("PlcPeriodCheck.......\n");
	SysClockReadCurrent(&clock);

	
	//if(clock.day < 3)  //一号或者2号进行月数据的冻结
  		//PlMonthReading = 1;
  	//else 
		//PlMonthReading = 0;

	// 23:50~0:10进行数据冻结，不能抄表
	i = (unsigned int)clock.hour * 60 + (unsigned int)clock.minute;
	//PrintLog(3, "clock.hour  = %d clock.minute = %d i = %d\n", (unsigned int)clock.hour,(unsigned int)clock.minute,i);
	if(i==2)
	{
		clear_read_meter_stat_flag = 1;
	}
	if(i==3)
	{
		if(clear_read_meter_stat_flag)
		{
			clear_read_meter_stat_flag = 0;
			clear_read_meter_stat();//清除抄表信息
		}
	}
	//printf("i = %d\n",i);
	//printf("query_pl_node_info = %d\n",query_pl_node_info);
	//printf("query_pl_node_info_start = %d\n",query_pl_node_info_start);
	if(i>7&&i<=240)
	{
		if(!point_read_start)
		{
			point_read_start = 1;
		}
	}
	if(i>240&&i<=1429)
	{
		if(point_read_start)
		{
			start_point_read_timer++;
			//if(start_point_read_timer>12*60)
			if(start_point_read_timer>12*45)	
			//if(start_point_read_timer>12*1)
			{
				start_point_read_timer = 0;
				point_read_start = 0;
			}
		}
		else
		{
			stop_point_read_timer++;
			//if(stop_point_read_timer>12*60)
			if(stop_point_read_timer>12*45)
			//if(stop_point_read_timer>12*1)
			{
				stop_point_read_timer = 0;
				point_read_start = 1;
			}
		}
	}
	if(active_rgst_start)
	{
		if(i==1140)//19:00
		{
			active_rgst_start = 0;
			stop_point_read_flag = 0;
			point_read_start = 1;
		}
	}

	
	if(i < 5) 
	{
		PlReadStopped = 1;
		PlNeedFrez = 0;
		PlFrezFinished = 0;
		
		return;
	}
	else if(i > 1430) 
	{
		//PlReadStopped = 1;
		//if(i > 1431) 
		//	PlNeedFrez = 1;
		//return;

		if(i >= 1431) 
			PlNeedFrez = 1;
		//if(i == 1432)
		//{
		//	query_pl_node_info = 1;
			//printf("query_pl_node_info = 1query_pl_node_info = 1query_pl_node_info = 1\n");
		//}
		point_read_start = 0;
		set_ruter_work_mode_start = 0;
		//if(query_pl_node_finish)
		//{
		//	query_pl_node_info = 0;
		//	query_pl_node_finish = 0;
		//}
		return;
	}
	else 
	{
		PlNeedFrez = 0;
		PlFrezFinished = 0;
		PlReadStopped = 0;
		query_pl_node_info = 0;
	}

	for(num=0; num<pcfg->periodnum; num++) 
	{
		if(i > pcfg->period[num].hour_start*60+pcfg->period[num].min_start && 
		   i < pcfg->period[num].hour_end*60+pcfg->period[num].min_end) 
		{
			PlReadStopped = 0;
			//PlReadStoppedFlag = 0;
			break;
		}		
	}

	if(num >= pcfg->periodnum) 
	{
		PlReadStopped = 1;
		//PlReadStoppedFlag = 1;
	}
	
}

void  check_read_meter_finished(void)
{
	if( meter_read_succ_cnt == (meter_total_cnt - cen_meter_cnt))
	{
		point_read_start = 0;
		if(!read_meter_finish)
		{
			SysClockReadCurrent(&read_meter_finish_time);
		}
		read_meter_finish = 1;
	}
	if((check_read_meter_succ_cnt()*100/(meter_total_cnt - cen_meter_cnt)) >= 90)
	{
		read_meter_major_frez = 1;
	}
}

void go_to_point_read(void)
{
	PrintLog(LOGTYPE_DOWNLINK, "go_to_point_read.....\n");
	
	point_read_start = 1;
	stop_point_read_flag = 0;
}

void stop_point_read(void)
{
	PrintLog(LOGTYPE_DOWNLINK, "stop_point_read.....\n");
	
	point_read_start = 0;
	stop_point_read_flag = 1;
}

void go_to_set_ruter_work_mode(void)
{
	int i;

	PrintLog(LOGTYPE_DOWNLINK, "go_to_set_ruter_work_mode.....\n");
	pl_shell_cmd_flag = 1;
	active_rgst_start = 0;
	for(i=0;i<10;i++)
	{
		if(set_ruter_work_mode() != 1)
		{
			PrintLog(LOGTYPE_DOWNLINK, "set_ruter_work_mode_fail\n");
		}
		else
		{
			PrintLog(LOGTYPE_DOWNLINK, "set_ruter_work_mode_succ\n");
			set_ruter_work_mode_start = 1;
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}
	pl_shell_cmd_flag = 0;
}

void go_to_active_register(void)
{
	int i;

	PrintLog(LOGTYPE_DOWNLINK, "go_to_active_register.....\n");
	pl_shell_cmd_flag = 1;
	point_read_start = 0;
	set_ruter_work_mode_start = 0;

	for(i=0;i<10;i++)
	{
		if(set_plnode_addr() != 1)
		{
			PrintLog(LOGTYPE_DOWNLINK, "set_plnode_addr_fail\n");
		}
		else
		{
			PrintLog(LOGTYPE_DOWNLINK, "set_plnode_addr_succ\n");
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}

	
	for(i=0;i<10;i++)
	{
		if(enable_active_rgst() != 1)
		{
			PrintLog(LOGTYPE_DOWNLINK, "enable_active_rgst_fail\n");
		}
		else
		{
			active_rgst_start = 1;
			PrintLog(LOGTYPE_DOWNLINK, "enable_active_rgst_succ\n");
			break;
		}
		Sleep(1000);
	}
	pl_shell_cmd_flag = 0;
}

void go_to_active_register_init(void)
{
	int i;

	PrintLog(LOGTYPE_DOWNLINK, "go_to_active_register_init.....\n");
	pl_shell_cmd_flag = 1;
	point_read_start = 0;
	set_ruter_work_mode_start = 0;
	
	for(i=0;i<10;i++)
	{
		if(init_param() != 1)
		{
			PrintLog(LOGTYPE_DOWNLINK, "init_param_fail\n");
		}
		else
		{
			PrintLog(LOGTYPE_DOWNLINK, "init_param_succ\n");
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}
	Sleep(1000);
	add_meter_to_moderm();
	Sleep(1000);

	for(i=0;i<10;i++)
	{
		if(set_plnode_addr() != 1)
		{
			PrintLog(LOGTYPE_DOWNLINK, "set_plnode_addr_fail\n");
		}
		else
		{
			PrintLog(LOGTYPE_DOWNLINK, "set_plnode_addr_succ\n");
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}

	
	for(i=0;i<10;i++)
	{
		if(enable_active_rgst() != 1)
		{
			PrintLog(LOGTYPE_DOWNLINK, "enable_active_rgst_fail\n");
		}
		else
		{
			active_rgst_start = 1;
			add_meter_start = 0;
			PrintLog(LOGTYPE_DOWNLINK, "enable_active_rgst_succ\n");
			break;
		}
		Sleep(1000);
	}
	pl_shell_cmd_flag = 0;
}




void go_to_query_pl_node_info(void)
{
	PrintLog(LOGTYPE_DOWNLINK, "go_to_query_pl_node_info.....\n");
	query_pl_node_info = 1;
}

void go_to_pl_add_meter_to_mod(void)
{
	static unsigned char active_rgst_start_bak = 0;
	static unsigned char set_ruter_work_mode_start_bak = 0;
	static unsigned char point_read_start_bak = 0;
	
	add_meter_start = 1;
	if(point_read_start)
	{
		point_read_start_bak = 1;
		point_read_start = 0;
	}
	if(set_ruter_work_mode_start)
	{
		set_ruter_work_mode_start_bak = 1;	
		set_ruter_work_mode_start = 0;
	}
	if(active_rgst_start)
	{
		active_rgst_start_bak = 1;	
		active_rgst_start = 0;
	}
	
	Sleep(1000);
	add_meter_to_moderm();
	Sleep(1000);
	add_meter_start = 0;

	if(point_read_start_bak)
		point_read_start = 1;
	if(set_ruter_work_mode_start_bak)
		set_ruter_work_mode_start = 1;
	if(active_rgst_start_bak)
		active_rgst_start = 1;	
}


/**
* @brief 载波冻结任务
*/
static void *PlFrezTask(void *arg)
{
	int pl_check_read_meter_cnt_timer = 0;
	
	Sleep(100);

	while(1) 
	{
		pl_check_read_meter_cnt_timer++;
		if(pl_check_read_meter_cnt_timer>=12)
		{
			pl_check_read_meter_cnt_timer = 0;
			meter_read_succ_cnt = check_read_meter_succ_cnt();
		}
		PlcPeriodCheck();
		if(!(meter_total_cnt - cen_meter_cnt))	continue;
		check_read_meter_finished();
		if(PlNeedFrez && (!PlFrezFinished) && (!read_meter_finish_frez)) //23:50开始数据的冻结
		{
			PlcTaskReseting = 1;
			PlFrezFinished = 1;
			SavePlMdb();
			SavePlStic();
		}
		
		if(read_meter_finish &&( !read_meter_finish_frez))
		{
			read_meter_finish_frez = 1;
			SavePlMdb();
			SavePlStic();
		}

		if(read_meter_major_frez && (!read_meter_major_frez_flag))
		{
			read_meter_major_frez = 0;
			read_meter_major_frez_flag = 1;
			SavePlMdb();
		}
		if(query_pl_node_finish)
		{
			query_pl_node_finish = 0;
			//SavePlNodeInfo();
			//SavePlCjqNodeInfo();
		}

		if(query_pl_node_info_start)
		{
			query_pl_node_time_out++;
		}
		else
		{
			query_pl_node_time_out = 0;
		}
		if(PlFrezTask_watchdog>30)
		{
			PlFrezTask_watchdog = 0;
		}
		Sleep(500);
	}
}

extern int PlcCommInit(void);
extern int PlcStateInit(void);
extern int PlMdbInit(void);

static void make_plc_meter_event(unsigned long event)
{
	SysSendEvent(&plc_meter_event, event);
}

static void RTimer_check_ruter_run_stat(unsigned long arg, utime_t utime)
{
	make_plc_meter_event(METEV_1MIN_CHECK_RUTER_RUN_STAT);
}

#if 0
static void RTimer_set_ruter_work_mod(unsigned long arg, utime_t utime)
{
	make_plc_meter_event(METEV_SET_RUTER_WORK_MOD);
}

static void RTimer_point_read(unsigned long arg, utime_t utime)
{
	make_plc_meter_event(METEV_PLC_POINT_READ);
}

static void RTimer_stop_point_read(unsigned long arg, utime_t utime)
{
	make_plc_meter_event(METEV_STOP_PLC_POINT_READ);
}
#endif
/**
* @brief 载波模块初始化函数
*/
DECLARE_INIT_FUNC(PlcInit);
int PlcInit(void)
{
	rtimer_conf_t conf;
	
	//Sleep(12000);
	PrintLog(LOGTYPE_DATA, "plc init...\n");
	PrintLog(LOGTYPE_DATA, "plc init...\n");
	printf("plc init...\n");
	PlcCommInit();
	//printf("plc init2...\n");
	PlcStateInit();
	//printf("plc init3...\n");
	PlMdbInit();
	//printf("plc init4...\n");
	clear_read_meter_stat();
	SysInitMutex(&Plc_Mutex);	
	//SysInitMutex(&Plc_Mutex_Read_Meter);	
	//printf("plc init5...\n");
	memset(&conf, 0, sizeof(conf));
	conf.curtime = UTimeReadCurrent();
	conf.bonce = 0;
	conf.tdev = 1;
	conf.tmod = UTIMEDEV_MINUTE;  
	SysAddRTimer(&conf, RTimer_check_ruter_run_stat, 0);

	/*
	memset(&conf, 0, sizeof(conf));
	conf.basetime.year = 11;
	conf.basetime.month = 1;
	conf.basetime.day = 3;
	conf.basetime.hour = 00;
	conf.basetime.minute = 03;
	conf.basetime.second = 00;
	conf.curtime = UTimeReadCurrent();
	conf.bonce = 0;
	conf.tdev = 1;
	conf.tmod = UTIMEDEV_HOUR;  // 1 hour
	SysAddRTimer(&conf, RTimer_point_read, 0);


	memset(&conf, 0, sizeof(conf));
	conf.basetime.year = 11;
	conf.basetime.month = 1;
	conf.basetime.day = 3;
	conf.basetime.hour = 01;
	conf.basetime.minute = 03;
	conf.basetime.second = 00;
	conf.curtime = UTimeReadCurrent();
	conf.bonce = 0;
	conf.tdev = 1;
	conf.tmod = UTIMEDEV_HOUR;  // 1 hour
	SysAddRTimer(&conf, RTimer_stop_point_read, 0);
	*/
	
	SysCreateTask(PlFrezTask, NULL);
	SysCreateTask(PlcTask, NULL);
	SysClockReadCurrent(&read_meter_start_time);
	//printf("plc init8...\n");
	//SysCreateTask(plc_rcv_mod_active_report, NULL);
	SET_INIT_FLAG(PlcInit);
	return 0;
}

static int shell_pltask(int argc, char *argv[])
{
	const para_commport_t *pcfg = GetParaCommPort(COMMPORT_PLC);
	int i = 0;
	unsigned char start_time[4];
	unsigned char finish_time[4];
	unsigned char cyc_read_time[4];

	pl_shell_cmd_flag = 1;
	meter_total_cnt = get_meter_total_cnt();
	cen_meter_cnt = get_cen_meter_total_cnt();
	imp_meter_cnt = get_imp_meter_total_cnt();
	imp_meter_read_succ_cnt = get_imp_meter_read_succ_cnt();

	memcpy(start_time,&read_meter_start_time.month,4);
	memcpy(finish_time,&read_meter_finish_time.month,4);
	memcpy(cyc_read_time,&cyc_read_meter_start_time.month,4);
	HexToBcd(start_time,4);
	HexToBcd(finish_time,4);
	HexToBcd(cyc_read_time,4);
	
	if(2 == argc && 0 == strcmp("reset", argv[1])) {
		ResetPlMdbDay();
		PlcTaskReseting = 1;
		PrintLog(0, "重新开始抄表\n");
		return 0;
	}
	else if(2 == argc && 0 == strcmp("cycle", argv[1])) {
		PrintLog(0, "总成功个数=%d\n", PlCycState[0].ok_mets);
		PrintLog(0, "首轮成功个数=%d\n", PlCycState[1].ok_mets);
		if(!PlCycState[1].rd_mets)
			PrintLog(0, "首轮成功率%d%\n\n", (PlCycState[1].ok_mets/PlCycState[1].rd_mets)*100);
		
		PrintLog(0, "当前轮次=%d\n", PlCycCount);
		PrintLog(0, "起始时间=%02x:%02x:%02x", PlCycState[0].time_start[2], PlCycState[0].time_start[1], PlCycState[0].time_start[0]);
		PrintLog(0, "结束时间=%02x:%02x:%02x", PlCycState[0].time_end[2], PlCycState[0].time_end[1], PlCycState[0].time_end[0]);
		
		for(i=1; i<PlCycCount; i++) {
			PrintLog(0, "轮次%d: 起始时间=%02x:%02x:%02x\n", i, PlCycState[i].time_start[2],PlCycState[i].time_start[1], PlCycState[i].time_start[0]);
		}
	}

	if(pcfg->flag&RDMETFLAG_ENABLE) PrintLog(0, "禁止自动抄表\n");
	else PrintLog(0, "允许自动抄表\n");

	if(PlReadStopped) PrintLog(0, "时段抄表停止\n");
	else PrintLog(0, "时段抄表启动\n");
	if(PlcTimeChecking) PrintLog(0, "正在校时\n");

	PrintLog(0, "电表总数=%d\n", meter_total_cnt);
	PrintLog(0, "集抄电表个数=%d\n", meter_total_cnt - cen_meter_cnt);
	PrintLog(0, "总表个数=%d\n", cen_meter_cnt);
	PrintLog(0, "重点表个数=%d\n", imp_meter_cnt);
	if(!(meter_total_cnt - cen_meter_cnt))	return 0;
	PrintLog(0, "抄用户表成功数=%d\n", check_read_meter_succ_cnt());
	PrintLog(0, "抄重点表成功数=%d\n", imp_meter_read_succ_cnt);
	PrintLog(0, "抄表成功百分比=%d%\n", check_read_meter_succ_cnt()*100/(meter_total_cnt - cen_meter_cnt));
	PrintLog(0, "抄表起始时间=%02x月%02x日%02x:%02x\n", start_time[0], start_time[1], start_time[2],start_time[3]);
	PrintLog(0, "抄表完成时间=%02x月%02x日%02x:%02x\n", finish_time[0], finish_time[1], finish_time[2],finish_time[3]);
	PrintLog(0, "抄表状态:");
	if(active_rgst_start)
	{
		PrintLog(0, "正在激活主动注册\n");
		
	}
	if(!point_read_start && !active_rgst_start)
	{
		PrintLog(0, "正在补组网\n");
	}
	if(!active_rgst_start && point_read_start)
	{
		PrintLog(0, "正在点抄\n");
	}

	if(query_pl_node_info)
	{
		PrintLog(0, "正在查询载波节点信息\n");
	}

	if(add_meter_start)
	{
		PrintLog(0, "正在加载电表\n");
	}

	PrintLog(0, "抄表轮次=%d\n",point_read_cyc + 1);
	//PrintLog(0, "补组网轮次=%d\n",set_ruter_work_mode_start_times);
	pl_shell_cmd_flag = 0;
	return 0;
}


static int shell_plfrz(int argc, char *argv[])
{
	//pl_shell_cmd_flag = 1;
	SavePlMdb();
	//pl_shell_cmd_flag = 0;
	return 0;
}



static int shell_pl_point_read(int argc, char *argv[])
{
	pl_shell_cmd_flag = 1;
	go_to_point_read();
	pl_shell_cmd_flag = 0;
	return 0;
}

static int shell_pl_stop_read(int argc, char *argv[])
{
	pl_shell_cmd_flag = 1;
	stop_point_read();
	pl_shell_cmd_flag = 0;
	return 0;
}

static int shell_pl_register_to_point_read(int argc, char *argv[])
{
	pl_shell_cmd_flag = 1;
	point_read_start = 1;
	stop_point_read_flag = 0;
	active_rgst_start = 0;
	pl_shell_cmd_flag = 0;
	return 0;
}


static int shell_pl_set_ruter_work_mode(int argc, char *argv[])
{
	go_to_set_ruter_work_mode();
	return 0;
}


static int shell_pl_active_register(int argc, char *argv[])
{
	go_to_active_register();
	return 0;
}

static int shell_pl_active_register_init(int argc, char *argv[])
{
	go_to_active_register_init();
	return 0;
}

static int shell_pl_query_pl_node_info(int argc, char *argv[])
{
	pl_shell_cmd_flag = 1;
	go_to_query_pl_node_info();
	pl_shell_cmd_flag = 0;
	return 0;
}

static int shell_pl_add_meter_to_mod(int argc, char *argv[])
{
	pl_shell_cmd_flag = 1;
	go_to_pl_add_meter_to_mod();
	pl_shell_cmd_flag = 0;
	return 0;
}


static int  shell_pl_rmmeter(int argc, char *argv[])
{
	unsigned short metid;

	pl_shell_cmd_flag = 1;
	ClearSaveParamFlag();
	for(metid=0; metid<MAX_METER; metid++) 
	{
		if((ParaMeter[metid].portcfg & METPORT_MASK) == 31) 
		{
			memset((unsigned char *)&ParaMeter[metid].index,0x00,sizeof(para_meter_t));
		}
	}

	SetSaveParamFlag(SAVEFLAG_METER);
	SaveParam();
	pl_shell_cmd_flag = 0;
	return 0;
}


static int  shell_pl_rmroute(int argc, char *argv[])
{
	int i;
	
	pl_shell_cmd_flag = 1;
	for(i=0;i<10;i++)
	{
		if(init_param() != 1)
		{
			PrintLog(LOGTYPE_DOWNLINK, "init_param_fail\n");
		}
		else
		{
			PrintLog(LOGTYPE_DOWNLINK, "init_param_succ\n");
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}
	pl_shell_cmd_flag = 0;
	return 0;
}
static int  shell_pl_noread(int argc, char *argv[])
{
	int i = 0,j = 0;

	pl_shell_cmd_flag = 1;
	PrintLog(0, "未抄回电表测量点号,表号,采集器号:\n");
	for(i=0;i<MAX_METER_CNT;i++)
	{
		if(check_buf(meter_ene_buffer[i].meter_ene) 
			&& (meter_ene_buffer[i].met_id>=2) 
			&&(meter_ene_buffer[i].portcfg&0x1F) == PLC_PORT)
		{
			PrintLog(0, "%04d  ",meter_ene_buffer[i].met_id);	
			PrintLog(0, "%02X%02X%02X%02X%02X%02X  ",meter_ene_buffer[i].meter_addr[5],meter_ene_buffer[i].meter_addr[4],meter_ene_buffer[i].meter_addr[3],meter_ene_buffer[i].meter_addr[2],meter_ene_buffer[i].meter_addr[1],meter_ene_buffer[i].meter_addr[0]);
			PrintLog(0, "%02X%02X%02X%02X%02X%02X\n",ParaMeter[i].owneraddr[5],ParaMeter[i].owneraddr[4],ParaMeter[i].owneraddr[3],ParaMeter[i].owneraddr[2],ParaMeter[i].owneraddr[1],ParaMeter[i].owneraddr[0]);
			j++;
			Sleep(10);
		}
	}
	PrintLog(0, "共%d个\n",j);
	pl_shell_cmd_flag = 0;
	return 0;
}


static int  shell_pl_test_read_meter(int argc, char *argv[])
{
	unsigned char test_meter_addr[6],test_receive_buf[32];
	int i,test_receive_len;
	plc_packet_t pak;

	pl_shell_cmd_flag = 1;
	for(i=0;i<6;i++)
	{
		test_meter_addr[i] = 0x11;
	}
	memset(test_receive_buf,0x00,32);
	test_receive_len = 0;
	test_receive_len = point_read(0x9010,test_meter_addr,test_receive_buf);
	if(test_receive_len>0)	
	{
		PrintLog(0, "test_receive_buf = ");
		for(i=0;i<test_receive_len;i++)
			PrintLog(0, "%02X ",test_receive_buf[i]);
		PrintLog(0, "\n");
		if(check_rcv_frame(test_receive_buf,test_receive_len) == 1)
		{
			memset((unsigned char *)&pak.head,0x00,sizeof(pak));
			memcpy((unsigned char *)&pak.head,test_receive_buf,test_receive_len);
			if(pak.afn == 0x13 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)	
			{
				PrintLog(0, "载波测试成功\n");
			}
			else
			{
				PrintLog(0, "收到异常报文\n");
			}

		}
	}
	else
	{
		PrintLog(0, "没有收到数据\n");
	}
	pl_shell_cmd_flag = 0;
	return 0;
}




static int  shell_pl_test_read_meter_frame1(int argc, char *argv[])
{
	unsigned char test_meter_addr[6],test_receive_buf[256];
	int i,test_receive_len;
	//plc_packet_t pak;

	pl_shell_cmd_flag = 1;
	test_meter_addr[0] = 0x10;
	for(i=1;i<6;i++)
	{
		test_meter_addr[i] = 0x00;
	}
	memset(test_receive_buf,0x00,sizeof(test_receive_buf));
	test_receive_len = 0;
	test_receive_len = point_read(0x03300001,test_meter_addr,test_receive_buf);
	PrintLog(0, "test_receive_len = %d\n",test_receive_len);
	if(test_receive_len>0)	
	{
		PrintLog(0, "test_receive_buf = ");
		for(i=0;i<test_receive_len;i++)
			PrintLog(0, "%02X ",test_receive_buf[i]);
		PrintLog(0, "\n");
		/*
		if(check_rcv_frame(test_receive_buf,test_receive_len) == 1)
		{
			memset((unsigned char *)&pak.head,0x00,sizeof(pak));
			memcpy((unsigned char *)&pak.head,test_receive_buf,test_receive_len);
			if(pak.afn == 0x13 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)	
			{
				PrintLog(0, "载波测试成功\n");
			}
			else
			{
				PrintLog(0, "收到异常报文\n");
			}

		}
		*/
	}
	else
	{
		PrintLog(0, "没有收到数据\n");
	}
	pl_shell_cmd_flag = 0;
	return 0;
}

static int  shell_pl_test_read_meter_frame2(int argc, char *argv[])
{
	unsigned char test_meter_addr[6],test_receive_buf[256];
	int i,test_receive_len;
	//plc_packet_t pak;

	pl_shell_cmd_flag = 1;
	test_meter_addr[0] = 0x10;
	for(i=1;i<6;i++)
	{
		test_meter_addr[i] = 0x00;
	}
	memset(test_receive_buf,0x00,sizeof(test_receive_buf));
	test_receive_len = 0;
	test_receive_len = point_read(0x03300101,test_meter_addr,test_receive_buf);
	PrintLog(0, "test_receive_len = %d\n",test_receive_len);
	if(test_receive_len>0)	
	{
		PrintLog(0, "test_receive_buf = ");
		for(i=0;i<test_receive_len;i++)
			PrintLog(0, "%02X ",test_receive_buf[i]);
		PrintLog(0, "\n");
		/*
		if(check_rcv_frame(test_receive_buf,test_receive_len) == 1)
		{
			memset((unsigned char *)&pak.head,0x00,sizeof(pak));
			memcpy((unsigned char *)&pak.head,test_receive_buf,test_receive_len);
			if(pak.afn == 0x13 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)	
			{
				PrintLog(0, "载波测试成功\n");
			}
			else
			{
				PrintLog(0, "收到异常报文\n");
			}

		}
		*/
	}
	else
	{
		PrintLog(0, "没有收到数据\n");
	}
	pl_shell_cmd_flag = 0;
	return 0;
}

static int  shell_pl_get_ruter_run_stat(int argc, char *argv[])
{
	unsigned char pl_node_cnt[2],pl_have_read_node_cnt[2],pl_fwd_read_node_cnt[2];
	unsigned char run_stat = 0;
	int i;

	pl_shell_cmd_flag = 1;
	for(i=0;i<10;i++)
	{
		if(get_ruter_run_stat(&run_stat,pl_node_cnt,pl_have_read_node_cnt,pl_fwd_read_node_cnt) != 1)
		{
			PrintLog(0, "get_ruter_run_stat_fail\n");
		}
		else
		{
			PrintLog(0, "get_ruter_run_stat_succ\n");
			if(run_stat & 0x02)
				PrintLog(0, "路由正在工作\n");
			if(!(run_stat & 0x02))
				PrintLog(0, "路由停止工作\n");
			if((run_stat & 0x01) == 1)
				PrintLog(0, "路由学习完成\n");
			if((run_stat & 0x01) == 0)
				PrintLog(0, "路由学习未完成\n");
			PrintLog(0, "从节点总数量= %d\n",MAKE_SHORT(pl_node_cnt));
			PrintLog(0, "已抄从节点数量= %d\n",MAKE_SHORT(pl_have_read_node_cnt));
			PrintLog(0, "中继数量= %d\n",MAKE_SHORT(pl_fwd_read_node_cnt));
			break;
		}
	}
	Sleep(300);
	pl_shell_cmd_flag = 0;
	return 0;
}


static int  shell_pl_get_meter_stic_info(int argc, char *argv[])
{
	int i;
	char filename[32];

	sprintf(filename,"/home/nandflash/data/plsticinfo");
	if(ReadBinFile(filename, PLMDB_MAGIC, (unsigned char *)&pl_read_meter_stic, sizeof(pl_read_meter_stic)) > 0) 
	{
		PrintLog(0, "读取文件成功\n");
	}
	else 
	{
		PrintLog(0, "读取文件失败\n");
		return 1;
	}

	PrintLog(0, "日期  抄表成功数  抄收成功率\n");
	for(i=0;i<31;i++)
	{
		PrintLog(0, " %02d      %04d        %02d%\n",i+1,pl_read_meter_stic[i].read_meter_succ_cnt,pl_read_meter_stic[i].read_meter_prd);
	}
	return 0;
}

static int  shell_pl_set_plnode_addr(int argc, char *argv[])
{
	int i;

	pl_shell_cmd_flag = 1;
	for(i=0;i<SHELL_MAX_QUERY_TIMES;i++)
	{
		if(set_plnode_addr() != 1)
		{
			PrintLog(0, "set_plnode_addr_fail\n");
		}
		else
		{
			PrintLog(0, "set_plnode_addr_succ\n");
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}
	pl_shell_cmd_flag = 0;
	return 0;
}


static int  shell_pl_check_route_mod(int argc, char *argv[])
{
	int i;

	pl_shell_cmd_flag = 1;
	for(i=0;i<SHELL_MAX_QUERY_TIMES;i++)
	{
		if(check_route_mod() != 1)
		{
			PrintLog(0, "check_route_mod_fail\n");
		}
		else
		{
			PrintLog(0, "check_route_mod_succ\n");
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}
	pl_shell_cmd_flag = 0;
	return 0;
}

static int  shell_pl_set_route_mod(int argc, char *argv[])
{
	int i;

	pl_shell_cmd_flag = 1;
	for(i=0;i<SHELL_MAX_QUERY_TIMES;i++)
	{
		if(set_route_mod() != 1)
		{
			PrintLog(0, "check_route_mod_fail\n");
		}
		else
		{
			PrintLog(0, "check_route_mod_succ\n");
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}
	pl_shell_cmd_flag = 0;
	return 0;
}

static int  shell_pl_query_mod_info(int argc, char *argv[])
{
	int i;

	pl_shell_cmd_flag = 1;
	for(i=0;i<SHELL_MAX_QUERY_TIMES;i++)
	{
		if(query_mod_info() != 1)
		{
			PrintLog(0, "check_route_mod_fail\n");
		}
		else
		{
			PrintLog(0, "check_route_mod_succ\n");
			Sleep(1000);
			break;
		}
		Sleep(1000);
	}
	pl_shell_cmd_flag = 0;
	return 0;
}

#if 0
static int  shell_pl_get_same_addr_meter(int argc, char *argv[])
{
typedef struct {
	unsigned char addr[6];    //通信地址
} same_addr_meter_t;


//MAX_METER
	unsigned short metid,i,j,metid_bak;
	same_addr_meter_t same_addr_meter[MAX_METER];
	
	PrintLog(0, "shell_pl_get_same_addr_meter........1\n");

	memset(same_addr_meter,0x00,sizeof(same_addr_meter));
	for(i=0,j=0;i<3; i++) 
	{
		for(metid=0; metid<2; metid++) 
		{
			if(!HexComp((unsigned char *)&ParaMeter[i].addr[0],(unsigned char *)&ParaMeter[metid].addr[0],6))
			{
				
				memcpy((unsigned char *)&same_addr_meter[j++].addr[0],(unsigned char *)&ParaMeter[metid].addr[0],6);
			}
		}
	}
	PrintLog(0, "shell_pl_get_same_addr_meter........2\n");

	PrintLog(0, "重复电表地址:\n");
	for(i=0; i<j; i++) 
	{
		PrintLog(0, "%02X%02X%02X%02X%02X%02X\n",same_addr_meter[i].addr[5],same_addr_meter[i].addr[4],same_addr_meter[i].addr[3],same_addr_meter[i].addr[2],same_addr_meter[i].addr[1],same_addr_meter[i].addr[0]);
		Sleep(10);
	}
	PrintLog(0, "shell_pl_get_same_addr_meter........3\n");
	PrintLog(0, "共%d个电表地址重复\n",j);
	return 0;
}
#endif


DECLARE_SHELL_CMD("pltask", shell_pltask, "显示自动抄表任务状态");

DECLARE_SHELL_CMD("plfrz", shell_plfrz, "冻结当日数据");

DECLARE_SHELL_CMD("plpointread", shell_pl_point_read, "启动点抄");

DECLARE_SHELL_CMD("plstopread", shell_pl_stop_read, "暂停点抄");

DECLARE_SHELL_CMD("plregisitertoread", shell_pl_register_to_point_read, "注册转入点抄");

DECLARE_SHELL_CMD("plsetroute", shell_pl_set_ruter_work_mode, "启动补组网");

DECLARE_SHELL_CMD("plsetregister", shell_pl_active_register, "激活主动注册");

DECLARE_SHELL_CMD("plsetregisterinit", shell_pl_active_register_init, "激活主动注册初始化");

DECLARE_SHELL_CMD("plnodeinfo", shell_pl_query_pl_node_info, "查询载波节点信息");

DECLARE_SHELL_CMD("plnaddmeter", shell_pl_add_meter_to_mod, "同步电表");

DECLARE_SHELL_CMD("plrmmeter", shell_pl_rmmeter, "清空电表档案");

DECLARE_SHELL_CMD("plrmroute", shell_pl_rmroute, "清除路由数据");

DECLARE_SHELL_CMD("plnoread", shell_pl_noread, "未抄回电表信息");

DECLARE_SHELL_CMD("pltest", shell_pl_test_read_meter, "测试载波模拟电表");

DECLARE_SHELL_CMD("plroute", shell_pl_get_ruter_run_stat, "查询路由状态");

DECLARE_SHELL_CMD("plsticinfo", shell_pl_get_meter_stic_info, "查询近一月抄表情况");

DECLARE_SHELL_CMD("pltest1", shell_pl_test_read_meter_frame1, "测试载波报文1");

DECLARE_SHELL_CMD("pltest2", shell_pl_test_read_meter_frame2, "测试载波报文2");

DECLARE_SHELL_CMD("plsetnodeaddr", shell_pl_set_plnode_addr, "同步载波主节点地址");
DECLARE_SHELL_CMD("plcheckroutemod", shell_pl_check_route_mod, "查询路由运行模式");
DECLARE_SHELL_CMD("plsetroutemod", shell_pl_set_route_mod, "设置路由标准运行模式");
DECLARE_SHELL_CMD("plquerymodinfo", shell_pl_query_mod_info, "查询厂商代码和版本信息");

//DECLARE_SHELL_CMD("plsameaddr", shell_pl_get_same_addr_meter, "查看重复电表地址");


