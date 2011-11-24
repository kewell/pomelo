/**
* dbconfig.c -- 历史数据库配置
* 
* 
* 创建时间: 2010-5-12
* 最后修改时间: 2010-5-12
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/param/capconf.h"
#include "include/sys/timeal.h"
#include "include/lib/dbtime.h"
#include "dbconfig.h"
#include "mdbfrez.h"
#include "curveday.h"
#include "curvemon.h"
#include "curvermd.h"

typedef struct {
	unsigned short dbid_base;
	unsigned short dbid_end;
	unsigned short itemsize;
	const char *filename;
} dbinfo_t;
static const dbinfo_t DbaseInfo[] = {
	{DBID_MET_DAY(0), DBID_MET_DAY(MAX_CENMETP), sizeof(db_metday_t), "daymet"},
	{DBID_MET_CPYDAY(0), DBID_MET_CPYDAY(MAX_CENMETP), sizeof(db_metrmd_t), "rmdmet"},
	{DBID_MET_MONTH(0), DBID_MET_MONTH(MAX_CENMETP), sizeof(db_metmon_t), "monmet"},
	{DBID_METSTIC_DAY(0), DBID_METSTIC_DAY(MAX_CENMETP), sizeof(db_msticday_t), "daymstic"},
	{DBID_METSTIC_MONTH(0), DBID_METSTIC_MONTH(MAX_CENMETP), sizeof(db_msticmon_t), "monmstic"},
	{DBID_TERMSTIC_DAY, DBID_TERMSTIC_DAY+1, sizeof(db_termday_t), "dayterm"},
	{DBID_TERMSTIC_MONTH, DBID_TERMSTIC_MONTH+1, sizeof(db_termmon_t), "monterm"},
	{DBID_MET_CURVE(0), DBID_MET_CURVE(MAX_CENMETP), sizeof(db_metcurve_t), "cuvmet"},
	{DBID_TGRP_DAY(0), DBID_TGRP_DAY(MAX_TGRP), sizeof(db_tgrpday_t), "daytgrp"},
	{DBID_TGRP_MONTH(0), DBID_TGRP_MONTH(MAX_TGRP), sizeof(db_tgrpmon_t), "montgrp"},
	{DBID_TGRP_CURVE(0), DBID_TGRP_CURVE(MAX_TGRP), sizeof(db_tgrpcurve_t), "cuvtgrp"},
};
#define SIZE_DBASEINFO		(sizeof(DbaseInfo)/sizeof(DbaseInfo[0]))

/**
* @brief 获取数据库的数据项大小
* @param dbid 数据库ID
* @return 成功返回数据项大小, 否则返回0
*/
unsigned short DbaseItemSize(unsigned short dbid)
{
	int i;


	for(i=0; i<SIZE_DBASEINFO; i++) {
		if(dbid >= DbaseInfo[i].dbid_base && dbid < DbaseInfo[i].dbid_end)
			return DbaseInfo[i].itemsize;
	}

	return 0;
}

/**
* @brief 获取数据库的储存文件名
* @param dbid 数据库ID
* @param clock 储存数据库文件时的时钟
* @param filename 返回的文件名
* @return 成功返回0, 否则失败
*/
int DbaseFileName(char *filename, unsigned short dbid, dbtime_t dbtime)
{
	int i, mid;

	for(i=0; i<SIZE_DBASEINFO; i++) {
		if(dbid >= DbaseInfo[i].dbid_base && dbid < DbaseInfo[i].dbid_end) {
			mid = dbid - DbaseInfo[i].dbid_base;
			if('m' == DbaseInfo[i].filename[0]) {  //month
				sprintf(filename, DBASE_SAVEPATH "%s%d@%02d%02d.db",
						DbaseInfo[i].filename, mid,
						dbtime.s.year, dbtime.s.month);
			}
			else { //day
				sprintf(filename, DBASE_SAVEPATH "%s%d@%02d%02d%02d.db",
						DbaseInfo[i].filename, mid,
						dbtime.s.year, dbtime.s.month, dbtime.s.day);
			}

			return 0;
		}
	}

	return 1;
}

