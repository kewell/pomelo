/**
* dbase.c -- 历史数据库操作
* 
* 
* 创建时间: 2010-5-11
* 最后修改时间: 2010-5-11
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/sys/bin.h"
#include "include/sys/cycsave.h"
#include "include/sys/mutex.h"
#include "include/sys/syslock.h"
#include "include/lib/dbtime.h"
#include "include/param/capconf.h"
#include "dbconfig.h"
#include "dbase.h"

#define DBASE_MAGIC		0x78652d31

typedef struct {
	unsigned short itemsize;
	unsigned char frez;
	unsigned char unused;
} dbhead_t;

typedef struct {
	dbtime_t dbtime;
	//unsigned char frez;
	unsigned char saved;
	unsigned unuse[3];
} dbwrite_cache_t;
static dbwrite_cache_t *WriteCacheMet[MAX_CENMETP] = {NULL};

static unsigned char WriteCacheAll[2048];
#define READCACHE_SIZE		0x8000
static unsigned char ReadCache[READCACHE_SIZE];

static int LockIdDbase = -1;
//static sys_mutex_t DbReadMutex;

#define DB_WRLOCK		LockSysLock(LockIdDbase)
#define DB_WRUNLOCK		UnlockSysLock(LockIdDbase)

/**
* @brief 锁住数据库读取(读取数据库数据之前必须锁住)
*/
void DbaseReadLock(void)
{
	DB_WRLOCK;
	//SysLockMutex(&DbReadMutex);
}

/**
* @brief 解锁数据库读取(读取数据库数据之后必须解锁)
*/
void DbaseReadUnlock(void)
{
	DB_WRUNLOCK;
	//SysUnlockMutex(&DbReadMutex);
}

/**
* @brief 获得读取数据库的结果数据缓存
* @return 结果数据缓存区指针
*/
unsigned char *DbaseReadCache(void)
{
	return(ReadCache+sizeof(dbhead_t));
}

//static void DbaseFlushOne(unsigned short dbid, dbwrite_cache_t *pcache);

/**
* @brief 更新数据库写缓存并返回缓存区指针
* @param dbid 数据库ID
* @param dblen 数据库长度
* @param clock 当前时钟
* @param blast 最后一个标志(为0:00置1, 其余置0)
* @return 成功返回缓存区指针, 否则返回NULL
*/
static dbwrite_cache_t *UpdateWriteCache(unsigned short dbid, unsigned char frez, int dblen, dbtime_t dbtime)
{
	dbwrite_cache_t *pcache, **pbase;
	unsigned char *pdata;
	unsigned short mid;

	if(dbid >= DBID_MET_CURVE(0) && dbid < DBID_MET_CURVE(MAX_CENMETP)) {
		pbase = WriteCacheMet;
		mid = dbid-DBID_MET_CURVE(0);
		pcache = WriteCacheMet[mid];
	}
	else return (dbwrite_cache_t *)WriteCacheAll;

	if(NULL != pcache) {
		if(DBTIME_ISONEDAY(pcache->dbtime, dbtime)) return pcache;
		//if((pcache->dbtime.u & 0xffffff00) == (dbtime.u & 0xffffff00)) return pcache;
		//else if(blast) return pcache;  // 0:00分数据是最后一个...
		//else DbaseFlushOne(dbid, pcache);  //保存数据
	}
	else {
		pcache = malloc(dblen + sizeof(dbwrite_cache_t));
		AssertLogReturn(NULL==pcache, NULL, "malloc %d bytes fail\n", dblen + sizeof(dbwrite_cache_t));
		pbase[mid] = pcache;
	}

	pcache->dbtime.u = dbtime.u;
	//pcache->frez = frez;
	pcache->saved = 0;
	
	pdata = (unsigned char *)(pcache+1);
	pdata += sizeof(dbhead_t);
	//memset(pdata, 0xee, dblen);
	//DbaseReadLock();
	if(DbaseRead(dbid, frez, pcache->dbtime) > 0) memcpy(pdata, DbaseReadCache(), dblen);
	else memset(pdata, 0xee, dblen);
	//DbaseReadUnlock();

	return pcache;
}

