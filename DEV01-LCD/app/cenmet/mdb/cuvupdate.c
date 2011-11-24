/**
* cuvupdate.c -- 历史数据库冻结(日,月, 抄表日)
* 
* 
* 创建时间: 2010-5-14
* 最后修改时间: 2010-5-14
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/param/capconf.h"
#include "include/param/metp.h"
#include "include/param/meter.h"
#include "include/param/hardware.h"
#include "mdbconf.h"
#include "mdbcur.h"
#include "mdbuene.h"
#include "mdbtgrp.h"
#include "dbconfig.h"
#include "dbase.h"
#include "curveday.h"
#include "curvemon.h"
#include "curvermd.h"
#include "mdbstic.h"
#include "mdbsave.h"

#define BUFFER_LEN	2048
static unsigned char CurvDbBuffer[BUFFER_LEN];

/**
* @brief 冻结测量点日数据
* @param dbtime 当前时间(数据库时间格式, 以下相同)
*/
static void UpdateCuvMetpDay(dbtime_t dbtime)
{
	unsigned short metpid;
	unsigned char *puc;
	db_metday_t *pmet;
	db_msticday_t *pstic;
	int len;
	unsigned char tmpbuf[60];

#define MAXLEN	((int)BUFFER_LEN-(int)(puc-CurvDbBuffer))

	PrintLog(0, "UpdateCuvMetpDay...\r\n");
	pmet = (db_metday_t *)CurvDbBuffer;
	pstic= (db_msticday_t *)CurvDbBuffer;

	for(metpid=1; metpid<=MAX_CENMETP; metpid++) {
		if(EMPTY_CENMETP(metpid-1)) continue;

		puc = pmet->rdtime;
		ReadMdbCurrent(metpid, 0x0401, puc, MAXLEN);

		puc = pmet->enena;
		len = ReadMdbCurrent(metpid, 0x0402, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+6, len-6);

		puc = pmet->dmnpa;
		len = ReadMdbCurrent(metpid, 0x0404, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+6, len-6);

		puc = pmet->dmnna;
		len = ReadMdbCurrent(metpid, 0x0408, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+6, len-6);

		puc = pmet->enepa_day;
		len = ReadMdbUseEne(metpid, 0x0501, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+1, len-1);

		puc = pmet->enepi_day;
		len = ReadMdbUseEne(metpid, 0x0502, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+1, len-1);

		puc = pmet->enena_day;
		len = ReadMdbUseEne(metpid, 0x0504, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+1, len-1);

		puc = pmet->eneni_day;
		len = ReadMdbUseEne(metpid, 0x0508, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+1, len-1);

		//puc = pmet->enepa_abc;
		len = ReadMdbCurrent(metpid, 0x0340, tmpbuf, 60);
		if(len > 0) {
			smallcpy(pmet->enepa_abc, tmpbuf+5, 5);
			smallcpy(pmet->enepa_abc+5, tmpbuf+23, 5);
			smallcpy(pmet->enepa_abc+10, tmpbuf+41, 5);
			smallcpy(pmet->enena_abc, tmpbuf+10, 5);
			smallcpy(pmet->enena_abc+5, tmpbuf+28, 5);
			smallcpy(pmet->enena_abc+10, tmpbuf+46, 5);
			smallcpy(pmet->enepi_abc, tmpbuf+15, 4);
			smallcpy(pmet->enepi_abc+4, tmpbuf+33, 4);
			smallcpy(pmet->enepi_abc+8, tmpbuf+51, 4);
			smallcpy(pmet->eneni_abc, tmpbuf+19, 4);
			smallcpy(pmet->eneni_abc+4, tmpbuf+37, 4);
			smallcpy(pmet->eneni_abc+8, tmpbuf+55, 4);
		}

		DbaseSave(DBID_MET_DAY(metpid-1), CurvDbBuffer, DBFREZ_DAY, dbtime);

		puc = pstic->rdtime;
		ReadMdbCurrent(metpid, 0x0302, puc, MAXLEN);

		puc = pstic->pwra_max;
		BakupSticMetp(0, metpid-1, puc);
		DbaseSave(DBID_METSTIC_DAY(metpid-1), CurvDbBuffer, DBFREZ_DAY, dbtime);
	}
}

