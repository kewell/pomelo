/**
* curve.c -- 历史数据查询接口
* 
* 
* 创建时间: 2010-5-13
* 最后修改时间: 2010-5-14
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/param/capconf.h"
#include "dbconfig.h"
#include "dbase.h"
#include "mdbconf.h"
#include "include/cenmet/qrycurve.h"
#include "include/lib/bcd.h"
#include "include/lib/dbtime.h"
#include "include/cenmet/sndtime.h"

static dbtime_t QueryDbTime;
static unsigned char QueryFrez;
static int QueryDataNum;
static int QueryTask;  // 1-主动任务发送, 0-查询
static unsigned char QueryDuid[4];
static int QueryStep;

typedef int (*query_fn)(unsigned short dbid, qrycurve_buffer_t **psendbuffer, const dbaseconfig_t *pconfig, int offset);

/**
* @brief 查询日冻结历史数据
* @param dbid 数据库ID
* @param sendbuffer 返回数据缓存链表
* @param pconfig 数据库配置信息
* @param offset 数据库内数据项信息索引偏移值
* @return 成功0, 否则失败
*/
static int QueryDbaseDay(unsigned short dbid, qrycurve_buffer_t **psendbuffer, const dbaseconfig_t *pconfig, int offset)
{
	const dbsonconfig_t *pson = pconfig->psons + offset;
	int cpylen, addlen;
	qrycurve_buffer_t *pbuffer = *psendbuffer;
	unsigned char *pcache, *puc;
	dbtime_t dbtime, dbtimecur;

	printf("QueryDbaseDay...1\n");
	dbtime.u = QueryDbTime.u;
	dbtime.s.tick = 0;
	{
		sysclock_t clock;
		SysClockReadCurrent(&clock);
		SYSCLOCK_DBTIME(&clock, dbtimecur);
	}
/*
 * 由于日冻结数据都是在0:00分冻结的，因此冻结的是前一天
 * 的数据，所以查询日期推后一天
 */
	#if 0
	if(0 == QueryTask) 
	{
		DbTimeAddOneDay(&dbtime);
	}
	#endif
	if(dbtime.u > dbtimecur.u) return 1;

mark_retry:
	while(DbaseRead(dbid, DBFREZ_DAY, dbtime) < 0) {
		if(0 == QueryTask) return 1;

		DbTimeAddOneDay(&dbtime);
		if(dbtime.u > dbtimecur.u) return 1;
	}

	printf("QueryDbaseDay...2\n");
	addlen = 0;
	if(pson->flag & DBFLAG_RDTIME) addlen += 5;
	if(pson->flag & DBFLAG_FENUM) addlen += 1;

	cpylen = (int)pson->len&0xff;
	QRYCBUF_SCAN(pbuffer, cpylen+addlen+7, *psendbuffer, QueryStep);
	if(NULL == pbuffer) return 1;

	pcache = DbaseReadCache();
	puc = QRYCBUF_DATA(pbuffer);

	puc[0] = QueryDuid[0];
	puc[1] = QueryDuid[1];
	puc[2] = QueryDuid[2];
	puc[3] = QueryDuid[3];
	puc += 4;
	if(0 == QueryTask) {
		puc[0] = QueryDbTime.s.day;
		puc[1] = QueryDbTime.s.month;
		puc[2] = QueryDbTime.s.year;
	}
	else {
		puc[0] = dbtime.s.day;
		puc[1] = dbtime.s.month;
		puc[2] = dbtime.s.year;
	}
	HexToBcd(puc, 3);
	puc += 3;

	if(pson->flag & DBFLAG_RDTIME) {
		smallcpy(puc, pcache, 5);
		puc += 5;
	}
	if(pson->flag & DBFLAG_FENUM) {
		*puc++ = MAXNUM_FEEPRD;
	}

	memcpy(puc, pcache + pson->offset, cpylen);
	QRYCBUF_ADD(pbuffer, cpylen + addlen + 7);

	if(QueryTask) {
		DbTimeAddOneDay(&dbtime);
		SetSndTime(QueryDuid, dbtime.u, QueryStep);
		if(dbtime.u > dbtimecur.u) return 0;
		goto mark_retry;
	}
	printf("QueryDbaseDay...3\n");
	return 0;
}