/**
* @brief 获取一个测量点的数据库的所有储存文件名通配符
* @param dbid 数据库ID
* @param filename 返回的文件名通配符
* @return 成功返回0, 否则失败
*/
int DbaseFileGroupName(char *filename, unsigned short dbid)
{
	int i, mid;

	i = 0;
	for(i=0; i<SIZE_DBASEINFO; i++) {
		if(dbid >= DbaseInfo[i].dbid_base && dbid < DbaseInfo[i].dbid_end) {
			mid = dbid - DbaseInfo[i].dbid_base;
			sprintf(filename, DBASE_SAVEPATH "%s%d@*.db", 
					DbaseInfo[i].filename, mid);
			return 0;
		}

		i++;
	}

	return 1;
}

//F1~F8 二类数据
static const dbsonconfig_t cons_grp1[] = {
	/*F1  */{0x03, 85, DB_OFFSET(db_metday_t, enepa[0])},
	/*F2  */{0x03, 85, DB_OFFSET(db_metday_t, enena[0])},
	/*F3  */{0x03, 70, DB_OFFSET(db_metday_t, dmnpa[0])},
	/*F4  */{0x03, 70, DB_OFFSET(db_metday_t, dmnna[0])},
	/*F5  */{0x02, 20, DB_OFFSET(db_metday_t, enepa_day[0])},
	/*F6  */{0x02, 20, DB_OFFSET(db_metday_t, enepi_day[0])},
	/*F7  */{0x02, 20, DB_OFFSET(db_metday_t, enena_day[0])},
	/*F8  */{0x02, 20, DB_OFFSET(db_metday_t, eneni_day[0])},
};

//F9~F12
static const dbsonconfig_t cons_grp2[] = {
	/*F9  */{0x03, 85, DB_OFFSET(db_metrmd_t, enepa[0])},
	/*F10 */{0x03, 85, DB_OFFSET(db_metrmd_t, enena[0])},
	/*F11 */{0x03, 70, DB_OFFSET(db_metrmd_t, dmnpa[0])},
	/*F12 */{0x03, 70, DB_OFFSET(db_metrmd_t, dmnna[0])},
};

//F17~F24
static const dbsonconfig_t cons_grp3[] = {
	/*F17 */{0x03, 85, DB_OFFSET(db_metmon_t, enepa[0])},
	/*F18 */{0x03, 85, DB_OFFSET(db_metmon_t, enena[0])},
	/*F19 */{0x03, 70, DB_OFFSET(db_metmon_t, dmnpa[0])},
	/*F20 */{0x03, 70, DB_OFFSET(db_metmon_t, dmnna[0])},
	/*F21 */{0x02, 20, DB_OFFSET(db_metmon_t, enepa_day[0])},
	/*F22 */{0x02, 20, DB_OFFSET(db_metmon_t, enepi_day[0])},
	/*F23 */{0x02, 20, DB_OFFSET(db_metmon_t, enena_day[0])},
	/*F28 */{0x02, 20, DB_OFFSET(db_metmon_t, eneni_day[0])},
};

//F25~F32
static const dbsonconfig_t cons_grp4[] = {
	/*F25 */{0x00, 32, DB_OFFSET(db_msticday_t, pwra_max[0])},
	/*F26 */{0x00, 24, DB_OFFSET(db_msticday_t, dm_max[0])},
	/*F27 */{0x00, 66, DB_OFFSET(db_msticday_t, vol_extime[0])},
	/*F28 */{0x00, 14, DB_OFFSET(db_msticday_t, amp_unb[0])},
	/*F29 */{0x00, 34, DB_OFFSET(db_msticday_t, amp_extime[0])},
	/*F30 */{0x00, 4 , DB_OFFSET(db_msticday_t, pwrv_extime[0])},
	/*F31 */{0x00, 10, DB_OFFSET(db_msticday_t, load[0])},
	/*F32 */{0x00, 57, DB_OFFSET(db_msticday_t, rdtime[0])},
};

