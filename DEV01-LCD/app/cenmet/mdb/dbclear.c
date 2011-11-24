/**
* dbclear.c -- 清除过时的历史数据库
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-5-20
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/syslock.h"
#include "include/sys/timeal.h"
#include "include/sys/diskspace.h"
#include "dbconfig.h"
#include "include/debug/shellcmd.h"

#define TMP_FILE	TEMP_PATH "dbclr.tmp"

static int DbClrLockId = -1;

/**
* @brief 获取一个数据库文件的储存时间
* @param filename 数据库文件名
* @param 返回的数据库储存时间
* @return 月日期返回2, 天日期返回1, 非法日期返回0
*/
static int DbFileDate(const char *filename, sysclock_t *clock)
{
	const char *ps, *pe;
	int i, skip;
	unsigned char *puc = &(clock->year);
	unsigned char date;

	//delete path
	i = strlen(filename);
	if(i <= 0) return 0;
	pe = filename + i - 1;
	for(; i>0; i--,pe--) {
		if('/' == *pe) break;
	}
	pe++;

	if('m' == *pe) {
		skip = 2; //month
		clock->day = 1;
	}
	else {
		skip = 3;  //day
	}

	ps = pe;
	while(0 != *ps) {
		if('@' == *ps) break;
		ps++;
	}
	if(0 == *ps) return 0;
	ps++;

	pe = ps;
	while(0 != *pe) {
		if('.' == *pe) break;
		pe++;
	}
	if(0 == *pe) return 0;

	if((int)(pe-ps) != (skip*2)) return 0;

	for(i=0; i<skip; i++,ps+=2) {
		date = (ps[0]-'0')*10;
		date += ps[1] - '0';
		*puc++ = date;
	}

	if(2 == skip) return 2;
	else return 1;
}

static int DaysInMonth(unsigned char year, unsigned char month)
{
	static const int idays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	int days;

	days = idays[month-1];
	
	if(2 == month && (year&0xfc) == 0) days += 1;

	return days;
}

static inline void RemoveFile(const char *filename)
{
	DebugPrint(LOGTYPE_SHORT, "remove %s\n", filename);
	remove(filename);
}

/**
* @brief 清除过时的历史数据库
*/
static void DbaseClearNormal(void)
{
	sysclock_t curclock, fileclock;
	FILE *pf;
	char filename[96];
	int rtn, filemon, curmon, i;

	PrintLog(LOGTYPE_ALARM, "start db clear check...\n");
	//PrintLog(0, "DbaseClearNormal.................................\n");
	//printf("DbaseClearNormal.................................\n");

	remove(TMP_FILE);
	system("ls " DBASE_SAVEPATH "*.db > " TMP_FILE);

	LockSysLock(DbClrLockId);

	pf = fopen(TMP_FILE, "r");
	if(NULL == pf) {
		ErrorLog("can not open %s\n", TMP_FILE);
		UnlockSysLock(DbClrLockId);
		return;
	}

	fileclock.hour = fileclock.minute = fileclock.second = 0;

	SysClockReadCurrent(&curclock);
	DebugPrint(LOGTYPE_SHORT, "current time=%s\n", SysClockFormat(&curclock));
	curmon = (int)curclock.year&0xff;
	curmon *= 12;
	curmon += (int)curclock.month&0xff;

	filename[95] = 0;
	while(NULL != fgets(filename, 95, pf)) {
		if(0 == filename[0]) continue;
		for(i=0; filename[i]!=0; i++) {
			if('\r' == filename[i] || '\n' == filename[i]) {
				filename[i] = 0;
				break;
			}
		}

		rtn = DbFileDate(filename, &fileclock);
		/*DebugPrint(0, "%s date = %02d-%d-%d, type=%d\n", 
			filename, fileclock.year, fileclock.month, fileclock.day, rtn);*/
		if(0 == rtn) {  //invalid file
			RemoveFile(filename);
			continue;
		}

		if(fileclock.month == 0 || fileclock.month > 12) {
			RemoveFile(filename);
			continue;
		}

		filemon = (int)fileclock.year&0xff;
		filemon *= 12;
		filemon += (int)fileclock.month&0xff;

		if(2 == rtn) { //month
			//清除1年以前的月数据
			if(filemon > curmon || (curmon-filemon) > 12) {
				RemoveFile(filename);
			}
		}
		else { //day
			//清除1月以前的天数据
			if(filemon > curmon || (curmon-filemon) > 1) {
				RemoveFile(filename);
			}
			else if((curmon - filemon) == 1) {
				i = DaysInMonth(fileclock.year, fileclock.month);
				i -= (int)fileclock.day&0xff;
				if(i < 0) {
					RemoveFile(filename);
				}
				else {
					i += (int)curclock.day&0xff;
					if(i > 31) {
						RemoveFile(filename);
					}
				}
			}
			else if(filemon == curmon && fileclock.day > curclock.day) {
				RemoveFile(filename);
			}
		}
	}

	fclose(pf);
	remove(TMP_FILE);

	UnlockSysLock(DbClrLockId);
	
	PrintLog(LOGTYPE_ALARM, "db clear check end\n");
}

