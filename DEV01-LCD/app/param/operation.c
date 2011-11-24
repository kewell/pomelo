/**
* operation.c -- 参数操作
* 
* 
* 创建时间: 2010-5-7
* 最后修改时间: 2010-5-8
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "param_config.h"
#include "include/debug.h"
#include "include/sys/syslock.h"
#include "include/param/capconf.h"
#include "include/param/operation.h"
#include "include/monitor/runstate.h"
#include "operation_inner.h"

typedef int (*operation_fn)(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
typedef int (*specialop_fn)(int flag, unsigned short metpid, param_specialop_t *option);

#define PRATTR_TERM		0   //终端参数
#define PRATTR_METP		1    //测量点参数
#define PRATTR_TGRP		2    //总加组参数
#define PRATTR_TASK1	3    //1类任务参数
#define PRATTR_TASK2	4    //2类任务参数

typedef struct {
	unsigned char special;//1-需结合源报文内容
	unsigned char attr;
	unsigned char datalen;
	unsigned int saveflag;
	operation_fn pfunc;
} paraitem_t;

typedef struct {
	unsigned bend;
	unsigned char grpid;
	unsigned char num;
	const paraitem_t *pitemlist;
} paragrp_t;

/*static int ParaOperationInvalid(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	return POERR_INVALID;
}*/

/*static int ParaOperationFatal(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	return POERR_FATAL;
}*/

static int ParaOperationZero(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) {
		memset(buf, 0, *actlen);
	}

	return 0;
}

static int ParaOperationZero2(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) {
		buf[0] = 0;
		*actlen = 1;
	}
	else {
		int alen;

		alen = (int)buf[0]&0xff;
		alen *= *actlen;
		*actlen = alen + 1;
		if(*actlen > len) return POERR_FATAL;
	}

	return 0;
}

static const paraitem_t ItemListF1[] = {//顺序排列
	/*F1 */ {0, PRATTR_TERM, 6, SAVEFLAG_TERM, ParaOperationF1},
	/*F2 */ {0, PRATTR_TERM, 1, SAVEFLAG_TERM, ParaOperationF2},
	/*F3 */ {0, PRATTR_TERM, 28, SAVEFLAG_TERM, ParaOperationF3},
	/*F4 */ {0, PRATTR_TERM, 16, SAVEFLAG_TERM, ParaOperationF4},
	/*F5 */ {0, PRATTR_TERM, 3, SAVEFLAG_TERM, ParaOperationF5},
	/*F6 */ {0, PRATTR_TERM, 16, SAVEFLAG_TERM, ParaOperationF6},
	/*F7 */ {0, PRATTR_TERM, 24, SAVEFLAG_TERM, ParaOperationF7},
	/*F8 */ {0, PRATTR_TERM, 8, SAVEFLAG_TERM, ParaOperationF8},
};

static const paraitem_t ItemListF9[] = {
	/*F9  */ {0, PRATTR_TERM, 16, SAVEFLAG_TERM, ParaOperationF9},
	/*F10 */ {1, PRATTR_TERM, 2, SAVEFLAG_METER, (operation_fn)ParaOperationF10},
	/*F11 */ {1, PRATTR_TERM, 1, SAVEFLAG_HARDW, (operation_fn)ParaOperationF11},
	/*F12 */ {0, PRATTR_TERM, 2, SAVEFLAG_HARDW, ParaOperationF12},
	/*F13 */ {1, PRATTR_TERM, 1, 0, (operation_fn)ParaOperationF13},
	/*F14 */ {1, PRATTR_TERM, 1, SAVEFLAG_HARDW, (operation_fn)ParaOperationF14},
	/*F15 */ {1, PRATTR_TERM, 1, SAVEFLAG_HARDW, (operation_fn)ParaOperationF15},
	/*F16 */ {0, PRATTR_TERM, 64, SAVEFLAG_TERM, ParaOperationF16},
};