//F33~F39
static const dbsonconfig_t cons_grp5[] = {
	/*F33 */{0x00, 32, DB_OFFSET(db_msticmon_t, pwra_max[0])},
	/*F34 */{0x00, 24, DB_OFFSET(db_msticmon_t, dm_max[0])},
	/*F35 */{0x00, 66, DB_OFFSET(db_msticmon_t, vol_extime[0])},
	/*F36 */{0x00, 16, DB_OFFSET(db_msticmon_t, amp_unb[0])},
	/*F37 */{0x00, 34, DB_OFFSET(db_msticmon_t, amp_extime[0])},
	/*F38 */{0x00, 4 , DB_OFFSET(db_msticmon_t, pwrv_extime[0])},
	/*F39 */{0x00, 12, DB_OFFSET(db_msticmon_t, load[0])},
};

//F43
static const dbsonconfig_t cons_grp6a[] = {
	/*F43 */{0x00, 6, DB_OFFSET(db_msticday_t, pwrf_ptime[0])},
};

//F44
static const dbsonconfig_t cons_grp6b[] = {
	/*F44 */{0x00, 6, DB_OFFSET(db_msticmon_t, pwrf_ptime[0])},
};

//F49~F50
static const dbsonconfig_t cons_grp7a[] = {
	/*F49 */{0x00, 4, DB_OFFSET(db_termday_t, pwr_time[0])},
	/*F50 */{0x00, 4, DB_OFFSET(db_termday_t, sw_time[0])},
};

//F51~F52
static const dbsonconfig_t cons_grp7b[] = {
	/*F51 */{0x00, 4, DB_OFFSET(db_termmon_t, pwr_time[0])},
	/*F52 */{0x00, 4, DB_OFFSET(db_termmon_t, sw_time[0])},
};

//F53
static const dbsonconfig_t cons_grp7c[] = {
	/*F53 */{0x00, 4, DB_OFFSET(db_termday_t, comm_bytes[0])},
};

//F54
static const dbsonconfig_t cons_grp7d[] = {
	/*F54 */{0x00, 4, DB_OFFSET(db_termmon_t, comm_bytes[0])},
};

//F57~F59
static const dbsonconfig_t cons_grp8a[] = {
	/*F57 */{0x00, 12, DB_OFFSET(db_tgrpday_t, pwra_time[0])},
	/*F58 */{0x00, 21, DB_OFFSET(db_tgrpday_t, fenum58)},
	/*F59 */{0x00, 21, DB_OFFSET(db_tgrpday_t, fenum59)},
};

//F60~F62
static const dbsonconfig_t cons_grp8b[] = {
	/*F60 */{0x00, 12, DB_OFFSET(db_tgrpmon_t, pwra_time[0])},
	/*F61 */{0x00, 21, DB_OFFSET(db_tgrpmon_t, fenum61)},
	/*F62 */{0x00, 21, DB_OFFSET(db_tgrpmon_t, fenum62)},
};

//F65~F66
static const dbsonconfig_t cons_grp9[] = {
	/*F65 */{0x00, 6, DB_OFFSET(db_tgrpmon_t, pcuene[0])},
	/*F66 */{0x00, 6, DB_OFFSET(db_tgrpmon_t, ecuene[0])},
};

//F73~F76
static const dbsonconfig_t cons_grp10[] = {
	/*F73 */{0x00, 2, DB_OFFSET(db_tgrpcurve_t, tpwra[0])},
	/*F74 */{0x00, 2, DB_OFFSET(db_tgrpcurve_t, tpwri[0])},
	/*F75 */{0x00, 4, DB_OFFSET(db_tgrpcurve_t, tenea[0])},
	/*F76 */{0x00, 4, DB_OFFSET(db_tgrpcurve_t, tenei[0])},
};
	
