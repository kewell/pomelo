/**
* plccomm.c -- 载波通信接口
* 
* 
* 创建时间: 2010-5-21
* 最后修改时间: 2010-5-21
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFINE_PLCOMM

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/mutex.h"
#include "include/sys/uart.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/lib/crc.h"
#include "include/lib/align.h"
#include "include/param/unique.h"
#include "include/param/meter.h"
#include "include/param/route.h"
#include "include/param/commport.h"
#include "plcomm.h"
#include "plcmet/module/plstatic.h"
#include "plcmet/module/pldynamic.h"
#include "plcmet/module/plvmet.h"
#include "plcmet/module/rs485bus.h"
#include "include/debug/shellcmd.h"

static sys_mutex_t PlcMutex;
#define PLC_LOCK	SysLockMutex(&PlcMutex)
#define PLC_UNLOCK	SysUnlockMutex(&PlcMutex)

int PlcTimeChecking = 0;//正在广播校时(广播校时300秒内不能巡检)

static unsigned char PlCommBuffer[PLCOMM_BUF_LEN];

/**
* @brief 获得通信缓存区指针
*/
unsigned char *GetPlCommBuffer(void)
{
	return PlCommBuffer;
}

struct plc_module {
	unsigned char devno;
	int (*read)(const plc_dest_t *dest, unsigned short itemid, unsigned char *buf, int len);
	int (*write)(const plc_dest_t *dest, const plwrite_config_t *pconfig);
	int (*checktime)(const plc_dest_t *dest, int flag);
};
static const struct plc_module PlcModules[] = {
	{1, PlStaticRead, PlStaticWrite, PlStaticCheckTime},
#ifdef USE_PLVMET
	{0, PlVmetRead, PlVmetWrite, PlVmetCheckTime},
#endif
	{2, PlDynamicRead, PlDynamicWrite, PlDynamicCheckTime},
	{3, Rs485BusRead, NULL, NULL},
};
#define NUM_MODULES		(sizeof(PlcModules)/sizeof(PlcModules[0]))
static int PlcModuleNo = 2;

/**
* @brief 生成目标地址
* @param metid 目的表号
* @param dest 返回目标地址变量指针
*/
void MakePlcDest(unsigned short metid, plc_dest_t *dest)
{
	int i;

	dest->metid = metid+1;
	for(i=0; i<6; i++) dest->dest[i] = ParaMeter[metid].addr[i];
	dest->portcfg = ParaMeter[metid].portcfg;
	dest->src[0] = ParaUni.addr_sn[0];
	dest->src[1] = ParaUni.addr_sn[1];
	dest->src[2] = ParaUni.addr_area[0];
	dest->src[3] = ParaUni.addr_area[1];
	dest->src[4] = dest->src[5] = 0;
	dest->proto = ParaMeter[metid].proto;
	dest->route.phase = 0;
	if(0 == ParaRoute[metid].num) dest->route.level = 0;
	else 
	{
		int level;
		//@change later: 多路由选择
		level = (int)ParaRoute[metid].route[0].level&0xff;
		if(level > PLC_ROUTENUM) 
		{
			ErrorLog("met %d invalid route level(%d)\n", metid, level);
			level = PLC_ROUTENUM;
		}
		dest->route.level = level;
		if(level) memcpy(dest->route.addr, ParaRoute[metid].route[0].addr, level*6);
	}
}

/**
* @brief 读数据
* @param dest 目的地址
* @param itemid 数据项标识
* @param buf 数据帧缓存区指针
* @param len 缓存区长度
* @return 成功返回实际数据长度, 失败返回-1
*/
int PlcRead(const plc_dest_t *dest, unsigned short itemid, unsigned char *buf, int len)
{
	int rtn;

	if(PlcTimeChecking) return 1;

	PLC_LOCK;
	rtn = (*PlcModules[PlcModuleNo].read)(dest, itemid, buf, len);
	PLC_UNLOCK;
	return rtn;
}

/**
* @brief 写数据
* @param dest 目的地址
* @param pconfig 写配置
* @return 成功返回0, 否则失败
*/
int PlcWrite(const plc_dest_t *dest, const plwrite_config_t *pconfig)
{
	int rtn;

	if(PlcTimeChecking) return 1;

	PLC_LOCK;
	rtn = (*PlcModules[PlcModuleNo].write)(dest, pconfig);
	PLC_UNLOCK;

	return rtn;
}

/**
* @brief 广播较时定时器
*/
static int CTimerPlcCheckTime(unsigned long arg)
{
	PlcTimeChecking = 0;
	return 1;
}

#define CHKTIME_HASHSIZE	2048
#define CHKTIME_HASHMASK	(CHKTIME_HASHSIZE-1)
#define CHKTIME_HASHARRY	(CHKTIME_HASHSIZE+128)

struct chktime_hnode {
	union {
		unsigned int u;
		struct {
			unsigned char level;
			unsigned char zero;
			unsigned short crc;
		} s;
	} check;