/**
* @brief 查询月冻结历史数据
* @param dbid 数据库ID
* @param sendbuffer 返回数据缓存链表
* @param pconfig 数据库配置信息
* @param offset 数据库内数据项信息索引偏移值
* @return 成功0, 否则失败
*/
static int QueryDbaseMonth(unsigned short dbid, qrycurve_buffer_t **psendbuffer, const dbaseconfig_t *pconfig, int offset)
{
	const dbsonconfig_t *pson = pconfig->psons + offset;
	int cpylen, addlen;
	qrycurve_buffer_t *pbuffer = *psendbuffer;
	unsigned char *pcache, *puc;
	dbtime_t dbtime, dbtimecur;

	dbtime.u = QueryDbTime.u;
	dbtime.s.day = 1;
	dbtime.s.tick = 0;
	{
		sysclock_t clock;
		SysClockReadCurrent(&clock);
		SYSCLOCK_DBTIME(&clock, dbtimecur);
	}

/*
 * 由于月冻结数据都是在0:00分冻结的，因此冻结的是前一月
 * 的数据，所以查询日期推后一月
 */
	if(0 == QueryTask) {
		DbTimeAddOneMonth(&dbtime);
	}
	if(dbtime.u > dbtimecur.u) return 1;

mark_retry:
	while(DbaseRead(dbid, DBFREZ_MONTH, dbtime) < 0) {
		if(0 == QueryTask) return 1;

		DbTimeAddOneMonth(&dbtime);
		if(dbtime.u > dbtimecur.u) return 1;
	}

	addlen = 0;
	if(pson->flag & DBFLAG_RDTIME) addlen += 5;
	if(pson->flag & DBFLAG_FENUM) addlen += 1;

	cpylen = (int)pson->len&0xff;
	QRYCBUF_SCAN(pbuffer, cpylen+addlen+6, *psendbuffer, QueryStep);
	if(NULL == pbuffer) return 1;

	pcache = DbaseReadCache();
	puc = QRYCBUF_DATA(pbuffer);

	puc[0] = QueryDuid[0];
	puc[1] = QueryDuid[1];
	puc[2] = QueryDuid[2];
	puc[3] = QueryDuid[3];
	puc += 4;
	if(0 == QueryTask) {
		puc[0] = QueryDbTime.s.month;
		puc[1] = QueryDbTime.s.year;
	}
	else {
		puc[0] = dbtime.s.month;
		puc[1] = dbtime.s.year;
	}
	HexToBcd(puc, 2);
	puc += 2;

	if(pson->flag & DBFLAG_RDTIME) {
		smallcpy(puc, pcache, 5);
		puc += 5;
	}
	if(pson->flag & DBFLAG_FENUM) {
		*puc++ = MAXNUM_FEEPRD;
	}

	smallcpy(puc, pcache + pson->offset, cpylen);
	QRYCBUF_ADD(pbuffer, cpylen+addlen+6);

	if(QueryTask) {
		DbTimeAddOneMonth(&dbtime);
		SetSndTime(QueryDuid, dbtime.u, QueryStep);
		if(dbtime.u > dbtimecur.u) return 0;
		goto mark_retry;
	}

	return 0;
}

static inline int DataIsEmpty(const unsigned char *buf, int len)
{
	int i;

	for(i=0; i<len; i++) {
		if(*buf++ != 0xee) return 0;
	}
	return 1;
}

#if 0
#define ADD_ONEDAY(dbtime, badded) { \
	if(!badded) { \
		badded = 1; \
		switch(QueryFrez) { \
		case DBFREZ_15MIN: dbtime.s.tick = 1; break; \
		case DBFREZ_30MIN: dbtime.s.tick = 2; break; \
		default: dbtime.s.tick = 4; break; \
		} \
	} \
	DbTimeAddOneDay(&(dbtime)); \
}
#else
#define ADD_ONEDAY(dbtime) { \
	dbtime.s.tick = 0; \
	DbTimeAddOneDay(&(dbtime)); \
}
#endif

