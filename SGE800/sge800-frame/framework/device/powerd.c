/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  powercheck.c
	描述		：  本文件定义了掉电上电检测接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.01
******************************************************************************/
//库配置头文件
#include "framework/config.h"

//模块启用开关
#ifdef CFG_POWERD_DEVICE

//C库头文件
#include <stdio.h>
//#include <fcntl.h> 		//ioctl
//#include <string.h> 		//memset
//#include <unistd.h>		//close
//#include <signal.h>
//#include <sys/ioctl.h>

//基础平台头文件
#include "sge_core/powercheck.h"
#include "sge_core/error.h"
#include "sge_core/pinio.h"
#include "sge_core/thread.h"

//业务平台头文件
#include "framework/device/powerd.h"
#include "framework/message.h"
#include "framework/base.h"

/*************************************************
  静态全局变量及宏定义
*************************************************/

/*************************************************
  API函数实现
*************************************************/
/******************************************************************************
*	函数:	thread_power_detect
*	功能:	扫描按键输入线程
*	参数:
*	返回:
*	说明:
 ******************************************************************************/
static void * thread_power_detect(void * arg)
{
	int ret=-1;
	message_t msg_power = {
		.type = MSG_POWER,
	};
	//输出周期为period，持续动作为last的电平
	while(1){
		ret = powercheck_check();
		if(ret == 0){
			msg_power.wpara 	= 	POWER_DOWN;
			message_dispatch(&msg_power);
		}else if(ret == 1){
			msg_power.wpara 	= 	POWER_UP;
			message_dispatch(&msg_power);
		}
	}
	pthread_exit(0);
}
/******************************************************************************
*	函数:	powerd_init
*	功能:	掉电检测设备初始化
*	参数:	mode			-	掉电检测模式
*	返回:	0				-	成功
			-ERR_NOFILE		-	没有此路径
			-ERR_BUSY		-	设备忙，已经打开
			-ERR_SYS		-	初始化锁失败，如内存不足；已经初始化
*	说明:	无
 ******************************************************************************/
int powerd_init ()
{
	int ret = -1;
	pthread_t powerd_id;

	ret = powercheck_init(POWERCHECK_MODE_BLOCK_UPDOWN);
//	printf("%s,%s,%d:ret = %d!\n",__FILE__,__FUNCTION__, __LINE__,ret);
	if(ret < 0 ){
		goto err;
	}

	ret = powercheck_setwaittime(0);
	if(ret < 0 ){
		goto err;
	}

	//创建掉电监测线程，实时线程，优先级80-90
	ret = thread_create_base(&powerd_id, thread_power_detect, NULL, THREAD_MODE_REALTIME, 89);
	if((ret < 0) ){
		goto err;
	}
err:
	return ret;
}

#endif
