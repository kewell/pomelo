/**
* plcurve.c -- 载波表2类数据查询
* 
* 
* 创建时间: 2010-5-22
* 最后修改时间: 2010-5-22
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/lib/dbtime.h"
#include "include/lib/bcd.h"
#include "include/lib/align.h"
#include "include/param/meter.h"
#include "include/param/mix.h"
#include "plmdb.h"
#include "include/cenmet/qrycurve.h"
#include "include/cenmet/sndtime.h"

static unsigned char PlMdbQryCache[sizeof(PlMdbMonth)+128];
static dbtime_t CacheDate;
static unsigned char CacheQryDbid = 0xff;
static dbtime_t PlQueryDbTime;
static int PlQueryTask;
static int PlQueryStep;
static int PlQueryDataNum;

static inline void ReadTimeToBuffer(dbtime_t filetime, unsigned short readtime, int flag, unsigned char *buf)
{
	buf[0] = readtime%60;
	readtime /= 60;
	buf[3] = filetime.s.month;
	buf[4] = filetime.s.year;

	if(flag) { //month
		buf[1] = readtime%24;
		buf[2] = (readtime/24)+1;
	}
	else { //day
		buf[1] = readtime;
		buf[2] = filetime.s.day;
	}

	HexToBcd(buf, 5);
}

/**
* @brief 查询日冻结历史数据
* @param sendbuffer 返回数据缓存链表
* @param mids 测量点组
* @return 成功0, 否则失败
*/
static int QueryDbaseDay(unsigned short mids, qrycurve_buffer_t **psendbuffer)
{
	unsigned short midstart, mid = 0, midmask;
	plmdb_day_t *pday = (plmdb_day_t *)PlMdbQryCache;
	int datalen, i;
	qrycurve_buffer_t *pbuffer = *psendbuffer;
	qrycurve_buffer_t *pbak;
	unsigned char *puc, *pmids;
	dbtime_t dbtime, dbtimecur;
	unsigned char duid[4];

	printf("QueryDbaseDay1...........................\n");
	
	duid[0] = (unsigned char)mids&0xff;
	duid[1] = (unsigned char)(mids>>8);
	duid[2] = 0x01;  //F161
	duid[3] = 0x14;

	//if((mids&0xff00) < 0x0200 || (mids&0x00ff) == 0) {
	//	ErrorLog("invalid mids(%04XH)\n", mids);
	//	return 1;
	//}

	
	midstart = ((mids>>8)-2)<<3;
	//midstart = ((mids>>8)-2);
	//midstart = 0;//add
	printf("midstart = %x\n",midstart);
	//printf("QueryDbaseDayQueryDbaseDay....................QueryDbaseDay\n");
	printf("mids = %x midstart = %x\n",mids,midstart);
	//printf("QueryDbaseDayQueryDbaseDay....................QueryDbaseDay\n");
	
	dbtime.u = PlQueryDbTime.u;
	dbtime.s.tick = 0;
	{
		sysclock_t clock;
		SysClockReadCurrent(&clock);
		SYSCLOCK_DBTIME(&clock, dbtimecur);
		dbtimecur.s.tick = 0;
	}
	printf("QueryDbaseDay2...\n");
#if 0
/*
 * 由于日冻结数据都是在0:00分冻结的，因此冻结的是前一天
 * 的数据，所以查询日期推后一天
*/
	if(0 == PlQueryTask) DbTimeAddOneDay(&dbtime);
#endif
	if(dbtime.u >= dbtimecur.u) return 1;

mark_retry:
	if(CacheQryDbid != PLMDB_DAY || CacheDate.u != dbtime.u) //requery
	{ 
		while(ReadPlMdbDay(pday, dbtime) < 0) 
		{
			if(0 == PlQueryTask) return 1;

			DbTimeAddOneDay(&dbtime);
			if(dbtime.u >= dbtimecur.u) return 1;
		}
		if(ReadPlMdbDay(pday, dbtime) < 0) 	return 1;
		CacheDate.u = dbtime.u;
		CacheQryDbid = PLMDB_DAY;
	}
	printf("QueryDbaseDay3...\n");
	QRYCBUF_SCAN(pbuffer, 18, *psendbuffer, PlQueryStep);
	if(NULL == pbuffer) return 1;
	printf("QueryDbaseDay4...\n");
	puc = QRYCBUF_DATA(pbuffer);
	puc[0] = 0;
	puc[1] = (unsigned char)(mids>>8);
	puc[2] = 0x01;  //F161
	puc[3] = 0x14;
	pmids = puc;
	puc += 4;
	datalen = 4;
	printf("QueryDbaseDay5...\n");
	printf("mid = %x\n",mid);
	mid = midstart;
	for(midmask=1; midmask<0x0100; midmask<<=1,mid++) {
		printf("QueryDbaseDay6...\n");
		if((midmask&mids) == 0) continue;
		printf("QueryDbaseDay7...\n");
		//if(PLTIME_EMTPY == pday[mid].readtime) continue;
		//printf("pday[%d].meter_ene[0] = %X\n",mid + 8,pday[mid + 8].meter_ene[0]);
		//if(0xEE == pday[mid + 8].meter_ene[0]) continue;
		printf("mid = %d\n",mid);
		printf("QueryDbaseDay8...\n");
		pbak = pbuffer;
		QRYCBUF_SCAN(pbuffer, datalen+14, *psendbuffer, PlQueryStep);
		if(NULL == pbuffer) return 1;
		printf("QueryDbaseDay9...\n");
		if(pbak != pbuffer && datalen) {
			memcpy(QRYCBUF_DATA(pbuffer), QRYCBUF_DATA(pbak), datalen);
			pmids = QRYCBUF_DATA(pbuffer);
			puc = pmids + datalen;
		}

		if(0 == PlQueryTask) {
			puc[0] = PlQueryDbTime.s.day;
			puc[1] = PlQueryDbTime.s.month;
			puc[2] = PlQueryDbTime.s.year;
		}
		else {
			puc[0] = dbtime.s.day;
			puc[1] = dbtime.s.month;
			puc[2] = dbtime.s.year;
		}
		HexToBcd(puc, 3);
		puc += 3;
		mid += 6;		//add
		ReadTimeToBuffer(dbtime, pday[mid].readtime, 0, puc);
		if(pday[mid].meter_ene[0] == 0xEE)
		{
			memset(puc,0xEE,5);
		}
		puc += 5;
		*puc++ = 0;//费率数
		if(pday[mid].meter_ene[0] == 0xEE)
		{
			*puc++ = 0xEE;//电量最高字节

		}
		else
		{
			*puc++ = 0;//电量最高字节
		}
		printf("mid = %d\n",mid);
		for(i=0; i<4; i++) 
		{
			*puc++ = pday[mid].meter_ene[i];
			printf("meter_ene[%d] = %x\n",i,pday[mid].meter_ene[i]);
		}
		datalen += 14;
		*pmids |= midmask;
	}

	if(datalen > 4) QRYCBUF_ADD(pbuffer, datalen);

	if(PlQueryTask) {
		DbTimeAddOneDay(&dbtime);
		SetSndTime(duid, dbtime.u, PlQueryStep);
		if(dbtime.u >= dbtimecur.u) return 0;
		goto mark_retry;
	}

	return 0;	
}

