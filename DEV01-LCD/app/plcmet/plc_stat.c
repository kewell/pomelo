/**
* plc_stat.h -- 载波通信状态
* 
* 
* 创建时间: 2010-5-21
* 最后修改时间: 2010-5-22
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/syslock.h"
#include "include/sys/grpbin.h"
#include "include/sys/reset.h"
#include "include/sys/timeal.h"
#include "include/lib/bcd.h"
#include "plc_stat.h"
#include "include/param/meter.h"
#include "include/param/metp.h"
#include "include/param/route.h"
#include "include/debug/shellcmd.h"

plc_state_t PlcState[MAX_METER];
struct pl_cycstat PlCycState[MAX_PLCYCSTAT];

#define PLCSTATE_MAGIC		0x561212f2
#define PLCSTATE_SAVE_PATH	DATA_PATH

/**
* @brief 载波通信状态初始化
* @return 成功0, 否则失败
*/
DECLARE_INIT_FUNC(PlcStateInit);
int PlcStateInit(void)
{
	#define MEMCACHE_SIZE	(sizeof(plc_state_t)+40)
	unsigned char memcache[MEMCACHE_SIZE];
	grpbin_ref_t ref;
	int i, readlen;
	plc_state_t *pdoc = (plc_state_t *)memcache;

	DebugPrint(0, "  load plc state(maxsize=%d)...", 
			(sizeof(plc_state_t)+2)*MAX_METER+12);

	if(OpenGrpBinFile(PLCSTATE_SAVE_PATH "plstat.dat", PLCSTATE_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no file\n");
		goto mark_end;
	}
	else DebugPrint(0, "ok\n");

	if(ref.itemlen > MEMCACHE_SIZE) {
		ErrorLog("invalid itemlen(%d)\n", ref.itemlen);
		CloseGrpBinFile(&ref);
		goto mark_end;
	}

	if(ref.itemnum > MAX_METER) {
		ErrorLog("too long itemnum(%d)\n", ref.itemnum);
		ref.itemnum = MAX_METER;
	}

	for(i=0; i<ref.itemnum; i++) {
		readlen = ReadGrpBinFileItem(&ref, memcache, MEMCACHE_SIZE);
		if(readlen < 0) break;

		if(pdoc->metid >= MAX_METER) continue;

		if(readlen > sizeof(plc_state_t)) readlen = sizeof(plc_state_t);
		memcpy(&PlcState[pdoc->metid], memcache, readlen);
	}

mark_end:
	SET_INIT_FLAG(PlcStateInit);
	return 0;
}

/**
* @brief 保存载波通信状态
* @return 成功0, 否则失败
*/
int PlcStateSave(void)
{
	grpbin_ref_t ref;
	int i;

	ref.itemlen = sizeof(plc_state_t);
	ref.itemnum = MAX_METER;
	if(OpenGrpBinFile(PLCSTATE_SAVE_PATH "plstat.dat", PLCSTATE_MAGIC, 'w', &ref)) {
		return 1;
	}

	for(i=PLC_BASEMETP; i<MAX_METER; i++) {
		if(ParaMeter[i].metp_id == 0) continue;

		PlcState[i].metid = i;
		WriteGrpBinFileItem(&ref, (unsigned char *)&PlcState[i]);
	}

	CloseGrpBinFile(&ref);
	
	return 0;
}

/**
* @breif 保存载波统计
* @return 成功0, 否则失败
*/
int PlStatisticSave(void)
{
	FILE *pf;
	char filename[64];
	sysclock_t clock;
	int cyc;

	SysClockReadCurrent(&clock);
	sprintf(filename, "%sdayplcycstat@%02d%02d%02d.db", 
		PLCSTATE_SAVE_PATH, clock.year, clock.month, clock.day);

	pf = fopen(filename, "w");
	if(NULL == pf) return 1;

	for(cyc=0; cyc<MAX_PLCYCSTAT; cyc++) {
		if(0 == cyc) fprintf(pf, "总体: ");
		else fprintf(pf, "轮次%2d: ", cyc);

		fprintf(pf, "开始时间:20%02X-%2X-%2X %2X:%2X:%2X, ", 
					PlCycState[cyc].time_start[5], PlCycState[cyc].time_start[4], 
					PlCycState[cyc].time_start[3], PlCycState[cyc].time_start[2], 
					PlCycState[cyc].time_start[1], PlCycState[cyc].time_start[0]);
		fprintf(pf, "结束时间:20%02X-%2X-%2X %2X:%2X:%2X, ", 
					PlCycState[cyc].time_end[5], PlCycState[cyc].time_end[4], 
					PlCycState[cyc].time_end[3], PlCycState[cyc].time_end[2], 
					PlCycState[cyc].time_end[1], PlCycState[cyc].time_end[0]);	
		fprintf(pf, "总共抄读%d块表,成功%d\r\n", PlCycState[cyc].rd_mets, PlCycState[cyc].ok_mets);
	}

	fclose(pf);
	return 0;
}

/**
* @brief 读取载波表状态数据
* @param metpid 测量点号, MAX_CENMETP+1 ~ MAX_METP
* @param itemid 数据项编号
* @param buf 输出缓存区指针
* @param len 输出缓存区长度
* @return 成功返回实际读取长度, 失败返回负数, 无此数据项返回-2, 缓存区溢出返回-1
*/
int ReadPlMdbState(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len)
{
	unsigned short mid;
	int i;

	if(metpid <= PLC_BASEMETP || metpid > MAX_METER) return -2;
	mid = metpid - 1;
	if(ParaMeter[mid].metp_id == 0) return -2;

	switch(itemid) {
	case 0x1501:  //F169
		{
			int actlen, level, num;

			actlen = 2;
			if(actlen > len) return -1;
			*buf++ = ParaMeter[mid].portcfg & METPORT_MASK;

			num = (int)ParaRoute[mid].num&0xff;
			if(num > MAX_ROUTE_ONEMET) {
				ErrorLog("met %d invalid route num(%d)\n", mid, ParaRoute[mid].num);
				num = MAX_ROUTE_ONEMET;
			}
			*buf++ = num;

			for(i=0; i<num; i++) {
				level = (int)ParaRoute[mid].route[i].level&0xff;
				if(level > MAX_ROUTE_LEVEL) {
					ErrorLog("met %d invalid route level(%d)\n", mid, level);
					level = MAX_ROUTE_LEVEL;
				}
				level *= 6;
				actlen += level + 1;
				if(actlen > len) return -1;
				*buf++ = level;
				if(level) memcpy(buf, ParaRoute[mid].route[i].addr, level);
				buf += level;
			}

			return actlen;
		}
		break;

	case 0x1502:  //F170
		if(len < 18) return -1;

		*buf++ = ParaMeter[mid].portcfg & METPORT_MASK;
		*buf++ = PlcState[mid].routes;
		*buf++ = PlcState[mid].phase;
		*buf++ = PlcState[mid].quality;
		*buf++ = PlcState[mid].okflag;

		buf[0] = PlcState[mid].oktime.second;
		buf[1] = PlcState[mid].oktime.minute;
		buf[2] = PlcState[mid].oktime.hour;
		buf[3] = PlcState[mid].oktime.day;
		buf[4] = PlcState[mid].oktime.month;
		buf[5] = PlcState[mid].oktime.year;
		HexToBcd(buf, 6);
		buf[4] &= 0x1f;
		i = PlcState[mid].oktime.week;
		if(i == 0) i = 6;
		else if(i == 6) i = 0;
		buf[4] |= ((i+1)<<5)&0xe0;
		buf += 6;

		buf[0] = PlcState[mid].failtime.second;
		buf[1] = PlcState[mid].failtime.minute;
		buf[2] = PlcState[mid].failtime.hour;
		buf[3] = PlcState[mid].failtime.day;
		buf[4] = PlcState[mid].failtime.month;
		buf[5] = PlcState[mid].failtime.year;
		HexToBcd(buf, 6);
		buf[4] &= 0x1f;
		i = PlcState[mid].failtime.week;
		if(i == 0) i = 6;
		else if(i == 6) i = 0;
		buf[4] |= ((i+1)<<5)&0xe0;
		buf += 6;

		*buf = PlcState[mid].failcount;

		return 18;

	case 0x1504: //F171
		if(len < (MAX_PLCYCSTAT*16)) return -1;

		memcpy(buf, PlCycState, MAX_PLCYCSTAT*16);

		return(MAX_PLCYCSTAT*16);

	default: return -2;
	}

	return -2;
}

static int shell_plstate(int argc, char *argv[])
{
	int metid;
	char flag;

	if(argc < 2) goto mark_invalid;

	flag = *argv[1];

	if('o' == flag || 'a' == flag) {
		int basemet, endmet, idx;
		char buf[128], *ps;

		if('a' == flag) {
			basemet = 0;
			endmet = MAX_METER-1;
		}
		else {
			if(argc != 4) goto mark_invalid;

			basemet = atoi(argv[2]);
			if(basemet <= 0 || basemet > MAX_METER) {
				PrintLog(0, "invalid start metid\n");
				return 1;
			}

			endmet = atoi(argv[3]);
			if(endmet <= 0 || endmet > MAX_METER || endmet <= basemet) {
				PrintLog(0, "invalid end metid\n");
				return 1;
			}

			endmet -= 1;
			basemet -= 1;
		}

		metid = basemet;
		while(metid <= endmet) {
			ps = buf;
			*ps = 0;
			sprintf(ps, "%4d: ", metid+1);
			ps += strlen(ps);
			for(idx=1; idx<=64; idx++,metid++) {
				if(metid > endmet) break;

				if(ParaMeter[metid].metp_id == 0) {
					*ps++ = '.';
					*ps = 0;
				}
				else if(ParaPlcMetp[metid].stopped) {
					*ps++ = 'S';
					*ps = 0;
				}
				else if(PlcState[metid].okflag) {
					*ps++ = 'O';
					*ps = 0;
				}
				else {
					*ps++ = 'X';
					*ps = 0;
				}

				if(0 == (idx&0x07)) {
					*ps++ = '|';
					*ps = 0;
				}
			}

			PrintLog(0, "%s\n", buf);
		}

		return 0;
	}
	else if('s' == flag) {
		if(argc != 3) goto mark_invalid;

		metid = atoi(argv[2]);
		if(metid <= 0 || metid > MAX_METER) {
			PrintLog(0, "invalid metid\n");
			return 1;
		}

		metid -= 1;

		PrintLog(0, "中继路由级数: %d\n", PlcState[metid].routes);
		PrintLog(0, "载波抄读相位: %d\n", PlcState[metid].phase);
		PrintLog(0, "载波信号品质: %d\n", PlcState[metid].quality);
		PrintLog(0, "最近一次抄表%s\n", (PlcState[metid].okflag)?"成功":"失败");
		PrintLog(0, "连续失败次数: %d\n", PlcState[metid].failcount);
		PrintLog(0, "最近一次抄表成功时间: %s\n", SysClockFormat(&PlcState[metid].oktime));
		PrintLog(0, "最近一次抄表失败时间: %s\n", SysClockFormat(&PlcState[metid].failtime));

		return 0;
	}

mark_invalid:
	PrintLog(0, "usage: plstate a/o/s [metid] [metid]\n");
	return 1;
}
DECLARE_SHELL_CMD("plstate", shell_plstate, "查询载波抄表状态");