/**
* @brief 查询曲线类历史数据
* @param dbid 数据库ID
* @param sendbuffer 返回数据缓存链表
* @param pconfig 数据库配置信息
* @param offset 数据库内数据项信息索引偏移值
* @return 成功0, 否则失败
*/
static int QueryDbaseCurve(unsigned short dbid, qrycurve_buffer_t **psendbuffer, const dbaseconfig_t *pconfig, int offset)
{
	const dbsonconfig_t *pson = pconfig->psons + offset;
	int cpylen, itemsize, itemnum, frezdev, ticknum;
	qrycurve_buffer_t *pbuffer = *psendbuffer;
	unsigned char *pcache, *puc;
	int starti, endi, i, maxi;
	dbtime_t dbtime, dbtimecur;

	/*DebugPrint(0, "query curve dbid=%d, offset=%d, %02d-%d-%d %d, frez=%d\n",
				dbid, offset, QueryDbTime.s.year, QueryDbTime.s.month, QueryDbTime.s.day,
				QueryDbTime.s.tick, QueryFrez);*/

	switch(QueryFrez) {
	case DBFREZ_15MIN: frezdev = 15; ticknum = 1; break;
	case DBFREZ_30MIN: frezdev = 30; ticknum = 2; break;
	case DBFREZ_1HOUR: frezdev = 60; ticknum = 4; break;
	default: return 1;
	}

	dbtime.u = QueryDbTime.u;
	{
		sysclock_t clock;
		SysClockReadCurrent(&clock);
		SYSCLOCK_DBTIME(&clock, dbtimecur);
	}
	if(dbtime.u > dbtimecur.u) return 1;

	itemsize = DbaseItemSize(dbid);
	if(itemsize <= 0) return 1;
	itemnum = DbGetItemNum(QueryFrez);
	if(itemnum <= 0) return 1;

	maxi = itemnum;
	if(DBTIME_ISONEDAY(dbtime, dbtimecur)) {
		maxi = DbGetItemOffset(itemnum, dbtimecur);
		maxi += 1;
	}
	//DebugPrint(0, "maxi=%d\n", maxi);

mark_retry:
	while(DbaseRead(dbid, QueryFrez, dbtime) < 0) {
		//DebugPrint(0, "read db fail\n");
		if(0 == QueryTask) return 1;

		ADD_ONEDAY(dbtime);
		if(dbtime.u >= dbtimecur.u) return 1;
	}

	cpylen = (int)pson->len&0xff;
	starti = DbGetItemOffset(itemnum, dbtime);

mark_recopy:
	//skip the head empty buffer
	pcache = DbaseReadCache();
	puc = pcache + starti*itemsize;
	for(;starti<maxi; starti++,puc+=itemsize) {
		//PrintHexLog(0, puc+pson->offset, cpylen);
		if(!DataIsEmpty(puc+pson->offset, cpylen)) break;
	}
	if(starti >= maxi) {
		//DebugPrint(0, "all empty\n");
		if(0 == QueryTask) return 1;

		ADD_ONEDAY(dbtime);
		if(dbtime.u >= dbtimecur.u) return 1;
		goto mark_retry;
	}

	endi = starti+1;
	puc = pcache + endi*itemsize;
	i = 0;
	for(; endi<maxi; endi++,puc+=itemsize) {
		if(DataIsEmpty(puc+pson->offset, cpylen)) {
			i++;
			if(i >= 4) break;  //太多空数据
		}
		else i = 0;
	}
	if(i) {
		if(endi < maxi) endi -= (i-1);
		else endi -= i;
	}

	//DebugPrint(0, "query start=%d, end=%d\n", starti, endi);

	i = endi - starti;
	if(i > QueryDataNum) {
		endi = starti + QueryDataNum;
		i = endi -starti;
	}

	QRYCBUF_SCAN(pbuffer, cpylen*i+11, *psendbuffer, QueryStep);
	if(NULL == pbuffer) return 1;
	puc = QRYCBUF_DATA(pbuffer);
	QRYCBUF_ADD(pbuffer, cpylen*i+11);

	puc[0] = QueryDuid[0];
	puc[1] = QueryDuid[1];
	puc[2] = QueryDuid[2];
	puc[3] = QueryDuid[3];
	puc += 4;
	puc[4] = dbtime.s.year;
	puc[3] = dbtime.s.month;
	puc[2] = dbtime.s.day;
	puc[1] = (frezdev*starti)/60;
	puc[0] = (frezdev*starti)%60;
	HexToBcd(puc, 5);
	puc[5] = QueryFrez;
	puc[6] = i;
	puc += 7;

	pcache += itemsize*starti;
	for(; starti<endi; starti++) {
		smallcpy(puc, pcache+pson->offset, cpylen);
		puc += cpylen;
		pcache += itemsize;
		QueryDataNum -= 1;
	}

	if(QueryTask) {
		dbtime_t dbtimesnd;

		dbtimesnd.u = dbtime.u;
		if(endi >= itemnum) {
			dbtimesnd.s.tick = 0;
			DbTimeAddOneDay(&dbtimesnd);
		}
		else {
			dbtimesnd.s.tick = ticknum * endi;
		}

		SetSndTime(QueryDuid, dbtimesnd.u, QueryStep);
	}

	if(QueryTask || QueryDataNum > 0) {
		if(QueryTask) QueryDataNum = 255;
		if(endi >= itemnum) {
			if(0 == QueryTask) return 0;

			ADD_ONEDAY(dbtime);
			if(dbtime.u >= dbtimecur.u) return 1;
			goto mark_retry;
		}
		else {
			starti = endi;
			goto mark_recopy;
		}
	}

	return 0;
}

