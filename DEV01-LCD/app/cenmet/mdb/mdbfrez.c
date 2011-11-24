/**
* dbase.c -- 小时冻结数据
* 
* 
* 创建时间: 2010-5-13
* 最后修改时间: 2010-5-13
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/param/capconf.h"
#include "include/param/meter.h"
#include "include/param/metp.h"
#include "include/param/hardware.h"
#include "include/lib/bcd.h"
#include "mdbfrez.h"
#include "mdbuene.h"
#include "mdbcur.h"
#include "mdbconf.h"
#include "dbconfig.h"
#include "dbase.h"
#include "mdbtgrp.h"
#include "include/debug/shellcmd.h"
#include "include/sys/diskspace.h"

static unsigned char dbfrez_buffer[256];

//由定时器锁定的时间, 避免由于抄表等长时操作使冻结延误
static utime_t UTimeFrez;
void SetUTimeFrez(utime_t utime)
{
	UTimeFrez = utime;
}

utime_t GetUTimeFrez(void)
{
	return UTimeFrez;
}

/**
* @brief 保存测量点曲线冻结数据
* @param clock 当前时间
*/
static void UpdateMdbFrezMetp(const sysclock_t *clock)
{
	unsigned char mid;
	db_metcurve_t *pbuffer = (db_metcurve_t *)dbfrez_buffer;
	unsigned char minu;
	dbtime_t dbtime;

	minu = clock->minute / 15;

#if CURVE_FREZ == 1
#elif CURVE_FREZ == 2
	if(0 != minu && 2 != minu) return;
#elif CURVE_FREZ == 3
	if(0 != minu) return;
#else
	return;
#endif

	SYSCLOCK_DBTIME(clock, dbtime);

	for(mid=0; mid<MAX_CENMETP; mid++) {
		if(EMPTY_CENMETP(mid)) continue;

		smallcpy(pbuffer->pwra, MdbCurrent[mid].pwra, 46);

		smallcpy(pbuffer->enepa, &MdbCurrent[mid].enepa[1], 4);
		smallcpy(pbuffer->enepi, MdbCurrent[mid].enepi, 4);
		smallcpy(pbuffer->enena, &MdbCurrent[mid].enena[1], 4);
		smallcpy(pbuffer->eneni, MdbCurrent[mid].eneni, 4);

		UpdateMdbUseEne15Min(mid);
		UnsignedToBcd(MdbUseEne[mid].uenepa*10, pbuffer->uenepa, 4);
		UnsignedToBcd(MdbUseEne[mid].uenepi*100, pbuffer->uenepi, 4);
		UnsignedToBcd(MdbUseEne[mid].uenena*10, pbuffer->uenena, 4);
		UnsignedToBcd(MdbUseEne[mid].ueneni*100, pbuffer->ueneni, 4);
		ClearMdbUseEne15Min(mid);

		smallcpy(pbuffer->vol_arc, MdbCurrent[mid].phase_arc, 12);

		smallcpy(pbuffer->enepi1, MdbCurrent[mid].enepi1, 4);
		smallcpy(pbuffer->enepi4, MdbCurrent[mid].enepi4, 4);
		smallcpy(pbuffer->eneni2, MdbCurrent[mid].eneni2, 4);
		smallcpy(pbuffer->eneni3, MdbCurrent[mid].eneni3, 4);

		DbaseSave(DBID_MET_CURVE(mid), (unsigned char *)pbuffer, CURVE_FREZ, dbtime);
	}
}

/**
* @brief 保存总加组曲线冻结数据
* @param clock 当前时间
*/
static void UpdateMdbFrezTGrp(const sysclock_t *clock)
{
	unsigned char tid;
	db_tgrpcurve_t *pbuffer = (db_tgrpcurve_t *)dbfrez_buffer;
	unsigned char minu;
	dbtime_t dbtime;

	minu = clock->minute / 15;

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		AddMdbTGrpUEne15Min();
	}

#if CURVE_FREZ == 1
#elif CURVE_FREZ == 2
	if(0 != minu && 2 != minu) return;
#elif CURVE_FREZ == 3
	if(0 != minu) return;
#else
	return;
