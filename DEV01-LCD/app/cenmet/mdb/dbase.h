/**
* dbase.h -- 历史数据库操作头文件
* 
* 
* 创建时间: 2010-5-11
* 最后修改时间: 2010-5-11
*/

#ifndef _DBASE_H
#define _DBASE_H

#include "include/sys/timeal.h"
#include "include/lib/dbtime.h"

#define DBFREZ_15MIN	1
#define DBFREZ_30MIN	2
#define DBFREZ_1HOUR	3
#define DBFREZ_DAY		4
#define DBFREZ_MONTH	5

static inline int DbGetItemNum(unsigned char frez)
{
	switch(frez) {
	case DBFREZ_15MIN: return 96; break;
	case DBFREZ_30MIN: return 48; break;
	case DBFREZ_1HOUR: return 24; break;
	case DBFREZ_DAY:
	case DBFREZ_MONTH: return 1; break;
	default: return 0;
	}
}

//曲线文件从0:15开始,到24:00结束
static inline int DbGetItemOffset(int itemnum, dbtime_t dbtime)
{
	int idx;

	if(1 == itemnum) return 0;

	idx = (int)dbtime.s.tick & 0xff;
	idx = idx/(96/itemnum);
	/*if(0 == idx) idx = itemnum - 1;
	else idx -= 1;*/

	return idx;
}

int DbaseSave(unsigned short dbid, const unsigned char *buf, unsigned char frez, dbtime_t dbtime);
void DbaseFlush(void);

void DbaseReadLock(void);
void DbaseReadUnlock(void);
unsigned char *DbaseReadCache(void);
int DbaseRead(unsigned short dbid, unsigned char frez, dbtime_t dbtime);

void DbaseDelete(unsigned short dbid);
void DbaseDeleteAll(void);
void DbaseFormat(void);

#endif /*_DBASE_H*/

