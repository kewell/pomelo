/**
* route.h -- 路由表参数
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-5-20
*/

#include <stdio.h>
#include <string.h>

#define DEFINE_PARAROUTE

#include "param_config.h"
#include "include/debug.h"
#include "include/param/meter.h"
#include "include/sys/grpbin.h"
#include "include/param/operation.h"
#include "operation_inner.h"
#include "include/param/route.h"

#define SAVE_ROUTE_SIZE		(MAX_ROUTE_LEVEL*6+4)
struct save_route_t {
	unsigned short index;
	unsigned char level;
	unsigned char unuse;
	unsigned char addr[MAX_ROUTE_LEVEL*6];
};

cfg_route_met_t ParaRoute[MAX_METER];

#define ROUTE_PARAM_MAGIC	0x84cc0901

/**
* @brief 从文件中载入路由参数
* @return 0成功, 否则失败
*/
int LoadParaRoute(void)
{
	#define MEMCACHE_SIZE	(sizeof(cfg_route_t)+36)

	unsigned char memcache[MEMCACHE_SIZE];
	grpbin_ref_t ref;
	int i, readlen, retry=0;
	struct save_route_t *pdoc = (struct save_route_t *)memcache;

	DebugPrint(0, "  load meter route...(maxsize=%d)", 
			(sizeof(struct save_route_t)+2)*MAX_METER*MAX_ROUTE_ONEMET+12);

	if(OpenGrpBinFile(PARAM_SAVE_PATH "route.gin", ROUTE_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no main, ");
		goto mark_fail;
	}

mark_retry:

	if(ref.itemlen > MEMCACHE_SIZE) {
		ErrorLog("invalid itemlen(%d)\n", ref.itemlen);
		CloseGrpBinFile(&ref);
		goto mark_fail;
		
	}

	if(ref.itemnum > (MAX_METER*MAX_ROUTE_ONEMET)) {
		ErrorLog("too long itemnum(%d)\n", ref.itemnum);
		ref.itemnum = (MAX_METER*MAX_ROUTE_ONEMET);
	}

	for(i=0; i<ref.itemnum; i++) {
		readlen = ReadGrpBinFileItem(&ref, memcache, MEMCACHE_SIZE);
		if(readlen < 2) break;

		if(pdoc->index >= MAX_METER) continue;

		if(ParaRoute[pdoc->index].num >= MAX_ROUTE_ONEMET) {
			ErrorLog("too more route in met %d\n", pdoc->index);
			continue;
		}

		if(readlen > SAVE_ROUTE_SIZE) readlen = SAVE_ROUTE_SIZE;
		readlen -= 2;
		memcpy(&(ParaRoute[pdoc->index].route[ParaRoute[pdoc->index].num]), &pdoc->level, readlen);
		ParaRoute[pdoc->index].num++;
	}

	DebugPrint(0, "ok\n");

	CloseGrpBinFile(&ref);
	return 0;

mark_fail:
	if(retry) {
		DebugPrint(0, "fail\n");
		return 1;
	}
	retry = 1;

	if(OpenGrpBinFile(PARAM_BAK_PATH "route.gin", ROUTE_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no bak\n");
		return 1;
	}

	goto mark_retry;
}

/**
* @brief 保存路由参数
* @return 0成功, 否则失败
*/
int SaveParaRoute(void)
{
	grpbin_ref_t ref;
	int i, j, retry = 0;
	struct save_route_t save;


	ref.itemlen = SAVE_ROUTE_SIZE;
	ref.itemnum = MAX_METER*MAX_ROUTE_ONEMET;
	if(OpenGrpBinFile(PARAM_SAVE_PATH "route.gin", ROUTE_PARAM_MAGIC, 'w', &ref)) return 1;

mark_retry:
	for(i=MAX_CENMETP; i<MAX_METER; i++) {
		if(ParaRoute[i].num == 0) continue;
		if(ParaRoute[i].num > MAX_ROUTE_ONEMET) {
			ErrorLog("met %d invalid route num(%d)\n", i, ParaRoute[i].num);
			ParaRoute[i].num = MAX_ROUTE_ONEMET;
		}

		save.index = i;
		for(j=0; j<ParaRoute[i].num; j++) {
			if(ParaRoute[i].route[j].level == 0) continue;
			memcpy(&save.level, &(ParaRoute[i].route[j]), sizeof(cfg_route_t));
			WriteGrpBinFileItem(&ref, (unsigned char *)&save);
		}
	}

	CloseGrpBinFile(&ref);

	if(retry) return 0;
	retry = 1;
	ref.itemlen = SAVE_ROUTE_SIZE;
	ref.itemnum = MAX_METER*MAX_ROUTE_ONEMET;
	if(OpenGrpBinFile(PARAM_BAK_PATH "route.gin", ROUTE_PARAM_MAGIC, 'w', &ref)) return 1;
	goto mark_retry;
	
	return 0;
}

/**
* @brief 终端参数F161操作
* @param flag 操作方式, 0-读, 1-写
* @param metpid 测量点号
* @param buf 缓存区指针
* @param len 缓存区长度
* @param actlen 有效数据长度(由函数返回)
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int ParaOperationF161(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned short mid = metpid - 1;
	int alen, level;
	unsigned char i;

	if(0 == flag) {
		if(mid < MAX_CENMETP) {
			*buf = 0;
			*actlen = 1;
		}
		else {
			alen = 1;
			if(ParaRoute[mid].num > MAX_ROUTE_ONEMET) {
				ErrorLog("met %d invalid route num(%d)\n", mid, ParaRoute[mid].num);
				ParaRoute[mid].num = MAX_ROUTE_ONEMET;
			}
			*buf++ = ParaRoute[mid].num;
			for(i=0; i<ParaRoute[mid].num; i++) {
				level = (int)ParaRoute[mid].route[i].level&0xff;
				if(level > MAX_ROUTE_LEVEL) {
					ErrorLog("met %d invalid route level(%d)\n", mid, level);
					ParaRoute[mid].route[i].level = MAX_ROUTE_LEVEL;
				}
				level *= 6;
				alen += level + 1;
				if(alen > len) return POERR_FATAL;
				*buf++ = ParaRoute[mid].route[i].level;
				if(level) memcpy(buf, ParaRoute[mid].route[i].addr, level);
				buf += level;
			}

			*actlen = alen;
		}
	}
	else {
		unsigned char num;

		num = *buf++;
		if(num > MAX_ROUTE_ONEMET) return POERR_FATAL;

		ParaRoute[mid].num = num;
		alen = 1;
		for(i=0; i<num; i++) {
			level = (int)(*buf++)&0xff;
			if(level > MAX_ROUTE_LEVEL) return POERR_FATAL;
			ParaRoute[mid].route[i].level = level;
			level *= 6;
			alen += level + 1;
			if(alen > len) return POERR_FATAL;

			if(level) memcpy(ParaRoute[mid].route[i].addr, buf, level);
			buf += level;
		}

		*actlen = alen;
	}

	return 0;
}