#endif

	SYSCLOCK_DBTIME(clock, dbtime);

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		PowerToGbformat02(MdbTGrpImm[tid].pwra, pbuffer->tpwra);
		PowerToGbformat02(MdbTGrpImm[tid].pwri, pbuffer->tpwri);

		ShortEneToGbformat03(MdbTGrpImm[tid].uenea, pbuffer->tenea);
		ShortEneToGbformat03(MdbTGrpImm[tid].uenei, pbuffer->tenei);

		MdbTGrpUEneFrezEnd();

		DbaseSave(DBID_MET_CURVE(tid), (unsigned char *)pbuffer, CURVE_FREZ, dbtime);
	}
}

/**
* @brief 保存曲线冻结数据(一类)
*/
void UpdateMdbFrez(void)
{
	sysclock_t clock;

	UTimeToSysClock(UTimeFrez, &clock);
	UpdateMdbFrezMetp(&clock);
	UpdateMdbFrezTGrp(&clock);
}

typedef struct {
	unsigned short itemid;
	unsigned short dbid;
	unsigned char len;
	unsigned char offset;
} mdbfrez_list_t;
static const mdbfrez_list_t cons_mdbfrez_list[] = {
	{0x0a01, DBID_TGRP_CURVE(0), 2, DB_OFFSET(db_tgrpcurve_t, tpwra[0])},//F81
	{0x0a02, DBID_TGRP_CURVE(0), 2, DB_OFFSET(db_tgrpcurve_t, tpwri[0])},//F82
	{0x0a04, DBID_TGRP_CURVE(0), 4, DB_OFFSET(db_tgrpcurve_t, tenea[0])},//F83
	{0x0a08, DBID_TGRP_CURVE(0), 4, DB_OFFSET(db_tgrpcurve_t, tenei[0])},//F84
	
	{0x0b01, DBID_MET_CURVE(0), 3, DB_OFFSET(db_metcurve_t, pwra[0])},//F89
	{0x0b02, DBID_MET_CURVE(0), 3, DB_OFFSET(db_metcurve_t, pwra[3])},//F90
	{0x0b04, DBID_MET_CURVE(0), 3, DB_OFFSET(db_metcurve_t, pwra[6])},//F91
	{0x0b08, DBID_MET_CURVE(0), 3, DB_OFFSET(db_metcurve_t, pwra[9])},//F92
	{0x0b10, DBID_MET_CURVE(0), 3, DB_OFFSET(db_metcurve_t, pwri[0])},//F93
	{0x0b20, DBID_MET_CURVE(0), 3, DB_OFFSET(db_metcurve_t, pwri[3])},//F94
	{0x0b40, DBID_MET_CURVE(0), 3, DB_OFFSET(db_metcurve_t, pwri[6])},//F95
	{0x0b80, DBID_MET_CURVE(0), 3, DB_OFFSET(db_metcurve_t, pwri[9])},//F96

	{0x0e01, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, pwrf[0])},//F113
	{0x0e02, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, pwrf[2])},//F114
	{0x0e04, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, pwrf[4])},//F115
	{0x0e08, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, pwrf[8])},//F116

	{0x0c01, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, vol[0])},//F97
	{0x0c02, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, vol[2])},//F98
	{0x0c04, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, vol[4])},//F99
	{0x0c08, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, amp[0])},//F100
	{0x0c10, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, amp[2])},//F101
	{0x0c20, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, amp[4])},//F102
	{0x0c40, DBID_MET_CURVE(0), 2, DB_OFFSET(db_metcurve_t, amp[6])},//F103

	{0x0d01, DBID_MET_CURVE(0), 4, DB_OFFSET(db_metcurve_t, uenepa[0])},//F105
	{0x0d02, DBID_MET_CURVE(0), 4, DB_OFFSET(db_metcurve_t, uenepi[0])},//F106
	{0x0d04, DBID_MET_CURVE(0), 4, DB_OFFSET(db_metcurve_t, uenena[0])},//F107
	{0x0d08, DBID_MET_CURVE(0), 4, DB_OFFSET(db_metcurve_t, ueneni[0])},//F108
	{0x0d10, DBID_MET_CURVE(0), 4, DB_OFFSET(db_metcurve_t, enepa[0])},//F109
	{0x0d20, DBID_MET_CURVE(0), 4, DB_OFFSET(db_metcurve_t, enepi[0])},//F110
	{0x0d40, DBID_MET_CURVE(0), 4, DB_OFFSET(db_metcurve_t, enena[0])},//F111
	{0x0d80, DBID_MET_CURVE(0), 4, DB_OFFSET(db_metcurve_t, eneni[0])},//F112

	{0, 0, 0},
};