static const paraitem_t ItemListF17[] = {
	/*F17 */ {0, PRATTR_TERM, 2, 0, ParaOperationZero},
	/*F18 */ {0, PRATTR_TERM, 12, 0, ParaOperationZero},
	/*F19 */ {0, PRATTR_TERM, 1, 0, ParaOperationZero},
	/*F20 */ {0, PRATTR_TERM, 1, 0, ParaOperationZero},
	/*F21 */ {0, PRATTR_TERM, 49, SAVEFLAG_MIX, ParaOperationF21},
	/*F22 */ {0, PRATTR_TERM, 1, 0, ParaOperationZero2},
	/*F23 */ {0, PRATTR_TERM, 3, 0, ParaOperationZero},
};

static const paraitem_t ItemListF25[] = {
	/*F25 */ {0, PRATTR_METP, 11, SAVEFLAG_CMETP, ParaOperationF25},
	/*F26 */ {0, PRATTR_METP, 54, SAVEFLAG_CMETP, ParaOperationF26},
	/*F27 */ {0, PRATTR_METP, 24, SAVEFLAG_CMETP, ParaOperationF27},
	/*F28 */ {0, PRATTR_METP, 4, SAVEFLAG_CMETP, ParaOperationF28},
	/*F29 */ {0, PRATTR_METP, 12, SAVEFLAG_METP, ParaOperationF29},
	/*F30 */ {0, PRATTR_METP, 1, SAVEFLAG_METP, ParaOperationF30},
	/*F31 */ {0, PRATTR_METP, 1, SAVEFLAG_CHIEND, ParaOperationF31},
};

static const paraitem_t ItemListF33[] = {
	/*F33 */ {1, PRATTR_TERM, 1, SAVEFLAG_MIX, (operation_fn)ParaOperationF33},
	/*F34 */ {1, PRATTR_TERM, 1, SAVEFLAG_MIX, (operation_fn)ParaOperationF34},
	/*F35 */ {0, PRATTR_TERM, 1, SAVEFLAG_MIX, ParaOperationF35},
	/*F36 */ {0, PRATTR_TERM, 4, SAVEFLAG_MIX, ParaOperationF36},
	/*F37 */ {0, PRATTR_TERM, 7, SAVEFLAG_MIX, ParaOperationF37},
	/*F38 */ {1, PRATTR_TERM, 2, SAVEFLAG_DATAUSE, (operation_fn)ParaOperationF38},
	/*F39 */ {1, PRATTR_TERM, 2, SAVEFLAG_DATAUSE, (operation_fn)ParaOperationF39},
};

static const paraitem_t ItemListF57[] = {
	/*F57 */ {0, PRATTR_TERM, 3, 0, ParaOperationZero},
	/*F58 */ {0, PRATTR_TERM, 1, 0, ParaOperationZero},
	/*F59 */ {0, PRATTR_TERM, 4, SAVEFLAG_MIX, ParaOperationF59},
	/*F60 */ {0, PRATTR_TERM, 80, SAVEFLAG_MIX, ParaOperationF60},
};

static const paraitem_t ItemListF65[] = {
	/*F65 */ {0, PRATTR_TASK1, 9, SAVEFLAG_TASK, ParaOperationF65},
	/*F66 */ {0, PRATTR_TASK2, 9, SAVEFLAG_TASK, ParaOperationF66},
	/*F67 */ {0, PRATTR_TASK1, 1, SAVEFLAG_TASK, ParaOperationF67},
	/*F68 */ {0, PRATTR_TASK2, 1, SAVEFLAG_TASK, ParaOperationF68},
};

static const paraitem_t ItemListF85[] = {
	/*F81 */ {0, PRATTR_TERM, 2, 0, ParaOperationZero},
	/*F82 */ {0, PRATTR_TERM, 12, 0, ParaOperationZero},
	/*F83 */ {0, PRATTR_TERM, 1, 0, ParaOperationZero},
	/*F84 */ {0, PRATTR_TERM, 1, 0, ParaOperationZero},
	/*F85*/ {0, PRATTR_TERM, 6, SAVEFLAG_UNI, ParaOperationF85},
	/*F85 */ {0, PRATTR_TERM, 1, 0, ParaOperationZero2},
	/*F86 */ {0, PRATTR_TERM, 3, 0, ParaOperationZero},
};

static const paraitem_t ItemListF161[] = {
	/*F161*/ {0, PRATTR_METP, 1, SAVEFLAG_ROUTE, ParaOperationF161},
};