/**
* @brief 冻结测量点月数据
*/
static void UpdateCuvMetpMonth(dbtime_t dbtime)
{
	unsigned short metpid;
	unsigned char *puc;
	db_metmon_t *pmet;
	db_msticmon_t *pstic;
	int len;
	unsigned char tmpbuf[60];

#define MAXLEN	((int)BUFFER_LEN-(int)(puc-CurvDbBuffer))

	pmet = (db_metmon_t *)CurvDbBuffer;
	pstic= (db_msticmon_t *)CurvDbBuffer;

	for(metpid=1; metpid<=MAX_CENMETP; metpid++) {
		if(EMPTY_CENMETP(metpid-1)) continue;

		puc = pmet->rdtime;
		ReadMdbCurrent(metpid, 0x0401, puc, MAXLEN);

		puc = pmet->enena;
		len = ReadMdbCurrent(metpid, 0x0402, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+6, len-6);

		puc = pmet->dmnpa;
		len = ReadMdbCurrent(metpid, 0x0404, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+6, len-6);

		puc = pmet->dmnna;
		len = ReadMdbCurrent(metpid, 0x0408, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+6, len-6);

		puc = pmet->enepa_day;
		len = ReadMdbUseEne(metpid, 0x0510, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+1, len-1);

		puc = pmet->enepi_day;
		len = ReadMdbUseEne(metpid, 0x0520, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+1, len-1);

		puc = pmet->enena_day;
		len = ReadMdbUseEne(metpid, 0x0540, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+1, len-1);

		puc = pmet->eneni_day;
		len = ReadMdbUseEne(metpid, 0x0580, puc, MAXLEN);
		if(len > 0) memmove(puc, puc+1, len-1);

		//puc = pmet->enepa_abc;
		len = ReadMdbCurrent(metpid, 0x0340, tmpbuf, 60);
		if(len > 0) {
			smallcpy(pmet->enepa_abc, tmpbuf+5, 5);
			smallcpy(pmet->enepa_abc+5, tmpbuf+23, 5);
			smallcpy(pmet->enepa_abc+10, tmpbuf+41, 5);
			smallcpy(pmet->enena_abc, tmpbuf+10, 5);
			smallcpy(pmet->enena_abc+5, tmpbuf+28, 5);
			smallcpy(pmet->enena_abc+10, tmpbuf+46, 5);
			smallcpy(pmet->enepi_abc, tmpbuf+15, 4);
			smallcpy(pmet->enepi_abc+4, tmpbuf+33, 4);
			smallcpy(pmet->enepi_abc+8, tmpbuf+51, 4);
			smallcpy(pmet->eneni_abc, tmpbuf+19, 4);
			smallcpy(pmet->eneni_abc+4, tmpbuf+37, 4);
			smallcpy(pmet->eneni_abc+8, tmpbuf+55, 4);
		}

		DbaseSave(DBID_MET_MONTH(metpid-1), CurvDbBuffer, DBFREZ_MONTH, dbtime);

		puc = pstic->pwra_max;
		BakupSticMetp(1, metpid-1, puc);
		DbaseSave(DBID_METSTIC_MONTH(metpid-1), CurvDbBuffer, DBFREZ_MONTH, dbtime);
	}
}

#if 0
/**
* @brief 冻结终端日数据
*/
static void UpdateCuvTermDay(dbtime_t dbtime)
{
	db_termday_t *pterm = (db_termday_t *)CurvDbBuffer;

	PrintLog(0, "UpdateCuvTermDay...\r\n");

	BakupSticTerm(0, pterm->pwr_time);

	DbaseSave(DBID_TERMSTIC_DAY, CurvDbBuffer, DBFREZ_DAY, dbtime);
}

/**
* @brief 冻结终端月数据
*/
static void UpdateCuvTermMonth(dbtime_t dbtime)
{
	db_termmon_t *pterm = (db_termmon_t *)CurvDbBuffer;

	BakupSticTerm(1, pterm->pwr_time);

	DbaseSave(DBID_TERMSTIC_MONTH, CurvDbBuffer, DBFREZ_MONTH, dbtime);
}

