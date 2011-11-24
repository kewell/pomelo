/**
* plmdb.c -- 载波表数据
* 
* 
* 创建时间: 2010-5-22
* 最后修改时间: 2010-5-22
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFINE_PLMDB

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/sys/bin.h"
#include "include/sys/cycsave.h"
#include "include/sys/mutex.h"
#include "include/sys/syslock.h"
#include "include/lib/dbtime.h"
#include "include/lib/bcd.h"
#include "include/param/meter.h"
#include "include/param/mix.h"
#include "plc_stat.h"
#include "plcomm.h"
#include "include/plcmet/plmdb.h"
#include "include/plcmet/pltask.h"
//#include "pltask.h"
//#include "plmdb.h"



#define PLMDB_SAVEPATH		DATA_PATH

plmdb_day_t PlMdbDay[MAX_PLCMET];
plmdb_mon_t PlMdbMonth[MAX_PLCMET];
plmdb_imp_t PlMdbImp[MAX_IMPORTANT_USER];


static int FlagDaySaved;//0-需要保存日数据
static int FlagMonthSaved;//0-需要保存月数据
static int FlagImpSaved;//0-需要保存重点用户数据

typedef struct {
	unsigned short metnum1;
	unsigned short metnum2;
} plmdb_savehead_t;

static int PlMdbLockId = -1;
#define LOCK_PLMDB		LockSysLock(PlMdbLockId)
#define UNLOCK_PLMDB	UnlockSysLock(PlMdbLockId)

void LockPlMdb(void)
{
	LockSysLock(PlMdbLockId);
}

void UnlockPlMdb(void)
{
	UnlockSysLock(PlMdbLockId);
}

static unsigned char PlMdbCache[sizeof(PlMdbMonth)+128];

static const char *PlFileName[] = {
	"daypl", "monpl", "dayimp",
};

/**
* @brief 获取数据库的储存文件名
* @param dbid 数据库ID
* @param clock 储存数据库文件时的时钟
* @param filename 返回的文件名
* @return 成功返回0, 否则失败
*/
static int PlMdbFileName(char *filename, unsigned char dbid, dbtime_t dbtime)
{
	AssertLogReturn(dbid > PLMDB_IMP, 1, "invalid dbid(%d)\n", dbid);

	if('m' == PlFileName[dbid][0]) {
		sprintf(filename, PLMDB_SAVEPATH "%s@%02d%02d.db",
						PlFileName[dbid], dbtime.s.year, dbtime.s.month);
	}
	else {
		sprintf(filename, PLMDB_SAVEPATH "%s@%02d%02d%02d.db",
						PlFileName[dbid], dbtime.s.year,dbtime.s.month, dbtime.s.day);
	}

	return 0;
}

/**
* @brief 读取日冻结表数据
* @param pday 缓存区指针
* @param dbtime 读取时间
* @return 成功返回读取表数, 失败返回-1
*/
int ReadPlMdbDay(plmdb_day_t *pday, dbtime_t dbtime)
{
	char filename[32];
	int readlen, i, metnum;
	unsigned char *pcache;
	plmdb_savehead_t *phead;

	printf("ReadPlMdbDay1...\n");
	PlMdbFileName(filename, PLMDB_DAY, dbtime);
	

	DebugPrint(0, "read %s...\n", filename);
	//将文件中的数据读到PlMdbCache全局数组
	readlen = ReadBinFileCache(filename, PLMDB_MAGIC, PlMdbCache, sizeof(PlMdbCache));
	if(readlen < 0) return -1;
	printf("ReadPlMdbDay2...\n");
	phead = (plmdb_savehead_t *)PlMdbCache;
	if(phead->metnum1 != phead->metnum2 || phead->metnum1 > MAX_PLCMET) return -1;
	metnum = (int)phead->metnum1&0xffff;
	i = metnum * sizeof(plmdb_day_t);
	printf("metnum = %d\n",metnum);
	if((i+sizeof(plmdb_savehead_t)) != readlen) return -1;
	printf("ReadPlMdbDay3...\n");
	pcache = (unsigned char *)(phead+1);
	printf("i = %d\n",i);
	memcpy(pday, pcache, i);
	if(metnum < MAX_PLCMET) 
	{
		pcache = (unsigned char *)pday;
		pcache += i;
		memset(pcache, PLDATA_EMPTY, MAX_PLCMET*sizeof(plmdb_day_t)-i);
	}
																				
	//DebugPrint(0, "read %d met\n", metnum);
																			
	return metnum;
}