/**
* @brief 紧急清除过时的历史数据库
*/
static void DbaseClearEmergy(void)
{
	sysclock_t curclock, fileclock;
	FILE *pf;
	char filename[96];
	int rtn, filemon, curmon, i;

	PrintLog(LOGTYPE_ALARM, "start db emergy clear check...\n");

	remove(TMP_FILE);
	system("ls " DBASE_SAVEPATH "*.db > " TMP_FILE);

	LockSysLock(DbClrLockId);

	pf = fopen(TMP_FILE, "r");
	if(NULL == pf) {
		ErrorLog("can not open %s\n", TMP_FILE);
		UnlockSysLock(DbClrLockId);
		return;
	}

	fileclock.hour = fileclock.minute = fileclock.second = 0;

	SysClockReadCurrent(&curclock);
	curmon = (int)curclock.year&0xff;
	curmon *= 12;
	curmon += (int)curclock.month&0xff;

	filename[95] = 0;
	while(NULL != fgets(filename, 95, pf)) {
		if(0 == filename[0]) continue;
		for(i=0; filename[i]!=0; i++) {
			if('\r' == filename[i] || '\n' == filename[i]) {
				filename[i] = 0;
				break;
			}
		}

		rtn = DbFileDate(filename, &fileclock);
		if(0 == rtn) {  //invalid file
			RemoveFile(filename);
			continue;
		}

		if(fileclock.month == 0 || fileclock.month > 12) {
			RemoveFile(filename);
			continue;
		}

		filemon = (int)fileclock.year&0xff;
		filemon *= 12;
		filemon += (int)fileclock.month&0xff;

		if(2 == rtn) { //month
			//清除1月以前的月数据
			if(filemon > curmon || (curmon-filemon) > 1) RemoveFile(filename);
		}
		else { //day
			//清除2天以前的天数据
			if(filemon > curmon || (curmon-filemon) > 1) RemoveFile(filename);
			else if((curmon - filemon) == 1) {
				i = DaysInMonth(fileclock.year, fileclock.month);
				i -= (int)fileclock.day&0xff;
				if(i < 0) RemoveFile(filename);
				else {
					i += (int)curclock.day&0xff;
					if(i > 2) RemoveFile(filename);
				}
			}
			else {
				i = (int)curclock.day&0xff;
				i -= (int)fileclock.day&0xff;
				if(i < 0 || i > 2) RemoveFile(filename);
			}
		}
	}

	fclose(pf);
	remove(TMP_FILE);

	UnlockSysLock(DbClrLockId);

	PrintLog(LOGTYPE_ALARM, "db emergy clear check end\n");
}

/**
* @brief 清除过时的历史数据库
*/
void DbaseClear(void)
{
	int usage;

	usage = DiskUsage(DBASE_SAVEPATH);
	if(usage > 85) DbaseClearEmergy();
	else DbaseClearNormal();
}

DECLARE_INIT_FUNC(DbClearInit);
int DbClearInit(void)
{
	DbClrLockId = RegisterSysLock();

	SET_INIT_FLAG(DbClearInit);
	return 0;
}

static int shell_dbclear(int argc, char *argv[])
{
	char flag;
	int i;

	if(2 != argc) {
		PrintLog(0, "dbclr n or e\n");
		return 1;
	}

	flag = *argv[1];

	if('n' == flag) DbaseClear();
	else if('e' == flag) DbaseClearEmergy();
	else {
		PrintLog(0, "非法的清除标志\n");
		return 1;
	}

	i = DiskUsage(DBASE_SAVEPATH);
	PrintLog(0, "数据区空间利用率=%d\%\n", i);

	PrintLog(0, "清除过时数据库成功\n");
	return 0;
}
DECLARE_SHELL_CMD("dbclr", shell_dbclear, "清除过时数据库");

