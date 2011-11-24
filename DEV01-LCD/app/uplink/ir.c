/**
* ir.c -- 红外口上行通信
* 
* 
* 创建时间: 2010-6-12
* 最后修改时间: 2010-6-12
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/debug.h"
#include "include/sys/task.h"
#include "include/sys/schedule.h"
#include "include/sys/uart.h"
#include "include/uplink/svrnote.h"
#include "uplink_pkt.h"
#include "uplink_dl.h"
#include "svrcomm.h"

#define IR_UART		0

/**
* @brief 红外通信任务(主动发送)
*/
void IrActiveTask(void)
{
	unsigned long ev;

	UplinkClearState(UPLINKITF_IR);

	while(1) 
	{
		SvrCommPeekEvent(SVREV_NOTE, &ev);

		if(ev&SVREV_NOTE) 
		{
			SvrNoteProc(UPLINKITF_IR);
		}

		if(!UplinkRecvPkt(UPLINKITF_IR)) 
		{
			SvrMessageProc(UPLINKITF_IR);
		}

		Sleep(10);
	}

	return;
}

/**
* @brief 红外通信任务(不主动发送)
*/
static void *IrPassiveTask(void *arg)
{
	UplinkClearState(UPLINKITF_IR);

	Sleep(100);

	while(1) 
	{
		if(!UplinkRecvPkt(UPLINKITF_IR)) {
			SvrMessageProc(UPLINKITF_IR);
		}

		Sleep(10);
	}

	return 0;
}

/**
* @brief 启动红外通信任务
* @param mode 0不启动任务, 1启动任务
* @return 成功返回0, 失败返回1
*/
DECLARE_INIT_FUNC(UplinkIrStart);
int UplinkIrStart(int mode)
{
	if(UartOpen(IR_UART)) 
	{
		printf("can not open uart %d\n", IR_UART);
		return 1;
	}

	UartSet(IR_UART, 1200, 8, 1, 'e');

	if(mode) 
	{
		SysCreateTask(IrPassiveTask, NULL);
		printf("ir passive task started\n");
	}

	SET_INIT_FLAG(UplinkIrStart);

	return 0;
}

/**
* @brief 从红外口通信接口读取一个字节
* @param buf 返回字符指针
* @return 成功0, 否则失败
*/
int IrGetChar(unsigned char *buf)
{
	if(UartRecv(IR_UART, buf, 1) > 0) return 0;
	else return 1;
}

/**
* @brief 向红外口通信接口发送数据
* @param buf 发送缓存区指针
* @param len 缓存区长度
* @return 成功0, 否则失败
*/
int IrRawSend(const unsigned char *buf, int len)
{
	return(UartSend(IR_UART, buf, len));
}