/**
* @brief 保存历史数据库
* @param dbid 数据ID
* @param buf 缓存区指针
* @param frez 冻结密度
* @param clock 当前时钟
* @return 成功返回0, 否则失败
*/
int DbaseSave(unsigned short dbid, const unsigned char *buf, unsigned char frez, dbtime_t dbtime)
{
	dbhead_t head;
	int dblen, itemlen, itemnum, offset;
	char filename[64];
	dbwrite_cache_t *pcache;
	unsigned char *pdata;

	itemlen = DbaseItemSize(dbid);
	AssertLogReturn(itemlen<=0, 1, "invalid dbid(%d)\n", dbid);

	itemnum = DbGetItemNum(frez);
	AssertLogReturn(0==itemnum, 1, "invalid frez(%d)\n", frez);

	dblen = itemlen * itemnum + sizeof(head);
	offset = DbGetItemOffset(itemnum, dbtime);

	if(DbaseFileName(filename, dbid, dbtime)) {
		ErrorLog("make db file name fail\n");
		return 1;
	}
	DebugPrint(LOGTYPE_ALARM, "save db %s(id=%d,frez=%d)...\n", filename, dbid, frez);

	DB_WRLOCK;

	pcache = UpdateWriteCache(dbid, frez, dblen, dbtime);
	if(NULL == pcache) {
		ErrorLog("UpdateWriteCache fail\n");
		DB_WRUNLOCK;
		return 1;
	}

	head.itemsize = itemlen;
	head.frez = frez;
	head.unused = 0;

	pdata = (unsigned char *)(pcache+1);
	smallcpy(pdata, &head, sizeof(head));
	memcpy(pdata+sizeof(head)+itemlen*offset, buf, itemlen);

#if 0
	if(frez == DBFREZ_DAY || frez == DBFREZ_MONTH) {
		SaveBinFile(filename, DBASE_MAGIC, pdata, dblen);
		DB_WRUNLOCK;
		return 0;
	}

	pcache->saved = 0;
#else
	SaveBinFile(filename, DBASE_MAGIC, pdata, dblen);
#endif

	DB_WRUNLOCK;
	return 0;
}

#if 0
static void DbaseFlushOne(unsigned short dbid, dbwrite_cache_t *pcache)
{
	int dblen;
	unsigned char *pdata;
	dbhead_t *phead;
	char filename[64];

	pdata = (unsigned char *)(pcache+1);
	phead = (dbhead_t *)pdata;

	dblen = phead->itemsize;
	dblen *= DbGetItemNum(phead->frez);
	if(dblen <= 0) {
		ErrorLog("invalid dblen(%d, %d*%d)\n", dblen, phead->itemsize, phead->frez);
		return;
	}
	dblen += sizeof(dbhead_t);

	if(DbaseFileName(filename, dbid, pcache->dbtime)) {
		ErrorLog("make db file name fail(%d)\n",dbid);
		return;
	}

	DebugPrint(LOGTYPE_ALARM, "flush db %s(id=%d,frez=%d)...\n", filename, dbid, phead->frez);

	SaveBinFile(filename, DBASE_MAGIC, pdata, dblen);
	pcache->saved = 1;
}

/**
* @brief 将在缓存里的数据存入文件中
*/
void DbaseFlush(void)
{
	int mid;
	unsigned short dbid = DBID_MET_CURVE(0);

	DB_WRLOCK;

	for(mid=0; mid<MAX_CENMETP; mid++,dbid++) {
		if(WriteCacheMet[mid] != NULL && WriteCacheMet[mid]->saved == 0) {
			DbaseFlushOne(dbid, WriteCacheMet[mid]);
		}
	}

	DB_WRUNLOCK;
	return;
}
DECLARE_CYCLE_SAVE(DbaseFlush, 0);
#endif

