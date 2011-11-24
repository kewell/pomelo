/**
* dbtime.c -- 数据库时间格式
* 
* 
* 创建时间: 2010-5-24
* 最后修改时间: 2010-5-24
*/

#include "include/lib/dbtime.h"

static const unsigned char MonthMaxDays[] = {
	0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0, 0, 0
};

/**
* @brief 增加1天
* @param pdbtime 数据库时间变量指针
*/
void DbTimeAddOneDay(dbtime_t *pdbtime)
{
	unsigned maxdays;

	maxdays = MonthMaxDays[pdbtime->s.month];
	if(2 == pdbtime->s.month && 0 == (pdbtime->s.year&0x03)) maxdays += 1;

	pdbtime->s.day++;
	if(pdbtime->s.day > maxdays) {
		pdbtime->s.day = 1;
		pdbtime->s.month++;
		if(pdbtime->s.month > 12) {
			pdbtime->s.month = 1;
			pdbtime->s.year++;
		}
	}
}

/**
* @brief 增加1月
* @param pdbtime 数据库时间变量指针
*/
void DbTimeAddOneMonth(dbtime_t *pdbtime)
{
	pdbtime->s.month++;
	if(pdbtime->s.month > 12) {
		pdbtime->s.month = 1;
		pdbtime->s.year++;
	}
}

/**
* @brief 增加15分钟
* @param pdbtime 数据库时间变量指针
*/
void DbTimeAddOneTick(dbtime_t *pdbtime)
{
	pdbtime->s.tick++;
	if(pdbtime->s.tick > 95) {
		pdbtime->s.tick = 0;
		DbTimeAddOneDay(pdbtime);
	}
}

