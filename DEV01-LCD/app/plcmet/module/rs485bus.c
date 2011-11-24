/**
* rs485bus.c -- 485总线抄表通信接口
* 
* 作者: 
* 创建时间: 2010-11-30
* 最后修改时间: 
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/timeal.h"
#include "include/param/capconf.h"
#include "plcmet/plcomm.h"
#include "plcmet/plc_stat.h"
#include "pldynamic_dl.h"
#include "plcmet/proto/plc_proto.h"
#include "include/sys/rs485.h"
#include "plcmet/module/rs485bus.h"

static unsigned char Rs485BusBuffer[RS485BUS_BUF_LEN];
/**
* @brief 接收数据帧
* @param dest 目的地址
* @param buf 接收缓存区指针
* @param len 接收缓存区长度
* @param timeout 超时时间(100ms)
* @param 成功返回接收到的数据长度, 失败返回负数(参见错误码PLCERR_XXX)
*/
int Rs485BusRecvPkt(const plc_dest_t *dest, unsigned char *buf, int len, int timeout)
{
	unsigned char state, recvlen, maxlen, cnt;
	unsigned char *pbuf = Rs485BusBuffer;
	unsigned char port;
	int times;
	AssertLog(len <= 0, "invalid len(%d)\n", len);

	state = 0;
	recvlen = 0;
	maxlen = 0;
	port = 1;
	cnt = 0;
	for(times=0; times<timeout; times++) {
		while(Rs485Recv(port, pbuf, 1) > 0) {
			PrintLog(LOGTYPE_DOWNLINK, "recv: %02X, %d\n", *pbuf, state);
			switch(state) {
			case 0:
				if(0x68 == *pbuf) {
					pbuf++;
					recvlen = 1;
					maxlen = 6;
					cnt = 0;
					state = 1;	
				}
				break;
			case 1:
				pbuf++;	
				recvlen++;
				cnt ++;
				if(cnt >= maxlen){
					state = 2;
				}	
				break;
			case 2:
				if(0x68 != *pbuf){
					pbuf = Rs485BusBuffer;
					state = 0;
					break;
				}
				pbuf++;
				recvlen++;
				state  = 3;
				break;
			case 3:
				pbuf++;
				recvlen++;
				state = 4;
				break;
			case 4:
				recvlen++;
				cnt = 0;
				maxlen = *pbuf;
				if(maxlen>128){
					pbuf = Rs485BusBuffer;
					state = 0;
					break;
				}
				pbuf++;	
				maxlen += 2;
				state  = 5;
				break;
			case 5:
				recvlen++;
				cnt++;
				if(cnt >= maxlen) {
					if(0x16 == *pbuf){
					       goto mark_rcvend;
					}
					else{
						pbuf = Rs485BusBuffer;
						state = 0;
						break;
					}
				}
				pbuf++;
			}
		}

		Sleep(10);
	}

	PrintLog(LOGTYPE_DOWNLINK, "Rs485 recv timeout(%d), time=%d00ms:\n", recvlen, times);
	if(recvlen) PrintHexLog(LOGTYPE_DOWNLINK, Rs485BusBuffer, recvlen);

	return PLCERR_TIMEOUT;

mark_rcvend:

	PrintLog(LOGTYPE_DOWNLINK, "Rs485 recv(%d), time=%d00ms:\n", recvlen, times);
	PrintHexLog(LOGTYPE_DOWNLINK, Rs485BusBuffer, recvlen);
	
	smallcpy(buf, Rs485BusBuffer, recvlen);
	
	return recvlen;
}

/**
* @brief 读数据
* @param dest 目的地址
* @param itemid 数据项标识
* @param buf 数据帧缓存区指针
* @param len 缓存区长度
* @return 成功返回实际数据长度, 失败返回-1
*/
int Rs485BusRead(const plc_dest_t *dest, unsigned short itemid, unsigned char *buf, int len)
{
	unsigned char *cache = GetPlCommBuffer();
	const plcmet_prot_t *pfunc;
	int applen, timeout, i;
	unsigned char port, uc;
	
	pfunc = GetPlcMetProto(1);
	if(NULL == pfunc) 
	     return -1;
	
	AssertLog(dest->metid > MAX_METER, "invalid metid(%d)\n", dest->metid);

	applen = (*pfunc->makeread)(dest->dest, itemid, cache, RS485BUS_BUF_LEN);
	if(applen <= 0) 
	     return -1;
	port = 2;
	Rs485Lock(port);
	Rs485Set(port, 1200, 8, 1, 'E');
	while(Rs485Recv(port, &uc, 1) > 0);

	PrintLog(LOGTYPE_DOWNLINK, "PLC Send(%d):\n", applen);
	Rs485Send(port, cache, applen);

	timeout = 50;
	applen = Rs485BusRecvPkt(dest, cache, RS485BUS_BUF_LEN, timeout);
	Rs485Unlock(port);
	if(applen > 0) {
		applen = (*pfunc->checkread)(dest->dest, itemid, cache, applen);
		if(applen > 0) {
			for(i=0; i<len; i++) {
				if(i < applen) *buf++ = *cache++;
				else *buf++ = 0;
			}
		}
	}

		if(!dest->metid) {
		if(applen <= 0) {
			PrintLog(LOGTYPE_DOWNLINK, "proto check error(%d)\n", applen);
			return -1;
		}
		else return applen;
	}

	return applen;
}
