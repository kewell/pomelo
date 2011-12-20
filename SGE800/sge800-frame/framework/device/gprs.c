/******************************************************************************
	项目名称	：  SGE800计量智能终端业务平台
	文件		：  gprs.c
	描述		：  本文件实现了GPRS设备的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
//库配置头文件
#include "framework/config.h"
	
//模块启用开关
#ifdef CFG_GPRS_DEVICE

//C库头文件
#include <stdio.h>
#include <string.h>		//memcpy

//基础平台头文件
#include "sge_core/error.h"
#include "sge_core/thread.h"
#include "sge_core/comport.h"

//业务平台头文件
#include "framework/base.h"
#include "framework/message.h"
#include "framework/device/gprs.h"
	
/*************************************************
  结构类型定义
*************************************************/



/*************************************************
  静态全局变量及宏定义
*************************************************/
#define GPRS_COM COMPORT1


/*************************************************
  API
*************************************************/
static int me3000_open(void)
{
	int ret;
	comport_config_t gprs_cfg = {
			.baud	=	9600,
			.verify	= COMPORT_VERIFY_NO,
			.ndata	= 8,
			.nstop	= 1,
			.timeout = 0,
			.rtscts = COMPORT_RTSCTS_ENABLE,
	};
	ret = comport_init(GPRS_COM, COMPORT_MODE_NORMAL);
	if (ret < 0 && ret != -6) {
		goto error;
	}
	ret = comport_setconfig(GPRS_COM, &gprs_cfg);
	if (ret < 0 ) {
		goto error;
	}

error:
	return ret;
}

static int me3000_connect(u8 mode, u8 *ip, u16 port)
{
	return 0;
}

static int me3000_disconnect(int cd)
{
	return 0;
}

static int me3000_senddata(int cd, u8 *buf, u32 count)
{
	return 0;
}

static int me3000_sendsms(int cd, u8 *buf, u32 count)
{
	return 0;
}

static int me3000_turnon(void)
{
	return 0;
}

static int me3000_turnoff(void)
{
	return 0;
}

static int me3000_getflow(u8 type)
{
	return 0;
}

static int me3000_getstat(int cd)
{
	return 0;
}

static int me3000_getsi(void)
{
	return 0;
}

gprs_device_t gprs_device[] = {
	{
		me3000_open,
		me3000_connect,
		me3000_disconnect,
		me3000_senddata,
		me3000_sendsms,
		me3000_turnon,
		me3000_turnoff,
		me3000_getflow,
		me3000_getstat,
		me3000_getsi
	},
};

#endif
