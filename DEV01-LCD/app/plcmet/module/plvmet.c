/**
* plvmet.c -- 虚拟载波表
* 
* 
* 创建时间: 2010-7-2
* 最后修改时间: 2010-7-2
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/timeal.h"
#include "include/lib/bcd.h"
#include "plcmet/plcomm.h"
#include "plcmet/plc_stat.h"
#include "plvmet.h"

#ifdef USE_PLVMET

/**
* @brief 读数据
* @param dest 目的地址
* @param itemid 数据项标识
* @param buf 数据帧缓存区指针
* @param len 缓存区长度
* @return 成功返回实际数据长度, 失败返回-1
*/
int PlVmetRead(const plc_dest_t *dest, unsigned short itemid, unsigned char *buf, int len)
{
	unsigned short metid;
	unsigned int ene;

	AssertLog(dest->metid > MAX_METER, "invalid metid(%d)\n", dest->metid);

	Sleep(20);

	ene = dest->dest[1] & 0xff;
	ene <<= 8;
	ene += dest->dest[0] & 0xff;
	if(ene > 9999) ene -= 9999;

	UnsignedToBcd(ene, buf, 2);
	buf[2] = 0x05;
	buf[3] = 0;

	if(!dest->metid) return 4;

	metid = dest->metid - 1;
	PlcState[metid].quality = 0;

	SysClockReadCurrent(&PlcState[metid].oktime);
	PlcState[metid].okflag = 1;

	return 4;
}

/**
* @brief 写数据
* @param dest 目的地址
* @param pconfig 写配置
* @return 成功返回0, 否则失败
*/
int PlVmetWrite(const plc_dest_t *dest, const plwrite_config_t *pconfig)
{
	return 0;
}

/**
* @brief 广播较时
* @param flag 0-发送较时命令, 1-查询较时方式
* @return flag=0时 成功返回0, 失败返回负数, flag=1时返回较时方式(PLCHKTIME_POLL)
*/
int PlVmetCheckTime(const plc_dest_t *dest, int flag)
{
	if(flag) return PLCHKTIME_POLL;

	return 0;
}


#endif /*USE_PLVMET*/