/**
* @brief 主动发送历史数据
* @param recvbuf 接收缓存区指针
* @param recvlen 接收缓存区长度
* @param sendbuf 返回数据缓存链表
* @param multir 曲线抽取倍率
* @return 成功0, 否则失败
*/
int ActiveSendCurve(const unsigned char *recvbuf, int recvlen, qrycurve_buffer_t *sendbuffer, unsigned char multir)
{
	unsigned short metpid;
	unsigned char pns, pnmask, grpid, fns, fnmask;

	if(recvlen < 5) return 1;

	DbaseReadLock();

	QueryTask = 1;
	QueryStep = 0;
	ClearSndTime();

	while(recvlen >= 4) {
		pns = recvbuf[0];
		if(0 == recvbuf[1]) metpid = 0;
		else metpid = ((unsigned short)(recvbuf[1]-1)<<3) + 1;
		grpid = recvbuf[3];
		fns = recvbuf[2];

		recvbuf += 4;
		recvlen -= 4;

		if(0 == metpid) pnmask = 0x80;
		else pnmask = 1;
		for(; pnmask!=0; pnmask<<=1,metpid++) {
			const dbaseconfig_t *pconfig;
			int offset;

			if(metpid && (pns&pnmask) == 0) continue;

			if((sendbuffer->maxlen-sendbuffer->datalen) < 4) {
				sendbuffer = sendbuffer->next;
				if(NULL == sendbuffer) return 1;
			}

		mark_retry:
			pconfig = DbaseConfig;
			for(; pconfig->cids!=0; pconfig++) {
				if(grpid == pconfig->grpid && (fns&pconfig->cids) != 0) break;
			}
			if(0 == pconfig->cids) goto mark_fail;

			for(fnmask=1; fnmask!=0; fnmask<<=1) {
				if(fnmask&pconfig->cids) break;
			}
			if(0 == fnmask) goto mark_fail;

			for(offset=0; fnmask!=0; fnmask<<=1,offset++) {
				int rtn;
				query_fn pfunc;
				unsigned short dbid;
				
				if((fnmask&pconfig->cids) == 0) break;;
				if((fnmask&fns) == 0) continue;
				fns &= ~fnmask;

				switch(pconfig->attr) {
				case DBATTR_METP:
					if(metpid == 0 || metpid > MAX_CENMETP) continue;
					dbid = pconfig->dbid + metpid - 1;
					break;
				case DBATTR_TGRP:
					if(metpid == 0 || metpid > MAX_TGRP) continue;
					dbid = pconfig->dbid + metpid - 1;
					break;
				case DBATTR_TERM:
					if(metpid != 0) continue;
					dbid = pconfig->dbid;
					break;
				default:
					ErrorLog("invalid db attr(%d)\n", pconfig->attr);
					continue;
				}

				if(0 == metpid) QueryDuid[0] = QueryDuid[1] = 0;
				else {
					QueryDuid[0] = pnmask;
					QueryDuid[1] = ((metpid-1)>>3) + 1;
				}
				QueryDuid[2] = fnmask;
				QueryDuid[3] = grpid;

				switch(pconfig->type) {
				case DBSAVE_DAY:
				case DBSAVE_CPYDAY:
					pfunc = QueryDbaseDay;
					QueryDbTime.u = GetSndTime(QueryDuid);
					QueryDbTime.s.tick = 0;
					break;

				case DBSAVE_MONTH:
					pfunc = QueryDbaseMonth;
					QueryDbTime.u = GetSndTime(QueryDuid);
					QueryDbTime.s.day = 1;
					QueryDbTime.s.tick = 0;
					break;

				case DBSAVE_CURVE:
					pfunc = QueryDbaseCurve;
					QueryDbTime.u = GetSndTime(QueryDuid);
					QueryDataNum = 255;
					switch(multir) {
					case 1: QueryFrez = CURVE_FREZ; break;
					case 2: QueryFrez = CURVE_FREZ+1; break;
					case 3: QueryFrez = CURVE_FREZ+1; break;
					case 4: QueryFrez = CURVE_FREZ+2; break;
					}
					if(QueryFrez > DBFREZ_1HOUR) QueryFrez = DBFREZ_1HOUR;
					break;
					
				default:
					ErrorLog("invalid db type(%d)\n", pconfig->type);
					goto mark_fail;
				}

				rtn = (*pfunc)(dbid, &sendbuffer, pconfig, offset);
				if(rtn && NULL == sendbuffer) goto mark_fail;
			}

			if(fns) goto mark_retry;  //还没查询完
		}
	}

	DbaseReadUnlock();
	return 0;

mark_fail:
	DbaseReadUnlock();
	return 1;
}