	unsigned int final_route;
};

static inline unsigned int HashOffset(const struct chktime_hnode *pnode)
{
	const unsigned short *pus = (const unsigned short *)pnode;
	int i;
	unsigned short us;

	for(i=0,us=0; i<sizeof(struct chktime_hnode)/2; i++,pus++) {
		us ^= *pus;
	}

	us &= CHKTIME_HASHMASK;

	return us;
}

static int InsertHashTable(const struct chktime_hnode *pnode, struct chktime_hnode *plist)
{
	int offset;

	offset = HashOffset(pnode);

	plist += offset;
	for(; offset<CHKTIME_HASHARRY; offset++,plist++) {
		if(0 == plist->check.u) {
			plist->check.u = pnode->check.u;
			plist->final_route = pnode->final_route;
			return 0;
		}
		else if(plist->check.u == pnode->check.u && plist->final_route == pnode->final_route) return 1;
	}

	return 0;
}

static inline int MakeHashNode(const plc_dest_t *dest, struct chktime_hnode *pnode)
{
	unsigned char level;

	if(dest->route.level == 0) return 1;

	level = dest->route.level;
	if(level > PLC_ROUTENUM) level = PLC_ROUTENUM;

	pnode->check.s.level = level;
	pnode->check.s.zero = 0;
	pnode->check.s.crc = CalculateCRC(dest->route.addr, (unsigned int)dest->route.level*6);
	pnode->final_route = MAKE_LONG(dest->route.addr+(unsigned int)(dest->route.level-1)*6);

	return 0;
}

/**
* @brief 广播较时
* @return 成功返回0, 否则失败
*/
int PlcCheckTime(void)
{
	int rtn, i;
	plc_dest_t dest;
	struct chktime_hnode *hlist;
	struct chktime_hnode node;
	unsigned short metid;

	if(PlcTimeChecking) return 1;

	for(i=0; i<6; i++) dest.dest[i] = 0x99;
	dest.src[0] = ParaUni.addr_sn[0];
	dest.src[1] = ParaUni.addr_sn[1];
	dest.src[2] = ParaUni.addr_area[0];
	dest.src[3] = ParaUni.addr_area[1];
	dest.src[4] = dest.src[5] = 0;
	dest.metid = 0;
	dest.portcfg = 0;
	dest.proto = 1;
	dest.route.level = 0;

	rtn = (*PlcModules[PlcModuleNo].checktime)(&dest, 1);
	if(PLCHKTIME_BROCAST == rtn) {
		PLC_LOCK;
		rtn = (*PlcModules[PlcModuleNo].checktime)(&dest, 0);
		PLC_UNLOCK;
		if(rtn > 0) {
			if(SysAddCTimer(rtn, CTimerPlcCheckTime, 0) >= 0) PlcTimeChecking = 1;
			return 0;
		}
		else return 1;
	}

	hlist = (struct chktime_hnode *)malloc(sizeof(node)*CHKTIME_HASHARRY);
	if(NULL == hlist) {
		ErrorLog("malloc %d bytes fail\n", sizeof(node)*CHKTIME_HASHARRY);
		return 1;
	}
	memset(hlist, 0, sizeof(node)*CHKTIME_HASHARRY);

	PLC_LOCK;
	(*PlcModules[PlcModuleNo].checktime)(&dest, 0);
	PLC_UNLOCK;

	for(metid=MAX_CENMETP; metid<MAX_METER; metid++) {
		if(ParaMeter[metid].metp_id == 0 || ParaRoute[metid].num == 0) continue;

		dest.route.level = ParaRoute[metid].route[0].level;
		if(dest.route.level > PLC_ROUTENUM) dest.route.level = PLC_ROUTENUM;
		if(dest.route.level) memcpy(dest.route.addr, ParaRoute[metid].route[0].addr, (unsigned int)dest.route.level*6);

		if(MakeHashNode(&dest, &node)) continue;

		if(InsertHashTable(&node, hlist)) continue;//判断路由表是否相同

		PLC_LOCK;
		(*PlcModules[PlcModuleNo].checktime)(&dest, 0);
		PLC_UNLOCK;
	}

	free(hlist);
	return 0;
}

/**
* @brief 生成转发目标地址
* @param rbuf 命令缓存区指针
* @param rlen 命令缓存区长度
* @param dest 返回目标地址缓存区指针
* @return 成功返回命令缓存区地址长度, 失败返回-1
*/
static int MakeForwardDest(const unsigned char *rbuf, int rlen, plc_dest_t *dest)
{
	unsigned char routenum;
	unsigned int i;

	if(rlen < 8) return -1;

	routenum = rbuf[1];
	if(routenum > PLC_ROUTENUM) return -1;

	i = (unsigned int)routenum * 6;
	if((i+8) > rlen) return -1;
	rbuf += 2;

	dest->metid = 0;
	dest->portcfg = 0;
	dest->proto = 0;
	dest->src[0] = ParaUni.addr_sn[0];
	dest->src[1] = ParaUni.addr_sn[1];
	dest->src[2] = ParaUni.addr_area[0];
	dest->src[3] = ParaUni.addr_area[1];
	dest->src[4] = dest->src[5] = 0;

	dest->route.level = routenum;
	dest->route.phase = 0;
	if(routenum) smallcpy(dest->route.addr, rbuf, i);
	rbuf += i;

	smallcpy(dest->dest, rbuf, 6);

	return(i+8);
}

