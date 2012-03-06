/*****************************************************************************/
/*许继电气股份有限公司                                     版权：2008-2015   */
/*****************************************************************************/
/* 本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许    */
/* 可不得擅自修改或发布，否则将追究相关的法律责任。                          */
/*                                                                           */
/*                      河南许昌许继股份有限公司                             */
/*                      www.xjgc.com                                         */
/*                      (0374) 321 2924                                      */
/*                                                                           */
/*****************************************************************************/


/******************************************************************************
    项目名称    ：  SGE800计量终端业务平台
    文 件 名       ：  sdev_net.c
    描          述    ：  本文件用于业务平台库数据流设备以太网功能的实现
    版          本    ：  0.1
    作          者    ：  孙锐
    创建日期    ：  2010.04
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		    //exit
#include <unistd.h>		    //sleep
#include <string.h>
#include <pthread.h>
#include <errno.h>

//平台库头文件
#include "include/sdev_net.h"
#include "include/net.h"
#include "private/config.h"
#include "include/error.h"
#include "include/sdev.h"

/*************************************************************
 *宏定义
 *************************************************************/
//客户端状态
static struct{
	u8 init;              		//初始化标志
	u8 timeout;					//设置串口接收超时
	u8 index;					//接收属性：阻塞/非阻塞
	u8 busy;    				//0：空闲；1：忙碌（发送等待状态）；
	u8 connect_state;      		//0:非在线状态；1：在线状态
	char ip[12];				//主站IP
	u16 port;					//端口号
}netstate;

/******************************************************************************
*	函数:	sdev_net_setconfig
*	功能:	配置网络客户端功能
*	参数:
*	返回:	0				-	成功
			-ESYS			-	系统错误
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化

*	说明:	无
******************************************************************************/
int sdev_net_setconfig(u8 cmd, void *cfg)
{
	int ret;
	memcpy(netstate.ip, ((struct netfig *)cfg)->ip,sizeof(((struct netfig *)cfg)->ip));
	netstate.port = ((struct netfig *)cfg) -> port;
	netstate.timeout = ((struct netfig *)cfg) ->timeout;
	netstate.index = ((struct netfig *)cfg) -> index;
	ret = net_client_init(netstate.timeout);
	if (ret){
		goto error;
	}
	netstate.init = 1;
error:
	return(ret);
}

/******************************************************************************
*	函数:	sdev_net_getconfig
*	功能:	获取网络客户端功能属性
*	参数:
*	返回:	0				-	成功


*	说明:	无
******************************************************************************/
int sdev_net_getconfig(u8 cmd, void *cfg)
{
	memcpy(((struct netfig *)cfg)->ip, netstate.ip, sizeof(((struct netfig *)cfg)->ip));
	((struct netfig *)cfg) -> port = netstate.port;
	((struct netfig *)cfg) -> timeout = netstate.timeout;
	((struct netfig *)cfg) -> index = netstate.index;
	return(0);
}

/******************************************************************************
*	函数:	sdev_net_send
*	功能:	网络客户端发送数据
*	参数:	buf				-	发送数据起始指针
			count			-	发送数据长度
*	返回:	>=0				-	发送成功的字节数
			-ESYS			-	系统错误
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化

*	说明:	超时时间通过配置发送超时定
******************************************************************************/
int sdev_net_send(u8 *buf, u32 count)
{
	int ret;
	if(netstate.init != 1){
		ret = -ERR_NOINIT;
		goto error;
	}
	ret = net_client_send(buf, (u16) count);
error:
	return(ret);
}

/******************************************************************************
*	函数:	sdev_net_recv
*	功能:	网络客户端接收数据
*	参数:	buf				-	接收数据起始指针
			count			-	接收缓冲区长度
*	返回:	>=0				-	接收成功的字节数
			-ERR_SYS		-	错误
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化

*	说明:
******************************************************************************/
int sdev_net_recv(u8 *buf, u32 count, u8 *type, u8 *tel)
{
	int ret;
	u16 length;
	if(netstate.init != 1){
		ret = -ERR_NOINIT;
		goto error;
	}
	if (count > 0xffff){
		ret = -ERR_INVAL;
		goto error;
	}
	ret = net_client_receive(buf, (u16)count, &length, netstate.index);
	if(ret != 0){
		goto error;
	}else{
		ret = length;
	}
error:
	return(ret);
}

/******************************************************************************
*	函数:	sdev_net_sendsmg
*	功能:	无
*	参数:
*	返回:

*	说明:	空函数
******************************************************************************/
int sdev_net_sendsmg(u8 *buf, u32 count, u8 *telnum, u8 tellen)
{
	return(0);
}

/******************************************************************************
*	函数:	sdev_net_recvsmg
*	功能:	无
*	参数:
*	返回:	0				-	成功(收到确认命令)
			-ERR_TIMEOUT	-	连接超时

*	说明:	空函数
******************************************************************************/
int sdev_net_recvsmg(u8 *buf, u32 count)
{
	return(0);
}
/******************************************************************************
*	函数:	sdev_net_connect
*	功能:
*	参数:	mode			-	连接方式：0创建新连接；1激活重连
*	返回:	0				-	成功(收到确认命令)
			-ERR_TIMEOUT	-	连接超时

*	说明:	无
******************************************************************************/
int sdev_net_connect(u8 mode)
{
	int ret;
	if(netstate.init != 1){
		ret = -ERR_NOINIT;
		goto error;
	}
	ret = net_client_connect(netstate.ip, netstate.port);
	if (ret){
		goto error;
	}
error:
	return(ret);
}

/******************************************************************************
*	函数:	sdev_net_disconnect
*	功能:	无
*	参数:
*	返回:	0				-	成功(收到确认命令)
			-ERR_TIMEOUT	-	连接超时

*	说明:	无
******************************************************************************/
int sdev_net_disconnect(void)
{
	int ret;
	ret = net_client_disconnect();
	return(ret);
}

/******************************************************************************
*	函数:	sdev_net_getstat
*	功能:	获得状态
*	参数:
*	返回:	0				-	成功


*	说明:	无
******************************************************************************/
int sdev_net_getstat(u8 cmd, void *stat)
{
	return(0);
}

/******************************************************************************
*	函数:	sdev_net_turnon
*	功能:	无
*	参数:
*	返回:	0				-	成功


*	说明:	无
******************************************************************************/
int sdev_net_turnon(void)
{
	return(0);
}

/******************************************************************************
*	函数:	sdev_net_turnoff
*	功能:	无
*	参数:
*	返回:

*	说明:	无
******************************************************************************/
int sdev_net_turnoff(void)
{
	return(0);
}


/******************************************************************************
*	函数:	sdev_net_poweron
*	功能:	上电
*	参数:
*	返回:	0				-	成功


*	说明:	无
******************************************************************************/
int sdev_net_poweron(void)
{
	return(0);
}

/******************************************************************************
*	函数:	sdev_net_poweroff
*	功能:	掉电
*	参数:
*	返回:	0				-	成功


*	说明:	无
******************************************************************************/
int sdev_net_poweroff(void)
{
	return(0);
}

/******************************************************************************
*	函数:	sdev_net_reset
*	功能:	无
*	参数:
*	返回:	0				-	成功


*	说明:	无
******************************************************************************/
int sdev_net_reset(void)
{
	return(0);
}