/**
* @brief 查询历史数据
* @param recvbuf 接收缓存区指针
* @param recvlen 接收缓存区长度
* @param sendbuf 返回数据缓存链表
* @return 成功0, 否则失败
*/
int QueryCurve(const unsigned char *recvbuf, int recvlen, qrycurve_buffer_t *sendbuffer)
{
	unsigned short metpid;
	unsigned char pns, pnmask, grpid, fns, fnmask;

	printf("QueryCurve...1\n");

	if(recvlen < 6) return 1;

	DbaseReadLock();

	QueryTask = 0;
	QueryStep = 0;

	while(recvlen >= 6) {
		pns = recvbuf[0];
		if(0 == recvbuf[1]) metpid = 0;
		else metpid = ((unsigned short)(recvbuf[1]-1)<<3) + 1;
		grpid = recvbuf[3];//fn
		fns = recvbuf[2];

		recvbuf += 4;
		recvlen -= 4;

		if(0 == metpid) pnmask = 0x80;
		else pnmask = 1;
		for(; pnmask!=0; pnmask<<=1,metpid++) {
			const dbaseconfig_t *pconfig;
			int offset;

			if(metpid && (pns&pnmask) == 0) continue;

			if((sendbuffer->maxlen-sendbuffer->datalen) < 4) {
				sendbuffer = sendbuffer->next;
				if(NULL == sendbuffer) return 1;
			}

		mark_retry:
			pconfig = DbaseConfig;
			printf("QueryCurve...2\n");
			for(; pconfig->cids!=0; pconfig++) {
				if(grpid == pconfig->grpid && (fns&pconfig->cids) != 0) break;
			}
			
			if(0 == pconfig->cids) goto mark_fail;
			printf("QueryCurve...3\n");
			for(fnmask=1; fnmask!=0; fnmask<<=1) {
				if(fnmask&pconfig->cids) break;
			}
			if(0 == fnmask) goto mark_fail;
			printf("QueryCurve...4\n");
			for(offset=0; fnmask!=0; fnmask<<=1,offset++) {
				int rtn;
				query_fn pfunc;
				unsigned short dbid;
				
				if((fnmask&pconfig->cids) == 0) break;
				if((fnmask&fns) == 0) continue;
				fns &= ~fnmask;
				printf("QueryCurve...5\n");
				switch(pconfig->type) {
				case DBSAVE_DAY:
				case DBSAVE_CPYDAY:
					printf("QueryCurve...6\n");
					if(recvlen < 3) goto mark_fail;
					QueryDbTime.s.day = recvbuf[0];
					QueryDbTime.s.month = recvbuf[1];
					QueryDbTime.s.year = recvbuf[2];
					BcdToHex(&QueryDbTime.s.day, 3);
					QueryDbTime.s.tick = 0;
					recvbuf += 3;
					recvlen -= 3;
					pfunc = QueryDbaseDay;
					break;

				case DBSAVE_MONTH:
					if(recvlen < 2) goto mark_fail;
					QueryDbTime.s.day = 1;
					QueryDbTime.s.month = recvbuf[0];
					QueryDbTime.s.year = recvbuf[1];
					BcdToHex(&QueryDbTime.s.month, 2);
					QueryDbTime.s.tick = 0;
					recvbuf += 2;
					recvlen -= 2;
					pfunc = QueryDbaseMonth;
					break;

				case DBSAVE_CURVE:
					if(recvlen < 7) goto mark_fail;
					{
						sysclock_t clock;

						clock.minute = recvbuf[0];
						clock.hour = recvbuf[1];
						clock.day = recvbuf[2];
						clock.month = recvbuf[3];
						clock.year = recvbuf[4];
						BcdToHex(&clock.year, 5);
						SYSCLOCK_DBTIME(&clock, QueryDbTime);
					}

					QueryFrez = recvbuf[5];
					QueryDataNum = recvbuf[6];
					recvbuf += 7;
					recvlen -= 7;
					pfunc = QueryDbaseCurve;
					break;

				default:
					ErrorLog("invalid db type(%d)\n", pconfig->type);
					goto mark_fail;
				}

				switch(pconfig->attr) {
				case DBATTR_METP:
					if(metpid == 0 || metpid > MAX_CENMETP) continue;
					dbid = pconfig->dbid + metpid - 1;
					break;
				case DBATTR_TGRP:
					if(metpid == 0 || metpid > MAX_TGRP) continue;
					dbid = pconfig->dbid + metpid - 1;
					break;
				case DBATTR_TERM:
					if(metpid != 0) continue;
					dbid = pconfig->dbid;
					break;
				default:
					ErrorLog("invalid db attr(%d)\n", pconfig->attr);
					continue;
				}

				if(0 == metpid) QueryDuid[0] = QueryDuid[1] = 0;
				else {
					QueryDuid[0] = pnmask;
					QueryDuid[1] = ((metpid-1)>>3) + 1;
				}
				QueryDuid[2] = fnmask;
				QueryDuid[3] = grpid;

				rtn = (*pfunc)(dbid, &sendbuffer, pconfig, offset);
				if(rtn && NULL == sendbuffer) goto mark_fail;
			}

			if(fns) goto mark_retry;  //还没查询完
		}
	}

	DbaseReadUnlock();
	return 0;

mark_fail:
	DbaseReadUnlock();
	return 1;
}