/**
* @brief 保存日冻结表数据
* @param pday 缓存区指针
* @param dbtime 保存时间
* @return 成功返回保存表数, 失败返回0
*/
static int SavePlMdbDay(const plmdb_day_t *pday, dbtime_t dbtime)
{
	char filename[32];
	int writelen, metnum;
	unsigned char *pcache;
	plmdb_savehead_t *phead;

	printf("SavePlMdbDay...\n");

	PlMdbFileName(filename, PLMDB_DAY, dbtime);

	DebugPrint(LOGTYPE_SHORT, "save %s...\n", filename);

	for(metnum=MAX_PLCMET-1; metnum>=0; metnum--) {
		if(ParaMeter[metnum+PLC_BASEMETP].metp_id >= PLC_BASEMETP) break;
		//if(ParaMeter[metnum].metp_id >= PLC_BASEMETP) break;
		//if(ParaMeter[metnum].metp_id >= PLC_BASEMETP) break;
	}
	if(metnum < 0) return 0;
	metnum += 1;
	
	phead = (plmdb_savehead_t *)PlMdbCache;
	phead->metnum1 = phead->metnum2 = metnum;
	pcache = (unsigned char *)(phead+1);
	writelen = metnum*sizeof(plmdb_day_t);
	memcpy(pcache, pday, writelen);
	printf("SaveBinFile...\n");
	SaveBinFile(filename, PLMDB_MAGIC, (unsigned char *)phead, writelen+sizeof(plmdb_savehead_t));
	printf("SavePlMdbDay...metnum = %d\n",metnum);
		
	return metnum;

}

int SavePlNodeInfo(void)
{
	char filename[32];
	LOCK_PLMDB;

	printf("SavePlNodeInfo...\n");
	PrintLog(LOGTYPE_DOWNLINK, "SavePlNodeInfo...\n");
	system("rm -r -f /home/nandflash/data/plnodeinfo");
	sprintf(filename,"/home/nandflash/data/plnodeinfo");
	SaveBinFile(filename, PLMDB_MAGIC, (unsigned char *)&pl_node_info,sizeof(pl_node_info));

	UNLOCK_PLMDB;
	return 1;
}


int SavePlCjqNodeInfo(void)
{
	char filename[32];

	LOCK_PLMDB;
	printf("SavePlCjqNodeInfo...\n");
	PrintLog(LOGTYPE_DOWNLINK, "SavePlCjqNodeInfo...\n");

	system("rm -r -f /home/nandflash/data/plcjqnodeinfo");
	sprintf(filename,"/home/nandflash/data/plcjqnodeinfo");
	SaveBinFile(filename, PLMDB_MAGIC, (unsigned char *)&pl_cjq_info,sizeof(pl_cjq_info));
	UNLOCK_PLMDB;	
	return 1;
}

#if 0
/**
* @brief 事件接口初始化
* @return 成功0, 否则失败
*/
DECLARE_INIT_FUNC(AlarmInit);
int AlarmInit(void)
{
	int idx, offset;
	char filename[32];
	alarm_t *pbuf;

	memset(AlarmBufferImportant, 0, sizeof(AlarmBufferImportant));
	memset(AlarmBufferNormal, 0, sizeof(AlarmBufferNormal));

	for(idx=0; idx<2; idx++) {
		if(0 == idx) pbuf = AlarmBufferImportant;
		else pbuf = AlarmBufferNormal;

		DebugPrint(0, "  load ");
		for(offset=0; offset<256; offset+=ALMNUM_PERFILE,pbuf+=ALMNUM_PERFILE) {
			AlarmFileName(idx, offset, filename);

			DebugPrint(0, "%c%d", (idx)?'n':'i', offset/ALMNUM_PERFILE);
			if(ReadBinFile(filename, ALARM_MAGIC, (unsigned char *)pbuf, sizeof(alarm_t)*ALMNUM_PERFILE) > 0) {
				DebugPrint(0, " ok, ");
			}
			else {
				DebugPrint(0, " fail, ");
			}
		}
		DebugPrint(0, "\n");
	}

	LockIdAlarm = RegisterSysLock();

	SET_INIT_FLAG(AlarmInit);

	return 0;
}