//F81~F88
static const dbsonconfig_t cons_grp11[] = {
	/*F81 */{0x00, 3, DB_OFFSET(db_metcurve_t, pwra[0])},
	/*F82 */{0x00, 3, DB_OFFSET(db_metcurve_t, pwra[3])},
	/*F83 */{0x00, 3, DB_OFFSET(db_metcurve_t, pwra[6])},
	/*F84 */{0x00, 3, DB_OFFSET(db_metcurve_t, pwra[9])},
	/*F85 */{0x00, 3, DB_OFFSET(db_metcurve_t, pwri[0])},
	/*F86 */{0x00, 3, DB_OFFSET(db_metcurve_t, pwri[3])},
	/*F87 */{0x00, 3, DB_OFFSET(db_metcurve_t, pwri[6])},
	/*F88 */{0x00, 3, DB_OFFSET(db_metcurve_t, pwri[9])},
};

//F89~F95
static const dbsonconfig_t cons_grp12[] = {
	/*F89 */{0x00, 2, DB_OFFSET(db_metcurve_t, vol[0])},
	/*F90 */{0x00, 2, DB_OFFSET(db_metcurve_t, vol[2])},
	/*F91 */{0x00, 2, DB_OFFSET(db_metcurve_t, vol[4])},
	/*F92 */{0x00, 2, DB_OFFSET(db_metcurve_t, amp[0])},
	/*F93 */{0x00, 2, DB_OFFSET(db_metcurve_t, amp[2])},
	/*F94 */{0x00, 2, DB_OFFSET(db_metcurve_t, amp[4])},
	/*F95 */{0x00, 2, DB_OFFSET(db_metcurve_t, amp[6])},
};

//F97~F104
static const dbsonconfig_t cons_grp13[] = {
	/*F97 */{0x00, 4, DB_OFFSET(db_metcurve_t, uenepa[0])},
	/*F98 */{0x00, 4, DB_OFFSET(db_metcurve_t, uenepi[0])},
	/*F99 */{0x00, 4, DB_OFFSET(db_metcurve_t, uenena[0])},
	/*F100*/{0x00, 4, DB_OFFSET(db_metcurve_t, ueneni[0])},
	/*F101*/{0x00, 4, DB_OFFSET(db_metcurve_t, enepa[0])},
	/*F102*/{0x00, 4, DB_OFFSET(db_metcurve_t, enepi[0])},
	/*F103*/{0x00, 4, DB_OFFSET(db_metcurve_t, enena[0])},
	/*F104*/{0x00, 4, DB_OFFSET(db_metcurve_t, eneni[0])},
};

//F105~F110
static const dbsonconfig_t cons_grp14[] = {
	/*F105*/{0x00, 2, DB_OFFSET(db_metcurve_t, pwrf[0])},
	/*F106*/{0x00, 2, DB_OFFSET(db_metcurve_t, pwrf[2])},
	/*F107*/{0x00, 2, DB_OFFSET(db_metcurve_t, pwrf[4])},
	/*F108*/{0x00, 2, DB_OFFSET(db_metcurve_t, pwrf[6])},
	/*F109*/{0x00, 6, DB_OFFSET(db_metcurve_t, vol_arc[0])},
	/*F110*/{0x00, 6, DB_OFFSET(db_metcurve_t, amp_arc[0])},
};

//F145~F148
static const dbsonconfig_t cons_grp19[] = {
	/*F145*/{0x00, 4, DB_OFFSET(db_metcurve_t, enepi1[0])},
	/*F146*/{0x00, 4, DB_OFFSET(db_metcurve_t, enepi4[0])},
	/*F147*/{0x00, 4, DB_OFFSET(db_metcurve_t, eneni2[0])},
	/*F148*/{0x00, 4, DB_OFFSET(db_metcurve_t, eneni3[0])},
};

//F153~F156
static const dbsonconfig_t cons_grp20a[] = {
	/*F153*/{0x01,15, DB_OFFSET(db_metday_t, enepa_abc[0])},
	/*F154*/{0x01,12, DB_OFFSET(db_metday_t, enepi_abc[0])},
	/*F155*/{0x01,15, DB_OFFSET(db_metday_t, enena_abc[0])},
	/*F156*/{0x01,12, DB_OFFSET(db_metday_t, eneni_abc[0])},
};

