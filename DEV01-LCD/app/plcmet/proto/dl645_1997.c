/**
* dl645_1997.c -- DL/T645-1997协议分析
* 
* 
* 创建时间: 2010-5-21
* 最后修改时间: 2010-5-21
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "plcmet/plcomm.h"

/**
* @brief 生成读数据命令帧
* @param addr 目的地址
* @param itemid 数据项标识
* @param buf 数据帧缓存区指针
* @param len 缓存区长度
* @return 成功返回数据帧长度, 失败返回-1
*/
int Dl1997MakeReadPkt(const unsigned char *addr, unsigned short itemid, unsigned char *buf, int len)
{
	unsigned char check, *puc;
	int i;

	AssertLogReturn(len < 14, -1, "too short buffer len(%d)\n", len);

	puc = buf;
	*puc++ = 0x68;
	for(i=0; i<6; i++) *puc++ = addr[i];
	*puc++ = 0x68;
	*puc++ = 0x01;
	*puc++ = 0x02;
	*puc++ = (unsigned char)(itemid) + 0x33;
	*puc++ = (unsigned char)(itemid>>8) + 0x33;
	puc = buf;
	check = 0;
	for(i=0; i<12; i++) check += *puc++;
	*puc++ = check;
	*puc = 0x16;

	return 14;
}

/**
* @brief 生成读数据命令帧
* @param addr 目的地址
* @param pconfig 写配置指针
* @param buf 数据帧缓存区指针
* @param len 缓存区长度
* @return 成功返回数据帧长度, 失败返回-1
*/
int Dl1997MakeWritePkt(const unsigned char *addr, const plwrite_config_t *pconfig, unsigned char *buf, int len)
{
	unsigned char check, *puc;
	int i;

	AssertLog(pconfig->cmdlen < 0 || pconfig->cmdlen > 241, "invalid cmdlen(%d)\n", pconfig->cmdlen);
	AssertLogReturn((pconfig->cmdlen+14) > len, -1, "too short buffer len(%d)\n", len);

	puc = buf;
	*puc++ = 0x68;
	for(i=0; i<6; i++) *puc++ = addr[i];
	*puc++ = 0x68;
	*puc++ = 0x04;
	*puc++ = (unsigned char)pconfig->cmdlen + 2;
	*puc++ = (unsigned char)(pconfig->itemid) + 0x33;
	*puc++ = (unsigned char)(pconfig->itemid>>8) + 0x33;
	puc = buf;
	check = 0;
	for(i=0; i<12; i++) check += *puc++;
	for(i=0; i<pconfig->cmdlen; i++) {
		*puc = pconfig->command[i] + 0x33;
		check += *puc++;
	}
	*puc++ = check;
	*puc = 0x16;

	return (pconfig->cmdlen+14);
}

/**
* @brief 生成较时数据帧
* @param clock 当前时间
* @param buf 数据帧缓存区指针
* @param len 缓存区长度
* @return 成功返回数据帧长度, 失败返回-1
*/
int Dl1997MakeChkTimePkt(const sysclock_t *pclock, unsigned char *buf, int len)
{
	unsigned char check, *puc;
	int i;

	AssertLogReturn(len < 18, -1, "too short buffer len(%d)\n", len);

	puc = buf;
	*puc++ = 0x68;
	for(i=0; i<6; i++) *puc++ = 0x99;
	*puc++ = 0x68;
	*puc++ = 0x08;
	*puc++ = 0x06;
	*puc++ = (pclock->second/10 << 4) + (pclock->second%10) + 0x33;
	*puc++ = (pclock->minute/10 << 4) + (pclock->minute%10) + 0x33;
	*puc++ = (pclock->hour/10 << 4) + (pclock->hour%10) + 0x33;
	*puc++ = (pclock->day/10 << 4) + (pclock->day%10) + 0x33;
	*puc++ = (pclock->month/10 << 4) + (pclock->month%10) + 0x33;
	*puc++ = (pclock->year/10 << 4) + (pclock->year%10) + 0x33;
	puc = buf;
	check = 0;
	for(i=0; i<16; i++) check += *puc++;
	*puc++ = check;
	*puc = 0x16;

	return 18;
}

/**
* @brief 检查读取数据回应帧并解析数据
* @param addr 目的地址
* @param itemid 数据项标识
* @param buf 帧缓存区指针(解析后的数据也放入这个缓存)
* @param len 缓存区长度
* @return 成功返回解析数据长度, 失败返回负数
*/
int Dl1997CheckRead(const unsigned char *addr, unsigned short itemid, unsigned char *buf, int len)
{
	unsigned char check, *puc;
	int i, datalen;
	unsigned short us;

	if(len < 14) return -1;
	if(0x68 != buf[0] || 0x68 != buf[7]) return -2;
	for(i=0; i<6; i++) {
		if(addr[i] != buf[i+1]) return -3;
	}
	if((buf[8]&0xdf) != 0x81) return -4;
	us = ((unsigned short)(buf[11]-0x33)<<8) + (unsigned short)(buf[10]-0x33);
	if(us != itemid) return -5;
	datalen = (int)buf[9]&0xff;
	if(datalen < 2 || (datalen+12) > len) return -6;
	datalen -= 2;

	puc = buf;
	check = 0;
	for(i=0; i<(datalen+12); i++) check += *puc++;
	if(check != *puc++) return -7;
	if(0x16 != *puc) return -8;

	puc = buf + 12;
	for(i=0; i<datalen; i++) buf[i] = (*puc++) - 0x33;

	return datalen;
}

/**
* @brief 检查写数据回应帧
* @param addr 目的地址
* @param itemid 数据项标识
* @param buf 帧缓存区指针
* @param len 缓存区长度
* @return 成功返回0, 失败返回负数
*/
int Dl1997CheckWrite(const unsigned char *addr, unsigned short itemid, const unsigned char *buf, int len)
{
	unsigned char check;
	int i, datalen;
	const unsigned char *puc;

	if(len < 11) return -1;
	if(0x68 != buf[0] || 0x68 != buf[7]) return -2;
	for(i=0; i<6; i++) {
		if(addr[i] != buf[i+1]) return -3;
	}
	if((buf[8]&0xdf) != 0x84) return -4;
	datalen = (int)buf[9]&0xff;
	if((datalen+12) > len) return -6;

	puc = buf;
	check = 0;
	for(i=0; i<(datalen+10); i++) check += *puc++;
	if(check != *puc++) return -7;
	if(0x16 != *puc) return -8;

	return 0;
}