/**
* @brief 保存事件
* @param idx 事件分类索引(重要事件,一般事件)
* @param pbuf 储存事件缓存区指针
*/
static void SaveAlarm(unsigned char idx, const alarm_t *pbuf)
{
	alarm_t *pbase;
	char filename[32];
	unsigned char idxbase;
	runstate_t *pstat = RunStateModify();

	AssertLogReturnVoid(idx>=2, "invalid idx(%d)\n", idx);

	if(0 == idx) pbase = AlarmBufferImportant;
	else pbase = AlarmBufferNormal;

	LockSysLock(LockIdAlarm);

	if(RunState.alarm.cur[idx] == RunState.alarm.head[idx])  //empty
		pstat->alarm.cur[idx] = pstat->alarm.head[idx] = 0;

	//pbase[RunState.alarm.cur[idx]] = *pbuf;
	memcpy(pbase+RunState.alarm.cur[idx], pbuf, sizeof(alarm_t));

	AlarmFileName(idx, RunState.alarm.cur[idx], filename);

	idxbase = RunState.alarm.cur[idx] & ALARM_BASEMASK;
	SaveBinFile(filename, ALARM_MAGIC, (unsigned char *)&pbase[idxbase], sizeof(alarm_t)*ALMNUM_PERFILE);

	pstat->alarm.cur[idx]++;
	if(RunState.alarm.cur[idx] == RunState.alarm.head[idx]) pstat->alarm.head[idx]++;

	UnlockSysLock(LockIdAlarm);
}
#endif

/**
* @brief 读取月冻结表数据
* @param pmon 缓存区指针
* @param dbtime 读取时间
* @return 成功返回读取表数, 失败返回-1
*/
int ReadPlMdbMonth(plmdb_mon_t *pmon, dbtime_t dbtime)
{
	char filename[32];
	int readlen, i, metnum;
	unsigned char *pcache;
	plmdb_savehead_t *phead;

	PlMdbFileName(filename, PLMDB_MONTH, dbtime);

	DebugPrint(0, "read %s...\n", filename);
	
	readlen = ReadBinFileCache(filename, PLMDB_MAGIC, PlMdbCache, sizeof(PlMdbCache));
	if(readlen < 0) return -1;
	phead = (plmdb_savehead_t *)PlMdbCache;
	if(phead->metnum1 != phead->metnum2 || phead->metnum1 > MAX_PLCMET) return -1;
	metnum = (int)phead->metnum1&0xffff;
	i = metnum * sizeof(plmdb_mon_t);
	if((i+sizeof(plmdb_savehead_t)) != readlen) return -1;

	pcache = (unsigned char *)(phead+1);
	memcpy(pmon, pcache, i);
	if(metnum < MAX_PLCMET) {
		pcache = (unsigned char *)pmon;
		pcache += i;
		memset(pcache, PLDATA_EMPTY, MAX_PLCMET*sizeof(plmdb_mon_t)-i);
	}

	return metnum;
}