static const paragrp_t GroupList[] = {
	{0, 0, sizeof(ItemListF1)/sizeof(paraitem_t), ItemListF1},
	{0, 1, sizeof(ItemListF9)/sizeof(paraitem_t), ItemListF9},
	{0, 2, sizeof(ItemListF17)/sizeof(paraitem_t), ItemListF17},
	{0, 3, sizeof(ItemListF25)/sizeof(paraitem_t), ItemListF25},
	{0, 4, sizeof(ItemListF33)/sizeof(paraitem_t), ItemListF33},
	{0, 7, sizeof(ItemListF57)/sizeof(paraitem_t), ItemListF57},
	{0, 8, sizeof(ItemListF65)/sizeof(paraitem_t), ItemListF65},
	{0, 10, sizeof(ItemListF85)/sizeof(paraitem_t), ItemListF85},
	{0, 20, sizeof(ItemListF161)/sizeof(paraitem_t), ItemListF161},

	{1, 0xff, 0, NULL},
};

/**
* @brief 写入参数
* @param buf 源接收报文缓冲区
* @param len 源接收报文数据长度
* @param pactlen 有效数据长度(由函数返回)
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int WriteParam(unsigned char *buf, int len, int *pactlen)
{
	const paragrp_t *pgrp = GroupList;
	const paraitem_t *pitem;
	unsigned short metpid;
	unsigned char pns, pnmask, fns, fnmask;
	int actlen, berror, idx, rtn, itemlen;

	printf("WriteParam...................1\n");

	AssertLogReturn(len<=4, POERR_INVALID, "invalid len(%d)\n", len);

	pns = buf[0];
	if(0 == buf[1]) metpid = 0;
	else metpid = ((unsigned short)(buf[1]-1)<<3) + 1;

	fns = buf[3];
	while(0 == pgrp->bend) {//查找组
		if(fns == pgrp->grpid) break;
		pgrp++;
	}
	if(pgrp->bend) return POERR_FATAL;
	printf("WriteParam...................2\n");
	fns = buf[2];

	buf += 4;
	actlen = 4;
	len -= 4;
	berror = 0;

	if(0 == metpid) pnmask = 0x80;//查询一次
	else pnmask = 1;//查询八次
	for(; pnmask!=0; pnmask<<=1,metpid++) {
		if(metpid && (pns&pnmask) == 0) continue;
		printf("WriteParam...................3\n");

		pitem = pgrp->pitemlist;
		for(idx=0,fnmask=1; idx<pgrp->num; idx++,fnmask<<=1,pitem++) {
			if(0 == (fnmask&fns)) continue;//顺序查找
			printf("WriteParam...................4\n");

			switch(pitem->attr) {
			case PRATTR_TERM:
				if(0 != metpid) return POERR_FATAL;
				break;
			case PRATTR_METP:
				if(0 == metpid || metpid > MAX_METP) return POERR_FATAL;
				break;
			case PRATTR_TGRP:
				if(0 == metpid || metpid > MAX_TGRP) return POERR_FATAL;
				break;
			case PRATTR_TASK1:
				if(0 == metpid || metpid > MAX_DTASK_CLS1) return POERR_FATAL;
				break;
			case PRATTR_TASK2:
				if(0 == metpid || metpid > MAX_DTASK_CLS2) return POERR_FATAL;
				break;
			default:
				ErrorLog("invalid attr(%d)\n", pitem->attr);
				return POERR_FATAL;
			}

			if(((int)pitem->datalen&0xff) > len) return POERR_FATAL;
			printf("WriteParam...................5\n");
			if(pitem->special) {
				specialop_fn pfunc = (specialop_fn)pitem->pfunc;
				param_specialop_t specialop;

				specialop.pn = metpid;
				specialop.rbuf = buf;
				specialop.rlen = len;
				specialop.ractlen = (int)pitem->datalen&0xff;
				specialop.wbuf = NULL;
				specialop.wlen = 0;
				specialop.wactlen = 0;

				rtn = (*pfunc)(1, metpid, &specialop);
				itemlen = specialop.ractlen;
				if(POERR_FATAL == rtn) return POERR_FATAL;
				else if(POERR_OK != rtn) berror = 1;
				else SetSaveParamFlag(pitem->saveflag);
			}
			else {
				itemlen = (int)pitem->datalen&0xff;
				rtn = (*(pitem->pfunc))(1, metpid, buf, len, &itemlen);
				if(POERR_FATAL == rtn) return POERR_FATAL;
				else if(POERR_OK != rtn) berror = 1;
				else SetSaveParamFlag(pitem->saveflag);
				printf("WriteParam...................6\n");
			}

			buf += itemlen;
			actlen += itemlen;
			len -= itemlen;
		}
	}

	*pactlen = actlen;
	if(berror) return POERR_INVALID;
	else return POERR_OK;
	printf("WriteParam...................7\n");
}

/**
* @brief 读取参数
* @param buf 输出缓存区指针
* @param len 发送缓存区最大长度
* @param pactlen 有效数据长度(由函数返回)
* @param readinfo 输入缓存区信息
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int ReadParam(unsigned char *buf, int len, int *pactlen, para_readinfo_t *readinfo)
{
	const paragrp_t *pgrp = GroupList;
	unsigned short metpid;
	unsigned char pns, pnmask, fns, *preadfns;
	int rbuflen, actlen;
	unsigned char *prbuf;

	*pactlen = 0;

	printf("ReadParam...................1\n");

	AssertLogReturn(len<4, POERR_INVALID, "invalid len(%d)\n", len);
	printf("ReadParam...................2\n");
	for(rbuflen=0; rbuflen<4; rbuflen++) buf[rbuflen] = readinfo->buf[rbuflen];
	pns = buf[0];
	if(0 == buf[1]) metpid = 0;
	else metpid = ((unsigned short)(buf[1]-1)<<3) + 1;

	fns = buf[3];
	while(0 == pgrp->bend) {
		if(fns == pgrp->grpid) break;//查找组
		pgrp++;
	}
	if(pgrp->bend) return POERR_INVALID;
	printf("ReadParam...................3\n");
	fns = buf[2];
	
	prbuf = readinfo->buf + 4;
	rbuflen = readinfo->len - 4;
	readinfo->actlen = 4;
	buf[2] = 0;
	preadfns = &buf[2];//信息类元为0
	buf += 4;
	len -= 4;
	actlen = 4;

	if(0 == metpid) pnmask = 0x80;//查询一次
	else pnmask = 1;//查询八次
	for(; pnmask!=0; pnmask<<=1,metpid++) {
		unsigned char fnmask;
		int idx, rtn, itemlen;
		const paraitem_t *pitem;

		if(metpid && (pns&pnmask) == 0) continue;

		pitem = pgrp->pitemlist;
		for(idx=0,fnmask=1; idx<pgrp->num; idx++,fnmask<<=1,pitem++) {
			if(0 == (fnmask&fns)) continue;//顺序查找
			printf("ReadParam...................34\n");
			switch(pitem->attr) {
			case PRATTR_TERM:
				if(0 != metpid) return POERR_FATAL;
				break;
			case PRATTR_METP:
				if(0 == metpid || metpid > MAX_METP) return POERR_FATAL;
				break;
			case PRATTR_TGRP:
				if(0 == metpid || metpid > MAX_TGRP) return POERR_FATAL;
				break;
			case PRATTR_TASK1:
				if(0 == metpid || metpid > MAX_DTASK_CLS1) return POERR_FATAL;
				break;
			case PRATTR_TASK2:
				if(0 == metpid || metpid > MAX_DTASK_CLS2) return POERR_FATAL;
				break;
			default:
				ErrorLog("invalid attr(%d)\n", pitem->attr);
				return POERR_FATAL;
			}
			printf("ReadParam...................4\n");
			if(((int)pitem->datalen&0xff) > len) return POERR_FATAL;
			printf("ReadParam...................5\n");
			if(pitem->special) {
				specialop_fn pfunc = (specialop_fn)pitem->pfunc;
				param_specialop_t specialop;

				specialop.pn = metpid;
				specialop.wbuf = buf;
				specialop.wlen = len;
				specialop.wactlen = (int)pitem->datalen&0xff;

				specialop.rbuf = prbuf;
				specialop.rlen = rbuflen;
				specialop.ractlen = 0;

				rtn = (*pfunc)(0, metpid, &specialop);
				itemlen = specialop.wactlen;
				if(POERR_FATAL == rtn) return POERR_FATAL;
				else if(POERR_OK == rtn) *preadfns |= fnmask;//置读取成功的信息类元有效
				else itemlen = 0;

				prbuf += specialop.ractlen;
				rbuflen -= specialop.ractlen;
				readinfo->actlen += specialop.ractlen;
				
			}
			else {
				itemlen = (int)pitem->datalen&0xff;
				rtn = (*(pitem->pfunc))(0, metpid, buf, len, &itemlen);
				printf("ReadParam...................6\n");
				if(POERR_FATAL == rtn) return POERR_FATAL;
				else if(POERR_OK == rtn) *preadfns |= fnmask;
				else itemlen = 0;
				printf("ReadParam...................7\n");
			}

			if(itemlen > 0) {
				buf += itemlen;
				len -= itemlen;
				actlen += itemlen;
			}
		}
	}

	if(0 == *preadfns) return POERR_INVALID;  //no data 信息类元为0
	*pactlen = actlen;
	printf("ReadParam...................8\n");
	return POERR_OK;
}

static int ParamSaveSysLock = -1;

/**
* @brief 初始化参数储存
* @return 0成功, 否则失败
*/
DECLARE_INIT_FUNC(ParamSaveInit);
int ParamSaveInit(void)
{
	printf("  param save init..\n");

	ParamSaveSysLock = RegisterSysLock();
	if(ParamSaveSysLock < 0) {
		ErrorLog("register syslock fail\n");
		return 1;
	}

	SET_INIT_FLAG(ParamSaveInit);

	return 0;
}