/**
* @brief 查询月冻结历史数据
* @param sendbuffer 返回数据缓存链表
* @param mids 测量点组
* @return 成功0, 否则失败
*/
static int QueryDbaseMonth(unsigned short mids, qrycurve_buffer_t **psendbuffer)
{
	unsigned short midstart, mid, midmask;
	plmdb_mon_t *pmon = (plmdb_mon_t *)PlMdbQryCache;
	int datalen, i, j, itemlen, fenum;
	qrycurve_buffer_t *pbuffer = *psendbuffer;
	qrycurve_buffer_t *pbak;
	unsigned char *puc, *pmids;
	dbtime_t dbtime, dbtimecur;
	unsigned char duid[4];

	duid[0] = (unsigned char)mids&0xff;
	duid[1] = (unsigned char)(mids>>8);
	duid[2] = 0x01;  //F177
	duid[3] = 0x16;

	if((mids&0xff00) < 0x0200 || (mids&0x00ff) == 0) {
		ErrorLog("invalid mids(%04XH)\n", mids);
		return 1;
	}
	midstart = ((mids>>8)-2)<<3;

	dbtime.u = PlQueryDbTime.u;
	dbtime.s.day = 1;
	dbtime.s.tick = 0;
	{
		sysclock_t clock;
		SysClockReadCurrent(&clock);
		SYSCLOCK_DBTIME(&clock, dbtimecur);
		//dbtimecur.s.day = 1;
		dbtimecur.s.tick = 0;
	}

#if 0
/*
 * 由于月冻结数据都是在0:00分冻结的，因此冻结的是前一月
 * 的数据，所以查询日期推后一月
 */
 	if(0 == PlQueryTask) DbTimeAddOneMonth(&dbtime);
#endif
	if(dbtime.u >= dbtimecur.u) return 1;

mark_retry:
	if(CacheQryDbid != PLMDB_MONTH || CacheDate.u != dbtime.u) {
		while(ReadPlMdbMonth(pmon, dbtime) < 0) {
			if(0 == PlQueryTask) return 1;

			DbTimeAddOneMonth(&dbtime);
			if(dbtime.u >= dbtimecur.u) return 1;
		}

		CacheDate.u = dbtime.u;
		CacheQryDbid = PLMDB_MONTH;
	}

	QRYCBUF_SCAN(pbuffer, 17, *psendbuffer, PlQueryStep);
	if(NULL == pbuffer) return 1;

	puc = QRYCBUF_DATA(pbuffer);
	puc[0] = 0;
	puc[1] = (unsigned char)(mids>>8);
	puc[2] = 0x01;  //F177
	puc[3] = 0x16;
	pmids = puc;
	puc += 4;
	datalen = 4;

	mid = midstart;
	for(midmask=1; midmask<0x0100; midmask<<=1,mid++) {
		if((midmask&mids) == 0) continue;
		if(PLTIME_EMTPY == pmon[mid].readtime) continue;

		fenum = (int)pmon[mid].fenum&0xff;
		if(fenum > MAX_PLMET_FENUM) {
			ErrorLog("invalid fenum(%d)\n", fenum);
			fenum = 0;
		}
		itemlen = (fenum+1)*5 + 8;

		pbak = pbuffer;
		QRYCBUF_SCAN(pbuffer, datalen+itemlen, *psendbuffer, PlQueryStep);
		if(NULL == pbuffer) return 1;
		if(pbak != pbuffer && datalen) {
			memcpy(QRYCBUF_DATA(pbuffer), QRYCBUF_DATA(pbak), datalen);
			pmids = QRYCBUF_DATA(pbuffer);
			puc = pmids + datalen;
		}

		if(0 == PlQueryTask) {
			puc[0] = PlQueryDbTime.s.month;
			puc[1] = PlQueryDbTime.s.year;
		}
		else {
			puc[0] = dbtime.s.month;
			puc[1] = dbtime.s.year;
		}
		HexToBcd(puc, 2);
		puc += 2;

		ReadTimeToBuffer(dbtime, pmon[mid].readtime, 1, puc);
		puc += 5;
		*puc++ = fenum;

		for(i=0; i<(fenum+1); i++) {
			*puc++ = 0;
			for(j=0; j<4; j++) *puc++ = pmon[mid].ene[i*4+j];
		}
		datalen += itemlen;
		*pmids |= midmask;
	}

	if(datalen > 4) QRYCBUF_ADD(pbuffer, datalen);

	if(PlQueryTask) {
		DbTimeAddOneMonth(&dbtime);
		SetSndTime(duid, dbtime.u, PlQueryStep);
		if(dbtime.u >= dbtimecur.u) return 0;
		goto mark_retry;
	}

	return 0;	
}

