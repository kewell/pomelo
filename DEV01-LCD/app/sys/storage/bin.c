/**
* bin.c -- 二进制文件储存
* 
* 
* 创建时间: 2010-5-6
* 最后修改时间: 2010-5-6
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/sys/bin.h"
#include "include/lib/crc.h"
#include "include/debug.h"

#define MAX_FILELEN		0x400000  // 4M Byte
#define MAX_MEMLEN		0x10000  // 64K Byte

typedef struct {
	unsigned short headcrc;
	unsigned short datacrc;
	unsigned long magic;
	int len;
} bin_filehead_t;

/**
* @brief 保存一个BIN文件
* @param file 文件名
* @param magic 文件标识字
* @param buffer 缓存区指针
* @param len 缓存区长度
* @return 成功返回0, 否则失败
*/
int SaveBinFile(const char *file, unsigned long magic, const unsigned char *buffer, int len)
{
	bin_filehead_t head;
	FILE *pf;
	unsigned char *headp;

	//printf("open %s\n", file);

	if((len <= 0) || (len > MAX_FILELEN)) {
		ErrorLog("invalid len(%d)\n", len);
		return 1;
	}
	AssertLogReturn(NULL==file, 1, "null file\n");

	head.magic = magic;
	head.len = len;
	head.datacrc = CalculateCRC(buffer, len);
	headp = (unsigned char *)&(head.datacrc);
	head.headcrc = CalculateCRC(headp, sizeof(head)-2);

	remove(file);
	pf = fopen(file, "wb");
	if(NULL == pf) {
		ErrorLog("can not open %s for write\n", file);
		return 1;
	}

	fwrite(&head, sizeof(head), 1, pf);
	fwrite(buffer, len, 1, pf);

	fclose(pf);

	return 0;
}

/**
* @brief 读取一个BIN文件
* @param file 文件名
* @param magic 文件标识字
* @param buffer 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度,失败返回-1
*/
int ReadBinFile(const char *file, unsigned long magic, unsigned char *buffer, int len)
{
	bin_filehead_t head;
	FILE *pf;
	unsigned short crc;
	unsigned char *memcache = NULL;
	int memlen, filelen;

	if((len <= 0) || (len > MAX_FILELEN)) {
		ErrorLog("invalid len(%d)\n", len);
		return -1;
	}
	AssertLogReturn(NULL==file, -1, "null file\n");

	pf = fopen(file, "rb");
	if(NULL == pf) return -1;

	if(fread(&head, sizeof(head), 1, pf) <= 0) {
		ErrorLog("%s file head too short\n", file);
		goto mark_fail;
	}

	if(head.magic != magic) {
		ErrorLog("%s magic invalid(0x%08X)\n", file, head.magic);
		goto mark_fail;
	}

	if(head.len <= 0 || head.len > MAX_FILELEN) {
		ErrorLog("%s len invalid(%d)\n", file, head.len);
		goto mark_fail;
	}

	crc = CalculateCRC((unsigned char *)&(head.datacrc), sizeof(head)-2);
	if(head.headcrc != crc) {
		ErrorLog("%s head crc erorr(0x%04X, should be 0x%04X)\n", file, head.headcrc, crc);
		goto mark_fail;
	}

	if(head.len > MAX_MEMLEN) memlen = MAX_MEMLEN;
	else memlen = head.len;

	memcache = malloc(memlen);
	if(NULL == memcache) {
		ErrorLog("malloc %d bytes fail\n", head.len);
		goto mark_fail;
	}

	crc = 0;
	filelen = head.len;
	while(filelen > 0) {
		if(fread(memcache, memlen, 1, pf) <= 0) {
			ErrorLog("%s len too long(%d)\n", file, head.len);
			goto mark_fail;
		}

		CalculateCRCStep(memcache, memlen, &crc);

		filelen -= memlen;
		if(filelen > 0 && filelen < memlen) memlen = filelen;
	}
	if(head.datacrc != crc) {
		ErrorLog("%s data crc erorr(0x%04X, should be 0x%04X)\n", file, head.datacrc, crc);
		goto mark_fail;
	}

	if(len > head.len) len = head.len;
	if(head.len > MAX_MEMLEN) {
		fseek(pf, sizeof(head), SEEK_SET);
		if(fread(buffer, len, 1, pf) <= 0) {
			ErrorLog("read file error\n");
			goto mark_fail;
		}
	}
	else {
		memcpy(buffer, memcache, len);
	}

	free(memcache);
	fclose(pf);
	return len;

mark_fail:
	if(NULL != memcache) free(memcache);
	fclose(pf);
	return -1;
}

/**
* @brief 读取一个BIN文件
*    与ReadBinFile不同的是,buffer内容在读取失败的情况下也有可能修改
*    因此需要应用程序分配专门的buffer
*    一般用来读取数据文件
*    操作比ReadBinFile少了内存分配操作, 但增加了数据不安全性, 使用时要小心
* @param file 文件名
* @param magic 文件标识字
* @param buffer Cache缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度,失败返回-1
*/
int ReadBinFileCache(const char *file, unsigned long magic, unsigned char *buffer, int len)
{
	bin_filehead_t head;
	FILE *pf;
	unsigned short crc;

	if((len <= 0) || (len > MAX_FILELEN)) {
		ErrorLog("invalid len(%d)\n", len);
		return -1;
	}
	AssertLogReturn(NULL==file, -1, "null file\n");

	pf = fopen(file, "rb");
	if(NULL == pf) return -1;

	if(fread(&head, sizeof(head), 1, pf) <= 0) {
		ErrorLog("%s file head too short\n", file);
		goto mark_fail;
	}

	if(head.magic != magic) {
		ErrorLog("%s magic invalid(0x%08X)\n", file, head.magic);
		goto mark_fail;
	}

	if(head.len <= 0 || head.len > len) {
		ErrorLog("%s len invalid(%d)\n", file, head.len);
		goto mark_fail;
	}

	crc = CalculateCRC((unsigned char *)&(head.datacrc), sizeof(head)-2);
	if(head.headcrc != crc) {
		ErrorLog("%s head crc erorr(0x%04X, should be 0x%04X)\n", file, head.headcrc, crc);
		goto mark_fail;
	}

	if(fread(buffer, head.len, 1, pf) <= 0) {
		ErrorLog("%s len too long(%d)\n", file, head.len);
		goto mark_fail;
	}

	crc = CalculateCRC(buffer, head.len);
	if(head.datacrc != crc) {
		ErrorLog("%s data crc erorr(0x%04X, should be 0x%04X)\n", file, head.datacrc, crc);
		goto mark_fail;
	}

	if(len > head.len) len = head.len;
	fclose(pf);
	return len;

mark_fail:
	fclose(pf);
	return -1;
}

