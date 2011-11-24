/**
* plstatic_dl.c -- 静态路由载波底层通信接口
* 
* 
* 创建时间: 2010-5-21
* 最后修改时间: 2010-5-21
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/uart.h"
#include "include/sys/timeal.h"
#include "plcmet/plcomm.h"
#include "plcmet/plc_stat.h"

static unsigned char PlStaticBuffer[256];

static plc_sinfo_t PlStaticInfo;

/**
* @brief 获取本次通信信息
* @return 信息变量指针
*/
const plc_sinfo_t *GetPlStaticInfo(void)
{
	return &PlStaticInfo;
}

/**
* @brief 发送载波通信帧
* @param dest 目的地址
* @param buf 发送缓存区指针
* @param len 缓存区长度
* @param proto 协议标识
* @return 成功返回0, 否则失败
*/
int PlStaticSendPkt(const plc_dest_t *dest, const unsigned char *buf, int len, unsigned char proto)
{
	unsigned char *pbuf = PlStaticBuffer;
	int slen, i;
	unsigned char check;

	AssertLog(len <= 0, "invalid len(%d)\n", len);
	AssertLog(dest->route.level>PLC_ROUTENUM, "invalid route level(%d)\n", dest->route.level);

	slen = len + 28;
	slen += dest->route.level*6;
	AssertLog(slen > 255, "too long slen(%d)\n", slen);
	

	*pbuf++ = 0x68;  //head
	*pbuf++ = slen;  //total len
	*pbuf++ = 0x41;  //ctrl

	*pbuf++ = ((dest->route.level&0x0f)<<4) + 0x05;  //info 0
	*pbuf++ = 0;  //info 1
	*pbuf++ = 0;  //info 2
	*pbuf++ = 0;  //info 3
	*pbuf++ = 0;  //info 4
	*pbuf++ = 0;  //info 5

	for(i=0; i<6; i++) *pbuf++ = dest->src[i];  //source addr
	i = (int)dest->route.level * 6;   //route addr
	memcpy(pbuf, dest->route.addr, i);
	pbuf += i;
	for(i=0; i<6; i++) *pbuf++ = dest->dest[i];  //dest addr

	*pbuf++ = 0x02;  //afn
	*pbuf++ = 0x01;  //F1
	*pbuf++ = 0x00;
	*pbuf++ = proto;  //proto
	*pbuf++ = len;  //user len

	slen -= (len+2);
	check = 0;
	for(i=0; i<len; i++) check += buf[i];
	pbuf = PlStaticBuffer+2;
	for(i=0; i<(slen-2); i++) check += *pbuf++;
	*pbuf++ = check;
	*pbuf++ = 0x16;

	PrintLog(LOGTYPE_DOWNLINK, "PLC Send(%d):\n", slen + len + 2);
	PrintHexLog(LOGTYPE_DOWNLINK, PlStaticBuffer, slen);
	PrintHexLog(LOGTYPE_DOWNLINK, buf, len);
	PrintHexLog(LOGTYPE_DOWNLINK, PlStaticBuffer+slen, 2);

	UartSend(PLC_UART_PORT, PlStaticBuffer, slen);//发送376.2
	UartSend(PLC_UART_PORT, buf, len);//发送645
	UartSend(PLC_UART_PORT, PlStaticBuffer+slen, 2);

	return 0;
}

/**
* @brief 检查收到数据帧的合法性
* @param dest 目的地址
* @param len 数据帧长度
* @return 正确返回应用数据长度字节偏移值, 错误返回负数
*/
static int CheckPacket(const plc_dest_t *dest, int len)
{
	unsigned char check, *pbuf;
	int i, rtn;

	if(PlStaticBuffer[len-1] != 0x16) 
		return -1;
	if(len < 28) 
		return -10;
	if((PlStaticBuffer[2]&0x80) == 0) 
		return -2;

	check = 0;
	pbuf = PlStaticBuffer + 2;
	for(i=0; i<(len-4); i++) 
		check += *pbuf++;

	if(*pbuf != check) 
		return -3;

	check = PlStaticBuffer[3]>>4;
	if(check > PLC_ROUTENUM) 
		return -4;

	i = (int)check*6;
	rtn = i+21;
	pbuf = PlStaticBuffer + rtn;
	if(*pbuf != 0x02) 
		return -5;
	pbuf += 4;
	i = (int)*pbuf&0xff;
	if((rtn+i+7) != len) 
		return -6;

	pbuf = PlStaticBuffer + 9;
	for(i=0; i<6; i++) 
	{
		if(*pbuf++ != dest->dest[i]) 
			return -7;
	}

	rtn += 4;
	return rtn;
}