#if 1

static int MakeImpList(unsigned char *kmids, unsigned short *pmids)
{
	int kmidnum;
	unsigned short pnmask, pns, mids;
	unsigned char impnum, impi;
	unsigned short midstart, midsok;

	mids = *pmids;
	midstart = (mids>>8)<<3;
	midstart += 1;
	pns = mids & 0x00ff;
	midsok = mids&0xff00;
	kmidnum = 0;
	impnum = ParaMix.impuser.num;
	if(impnum == 0) return 0;
	if(impnum > MAX_IMPORTANT_USER) impnum = MAX_IMPORTANT_USER;

	for(pnmask=1; pnmask<0x0100; pnmask<<=1,midstart++) {
		if((mids&pnmask) == 0) continue;

		for(impi=0; impi<impnum; impi++) {
			if(ParaMix.impuser.metid[impi] == midstart) {
				kmids[kmidnum] = impi;
				kmidnum++;
				midsok |= pnmask;
				break;
			}
		}
	}

	*pmids = midsok;
	return kmidnum;
}

static int IsEmptyImpEne(const unsigned char *kmids, int kmnum, unsigned char offset)
{
	int i;
	unsigned int ui;
	plmdb_imp_t *pimp = (plmdb_imp_t *)PlMdbQryCache;

	for(i=0; i<kmnum; i++) {
		ui = MAKE_LONG(&pimp[i].ene[offset<<2]);
		if(ui != 0xeeeeeeee) return 0;
	}

	return 1;
}