#if 0
/**
* @brief 保存月冻结表数据
* @param pmon 缓存区指针
* @param dbtime 保存时间
* @return 成功返回保存表数, 失败返回0
*/
static int SavePlMdbMonth(const plmdb_mon_t *pmon, dbtime_t dbtime)
{
	char filename[32];
	int writelen, metnum;
	unsigned char *pcache;
	plmdb_savehead_t *phead;

	printf("SavePlMdbMonth...\n");
	PlMdbFileName(filename, PLMDB_MONTH, dbtime);

	DebugPrint(LOGTYPE_SHORT, "save %s...\n", filename);

	for(metnum=MAX_PLCMET-1; metnum>=0; metnum--) {
		if(ParaMeter[metnum+PLC_BASEMETP].metp_id >= PLC_BASEMETP) break;
	}
	if(metnum < 0) return 0;
	metnum += 1;

	phead = (plmdb_savehead_t *)PlMdbCache;
	phead->metnum1 = phead->metnum2 = metnum;
	pcache = (unsigned char *)(phead+1);
	writelen = metnum*sizeof(plmdb_mon_t);
	memcpy(pcache, pmon, writelen);
	SaveBinFile(filename, PLMDB_MAGIC, (unsigned char *)phead, writelen+sizeof(plmdb_savehead_t));

	return metnum;
}
#endif

/**
* @brief 读取重点用户表数据
* @param pimp 缓存区指针
* @param dbtime 读取时间
* @return 成功返回读取表数, 失败返回-1
*/
int ReadPlMdbImp(plmdb_imp_t *pimp, dbtime_t dbtime)
{
	char filename[32];
	int readlen;

	PlMdbFileName(filename, PLMDB_IMP, dbtime);

	DebugPrint(LOGTYPE_SHORT, "read %s...\n", filename);
	
	readlen = ReadBinFileCache(filename, PLMDB_MAGIC, PlMdbCache, sizeof(PlMdbImp));
	if(readlen != sizeof(PlMdbImp)) return -1;

	memcpy(pimp, PlMdbCache, sizeof(PlMdbImp));
	return MAX_IMPORTANT_USER;
}

#if 0
/**
* @brief 保存重点用户表数据
* @param pimp 缓存区指针
* @param dbtime 保存时间
* @return 成功返回保存表数, 失败返回0
*/
static int SavePlMdbImp(const plmdb_imp_t *pimp, dbtime_t dbtime)
{
	char filename[32];

	printf("SavePlMdbImp...\n");
	if(0 == ParaMix.impuser.num) return 0;

	PlMdbFileName(filename, PLMDB_IMP, dbtime);

	DebugPrint(LOGTYPE_SHORT, "save %s...\n", filename);

	SaveBinFile(filename, PLMDB_MAGIC, (unsigned char *)pimp,sizeof(PlMdbImp));

	return MAX_IMPORTANT_USER;
}
#endif
/**
* @brief 保存载波表数据
*/
void SavePlMdb(void)
{
	sysclock_t clock;
	dbtime_t dbtime;

	printf("SavePlMdb...\n");
	PrintLog(3, "SavePlMdb...\n");

	SysClockReadCurrent(&clock);//读系统时间
	SYSCLOCK_DBTIME(&clock, dbtime);

	LOCK_PLMDB;

	//if(!FlagDaySaved) 
	//{
		SavePlMdbDay(PlMdbDay, dbtime);//根据时标存储日冻结数据
		//FlagDaySaved = 1;
	//}
/*
	if(!FlagMonthSaved) 
	{
		SavePlMdbMonth(PlMdbMonth, dbtime);
		FlagMonthSaved = 1;
	}

	if(!FlagImpSaved)
	{
		SavePlMdbImp(PlMdbImp, dbtime);
		FlagImpSaved = 1;
	}

	PlcStateSave();
*/
	UNLOCK_PLMDB;
}

void SavePlStic(void)
{
	sysclock_t clock;
	int i = 0;
	char filename[32];

	printf("SavePlStic...\n");
	PrintLog(0, "SavePlStic...\n");
	
	sprintf(filename,"/home/nandflash/data/plsticinfo");
	SysClockReadCurrent(&clock);//读系统时间

	if(ReadBinFile(filename, PLMDB_MAGIC, (unsigned char *)&pl_read_meter_stic, sizeof(pl_read_meter_stic)) > 0) 
	{
		PrintLog(0, "ReadBinFileSucc...\n");
	}
	else 
	{
		PrintLog(0, "ReadBinFileFail...\n");
	}
	i = clock.day;
	if(i < 1 || i > 31)		return;
	pl_read_meter_stic[i - 1].read_meter_succ_cnt = check_read_meter_succ_cnt();
	pl_read_meter_stic[i - 1].read_meter_prd = check_read_meter_succ_cnt()*100/(meter_total_cnt - cen_meter_cnt);

	LOCK_PLMDB;
	system("rm -r -f /home/nandflash/data/plsticinfo");
	SaveBinFile(filename, PLMDB_MAGIC, (unsigned char *)&pl_read_meter_stic,sizeof(pl_read_meter_stic));
	UNLOCK_PLMDB;
}