int PlcRecvPkt(unsigned char *buf,int timeout)
{
	int times, state, recvlen, maxlen;
	unsigned char *pbuf = PlStaticBuffer;
	
	//AssertLog(len <= 0, "invalid len(%d)\n", len);
	//printf("PlcRecvPkt\n");
	//timeout = 1000;

	//printf("PlcRecvPkt........................\n");
	state = 0;
	recvlen = 0;
	maxlen = 0;
	memset(PlStaticBuffer,0x00,sizeof(PlStaticBuffer));
	for(times=0; times<timeout; times++) 
	{
		while(UartRecv(PLC_UART_PORT, pbuf, 1) > 0) 
		{
			//DebugPrint(0, "pl recv: %02X, %d\n", *pbuf, state);
			switch(state) 
			{
			case 0:
				if(0x68 == *pbuf) 
				{
					pbuf++;
					recvlen = 1;
					state = 1;
					//printf("PlcRecvPkt   0x68\n");
				}
				break;
			case 1:
				maxlen = (int)*pbuf++&0xff;
				//printf("maxlen = %d\n",maxlen);
				if(maxlen < 5) 
				{
					pbuf = PlStaticBuffer;
					state = 0;
					break;
				}
				recvlen++;
				state = 2;
				break;
			case 2:
				recvlen++;
				if(recvlen >= maxlen) 
				{
					goto mark_rcvend;
				}
				pbuf++;
			}
		}

		Sleep(10);
	}
	return PLCERR_TIMEOUT;
mark_rcvend:
		smallcpy(buf, PlStaticBuffer,maxlen);
		return maxlen;
}




/**
* @brief 接收数据帧
* @param dest 目的地址
* @param buf 接收缓存区指针
* @param len 接收缓存区长度
* @param timeout 超时时间(100ms)
* @param 成功返回接收到的数据长度, 失败返回负数(参见错误码PLCERR_XXX)
*/
int PlStaticRecvPkt(const plc_dest_t *dest, unsigned char *buf, int len, int timeout)
{
	int times, state, recvlen, maxlen;
	unsigned char *pbuf = PlStaticBuffer;

	AssertLog(len <= 0, "invalid len(%d)\n", len);

	timeout = 100;

	state = 0;
	recvlen = 0;
	maxlen = 0;
	for(times=0; times<timeout; times++) 
	{
		while(UartRecv(PLC_UART_PORT, pbuf, 1) > 0) 
		{
			DebugPrint(0, "pl recv: %02X, %d\n", *pbuf, state);
			switch(state) 
			{
			case 0:
				if(0x68 == *pbuf) 
				{
					pbuf++;
					recvlen = 1;
					state = 1;
				}
				break;
			case 1:
				maxlen = (int)*pbuf++&0xff;
				if(maxlen < 5) 
				{
					pbuf = PlStaticBuffer;
					state = 0;
					break;
				}
				recvlen++;
				state = 2;
				break;
			case 2:
				recvlen++;
				if(recvlen >= maxlen) 
				{
					goto mark_rcvend;
				}
				pbuf++;
			}
		}

		Sleep(10);
	}

	PlStaticInfo.ctime = times;

	PrintLog(LOGTYPE_DOWNLINK, "PLC recv timeout(%d), time=%d00ms:\n", recvlen, times);
	if(recvlen) PrintHexLog(LOGTYPE_DOWNLINK, PlStaticBuffer, recvlen);

	return PLCERR_TIMEOUT;

mark_rcvend:
	PlStaticInfo.ctime = times;

	PrintLog(LOGTYPE_DOWNLINK, "PLC recv(%d), time=%d00ms:\n", maxlen, times);
	PrintHexLog(LOGTYPE_DOWNLINK, PlStaticBuffer, maxlen);

	recvlen = CheckPacket(dest, maxlen);
	if(recvlen < 0) {
		PrintLog(LOGTYPE_DOWNLINK, "check error(%d):\n", recvlen);
		return PLCERR_INVALID;
	}

	pbuf = PlStaticBuffer + recvlen;
	recvlen = (int)(*pbuf)&0xff;
	if(recvlen > len) {
		PrintLog(LOGTYPE_DOWNLINK, "buffer overflow(%d:%d):\n", recvlen, len);
		return PLCERR_INVALID;
	}
	smallcpy(buf, pbuf+1, recvlen);

	pbuf = PlStaticBuffer + 3;  //info area
	PlStaticInfo.routes = dest->route.level;
	PlStaticInfo.phase = pbuf[2]&0x0f;
	PlStaticInfo.quality = 0;

	return recvlen;
}

