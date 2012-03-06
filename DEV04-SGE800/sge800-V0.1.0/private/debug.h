/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/******************************************************************************
	项目名称	：SGE800计量智能终端平台
	文件		：debug.h
	描述		：本文件定义了平台调试方法
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2009.12
******************************************************************************/
#ifndef _DEBUG_H
#define _DEBUG_H

//开入检测模块打印调试
#ifdef CFG_DEBUG_DIN
#define	DINPRINTF(x...)			printf("DIN:" x)
#else
#define DINPRINTF(x...)
#endif

//脉冲检测模块打印调试
#ifdef CFG_DEBUG_PULSE
#define	PULSEPRINTF(x...)		printf("PULSE:" x)
#else
#define PULSEPRINTF(x...)
#endif

//掉电检测模块打印调试
#ifdef CFG_DEBUG_POWERCHECK
#define	POWERCHECKPRINTF(x...)	printf("POWERCHECK:" x)
#else
#define POWERCHECKPRINTF(x...)
#endif

//AD转换模块打印调试
#ifdef CFG_DEBUG_ADC
#define	ADCPRINTF(x...)			printf("ADC:" x)
#else
#define ADCPRINTF(x...)
#endif

//RTC模块打印调试
#ifdef CFG_DEBUG_RTC
#define	RTCPRINTF(x...)			printf("RTC:" x)
#else
#define RTCPRINTF(x...)
#endif

//定时器模块打印调试
#ifdef CFG_DEBUG_TIMER
#define	TIMERPRINTF(x...)		printf("TIMER:" x)
#else
#define TIMERPRINTF(x...)
#endif

//IO模块打印调试
#ifdef CFG_DEBUG_GPIO
#define	GPIOPRINTF(x...)		printf("GPIO:" x)
#else
#define GPIOPRINTF(x...)
#endif

//串口模块打印调试
#ifdef CFG_DEBUG_COMPORT
#define	COMPORTPRINTF(x...)		printf("COMPORT:" x)
#else
#define COMPORTPRINTF(x...)
#endif

//网络模块打印调试
#ifdef CFG_DEBUG_NET
#define	NETPRINTF(x...)			printf("NET:" x)
#else
#define NETPRINTF(x...)
#endif

//USB从口虚拟串口模块打印调试
#ifdef CFG_DEBUG_GSERIAL
#define	USBDPRINTF(x...)		printf("GSERIAL:" x)
#else
#define USBDPRINTF(x...)
#endif

//线程模块打印调试
#ifdef CFG_DEBUG_THREAD
#define	THREADPRINTF(x...)		printf("THREAD:" x)
#else
#define THREADPRINTF(x...)
#endif

//消息模块打印调试
#ifdef CFG_DEBUG_MSG
#define	MSGPRINTF(x...)			printf("MSG:" x)
#else
#define MSGPRINTF(x...)
#endif

#endif /* _DEBUG_H */
