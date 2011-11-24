/**
* mdb.c -- 表计数据读取
* 
* 
* 创建时间: 2010-5-15
* 最后修改时间: 2010-5-15
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/param/capconf.h"
#include "mdbconf.h"
#include "mdbstat.h"
#include "mdbcur.h"
#include "mdbstic.h"
#include "mdbuene.h"
#include "mdbfrez.h"
#include "mdbtgrp.h"

typedef int (*qrymdb_pf)(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len);

typedef struct {
	unsigned char grpid;
	qrymdb_pf pfunc;
} mdbfunc_t;
static const mdbfunc_t MdbFuncList[] = {//一类数据
	//集中器状态值
	{0, ReadMdbState},//组0 F1~F8
	{1, ReadMdbState},//F9~F11
	//总加组
	{2, ReadMdbTGrp},//F17~F22
	//电压电流功率
	{3, ReadMdbCurrent},//F25~F32
	//当月上月有无一四示值
	{4, ReadMdbCurrent},//F33~F40
	//当日当月电能量
	{5, ReadMdbUseEne},//F41~F48
	//当前电压电流相位角
	{6, ReadMdbCurrent},//F49
	//谐波
	{7, ReadMdbCurrent},//F57~F58
	//小时冻结
	{10, ReadMdbFrez},//F81~F83
	{11, ReadMdbFrez},//F89~F96
	{12, ReadMdbFrez},//FF97~F104
	{13, ReadMdbFrez},//F105~F112
	{14, ReadMdbFrez},//F113~F116
	//当前电能量示值
	{16, ReadMdbCurrent},//F129~F136
	//上月电能量示值
	{17, ReadMdbCurrent},//F137~F144
	//当月需量及发生时间
	{18, ReadMdbCurrent},//F145~F152
	//时区冻结正向有功电能示值
	{19, ReadMdbCurrent},//F153~F160
};

static qrymdb_pf FindMdbFunc(unsigned char grpid)
{
	int i;

	for(i=0; i<sizeof(MdbFuncList)/sizeof(mdbfunc_t); i++) {
		if(grpid == MdbFuncList[i].grpid) return MdbFuncList[i].pfunc;
	}

	return NULL;
}

/**
* @brief 读取1类数据
* @param pnfn 数据标识指针
* @param buf 输出缓存区指针
* @param len 输出缓存区长度
* @param pactlen 返回实际读取长度变量指针
* @return 成功0, 否则失败
*/
int ReadMdb(const unsigned char *pnfn, unsigned char *buf, int len, int *pactlen)
{
	unsigned short metpid, itemid;
	unsigned char pns, pnmask, fns, grpid;
	int actlen;
	qrymdb_pf pfunc;
	unsigned char *preadinfo = &buf[2];

	*pactlen = 0;

	pns = pnfn[0];
	if(0 == pnfn[1]) metpid = 0;
	else metpid = ((unsigned short)(pnfn[1]-1)<<3) + 1;
	fns = pnfn[2];
	grpid = pnfn[3];

	pfunc = FindMdbFunc(grpid);//根据fn得到查询的函数
	if(NULL == pfunc) return 1;

	buf[0] = pnfn[0];
	buf[1] = pnfn[1];
	buf[2] = 0;
	buf[3] = pnfn[3];
	actlen = 4;
	len -= 4;
	buf += 4;

	if(0 == metpid) pnmask = 0x80;
	else pnmask = 1;
	for(; pnmask!=0; pnmask<<=1,metpid++) {
		unsigned char fnmask;
		int rtn;

		if(metpid && (pns&pnmask) == 0) continue;

		for(fnmask=1; fnmask!=0; fnmask<<=1) {
			if((fns&fnmask) == 0) continue;

			itemid = ((unsigned short)grpid<<8) + (unsigned short)fnmask;
			PrintLog(0, "metpid = %d itemid = %d\n",metpid,itemid);
			rtn = (*pfunc)(metpid, itemid, buf, len);
			if(rtn == -2 || rtn == 0) continue;
			else if(rtn < 0) goto mark_end;

			len -= rtn;
			if(len < 0) goto mark_end;
			buf += rtn;
			actlen += rtn;
			*preadinfo |= fnmask;
		}
	}

mark_end:
	if(*preadinfo == 0) return 1;

	*pactlen = actlen;
	return 0;
}

extern int DbaseInit(void);
extern int MdbAnaInit(void);
extern int MdbCurrentInit(void);
extern int MdbSaveInit(void);
extern int SndTimeInit(void);

/**
* @brief MDB表计数据模块初始化
* @return 成功0, 否则失败
*/
int MdbInit(void)
{
	printf("  mdb init...\n");

	DbaseInit();
	MdbAnaInit();
	MdbCurrentInit();
	MdbSaveInit();
	SndTimeInit();

	return 0;
}

