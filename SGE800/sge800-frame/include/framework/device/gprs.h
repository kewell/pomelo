/******************************************************************************
	项目名称	：  SGE800计量智能终端业务平台
	文件		：  gprs.h
	描述		：  本文件定义了GPRS设备的接口
	版本		：  0.1
	作者		：  万鸿俊
	创建日期	：  2010.12
******************************************************************************/
#ifndef _GPRS_H
#define _GPRS_H

#include "sge_core/typedef.h"
	
/*************************************************
  宏定义
*************************************************/
//GPRS模块类型编号
#define GPRS_ME3000		0		//ME3000

//连接模式定义
#define GPRS_MODE_TCP_CLIENT	0
#define GPRS_MODE_TCP_SERVER	1
#define GPRS_MODE_UDP_CLIENT	2
#define GPRS_MODE_UDP_SERVER	3

//获取流量类型定义
#define GPRS_FLOW_ALL		0
#define GPRS_FLOW_SEND		1
#define GPRS_FLOW_RECV		2

/*************************************************
  结构类型定义
*************************************************/
typedef struct {
	int (*open)(void);
	int (*connect)(u8 mode, u8 *ip, u16 port);
	int (*disconnect)(int cd);
	int (*senddata)(int cd, u8 *buf, u32 count);
	int (*sendsms)(int cd, u8 *buf, u32 count);
	int (*turnon)(void);
	int (*turnoff)(void);
	int (*getflow)(u8 type);
	int (*getstat)(int cd);
	int (*getsi)(void);
} gprs_device_t;

extern gprs_device_t gprs_device[];

#endif		//_GPRS_H