/**
* @brief 读取当前表计小时冻结数据
* @param metpid 测量点号, 1~MAX_CENMETP
* @param itemid 数据项编号
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度, 失败返回负数, 无此数据项返回-2, 缓存区溢出返回-1
*/
int ReadMdbFrez(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len)
{
	unsigned short dbid;
	const mdbfrez_list_t *plist = (const mdbfrez_list_t *)cons_mdbfrez_list;
	utime_t utime;
	int offset, itemlen;
	int cpynum, i;
	unsigned char *pbuffer;
	dbtime_t dbtime;

	if((metpid==0) ||(metpid > MAX_CENMETP)) return -2;
	metpid -= 1;
	if(0x0a00 == (itemid&0xff00)) {
		if(metpid >= MAX_TGRP) return -2;
	}

	while(0 != plist->itemid) {
		if(itemid == plist->itemid) break;
		plist++;
	}
	if(0 == plist->itemid) return -2;

	dbid = plist->dbid;
	dbid += metpid;

#if CURVE_FREZ == 1  // 15min
	cpynum = 4; i=900;
#elif CURVE_FREZ == 2  // 30min
	cpynum = 2; i=1800;
#elif CURVE_FREZ == 3  // 60min
	cpynum = 1; i=3600;
#else
	return -2;
#endif

	if((cpynum*plist->len+2) > len) return -1;
	len = cpynum*plist->len+2;

	utime = UTimeReadCurrent();
#if CURVE_FREZ != 3
	utime -= 3600;  // one hour before
	utime = utime - (utime%3600);
#endif

	*buf = (unsigned char)((utime/3600)%24);
	HexToBcd(buf, 1);
	buf++;
	*buf = CURVE_FREZ;
	buf++;

#if CURVE_FREZ != 3
	utime += i;
#endif

	UTIME_DBTIME(utime, dbtime);
	itemlen = DbaseItemSize(dbid);
	i = DbGetItemNum(CURVE_FREZ);
	offset = DbGetItemOffset(i, dbtime);

	//@change later: 23:15~24:00中的24:00看不到

#if CURVE_FREZ != 3
	if((offset + cpynum) >= i) {
		cpynum -= 1;
	}
#endif

	DbaseReadLock();
	if(DbaseRead(dbid, CURVE_FREZ, dbtime) < 0) {
		DbaseReadUnlock();
		return -2;
	}
	pbuffer = DbaseReadCache();
	pbuffer += offset*itemlen;

	for(i=0; i<cpynum; i++,buf+=plist->len,pbuffer+=itemlen) {
		smallcpy(buf, pbuffer+plist->offset, plist->len);
	}

	DbaseReadUnlock();
	return len;
}

#if 0

static int shell_dbtest(int argc, char *argv[])
{
	int i, num;
	sysclock_t clock;
	utime_t utime;

	if(4 != argc) {
		PrintLog(0, "usage: dbtest month day num\n");
		return 1;
	}

	SysClockReadCurrent(&clock);

	clock.hour = 0;
	clock.minute = 0;
	clock.second = 0;

	i = atoi(argv[1]);
	if(i < 1 || i > 12) {
		PrintLog(0, "invalid month\n");
		return 1;
	}
	clock.month = i;

	i = atoi(argv[2]);
	if(i < 1 || i > 31) {
		PrintLog(0, "invalid day\n");
		return 1;
	}
	clock.day = i;

	num = atoi(argv[3]);
	if(num <= 0 || num > 2000) {
		PrintLog(0, "invalid len\n");
		return 1;
	}

	utime = SysClockToUTime(&clock);

	for(i=0; i<num; i++) {
		UpdateMdbFrezMetp(&clock);

		//utime += 900;
		utime += 86400;  // 1 day
		UTimeToSysClock(utime, &clock);
	}

	i = DiskUsage(DBASE_SAVEPATH);
	PrintLog(0, "dis usage = %d\%\n", i);

	PrintLog(0, "make %d db items ok\n", num);
	return 0;
}
DECLARE_SHELL_CMD("dbtest", shell_dbtest, "曲线数据库测试");

#endif

