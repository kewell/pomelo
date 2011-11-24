/**
* pltask_test.c -- 载波测试任务
* 
* 
* 创建时间: 2010-10-16
* 最后修改时间: 2010-10-16
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/event.h"
#include "include/sys/task.h"
#include "include/sys/schedule.h"
#include "include/sys/bin.h"
#include "include/sys/xin.h"
#include "include/lib/dbtime.h"
#include "include/lib/bcd.h"
#include "include/param/meter.h"
#include "include/param/commport.h"
#include "include/param/metp.h"
#include "include/monitor/runstate.h"
#include "plmdb.h"
#include "plcomm.h"
#include "plc_stat.h"
#include "include/debug/shellcmd.h"

#define LEN_FILE_CACHE	0x10000
#define PLT_FILEPATH	DATA_PATH

#define DEFAULT_FILE_MET	256
#define DEFAULT_FILE_PRD	12
#define MAX_FILE_PRD	16
#define MAGIC_FILE_PLT	0x20091015

#define MAX_CPY_METID	(PltConfig.maxmet+PLC_BASEMETP)

#define PRD_HEAD_LEN	12
#define MET_DATA_LEN	sizeof(plt_metdata_t)
#define FILE_HEAD_LEN	4

typedef struct {
	unsigned char sec;
	unsigned char min;
	unsigned char hour;
	unsigned char routes;
	unsigned char ene[4];
} plt_metdata_t;

typedef struct {
	unsigned char start_sec;
	unsigned char start_min;
	unsigned char start_hour;
	unsigned char end_sec;
	unsigned char end_min;
	unsigned char end_hour;
	unsigned short ok_rate;
	unsigned short ok_rate_1;
	unsigned char unuse[2];
	plt_metdata_t mets[MAX_PLCMET];
} plt_prddata_t;

typedef struct {
	unsigned char max_prd;
	unsigned char invaid_prd;
	unsigned short max_met;
	plt_prddata_t prd[MAX_FILE_PRD];
} plt_filedata_t;

typedef struct {
	int maxmet;
	int maxprd;
	int prdcount;
	struct {
		unsigned short start_time;  //hour*60+minute
		unsigned short end_time;
	} prdconfig[MAX_FILE_PRD];
} plt_config_t;

static plt_filedata_t PltFileData;
static plt_config_t PltConfig;
static unsigned char PltFileCache[LEN_FILE_CACHE];
static dbtime_t PltFileDate;

static int PrdCurrent = -1;
static int CycCount;
static int ReadMetsCount, OkMetsCount;
static int ReadMetsCount1, OkMetsCount1;
static int PltTaskReseting = 0;
static int PltReadEnd = 0;
static unsigned short PltCurMetid = PLC_BASEMETP;

static void SavePltData(const sysclock_t *pclk)
{
	char filename[64];
	int len, prd, itemlen;
	plt_filedata_t *pcache;
	unsigned char *pbuf;

	sprintf(filename, "%sdaytest@%02d%02d%02d.db", PLT_FILEPATH, pclk->year, pclk->month, pclk->day);

	itemlen = PltConfig.maxmet*MET_DATA_LEN+PRD_HEAD_LEN;
	len = itemlen*PltConfig.maxprd + FILE_HEAD_LEN;
	AssertLogReturnVoid(len > LEN_FILE_CACHE, "too long file(%d)\n", len);

	pcache = (plt_filedata_t *)PltFileCache;
	pcache->max_prd = PltConfig.maxprd;
	pcache->invaid_prd = PltFileData.invaid_prd;
	pcache->max_met = PltConfig.maxmet;
	pbuf = (unsigned char *)pcache->prd;

	for(prd=0; prd<PltConfig.maxprd; prd++) {
		memcpy(pbuf, &PltFileData.prd[prd], itemlen);
		pbuf += itemlen;
	}

	SaveBinFile(filename, MAGIC_FILE_PLT, PltFileCache, len);
}

static void ResetPltData(void)
{
	int i;

	memset(&PltFileData, 0, sizeof(PltFileData));

	for(i=0; i<MAX_FILE_PRD; i++) {
		memset(PltFileData.prd[i].mets, 0xee, sizeof(plt_metdata_t)*MAX_PLCMET);
	}

	PltFileData.max_prd = PltConfig.maxprd;
	PltFileData.max_met = PltConfig.maxmet;
}

static void LoadPltData(void)
{
	char filename[64];
	sysclock_t clock;
	int len, itemlen, i, maxprd;
	plt_filedata_t *pcache;
	unsigned char *pbuf;

	ResetPltData();

	SysClockRead(&clock);
	sprintf(filename, "%sdaytest@%02d%02d%02d.db", PLT_FILEPATH, clock.year, clock.month, clock.day);
	SYSCLOCK_DBTIME(&clock, PltFileDate);
	PltFileDate.s.tick = 0;

	len = ReadBinFileCache(filename, MAGIC_FILE_PLT, PltFileCache, sizeof(PltFileCache));
	if(len < 4) return;

	pcache = (plt_filedata_t *)PltFileCache;
	if(pcache->max_prd == 0 || pcache->max_prd > MAX_FILE_PRD) return;
	if(pcache->max_met == 0 || pcache->max_met > MAX_PLCMET) return;
	itemlen = (int)pcache->max_met&0xffff;
	itemlen = itemlen*MET_DATA_LEN+PRD_HEAD_LEN;
	maxprd = (int)pcache->max_prd&0xff;
	i = maxprd*itemlen+FILE_HEAD_LEN;
	if(len != i) {
		ErrorLog("invalid file len(%d, %d)\n", len, i);
		return;
	}

	PltFileData.invaid_prd = pcache->invaid_prd;

	pbuf = (unsigned char *)pcache->prd;
	for(i=0; i<maxprd; i++) {
		memcpy(&PltFileData.prd[i], pbuf, itemlen);
		pbuf += itemlen;
	}
}

static void StringToPrdTime(char *str, int prd)
{
	int timestart, timeend, i;
	char *pstr;

	pstr = str;
	while(0 != *pstr) {
		if(':' == *pstr) {
			*pstr = 0;
			pstr++;
			break;
		}
		pstr++;
	}
	if(0 == *pstr) return;
	i = atoi(str);
	if(i < 0 || i > 23) return;
	timestart = i*60;
	str = pstr;

	while(0 != *pstr) {
		if('~' == *pstr) {
			*pstr = 0;
			pstr++;
			break;
		}
		pstr++;
	}
	if(0 == *pstr) return;
	i = atoi(str);
	if(i < 0 || i > 59) return;
	timestart += i;
	str = pstr;

	while(0 != *pstr) {
		if(':' == *pstr) {
			*pstr = 0;
			pstr++;
			break;
		}
		pstr++;
	}
	if(0 == *pstr) return;
	i = atoi(str);
	if(i < 0 || i > 23) return;
	timeend = i*60;
	str = pstr;

	i = atoi(str);
	if(i < 0 || i > 59) return;
	timeend += i;

	if(timeend > 1438) return;

	PltConfig.prdconfig[prd].start_time = (unsigned short)timestart;
	PltConfig.prdconfig[prd].end_time = (unsigned short)timeend;
}

static void LoadPrdConfig(void)
{
	char str[64], name[16];
	XINREF pf;
	int i;

	memset(&PltConfig, 0, sizeof(PltConfig));

	pf = XinOpen("/work/pltestprd.xin", 'r');
	if(NULL == pf) {
		printf("load prd config failed\n");
		return;
	}

	PltConfig.maxmet = XinReadInt(pf, "maxmet", DEFAULT_FILE_MET);
	if(PltConfig.maxmet <= 0 || PltConfig.maxmet > MAX_PLCMET) PltConfig.maxmet = DEFAULT_FILE_MET;

	PltConfig.maxprd = XinReadInt(pf, "maxprd", DEFAULT_FILE_PRD);
	if(PltConfig.maxprd <= 0 || PltConfig.maxprd > MAX_FILE_PRD) PltConfig.maxprd = DEFAULT_FILE_PRD;

	i = (PltConfig.maxmet*MET_DATA_LEN+PRD_HEAD_LEN)*PltConfig.maxprd + FILE_HEAD_LEN;
	if(i > LEN_FILE_CACHE) {
		ErrorLog("too much file len(%d)\n", i);
		PltConfig.maxmet = DEFAULT_FILE_MET;
		PltConfig.maxprd = DEFAULT_FILE_PRD;
		XinClose(pf);
		return;
	}

	PltConfig.prdcount = XinReadInt(pf, "count", 0);
	if(PltConfig.prdcount < 0 || PltConfig.prdcount > PltConfig.maxprd) {
		printf("invalid prd count\n");
		PltConfig.prdcount = 0;
		XinClose(pf);
		return;
	}

	for(i=0; i<PltConfig.prdcount; i++) {
		sprintf(name, "prd%d", i+1);
		if(XinReadString(pf, name, str, 64) > 0) {
			StringToPrdTime(str, i);
		}
	}

	XinClose(pf);
}

static void CalculateMetCount(void)
{
	int i;
	unsigned short metid;

	for(i=0,metid=PLC_BASEMETP; metid<MAX_CPY_METID; metid++) {
		if(ParaMeter[metid].metp_id == 0) continue;
		if(ParaPlcMetp[metid].stopped) continue;

		i++;
	}

	ReadMetsCount = i;
}

static void PrdCheck(void)
{
	int prd, i;
	sysclock_t clock;
	unsigned short ctime;

	SysClockReadCurrent(&clock);
	ctime = clock.hour;
	ctime *= 60;
	ctime += clock.minute;

	{
		dbtime_t dbtime;

		SYSCLOCK_DBTIME(&clock, dbtime);
		dbtime.s.tick = 0;

		if(dbtime.u != PltFileDate.u) {
			ResetPltData();
			CycCount = 0;
			ReadMetsCount = OkMetsCount = 0;
			ReadMetsCount1 = OkMetsCount1 = 0;
			PltCurMetid = PLC_BASEMETP;
			CalculateMetCount();
			PltReadEnd = 0;

			PltFileDate.u = dbtime.u;
		}
	}

	for(prd=0; prd<PltConfig.prdcount; prd++) {
		if(ctime >= PltConfig.prdconfig[prd].start_time && ctime < PltConfig.prdconfig[prd].end_time) break;
	}
	if(prd >= PltConfig.prdcount) prd = -1;

	if(prd == PrdCurrent) return;

	if(PrdCurrent >= 0) {
		if(PltFileData.prd[PrdCurrent].end_hour) i = 1;
		else if(PltFileData.prd[PrdCurrent].end_min) i = 1;
		else if(PltFileData.prd[PrdCurrent].end_sec) i = 1;
		else i = 0;
		if(0 == i) {
			PltFileData.prd[PrdCurrent].end_hour = clock.hour;
			PltFileData.prd[PrdCurrent].end_min = clock.minute;
			PltFileData.prd[PrdCurrent].end_sec = clock.second;
		}

		if(ReadMetsCount > 0) {
			i = (OkMetsCount * 1000)/ReadMetsCount;
			if(i >= 0) PltFileData.prd[PrdCurrent].ok_rate = (unsigned short)i;
		}

		if(ReadMetsCount1 > 0) {
			i = (OkMetsCount1 * 1000)/ReadMetsCount1;
			if(i >= 0) PltFileData.prd[PrdCurrent].ok_rate_1 = (unsigned short)i;
		}

		PltFileData.invaid_prd = PrdCurrent+1;

		SavePltData(&clock);

		if((PrdCurrent+1) == PltConfig.prdcount) ResetPltData();
	}

	CycCount = 0;
	ReadMetsCount = OkMetsCount = 0;
	ReadMetsCount1 = OkMetsCount1 = 0;
	PltCurMetid = PLC_BASEMETP;
	CalculateMetCount();
	PltReadEnd = 0;

	if(prd >= 0) {
		PltFileData.prd[prd].start_hour = clock.hour;
		PltFileData.prd[prd].start_min = clock.minute;
		PltFileData.prd[prd].start_sec = clock.second;
	}

	PrdCurrent = prd;
}

static int PltReadFinished(unsigned short metid)
{
	unsigned mid = metid - PLC_BASEMETP;

	if(PltFileData.prd[PrdCurrent].mets[mid].sec == 0xee) return 0;
	else return 1;
}

static void PltReadMet(unsigned short metid)
{
	plc_dest_t dest;
	unsigned char databuf[4];
	unsigned short mid = metid - PLC_BASEMETP;

	PrintLog(LOGTYPE_DOWNLINK, "read met%d ...\n", metid);

	if(0 == CycCount) ReadMetsCount1++;

	MakePlcDest(metid, &dest);

	if(PlcRead(&dest, 0x9010, databuf, 4) > 0) {
		DebugPrint(LOGTYPE_DOWNLINK, "9010=%02X%02X%02X.%02X\n", 
			databuf[3], databuf[2], databuf[1], databuf[0]);

		PltFileData.prd[PrdCurrent].mets[mid].sec = PlcState[metid].oktime.second;
		PltFileData.prd[PrdCurrent].mets[mid].min = PlcState[metid].oktime.minute;
		PltFileData.prd[PrdCurrent].mets[mid].hour = PlcState[metid].oktime.hour;
		PltFileData.prd[PrdCurrent].mets[mid].routes = PlcState[metid].routes & 0x0f;
		PltFileData.prd[PrdCurrent].mets[mid].routes |= (PlcState[metid].quality<<4) & 0xf0;
		smallcpy(PltFileData.prd[PrdCurrent].mets[mid].ene, databuf, 4);

		OkMetsCount++;
		if(0 == CycCount) OkMetsCount1++;
	}
}

static void *PlcTestTask(void *arg)
{
	const para_commport_t *pcfg = GetParaCommPort(COMMPORT_PLC);
	int readflag, endflag;

	Sleep(300);
	while(RunState.pwroff) Sleep(100);


	PltCurMetid = PLC_BASEMETP;
	PrdCurrent = -1;
	PltReadEnd = 0;
	endflag = 0;

	while(1) {
		if(PltTaskReseting) {
			PltTaskReseting = 0;
			ResetPltData();
			PrdCurrent = -1;
		}

		while(RunState.pwroff) Sleep(100);

		if(pcfg->flag & RDMETFLAG_ENABLE) {
			Sleep(50);
			continue;
		}

		PrdCheck();

		if(PrdCurrent < 0) {
			Sleep(200);
			continue;
		}

		if(!PltReadEnd && PLC_BASEMETP == PltCurMetid) endflag = 1;

		readflag = 0;
		for(; PltCurMetid<MAX_CPY_METID; PltCurMetid++) {
			if(ParaMeter[PltCurMetid].metp_id == 0) continue;
			if(ParaPlcMetp[PltCurMetid].stopped) continue;

			if(!PltReadFinished(PltCurMetid)) readflag = 1;
			if(readflag) {
				endflag = 0;
				break;
			}
		}
		if(PltCurMetid >= MAX_CPY_METID) {
			PltCurMetid = PLC_BASEMETP;
			CycCount++;
			if(!PltReadEnd && endflag) {
				sysclock_t clock;

				SysClockReadCurrent(&clock);
				PltFileData.prd[PrdCurrent].end_hour = clock.hour;
				PltFileData.prd[PrdCurrent].end_min = clock.minute;
				PltFileData.prd[PrdCurrent].end_sec = clock.second;

				PltReadEnd = 1;
			}
		}

		if(readflag) {
			PltReadMet(PltCurMetid);
			PltCurMetid++;
		}
		else Sleep(200);
	}
}

extern int PlcCommInit(void);
extern int PlcStateInit(void);
extern int PlMdbInit(void);

DECLARE_INIT_FUNC(PlcInit);
int PlcInit(void)
{
	//rtimer_conf_t conf;

	printf("plc init...\n");

	//SysInitEvent(&PlcEvent);

	LoadPrdConfig();
	LoadPltData();

	PlcCommInit();
	PlcStateInit();
	PlMdbInit();

	SysCreateTask(PlcTestTask, NULL);

	SET_INIT_FLAG(PlcInit);
	return 0;
}

static int shell_pltask(int argc, char *argv[])
{
	const para_commport_t *pcfg = GetParaCommPort(COMMPORT_PLC);

	if(2 == argc) {
		if(0 == strcmp("reset", argv[1])) {
			PltTaskReseting = 1;
			PrintLog(0, "重新开始抄表\n");
			return 0;
		}
		else if(0 == strcmp("prd", argv[1])) {
			int i;

			PrintLog(0, "当前时段=%d\n", PrdCurrent);

			for(i=0; i<PltConfig.maxprd; i++) {
				PrintLog(0, "时段%2d: %02d:%02d:%02d~%02d:%02d:%02d, %d.%d%%, %d.%d%%\n",
					i+1,
					PltFileData.prd[i].start_hour, PltFileData.prd[i].start_min, PltFileData.prd[i].start_sec, 
					PltFileData.prd[i].end_hour, PltFileData.prd[i].end_min, PltFileData.prd[i].end_sec, 
					PltFileData.prd[i].ok_rate/10, PltFileData.prd[i].ok_rate%10,
					PltFileData.prd[i].ok_rate_1/10, PltFileData.prd[i].ok_rate_1%10);
			}

			return 0;
		}
		else if(0 == strcmp("config", argv[1])) {
			int i;

			PrintLog(0, "最大时段=%d\n", PltConfig.maxprd);
			PrintLog(0, "最大表数=%d\n", PltConfig.maxmet);
			PrintLog(0, "时段数=%d\n", PltConfig.prdcount);
			for(i=0; i<PltConfig.prdcount; i++) {
				PrintLog(0, "时段%2d: %02d:%02d - %02d:%02d\n",
					i+1,
					PltConfig.prdconfig[i].start_time/60, PltConfig.prdconfig[i].start_time%60, 
					PltConfig.prdconfig[i].end_time/60, PltConfig.prdconfig[i].end_time%60);
			}

			return 0;
		}
	}

	if(pcfg->flag&RDMETFLAG_ENABLE) PrintLog(0, "禁止自动抄表\n");
	else PrintLog(0, "允许自动抄表\n");

	PrintLog(0, "当前时段=%d\n", PrdCurrent);
	PrintLog(0, "当前轮次=%d\n", CycCount);
	PrintLog(0, "时段抄表数=%d, 成功%d\n", ReadMetsCount, OkMetsCount);
	PrintLog(0, "首轮抄表数=%d, 成功%d\n", ReadMetsCount1, OkMetsCount1);

	return 0;
}
DECLARE_SHELL_CMD("pltask", shell_pltask, "显示自动抄表任务状态");