/**
* @brief 查询重点用户表历史数据
* @param sendbuffer 返回数据缓存链表
* @param mids 测量点组
* @return 成功0, 否则失败
*/
static int QueryDbaseImp(unsigned short mids, qrycurve_buffer_t **psendbuffer)
{
	plmdb_imp_t *pimp = (plmdb_imp_t *)PlMdbQryCache;
	int datalen, i;
	qrycurve_buffer_t *pbuffer = *psendbuffer;
	dbtime_t dbtime, dbtimecur;
	unsigned char duid[4];
	unsigned char kmid[MAX_IMPORTANT_USER];
	unsigned char *puc;
	int kmidnum, requery;
	unsigned char starti, endi, maxi;

	duid[0] = (unsigned char)mids&0xff;
	duid[1] = (unsigned char)(mids>>8);
	duid[2] = 0x10;  //F101
	duid[3] = 0x0c;	

	kmidnum = MakeImpList(kmid, &mids);
	if(kmidnum <= 0 || 0 == (mids&0x00ff)) return 1;

	dbtime.u = PlQueryDbTime.u;
	{
		sysclock_t clock;
		SysClockReadCurrent(&clock);
		SYSCLOCK_DBTIME(&clock, dbtimecur);
	}

mark_retry:
	requery = 0;
	if(CacheQryDbid != PLMDB_IMP) requery = 1;
	else if(!DBTIME_ISONEDAY(CacheDate, dbtime)) requery = 1;
	if(requery) { //requery
		while(ReadPlMdbImp(pimp, dbtime) < 0) {
			if(0 == PlQueryTask) return 1;

			DbTimeAddOneDay(&dbtime);
			dbtime.s.tick = 0;
			if(dbtime.u >= dbtimecur.u) return 1;
		}

		CacheDate.u = dbtime.u;
		CacheQryDbid = PLMDB_DAY;
	}

	starti = dbtime.s.tick >> 2;
	maxi = 24;
	if(DBTIME_ISONEDAY(dbtime, dbtimecur)) {
		maxi = (dbtimecur.s.tick>>2) + 1;
	}

mark_recopy:
	//skip the head empty buffer
	for(; starti<maxi; starti++) {
		if(!IsEmptyImpEne(kmid, kmidnum, starti)) break;
	}
	if(starti >= maxi) {
		if(0 == PlQueryTask) return 1;

		DbTimeAddOneDay(&dbtime);
		dbtime.s.tick = 0;
		if(dbtime.u >= dbtimecur.u) return 1;
		goto mark_retry;
	}

	i = 0;
	endi = starti+1;
	for(; endi<maxi; endi++) {
		if(IsEmptyImpEne(kmid, kmidnum, endi)) {
			i++;
			if(i >= 4) break;  //太多空数据
		}
		else i = 0;
	}
	if(i) {
		if(endi < maxi) endi -= (i-1);
		else endi -= i;
	}

	i = endi - starti;
	if(i > PlQueryDataNum) {
		endi = starti + PlQueryDataNum;
		i = endi -starti;
	}

	datalen = 4+kmidnum*(7+i*4);
	while(QRYCBUF_MAXLEN(pbuffer) < datalen) {
		datalen = 4+kmidnum*11;
		for(i=1; i<(endi-starti); i++) {
			if(QRYCBUF_MAXLEN(pbuffer) < datalen) break;
			datalen += kmidnum*4;
		}

		if(i <= 1) {
			QRYCBUF_SCAN(pbuffer, datalen, *psendbuffer, PlQueryStep);
			if(NULL == pbuffer) return 1;
		}
		else {
			endi = starti + i;
			datalen = 4+kmidnum*(7+i*4);
			break;
		}
	}
	PlQueryDataNum -= i;

	puc = QRYCBUF_DATA(pbuffer);
	QRYCBUF_ADD(pbuffer, datalen);
	puc[0] = (unsigned char)mids&0xff;
	puc[1] = (unsigned char)(mids>>8);
	puc[2] = 0x10;  //F101
	puc[3] = 0x0c;	
	puc += 4;

	datalen = (endi-starti)*4;
	for(i=0; i<kmidnum; i++) {
		puc[4] = dbtime.s.year;
		puc[3] = dbtime.s.month;
		puc[2] = dbtime.s.day;
		puc[1] = starti;
		puc[0] = 0;
		HexToBcd(puc, 5);
		puc[5] = 3;
		puc[6] = endi-starti;
		puc += 7;

		smallcpy(puc, pimp[kmid[i]].ene+starti*4, datalen);
		puc += datalen;
	}

	if(PlQueryTask) {
		dbtime_t dbtimesnd;

		dbtimesnd.u = dbtime.u;
		if(endi >= 24) {
			dbtimesnd.s.tick = 0;
			DbTimeAddOneDay(&dbtimesnd);
		}
		else {
			dbtimesnd.s.tick = endi*4;
		}

		SetSndTime(duid, dbtimesnd.u, PlQueryStep);
	}

	if(PlQueryTask || PlQueryDataNum > 0) {
		if(PlQueryTask) PlQueryDataNum = 255;
		if(endi >= 24) {
			if(0 == PlQueryTask) return 0;

			DbTimeAddOneDay(&dbtime);
			dbtime.s.tick = 0;
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

#endif

/**
* @brief 主动发送载波表历史数据
* @param recvbuf 接收缓存区指针
* @param recvlen 接收缓存区长度
* @param sendbuf 返回数据缓存链表
* @param multir 曲线抽取倍率
* @return 成功0, 否则失败
*/
int ActiveSendPlCurve(const unsigned char *recvbuf, int recvlen, qrycurve_buffer_t *sendbuffer, unsigned char multir)
{
	typedef int (*qry_fn)(unsigned short mids, qrycurve_buffer_t **psendbuffer);

	unsigned short fn, mids;
	int rtn;
	unsigned char pngrp, pns;
	qry_fn pfunc;

	printf("ActiveSendPlCurve...\n");
	LockPlMdb();

	PlQueryTask = 1;
	PlQueryStep = 0;
	ClearSndTime();

	while(recvlen >= 4) {
		pns = recvbuf[0];
		pngrp = recvbuf[1];
		fn = MAKE_SHORT(recvbuf+2);
		PlQueryDbTime.u = GetSndTime(recvbuf);

		recvlen -= 4;
		recvbuf += 4;
		//if(pns == 0) continue;
		//if(pngrp < 2) goto mark_fail;

		switch(fn) {
		case 0x1401: //F161
			PlQueryDbTime.s.tick = 0;
			pfunc = QueryDbaseDay;
			break;
		case 0x1601: //F177
			PlQueryDbTime.s.day = 1;
			PlQueryDbTime.s.tick = 0;
			pfunc = QueryDbaseMonth;
			break;
		case 0x0c10: //F101
			pfunc = QueryDbaseImp;
			PlQueryDataNum = 255;
			break;
		default: goto mark_fail;
		}

		mids = ((unsigned short)pngrp<<8) + (unsigned short)pns;

		rtn = (*pfunc)(mids, &sendbuffer);
		if(rtn && NULL == sendbuffer) goto mark_fail;
	}

	UnlockPlMdb();
	return 0;

mark_fail:
	UnlockPlMdb();
	return 1;
}


/**
* @brief 查询载波表历史数据
* @param recvbuf 接收缓存区指针
* @param recvlen 接收缓存区长度
* @param sendbuf 返回数据缓存链表
* @return 成功0, 否则失败
*/
int QueryPlCurve(const unsigned char *recvbuf, int recvlen, qrycurve_buffer_t *sendbuffer)
{
	typedef int (*qry_fn)(unsigned short mids, qrycurve_buffer_t **psendbuffer);

	unsigned short fn, mids;
	int qrylen, num, rtn;
	unsigned char pngrp, pns, mask;
	qry_fn pfunc;
	int i = 0,pn = 0;

	printf("QueryPlCurve...........");
	LockPlMdb();//锁住系统资源

	PlQueryTask = 0;
	PlQueryStep = 0;

	while(recvlen >= 6) //最小为月冻结(2)+数据项(4)
	{
		pns = recvbuf[0];//pn		测量点号
		pngrp = recvbuf[1];
		fn = MAKE_SHORT(recvbuf+2);//fn
		printf("pns = %x\n",pns);
		printf("pngrp = %x\n",pngrp);

		recvlen -= 4;
		recvbuf += 4;
		//if(pns == 0) continue;
		//if(pngrp < 2) goto mark_fail;

		switch(fn) //查询哪一类数据
		{
		case 0x1401: //F161
		case 0x0001: //F1
			qrylen = 3;//日冻结数据类时标长度
			PlQueryDbTime.s.day = recvbuf[0];//查询冻结数据时间
			PlQueryDbTime.s.month = recvbuf[1];
			PlQueryDbTime.s.year = recvbuf[2];
			BcdToHex(&PlQueryDbTime.s.day, 3);
			PlQueryDbTime.s.tick = 0;
			pfunc = QueryDbaseDay;
			break;
		case 0x1601: //F177
			qrylen = 2;//月冻结数据类时标长度
			PlQueryDbTime.s.day = 1;
			PlQueryDbTime.s.month = recvbuf[0];
			PlQueryDbTime.s.year = recvbuf[1];
			BcdToHex(&PlQueryDbTime.s.month, 2);
			PlQueryDbTime.s.tick = 0;
			pfunc = QueryDbaseMonth;
			break;
		case 0x0c10: //F101
			qrylen = 7;//曲线数据类时标长度
			{
				sysclock_t clock;

				clock.minute = recvbuf[0];
				clock.hour = recvbuf[1];
				clock.day = recvbuf[2];
				clock.month = recvbuf[3];
				clock.year = recvbuf[4];
				BcdToHex(&clock.year, 5);
				SYSCLOCK_DBTIME(&clock, PlQueryDbTime);
			}
			PlQueryDataNum = recvbuf[6];
			pfunc = QueryDbaseImp;
			break;
		default: goto mark_fail;
		}

		for(num=0,mask=1; mask != 0; mask <<= 1) {
			if(mask & pns) num++;
		}
		qrylen *= num;

		if(qrylen > recvlen) goto mark_fail;
		recvlen -= qrylen;
		recvbuf += qrylen;
		//得到要查询的测量点号

		//printf("recvbuf[0] = %x\n",recvbuf[0]);
		//printf("recvbuf[1] = %x\n",recvbuf[1]);
		for(i=0;i<8;i++)
		{
			if(pns>>i)	continue;
			else break;
		}
		pn = (pngrp -1) * 8 + i;

		printf("pn = %d\n",pn);
		//pn += 6;
		printf("pn = %d\n",pn);



		//sprintf(str, "%02X %02X ", 1<<((m_iPn-1)&0x07), (m_iPn-1)/8+1);
		//pngrp = (1<<((pn-1)&0x07));
		//pns = (pn-1)/8+1;

		pns = (1<<((pn-1)&0x07));
		pngrp = (pn-1)/8+1;

		
		mids = ((unsigned short)pngrp<<8) + (unsigned short)pns;

		printf("mids = %x\n",mids);
		rtn = (*pfunc)(mids, &sendbuffer);//查询一个载波表的冻结数据
		if(rtn && NULL == sendbuffer) goto mark_fail;
	}

	UnlockPlMdb();
	return 0;

mark_fail:
	UnlockPlMdb();
	return 1;
}

