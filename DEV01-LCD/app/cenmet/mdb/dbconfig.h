/**
* dbconfig.h -- 历史数据库配置
* 
* 
* 创建时间: 2010-5-11
* 最后修改时间: 2010-5-11
*/

#ifndef _DB_CONFIG_H
#define _DB_CONFIG_H

#include "include/environment.h"

#define DBASE_SAVEPATH		DATA_PATH

#include "include/lib/dbtime.h"

#define DBID_MET_DAY(mid)			((unsigned short)0+(mid)) 
#define DBID_MET_CPYDAY(mid)		((unsigned short)20+(mid))
#define DBID_MET_MONTH(mid)			((unsigned short)40+(mid))
#define DBID_METSTIC_DAY(mid)		((unsigned short)60+(mid))
#define DBID_METSTIC_MONTH(mid)		((unsigned short)80+(mid))
#define DBID_TERMSTIC_DAY			((unsigned short)100)
#define DBID_TERMSTIC_MONTH			((unsigned short)101)
#define DBID_MET_CURVE(mid)			((unsigned short)120+(mid))
#define DBID_TGRP_DAY(tid)			((unsigned short)140+(tid))   //F57~F59
#define DBID_TGRP_MONTH(tid)		((unsigned short)160+(tid))    //F60~F62, F65~F66
#define DBID_TGRP_CURVE(tid)		((unsigned short)180+(tid))    //F73~F76

#define DBSAVE_DAY    0    //日冻结
#define DBSAVE_CPYDAY    1    //抄表日冻结
#define DBSAVE_MONTH   2    //月冻结
#define DBSAVE_CURVE    3    //曲线

#define DBATTR_METP    0    //测量点数据
#define DBATTR_TGRP    1    //总加组数据
#define DBATTR_TERM    2    //终端数据

#define DBFLAG_RDTIME	0x01	//带抄表时间
#define DBFLAG_FENUM	0x02   //带费率数

typedef struct {
	unsigned char flag;    //数据标志
	unsigned char len;    //子项数据长度
	unsigned short offset;    //子项数据地址基于数据库基地址的偏移
} dbsonconfig_t;

typedef struct {
	unsigned char grpid;    //组标识, F1~F8=0, F9~F16=1,...
	unsigned char cids;   //数据单元掩码
	unsigned short dbid;   //数据库ID
	unsigned char type;   //数据库类型
	unsigned char attr;    //数据库属性
	const dbsonconfig_t *psons;
} dbaseconfig_t;
extern const dbaseconfig_t DbaseConfig[];

#define DB_OFFSET(type, var)	((unsigned short)&(((type *)0)->var))

unsigned short DbaseItemSize(unsigned short dbid);
int DbaseFileName(char *filename, unsigned short dbid, dbtime_t dbtime);
int DbaseFileGroupName(char *filename, unsigned short dbid);

#endif /*_DB_CONFIG_H*/