DECLARE_CYCLE_SAVE(SavePlMdb, 0);

/**
* @brief 载波表数据初始化
*/
DECLARE_INIT_FUNC(PlMdbInit);
int PlMdbInit(void)
{
	sysclock_t clock;
	dbtime_t dbtime;
	int rtn;

	FlagDaySaved = FlagMonthSaved = 1;
	FlagImpSaved = 1;

	memset(PlMdbDay, PLDATA_EMPTY, sizeof(PlMdbDay));
	memset(PlMdbMonth, PLDATA_EMPTY, sizeof(PlMdbMonth));
	memset(PlMdbImp, PLDATA_EMPTY, sizeof(PlMdbImp));

	PlMdbLockId = RegisterSysLock();

	SysClockRead(&clock);
	SYSCLOCK_DBTIME(&clock, dbtime);//根据冻结时间得到冻结文件
	rtn = ReadPlMdbDay(PlMdbDay, dbtime);
	if(rtn > 0) DebugPrint(0, "load %d mets plmdb day data\n", rtn);
	//rtn = ReadPlMdbMonth(PlMdbMonth, dbtime);
	//if(rtn > 0) DebugPrint(0, "load %d mets plmdb month data\n", rtn);
	//rtn = ReadPlMdbImp(PlMdbImp, dbtime);
	//if(rtn > 0) DebugPrint(0, "load plmdb imp ok\n");
	
	SET_INIT_FLAG(PlMdbInit);
	return 0;
}

void ResetPlMdbDay(void)
{
	memset(PlMdbDay, PLDATA_EMPTY, sizeof(PlMdbDay));
	memset(PlMdbImp, PLDATA_EMPTY, sizeof(PlMdbImp));
	FlagDaySaved = 1;
	FlagImpSaved = 1;
}

void ResetPlMdbMonth(void)
{
	memset(PlMdbMonth, PLDATA_EMPTY, sizeof(PlMdbMonth));
	FlagMonthSaved = 1;
}

/**
* @brief 时钟转换为读取时间格式
* @param clock 时钟
* @param flag 0-转换为日冻结时间, 1-转换为月冻结时间
* @return 读取时间
*/
static inline unsigned short ClockToReadTime(const sysclock_t *clock, int flag)
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

/**
* @brief 时钟转换为读取时间格式
* @param fileclock 文件时间
* @param readtime 读取时间
* @param flag 0-转换为日冻结时间, 1-转换为月冻结时间
* @param clock 返回时钟
*/
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

void SetPlMdbFenum(unsigned short mid, unsigned char fenum)
{
	AssertLogReturnVoid(mid < PLC_BASEMETP || mid >= MAX_METER, "invalid mid(%d)\n", mid);

	PlMdbMonth[mid - PLC_BASEMETP].fenum = fenum;
}

