/**
* flat.c -- 平滑储存
* 
* 
* 创建时间: 2010-5-10
* 最后修改时间: 2010-5-15
*/

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include "mtd-abi.h"

#include "include/environment.h"
#include "include/debug.h"
#include "include/lib/crc.h"
#include "include/sys/syslock.h"

#define FLAT_DEVNAME	"/dev/mtdflat"
#define CACHE_DIR		DATA_PATH

#define MAX_SECTOR		4
#define SECTOR_SIZE		0x20000

static int FlatFid = -1;

#define FLAT_MAGIC		0x37a5
typedef struct {
	unsigned short magic;
	unsigned short crc;
} rec_head_t;

typedef struct {
	unsigned int recsize;
	unsigned int offset;
} sector_conf_t;
static const sector_conf_t SectorConfig[MAX_SECTOR] = {
	{128, 0},
	{128, SECTOR_SIZE},
	{128, SECTOR_SIZE*2},
	{128, SECTOR_SIZE*3},
};
#define REC_DATASIZE(sector)	(SectorConfig[sector].recsize-sizeof(rec_head_t))
#define RECORD_SIZE(sector)		(SectorConfig[sector].recsize)
#define SECTOR_BASE(sector)		(SectorConfig[sector].offset)
#define SECTOR_END(sector)		(SectorConfig[sector].offset+SECTOR_SIZE)

static unsigned int OffsetWrite[MAX_SECTOR];

static unsigned char memcache[128];

static int LockIdFlat;
#define FLAT_LOCK		LockSysLock(LockIdFlat)
#define FLAT_UNLOCK		UnlockSysLock(LockIdFlat)

/**
* @brief 平滑储存初始化
* @return 成功0, 否则失败
*/
DECLARE_INIT_FUNC(FlatInit);
int FlatInit(void)
{
	int devfd;
	struct mtd_info_user mtd;

	//SysInitMutex(&FlatMutex);
	LockIdFlat = RegisterSysLock();

	devfd = open(FLAT_DEVNAME, O_SYNC|O_RDWR);
	if(devfd < 0) {
		system("mknod " FLAT_DEVNAME " c 90 8");
		devfd = open(FLAT_DEVNAME, O_SYNC|O_RDWR);
	}
	AssertLogReturn(devfd<0, 1, "can not open %s\n", FLAT_DEVNAME);

	if(ioctl(devfd, MEMGETINFO, &mtd) < 0) {
		ErrorLog("get mtd info failed\n");
		close(devfd);
		return 1;
	}

	printf("flat size=%dK:%08XH, erasesize=%dK\n", 
		mtd.size/1024, mtd.size, mtd.erasesize/1024);

	FlatFid = devfd;
	SET_INIT_FLAG(FlatInit);
	return 0;
}

/**
* @brief 检查记录的合法性
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 合法记录返回1, 非法记录返回0, 空记录返回-1
*/
static inline int ValidRecord(const unsigned char *buf, int len)
{
	unsigned short crc;
	rec_head_t *phead = (rec_head_t *)buf;
	int i;

	if(phead->magic != FLAT_MAGIC) {
		if(phead->magic == 0xffff && phead->crc == 0xffff) goto mark_empty;
		else return 0;
	}

	crc = CalculateCRC(buf+sizeof(rec_head_t), len-sizeof(rec_head_t));
	if(crc != phead->crc) return 0;

	return 1;

mark_empty:
	buf += sizeof(rec_head_t);
	len -= sizeof(rec_head_t);
	for(i=0; i<len; i++) {
		if(0xff != *buf++) return 0;
	}

	return -1;
}

/**
* @brief 遍历扇区
* @param 扇区号
* @return 成功返回最后一个有效数据的偏移值, 失败返回-1
*/
static int ScanSector(int sector)
{
	int i, recnum, recsize, rtn;
	int offset_valid, offset, offset_empty;

	offset_valid = -1;
	offset_empty = -1;
	offset = SECTOR_BASE(sector);
	recsize = RECORD_SIZE(sector);
	recnum = SECTOR_SIZE/recsize;

	lseek(FlatFid, offset, SEEK_SET);
	for(i=0; i<recnum; i++, offset+=recsize) {
		if(read(FlatFid, memcache, recsize) < recsize) break;

		rtn = ValidRecord(memcache, recsize);
		if(rtn > 0) {
			offset_empty = -1;
			offset_valid = offset;
		}
		else if(0 == rtn) offset_empty = -1;
		else {
			if(offset_empty < 0) offset_empty = offset;
		}
	}

	if(i < recnum || offset_empty < 0) {
		AssertLog(i<recnum, "not full scan(%d, %d)\n", i, recnum);
		OffsetWrite[sector] = SECTOR_END(sector);
	}
	else {
		OffsetWrite[sector] = offset_empty;
	}

	return offset_valid;
}

