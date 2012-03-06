#ifndef _CONFIG_H
#define _CONFIG_H

#include "../include/pinio.h"

//调试配置
//#define CFG_DEBUG 1

#ifdef CFG_DEBUG
#define CFG_DEBUG_DIN
#define CFG_DEBUG_PULSE
#define CFG_DEBUG_POWERCHECK
#define CFG_DEBUG_ADC
#define CFG_DEBUG_RTC
#define CFG_DEBUG_TIMER
#define CFG_DEBUG_GPIO

#define CFG_DEBUG_COMPORT
#define CFG_DEBUG_NET
#define CFG_DEBUG_GSERIAL

#define CFG_DEBUG_THREAD
#define CFG_DEBUG_MSG
#endif

//AD转换模块配置
#define CFG_ADC_MODULE				1			//是否启用
#define CFG_ADC_NUM					8			//当前ADC通道数量
#define CFG_ADC_0					0			//0号通道对应的硬件通道
#define CFG_ADC_1					1			//1号通道对应的硬件通道
#define CFG_ADC_2					2			//2号通道对应的硬件通道
#define CFG_ADC_3					3			//3号通道对应的硬件通道
#define CFG_ADC_4					4			//4号通道对应的硬件通道
#define CFG_ADC_5					5			//5号通道对应的硬件通道
#define CFG_ADC_6					6			//6号通道对应的硬件通道
#define CFG_ADC_7					7			//7号通道对应的硬件通道

//定时器模块配置
#define CFG_TIMER_MODULE			1			//是否启用
#define CFG_TIMER_PWM_S				1			//脉宽调制时钟源，高精度
#define CFG_TIMER_MEASURE_S			2			//频率测量时钟源，低精度

//RTC模块配置
#define CFG_RTC_MODULE				1				//是否启用

//IO操作模块配置
#define CFG_GPIO_MODULE				1				//是否启用

//掉电上电检测模块配置
#define CFG_POWERCHECK_MODULE			1			//是否启用
#define CFG_POWERCHECK_IO				PIN_PC12	//是否启用

//网络模块
#define CFG_NET_MODULE              1
#define CFG_NET_SERVPORT            3333                 //端口号
#define CFG_NET_BACKLOG             8                    //队列
#define CFG_NET_MAXSIZE             8                    //最大连入客户端数--最大为64

//串口配置	
#define CFG_COMPORT_MODULE		1			//是否启用

//USB从口虚拟串口配置	
#define CFG_GSERIAL_MODULE		1			//是否启用

//线程操作模块配置
#define CFG_THREAD_MODULE			1				//是否启用
#define CFG_THREAD_MAX				32				//最大支持线程数

//消息服务模块配置
#define CFG_MSG_MODULE				1				//是否启用
#define CFG_MSG_MAX					32				//最大支持消息队列数
#define CFG_MSG_SIZE				32				//消息队列所能容纳的消息数


#endif /* _CONFIG_H */