void UpdatePlMdb(unsigned short mid, unsigned short itemid, const unsigned char *buf, int len)
{
	if(mid < PLC_BASEMETP || mid >= MAX_METER) return;
	mid -= PLC_BASEMETP;

	switch(itemid) {
	case 0x9010:
	case 0x9210:
		smallcpy(PlMdbDay[mid].meter_ene, buf, 4);
		PlMdbDay[mid].readtime = ClockToReadTime(&PlcState[mid+PLC_BASEMETP].oktime, 0);
		/*smallcpy(PlMdbMonth[mid].ene, buf, 4);
		PlMdbMonth[mid].readtime = ClockToReadTime(&PlcState[mid+PLC_BASEMETP].oktime, 1);*/
		FlagDaySaved = 0;
		//FlagMonthSaved = 0;
		break;

	case 0x9410:
		smallcpy(PlMdbMonth[mid].ene, buf, 4);
		FlagMonthSaved = 0;
		PlMdbMonth[mid].readtime = ClockToReadTime(&PlcState[mid+PLC_BASEMETP].oktime, 1);
		break;

	//case 0x9011:
	case 0x9411:
		smallcpy(PlMdbMonth[mid].ene+4, buf, 4);
		FlagMonthSaved = 0;
		//PlMdbMonth[mid].readtime = ClockToReadTime(&PlcState[mid+PLC_BASEMETP].oktime, 1);
		break;

	//case 0x9012:
	case 0x9412:
		smallcpy(PlMdbMonth[mid].ene+8, buf, 4);
		FlagMonthSaved = 0;
		//PlMdbMonth[mid].readtime = ClockToReadTime(&PlcState[mid+PLC_BASEMETP].oktime, 1);
		break;

	//case 0x9013:
	case 0x9413:
		smallcpy(PlMdbMonth[mid].ene+12, buf, 4);
		FlagMonthSaved = 0;
		//PlMdbMonth[mid].readtime = ClockToReadTime(&PlcState[mid+PLC_BASEMETP].oktime, 1);
		break;

	//case 0x9014:
	case 0x9414:
		smallcpy(PlMdbMonth[mid].ene+16, buf, 4);
		FlagMonthSaved = 0;
		//PlMdbMonth[mid].readtime = ClockToReadTime(&PlcState[mid+PLC_BASEMETP].oktime, 1);
		break;

	case 0xC010:  //状态
		//PlMdbDay[mid].state[0] = buf[0];
		//PlMdbDay[mid].state[1] = buf[1];
		//FlagDaySaved = 0;
		//PlMdbMonth[mid].state[0] = buf[0];
		//PlMdbMonth[mid].state[0] = buf[0];
		break;

	case 0xF001:  //重点用户电能量
		{
			sysclock_t clock;
			unsigned int offset;
			//unsigned char kmid, impnum;
			//unsigned short metpid;

			/*metpid = mid + PLC_BASEMETP + 1;

			impnum = ParaMix.impuser.num;
			if(0 == impnum) break;
			else if(impnum > MAX_IMPORTANT_USER) impnum = MAX_IMPORTANT_USER;

			for(kmid=0; kmid<impnum; kmid++) {
				if(metpid == ParaMix.impuser.metid[kmid]) break;
			}
			if(kmid >= impnum) break;*/

			if(mid >= MAX_IMPORTANT_USER) break;
			
			SysClockReadCurrent(&clock);
			offset = clock.hour;
			offset <<= 2;

			smallcpy(PlMdbImp[mid].ene+offset, buf, 4);
			FlagImpSaved = 0;
		}
		break;

	default:
		break;
	}
}