/**
* @brief 冻结总加组日数据
*/
static void UpdateCuvTGrpDay(dbtime_t dbtime)
{
	unsigned short tid;
	db_tgrpday_t *ptgrpday = (db_tgrpday_t *)CurvDbBuffer;
	unsigned char *puc;

#define MAXLEN	((int)BUFFER_LEN-(int)(puc-CurvDbBuffer))

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		puc = &ptgrpday->fenum58;
		ReadMdbTGrp(tid+1, 0x0204, puc, MAXLEN);
		puc = &ptgrpday->fenum59;
		ReadMdbTGrp(tid+1, 0x0208, puc, MAXLEN);
		BakupSticTGrpDay(tid, ptgrpday->pwra_time);

		DbaseSave(DBID_TGRP_DAY(tid), CurvDbBuffer,DBFREZ_DAY, dbtime);
	}
}

/**
* @brief 冻结总加组月数据
*/
static void UpdateCuvTGrpMonth(dbtime_t dbtime)
{
	unsigned short tid;
	db_tgrpmon_t *ptgrpmon = (db_tgrpmon_t *)CurvDbBuffer;
	unsigned char *puc;

#define MAXLEN	((int)BUFFER_LEN-(int)(puc-CurvDbBuffer))

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		puc = &ptgrpmon->fenum61;
		ReadMdbTGrp(tid+1, 0x0210, puc, MAXLEN);
		puc = &ptgrpmon->fenum62;
		ReadMdbTGrp(tid+1, 0x0220, puc, MAXLEN);
		BakupSticTGrpMon(tid, ptgrpmon->pwra_time);

		DbaseSave(DBID_TGRP_MONTH(tid), CurvDbBuffer,DBFREZ_MONTH, dbtime);
	}
}
#endif

/**
* @brief 冻结日数据
*/
void UpdateCurveDay(void)
{
	sysclock_t clock;
	dbtime_t dbtime;

	SysClockReadCurrent(&clock);
	SYSCLOCK_DBTIME(&clock, dbtime);

	//UpdateCuvTermDay(dbtime);// 冻结终端日数据
	UpdateCuvMetpDay(dbtime);//冻结测量点日数据
	//UpdateCuvTGrpDay(dbtime);//冻结总加组日数据

	ResetMdbDay();
	SaveMdb();
}

/**
* @brief 冻结月数据
*/
void UpdateCurveMonth(void)
{
	sysclock_t clock;
	dbtime_t dbtime;

	SysClockReadCurrent(&clock);
	SYSCLOCK_DBTIME(&clock, dbtime);

	//UpdateCuvTermMonth(dbtime);
	UpdateCuvMetpMonth(dbtime);
	//UpdateCuvTGrpMonth(dbtime);

	ResetMdbMonth();
	SaveMdb();
}


/**
* @brief 冻结抄表日数据
*/
void UpdateCurveRmd(void)
{
	unsigned short metpid;
	unsigned char *puc;
	db_metrmd_t *prmd = (db_metrmd_t *)CurvDbBuffer;
	int len;
	sysclock_t clock;
	dbtime_t dbtime;

	#define MAXLEN	((int)BUFFER_LEN-(int)(puc-CurvDbBuffer))

	for(metpid=1; metpid<=MAX_CENMETP; metpid++) {
		if(EMPTY_CENMETP(metpid-1)) continue;

		puc = prmd->rdtime;
		ReadMdbCurrent(metpid, 0x0401, puc, MAXLEN);//F33 0C

		puc = prmd->enena;
		len = ReadMdbCurrent(metpid, 0x0402, puc, MAXLEN);//F34 
		if(len > 0) memmove(puc, puc+6, len-6);

		puc = prmd->dmnpa;
		len = ReadMdbCurrent(metpid, 0x0404, puc, MAXLEN);//F35 
		if(len > 0) memmove(puc, puc+6, len-6);

		puc = prmd->dmnna;
		len = ReadMdbCurrent(metpid, 0x0408, puc, MAXLEN);//F36
		if(len > 0) memmove(puc, puc+6, len-6);

		SysClockReadCurrent(&clock);
		SYSCLOCK_DBTIME(&clock, dbtime);
		DbaseSave(DBID_MET_CPYDAY(metpid-1), CurvDbBuffer, DBFREZ_DAY, dbtime);
	}
}