static unsigned int SaveFlag = 0;

/**
* @brief 清除参数储存标志
*/
void ClearSaveParamFlag(void)
{
	SaveFlag = 0;
}

/**
* @brief 设置参数储存标志
* @param flag 标志位
*/
void SetSaveParamFlag(unsigned int flag)
{
	SaveFlag |= flag;
}

#if 0
/**
* @brief for debug
*/
static int DummySave(void)
{
	return 0;
}
#endif

typedef int (*savefunc_t)(void);
/*
static const savefunc_t FunctionSave[FILEINDEX_MAX] = {
	SaveParaTerm, SaveParaMix, DummySave, SaveParaHardw,
	SaveParaMeter, SaveParaPlcMetp, SaveParaTask, SaveParaCenMetp, 
	SaveParaChildEnd, SaveParaDataUse, SaveParaRoute,SaveParaManuf
};
*/
static const savefunc_t FunctionSave[FILEINDEX_MAX] = {
	SaveParaTerm, SaveParaMix, SaveParaManuf, SaveParaHardw,
	SaveParaMeter, SaveParaPlcMetp, SaveParaTask, SaveParaCenMetp, 
	SaveParaChildEnd, SaveParaDataUse, SaveParaRoute
};

/**
* @brief 储存参数
*/
void SaveParam(void)
{
	int index;
	unsigned int mask;

	LockSysLock(ParamSaveSysLock);

	for(index=0,mask=1; index<FILEINDEX_MAX; index++,mask<<=1) 
	{
		if(mask&SaveFlag) (*FunctionSave[index])();
	}

	UnlockSysLock(ParamSaveSysLock);

	if(SaveFlag & SAVEFLAG_RUNSTATE) SaveRunState();

	SaveFlag = 0;
}

extern void ClearRunState(void);
extern void ClearMdbSave(void);
extern void DbaseFormat(void);

/**
* @brief 清除所有数据
*/
void ClearAllData(void)
{
	//ClearRunState();
	//ClearMdbSave();
	//DbaseFormat();
}

/**
* @brief 清除所有参数(除通信参外)
*/
void ClearAllParam(void)
{
	//system("rm -f " PARAM_SAVE_PATH "*.bin *.gin");//system为系统shell命令
	//system("rm -f " PARAM_BAK_PATH "*.bin *.gin");
}