/**
* @brief 读取载波表即时数据
* @param metpid 测量点号, MAX_CENMETP+1 ~ MAX_METP
* @param itemid 数据项编号
* @param buf 输出缓存区指针
* @param len 输出缓存区长度
* @return 成功返回实际读取长度, 失败返回负数, 无此数据项返回-2, 缓存区溢出返回-1
*/
static int ReadPlMdbCurrent(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len)
{
#define RETRY_TIMES		3
	unsigned short mid;
	plc_dest_t dest;
	int i, retry;
	unsigned char databuf[4];
	sysclock_t clock;
	unsigned char point_read_receive_buf[256];
	int point_read_receive_len = 0;
	plc_packet_t pak;
	pak_645_t pak_645;
	unsigned char meter_addr_tmp[6];
	unsigned char ene_buf[20];
	int meter_postion = 0;

	PrintLog(0, "ReadPlMdbCurrent1\n");
	if(metpid <= PLC_BASEMETP || metpid > MAX_METER) return -2;
	mid = metpid - 1;
	if(ParaMeter[mid].metp_id == 0) return -2;
	
	PrintLog(0, "ReadPlMdbCurrent2\n");

	//MakePlcDest(mid, &dest);
	//dest.metid = 0;
	PrintLog(0, "itemid = %x\n",itemid);
	switch(itemid) {
	case 0x1001:  //F129 即时读,如果失败,则读库
	case 0x0401:  //F33 即时读,如果失败,则读库
		if(len < 11) return -1;
		PrintLog(0, "ReadPlMdbCurrent3\n");
		for(retry=0; retry<RETRY_TIMES; retry++) 
		{
			if(ParaMeter[mid].proto == PROTO_DL1997_376_1)
			{
				point_read_receive_len = point_read(0x9010,(unsigned char *)&ParaMeter[mid].addr[0],point_read_receive_buf);
				pak_645.proto = PROTO_DL1997;
			}
			else if(ParaMeter[mid].proto == PROTO_DL2007_376_1)
			{
				point_read_receive_len = point_read(0x00010000,(unsigned char *)&ParaMeter[mid].addr[0],point_read_receive_buf);
				pak_645.proto = PROTO_DL2007;
			}
			PrintLog(0, "ReadPlMdbCurrent4\n");
			//point_read_receive_len = point_read(0x9010,(unsigned char *)&ParaMeter[mid].addr[0],point_read_receive_buf);
			if(point_read_receive_len>0)	
			{
				printf("point_read_receive_len = %d\n",point_read_receive_len);
				printf("point_read_receive_buf = ");
				for(i=0;i<point_read_receive_len;i++)
					printf("%02X ",point_read_receive_buf[i]);
				printf("\n");
				PrintLog(0, "ReadPlMdbCurrent4\n");
				if(check_rcv_frame(point_read_receive_buf,point_read_receive_len) == 1)
				{
					printf("rcv_point_read_report_data...1\n");
					memset((unsigned char *)&pak.head,0x00,sizeof(pak));
					memset((unsigned char *)pak_645.meter_index,0x00,sizeof(pak_645));
					memcpy((unsigned char *)&pak.head,point_read_receive_buf,point_read_receive_len);
					if(pak.afn == 0x13 && pak.fn[0] == 0x01 && pak.fn[1] == 0x00)	
					{
						printf("rcv_point_read_report_data...2\n");
						memset(meter_addr_tmp,0x00,6);
						memset(ene_buf,0x00,20);
						//pak_645.proto = PROTO_DL1997;
						pak_645.len = pak.data[1];
						meter_postion = process_rcv_645_pak((unsigned char *)pak.data,pak_645.len,meter_addr_tmp,ene_buf);
						printf("meter_postion = %d\n",meter_postion);
						printf("rcv_point_read_report_data...3\n");
						//if(meter_postion>=8)
						//if(meter_postion>=2)	
						//if(process_rcv_645_pak((unsigned char *)pak.data,pak_645.len,pak_645.proto,meter_addr_tmp,ene_buf) == 1)
						//{
							//for(i=0; i<4; i++) ene_buf[i] = databuf[i];
							printf("rcv_point_read_report_data...4\n");
							for(i=0; i<4; i++) 
							{
								databuf[i] = ene_buf[i];
								PrintLog(0, "ene_buf[%d] = %x\n",i,ene_buf[i]);
							}
							PrintLog(0, "retry = %x\n",retry);
							break;
						//}
					}
				}
			}
		}
		if(retry >= RETRY_TIMES) 
		{
			sysclock_t clock2;

			if(PLTIME_EMTPY == PlMdbDay[mid-PLC_BASEMETP].readtime) return -2;

			SysClockReadCurrent(&clock);
			ReadTimeToClock(&clock, PlMdbDay[mid-PLC_BASEMETP].readtime, 0, &clock2);
			buf[0] = clock2.minute;
			buf[1] = clock2.hour;
			buf[2] = clock2.day;
			buf[3] = clock2.month;
			buf[4] = clock2.year;
			HexToBcd(buf, 5);
			buf[5] = 0;//费率数
			buf[6] = 0;
			for(i=0; i<4; i++) buf[i+7] = PlMdbDay[mid-PLC_BASEMETP].meter_ene[i];
		}
		else 
		{
			SysClockReadCurrent(&clock);
			buf[0] = clock.minute;
			buf[1] = clock.hour;
			buf[2] = clock.day;
			buf[3] = clock.month;
			buf[4] = clock.year;
			HexToBcd(buf, 5);
			buf[5] = 0;//费率数
			buf[6] = 0;
			for(i=0; i<4; i++) buf[i+7] = databuf[i];
		}

		return 11;
		break;

	case 0x1101: //F137
		//@change later 多费率, 重读
		if(len < 11) return -1;
		if(PlcRead(&dest, 0x9410, databuf, 4) <= 0) {
			sysclock_t clock2;

			if(PLTIME_EMTPY == PlMdbMonth[mid-PLC_BASEMETP].readtime) return -2;

			SysClockReadCurrent(&clock);
			ReadTimeToClock(&clock, PlMdbMonth[mid-PLC_BASEMETP].readtime, 1, &clock2);
			buf[0] = clock2.minute;
			buf[1] = clock2.hour;
			buf[2] = clock2.day;
			buf[3] = clock2.month;
			buf[4] = clock2.year;
			HexToBcd(buf, 5);
			buf[5] = 0;
			buf[6] = 0;
			for(i=0; i<4; i++) buf[i+7] = PlMdbMonth[mid-PLC_BASEMETP].ene[i];
		}
		else {
			SysClockReadCurrent(&clock);
			buf[0] = clock.minute;
			buf[1] = clock.hour;
			buf[2] = clock.day;
			buf[3] = clock.month;
			buf[4] = clock.year;
			HexToBcd(buf, 5);
			buf[5] = 0;
			buf[6] = 0;
			for(i=0; i<4; i++) buf[i+7] = databuf[i];
		}

		return 11;
		break;

	default: return -2;
	}

	return -2;
}