/**
* @brief 转发抄读数据命令
* @param rbuf 命令缓存区指针
* @param rlen 命令缓存区长度
* @param sbuf 返回数据缓存区指针
* @param smaxlen 返回数据缓存区最大长度
* @return 成功返回返回数据长度, 失败返回-1
*/
static int PlcForwardRead(const unsigned char *rbuf, int rlen, unsigned char *sbuf, int smaxlen)
{
	plc_dest_t dest;
	int i, applen;
	unsigned short itemid;
	unsigned char errcode;
	unsigned char tmpbuf[64];

	i = MakeForwardDest(rbuf, rlen, &dest);
	if(i < 0) return -1;

	rbuf += i;
	rlen -= i;
	if(rlen < 5 || smaxlen < 15) return -1;

	itemid = MAKE_SHORT(rbuf+1);

	if(0 != rbuf[0]) {
		errcode = 0;
		applen = 0;
		goto mark_return;
	}

	applen = PlcRead(&dest, itemid, tmpbuf, 64);
	if(applen <= 0) errcode = 1;
	else errcode = 5;

mark_return:
	sbuf[0] = sbuf[1] = 0;
	sbuf[2] = 1;  //F9
	sbuf[3] = 1;
	sbuf += 4;

	*sbuf++ = COMMPORT_PLC + 1;
	smallcpy(sbuf, dest.dest, 6); sbuf += 6;
	*sbuf++ = errcode;
	if(errcode != 5) {
		*sbuf++ = 2;
		DEPART_SHORT(itemid, sbuf);
		return 15;
	}

	if((applen + 15) > smaxlen) return -1;

	*sbuf++ = applen+2;
	DEPART_SHORT(itemid, sbuf); sbuf += 2;
	smallcpy(sbuf, tmpbuf, applen);

	return (applen+15);
}

/**
* @brief 转发载波透明转发命令
* @param rbuf 命令缓存区指针
* @param rlen 命令缓存区长度
* @param sbuf 返回数据缓存区指针
* @param smaxlen 返回数据缓存区最大长度
* @return 成功返回返回数据长度, 失败返回-1
*/
int PlcForward(const unsigned char *rbuf, int rlen, unsigned char *sbuf, int smaxlen)
{
	int rtn;
	unsigned char cmd;

	if(rlen < 4) return -1;

	if(0 != rbuf[0] || 0 != rbuf[1]) return -1;
	if(1 != rbuf[3]) return -1;
	cmd = rbuf[2];

	rbuf += 4;
	rlen -= 4;

	switch(cmd) {
	case 0x01:
		rtn = PlcForwardRead(rbuf, rlen, sbuf, smaxlen);
		break;

	default:
		rtn = -1;
		break;
	}

	return rtn;
}

/**
* @brief 载波通信初始化函数
* @return 返回0表示成功, 否则失败
*/
DECLARE_INIT_FUNC(PlcCommInit);
int PlcCommInit(void)
{
	printf("  plccomm init..\n");

	//SysInitMutex(&PlcMutex);
	//SysInitMutex(&Plc_Mutex);	

	if(UartOpen(PLC_UART_PORT)) return 1;

	UartSet(PLC_UART_PORT, 9600, 8, 1, 'E');
	//UartSet(PLC_UART_PORT, 9600, 8, 1, 'n');
	//UartSet(PLC_UART_PORT, 19200, 8, 1, 'n');

	SET_INIT_FLAG(PlcCommInit);

	return 0;
}

static int shell_readplc(int argc, char *argv[])
{
	plc_dest_t dest;
	unsigned char tmpbuf[64];

	dest.metid = 0;
	dest.portcfg = 0;
	dest.proto = 0;
	dest.route.level = 0;
	dest.route.phase = 0;

	dest.src[0] = ParaUni.addr_sn[0];
	dest.src[1] = ParaUni.addr_sn[1];
	dest.src[2] = ParaUni.addr_area[0];
	dest.src[3] = ParaUni.addr_area[1];
	dest.src[4] = dest.src[5] = 0;

	dest.dest[0] = 1;
	dest.dest[1] = 0;
	dest.dest[2] = 0;
	dest.dest[3] = 0;
	dest.dest[4] = 0;
	dest.dest[5] = 0;

	PlcRead(&dest, 0x9010, tmpbuf, 64);
	PrintLog(0, "发送载波读取报文成功\n");

	return 0;
}
DECLARE_SHELL_CMD("plcr", shell_readplc, "测试载波读取");