//F157~F160
static const dbsonconfig_t cons_grp20b[] = {
	/*F157*/{0x01,15, DB_OFFSET(db_metmon_t, enepa_abc[0])},
	/*F158*/{0x01,12, DB_OFFSET(db_metmon_t, enepi_abc[0])},
	/*F159*/{0x01,15, DB_OFFSET(db_metmon_t, enena_abc[0])},
	/*F160*/{0x01,12, DB_OFFSET(db_metmon_t, eneni_abc[0])},
};

//F161~F164
static const dbsonconfig_t cons_grp21b[] = {
	/*F161*/{0x03,25, DB_OFFSET(db_metday_t,enepa[0])},
	/*F162*/{0x03,20, DB_OFFSET(db_metday_t, enepi[0])},
	/*F163*/{0x03,25, DB_OFFSET(db_metday_t, enena[0])},
	/*F164*/{0x03,20, DB_OFFSET(db_metday_t, eneni[0])},
};


const dbaseconfig_t DbaseConfig[] = {
	{0,   0xff, DBID_MET_DAY(0),      DBSAVE_DAY,   DBATTR_METP, cons_grp1},
	{1,   0x0f, DBID_MET_CPYDAY(0),   DBSAVE_CPYDAY,DBATTR_METP, cons_grp2},
	{2,   0xff, DBID_MET_MONTH(0),    DBSAVE_MONTH, DBATTR_METP, cons_grp3},
	{3,   0xff, DBID_METSTIC_DAY(0),  DBSAVE_DAY,   DBATTR_METP, cons_grp4},
	{4,   0x7f, DBID_METSTIC_MONTH(0),DBSAVE_MONTH, DBATTR_METP, cons_grp5},
	{5,   0x04, DBID_METSTIC_DAY(0),  DBSAVE_DAY,   DBATTR_METP, cons_grp6a},
	{5,   0x08, DBID_METSTIC_MONTH(0),DBSAVE_MONTH, DBATTR_METP, cons_grp6b},
	{6,   0x03, DBID_TERMSTIC_DAY,    DBSAVE_DAY,   DBATTR_TERM, cons_grp7a},
	{6,   0x0c, DBID_TERMSTIC_MONTH,  DBSAVE_MONTH, DBATTR_TERM, cons_grp7b},
	{6,   0x10, DBID_TERMSTIC_DAY,    DBSAVE_DAY,   DBATTR_TERM, cons_grp7c},
	{6,   0x20, DBID_TERMSTIC_MONTH,  DBSAVE_MONTH, DBATTR_TERM, cons_grp7d},
	{7,   0x07, DBID_TGRP_DAY(0),     DBSAVE_DAY,   DBATTR_TGRP, cons_grp8a},
	{7,   0x38, DBID_TGRP_MONTH(0),   DBSAVE_MONTH, DBATTR_TGRP, cons_grp8b},
	{8,   0x03, DBID_TGRP_MONTH(0),   DBSAVE_MONTH, DBATTR_TGRP, cons_grp9},
	{9,   0x0f, DBID_TGRP_CURVE(0),   DBSAVE_CURVE, DBATTR_TGRP, cons_grp10},
	{10,  0xff, DBID_MET_CURVE(0),    DBSAVE_CURVE, DBATTR_METP, cons_grp11},
	{11,  0x7f, DBID_MET_CURVE(0),    DBSAVE_CURVE, DBATTR_METP, cons_grp12},
	{12,  0xff, DBID_MET_CURVE(0),    DBSAVE_CURVE, DBATTR_METP, cons_grp13},
	{13,  0x3f, DBID_MET_CURVE(0),    DBSAVE_CURVE, DBATTR_METP, cons_grp14},
	{18,  0x0f, DBID_MET_CURVE(0),    DBSAVE_CURVE, DBATTR_METP, cons_grp19},
	{19,  0x0f, DBID_MET_DAY(0),      DBSAVE_DAY,   DBATTR_METP, cons_grp20a},
	{19,  0xf0, DBID_MET_MONTH(0),    DBSAVE_MONTH, DBATTR_METP, cons_grp20b},
	{20,  0x0f, DBID_MET_DAY(0),    DBSAVE_DAY,   DBATTR_METP, cons_grp21b},

	{0, 0, 0, 0, 0, NULL},
};