/**
* @brief 读取FLAT文件数据
* @param sector 文件扇区号
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度, 失败返回-1
*/
int ReadFlatFile(unsigned int sector, unsigned char *buf, int len)
{
	int offset;
	FILE *pf;
	char filename[32];
	struct erase_info_user erase;

	AssertLogReturn(FlatFid<0, -1, "invalid flat id\n");
	AssertLogReturn(sector >= MAX_SECTOR, -1, "invalid sector(%d)\n", sector);
	AssertLog(len<=0, "invalid len(%d)", len);	

	FLAT_LOCK;

	sprintf(filename, "%sflat%d.cache", CACHE_DIR, sector);
	pf = fopen(filename, "rb");
	if(NULL != pf) {
		if(fread(memcache, RECORD_SIZE(sector), 1, pf) == 1) {
			if(ValidRecord(memcache, RECORD_SIZE(sector)) > 0) {

				//DebugPrint(0, "read cache..\n");
				
				if(len > REC_DATASIZE(sector)) len = REC_DATASIZE(sector);
				memcpy(buf, memcache+sizeof(rec_head_t), len);
				fclose(pf);

				erase.start = SECTOR_BASE(sector);
				erase.length = SECTOR_SIZE;
				if(ioctl(FlatFid, MEMERASE, &erase) < 0) {
					ErrorLog("can not erase sector %d\n", sector);
					FLAT_UNLOCK;
					return len;
				}
				OffsetWrite[sector] = SECTOR_BASE(sector);

				lseek(FlatFid, OffsetWrite[sector], SEEK_SET);
				write(FlatFid, memcache, RECORD_SIZE(sector));
				OffsetWrite[sector] += RECORD_SIZE(sector);
				
				remove(filename);

				FLAT_UNLOCK;
				return len;
			}
		}
	}

	offset = ScanSector(sector);
	if(offset < 0) {
		FLAT_UNLOCK;
		return -1;
	}

	lseek(FlatFid, offset+sizeof(rec_head_t), SEEK_SET);

	if(len > REC_DATASIZE(sector)) len = REC_DATASIZE(sector);

	offset = read(FlatFid, buf, len);
	FLAT_UNLOCK;

	return offset;
}

/**
* @brief 写入FLAT文件数据
* @param sector 文件扇区号
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际写入长度, 失败返回-1
*/
int WriteFlatFile(unsigned int sector, const unsigned char *buf, int len)
{
	rec_head_t head;
	char filename[32];

	AssertLogReturn(FlatFid<0, -1, "invalid flat id\n");
	AssertLogReturn(sector >= MAX_SECTOR, -1, "invalid sector(%d)\n", sector);
	AssertLogReturn(len <= 0 || len > REC_DATASIZE(sector), -1, "invalid len(%d)", len);

	FLAT_LOCK;

	if(REC_DATASIZE(sector)-len) memset(memcache+len, 0, REC_DATASIZE(sector)-len);
	memcpy(memcache, buf, len);

	if(len < REC_DATASIZE(sector)) len = REC_DATASIZE(sector);
	filename[0] = 0;

	head.magic = FLAT_MAGIC;
	head.crc = CalculateCRC(memcache, len);

	if(OffsetWrite[sector] > (SECTOR_END(sector)-RECORD_SIZE(sector))) {  //need erase
		FILE *pf;
		struct erase_info_user erase;

		//DebugPrint(0, "need cache...\n");

		sprintf(filename, "%sflat%d.cache", CACHE_DIR, sector);
		//remove(filename);
		pf = fopen(filename, "wb");
		if(NULL == pf) {
			ErrorLog("can not open file %s\n", filename);
			FLAT_UNLOCK;
			return -1;
		}
		fwrite(&head, sizeof(head), 1, pf);
		fwrite(memcache, len, 1, pf);
		fclose(pf);

		erase.start = SECTOR_BASE(sector);
		erase.length = SECTOR_SIZE;

		if(ioctl(FlatFid, MEMERASE, &erase) < 0) {
			ErrorLog("can not erase sector %d\n", sector);
			FLAT_UNLOCK;
			return -1;
		}

		OffsetWrite[sector] = SECTOR_BASE(sector);
	}

	lseek(FlatFid, OffsetWrite[sector], SEEK_SET);
	write(FlatFid, &head, sizeof(head));
	write(FlatFid, memcache, len);
	OffsetWrite[sector] += RECORD_SIZE(sector);

	if(0 != filename[0]) remove(filename);

	FLAT_UNLOCK;
	return len;
}

/*void FlatTest(void)
{
	static unsigned char buf[128];
	int i, num, count;

	printf("start flat test...\n");

	i = ReadFlatFile(2, buf, 128);
	printf("read %d bytes, 0%02X %02X\n", i, buf[0], buf[1]);
	printf("cur write = %d:%d\n", OffsetWrite[2], (OffsetWrite[2]-SECTOR_BASE(2))/RECORD_SIZE(2));

	for(i=0; i<128; i++) buf[i] = i;

	printf("please input write count: ");
	scanf("%d", &count);

	for(num=0; num<count; num++) {
		buf[0] = num&0xff;
		buf[1] = (num>>8)&0xff;

		i = WriteFlatFile(2, buf, 124);
		if(i < 124) printf("write %d fail\n", num);
	}
	printf("write %d count\n", num);

	printf("cur write = %d:%d\n", OffsetWrite[2], (OffsetWrite[2]-SECTOR_BASE(2))/RECORD_SIZE(2));

	printf("end\n");
}*/