/**
* @brief 读取载波表1类数据
* @param pnfn 数据标识指针
* @param buf 输出缓存区指针
* @param len 输出缓存区长度
* @param pactlen 返回实际读取长度变量指针
* @return 成功0, 否则失败
*/
int ReadPlMdb(const unsigned char *pnfn, unsigned char *buf, int len, int *pactlen)
{
typedef int (*qrymdb_pf)(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len);

	unsigned short metpid, itemid;
	unsigned char pns, pnmask, fns, grpid;
	int actlen;
	qrymdb_pf pfunc;
	unsigned char *preadinfo = &buf[2];

	*pactlen = 0;

	pns = pnfn[0];
	if(0 == pnfn[1]) metpid = 0;
	else metpid = ((unsigned short)(pnfn[1]-1)<<3) + 1;
	fns = pnfn[2];
	grpid = pnfn[3];

	if(grpid == 0x10 || grpid == 0x11) pfunc = ReadPlMdbCurrent;//组17，18
	else if(grpid == 0x15) pfunc = ReadPlMdbState;//组22
	else return 1;

	buf[0] = pnfn[0];
	buf[1] = pnfn[1];
	buf[2] = 0;
	buf[3] = pnfn[3];
	buf += 4;
	actlen = 4;
	len -= 4;

	if(0 == metpid) pnmask = 0x80;
	else pnmask = 1;
	for(; pnmask!=0; pnmask<<=1,metpid++) {
		unsigned char fnmask;
		int rtn;

		if(metpid && (pns&pnmask) == 0) continue;

		for(fnmask=1; fnmask!=0; fnmask<<=1) {
			if((fns&fnmask) == 0) continue;

			itemid = ((unsigned short)grpid<<8) + (unsigned short)fnmask;
			rtn = (*pfunc)(metpid, itemid, buf, len);
			if(rtn == -2 || rtn == 0) continue;
			else if(rtn < 0) goto mark_end;

			len -= rtn;
			if(len < 0) goto mark_end;
			buf += rtn;
			actlen += rtn;
			*preadinfo |= fnmask;
		}
	}

mark_end:
	if(*preadinfo == 0) return 1;

	*pactlen = actlen;
	return 0;
}