/**
* @brief 读取历史数据库数据
* @param dbid 数据ID
* @param frez 冻结密度
* @param clock 查询时钟
* @return 成功返回实际读取长度, 失败返回-1
*/
int DbaseRead(unsigned short dbid, unsigned char frez, dbtime_t dbtime)
{
	dbhead_t *phead;
	int dblen, itemlen, itemnum, filelen, fileitemnum, i;
	char filename[64];
	unsigned char *pdata, *pdst;

	itemlen = DbaseItemSize(dbid);
	if(itemlen <= 0) return -1;

	itemnum = DbGetItemNum(frez);
	if(0 == itemnum) return -1;

	dblen = itemlen * itemnum;

	if(DbaseFileName(filename, dbid, dbtime)) return -1;

	filelen = ReadBinFileCache(filename, DBASE_MAGIC, ReadCache, READCACHE_SIZE);
	if(filelen <= sizeof(dbhead_t)) goto mark_fail;

	phead = (dbhead_t *)ReadCache;
	filelen -= sizeof(dbhead_t);
	if(phead->itemsize != itemlen) goto mark_fail;;

	fileitemnum = DbGetItemNum(phead->frez);
	if(fileitemnum == 0) goto mark_fail;
	if(fileitemnum*itemlen != filelen) goto mark_fail;

	pdata = (unsigned char *)(phead+1);

	/*@change later: need test*/
	if(fileitemnum < itemnum) {
		itemnum = itemlen*(itemnum/fileitemnum);
		pdst = pdata + dblen;
		pdata += filelen;
		for(i=1; i<fileitemnum; i++) {
			pdst -= itemnum;
			pdata -= itemlen;
			memset(pdst, 0xee, itemnum);
			memcpy(pdst, pdata, itemlen);
			
		}
	}
	else if(fileitemnum > itemnum) {
		fileitemnum = itemlen*(fileitemnum/itemnum);
		pdst = pdata;
		for(i=1; i<itemnum; i++) {
			pdst += itemlen;
			pdata += fileitemnum;
			memcpy(pdst, pdata, itemlen);
		}
	}

	return dblen;

mark_fail:
	return -1;
}

void DbaseDelete(unsigned short dbid)
{
	char cmd[48];
	char filename[32];

	if(DbaseFileGroupName(filename, dbid)) return;

	strcpy(cmd, "rm -f ");
	strcat(cmd, filename);
	system(cmd);

	//SysRemoveFiles(filename);
}

void DbaseDeleteAll(void)
{
	system("rm -f " DBASE_SAVEPATH "*.db");
}

void DbaseFormat(void)
{
	system("rm -f " DBASE_SAVEPATH "*");
}

extern int DbClearInit(void);

DECLARE_INIT_FUNC(DbaseInit);
int DbaseInit(void)
{
	unsigned short mid;

	DbClearInit();

	LockIdDbase = RegisterSysLock();
	//SysInitMutex(&DbReadMutex);

	for(mid=0; mid<MAX_CENMETP; mid++) WriteCacheMet[mid] = NULL;

	DebugPrint(0, "db met day(%d) item size = %d\n", DBID_MET_DAY(0), DbaseItemSize(DBID_MET_DAY(0)));
	DebugPrint(0, "db met rmd(%d) item size = %d\n", DBID_MET_CPYDAY(0), DbaseItemSize(DBID_MET_CPYDAY(0)));
	DebugPrint(0, "db met mon(%d) item size = %d\n", DBID_MET_MONTH(0), DbaseItemSize(DBID_MET_MONTH(0)));
	DebugPrint(0, "db met sticday(%d) item size = %d\n", DBID_METSTIC_DAY(0), DbaseItemSize(DBID_METSTIC_DAY(0)));
	DebugPrint(0, "db met sticmon(%d) item size = %d\n", DBID_METSTIC_MONTH(0), DbaseItemSize(DBID_METSTIC_MONTH(0)));
	DebugPrint(0, "db term sticday(%d) item size = %d\n", DBID_TERMSTIC_DAY, DbaseItemSize(DBID_TERMSTIC_DAY));
	DebugPrint(0, "db term sticmon(%d) item size = %d\n", DBID_TERMSTIC_MONTH, DbaseItemSize(DBID_TERMSTIC_MONTH));
	DebugPrint(0, "db met curve(%d) item size = %d\n", DBID_MET_CURVE(0), DbaseItemSize(DBID_MET_CURVE(0)));

	SET_INIT_FLAG(DbaseInit);
	return 0;
}

