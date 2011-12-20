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
#include "private/config.h"

//模块启用开关
#ifdef CFG_POWERCHECK_MODULE

//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//ioctl
#include <string.h> 	//memset
#include <unistd.h>		//close
#include <signal.h>		//
#include <sys/ioctl.h>

//提供给用户的头文件
#include "include/powercheck.h"
#include "include/error.h"

//驱动调用头文件
#include "private/drvlib/powerchecklib.h"

/*************************************************
  静态全局变量及宏定义
*************************************************/
#define PINBASE 32			//传入到驱动的io口地址基址
static int fd;				//保存pwrd驱动文件描述符
static u8  pwrd_count = 0;		//模快打开计数
static u8  pwrd_mode;			//模块打开模式
static u16 pwrd_timeout;		//等待超时时间

/*************************************************
  API函数实现
*************************************************/
/******************************************************************************
*	函数:	powercheck_init
*	功能:	掉电检测模块初始化
*	参数:	mode			-	掉电检测模式
*	返回:	0				-	成功
			-ERR_NOFILE		-	没有此路径
			-ERR_BUSY		-	设备忙，已经打开
			-ERR_SYS		-	初始化锁失败，如内存不足；已经初始化
*	说明:	无
 ******************************************************************************/
int powercheck_init (u8 mode)
{
	int ret = -1;
	if(pwrd_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}
	if(mode == POWERCHECK_MODE_NOBLOCK){
		//设置为非阻塞方式读取
		fd = open("/dev/power_detect",O_RDWR | O_NOCTTY | O_NONBLOCK);		
		if (fd < 0){
			ret = -ERR_NOFILE;		//没有此路径
			goto err;
		} 
	}else if(mode == POWERCHECK_MODE_BLOCK_UP || mode == POWERCHECK_MODE_BLOCK_DOWN ||
		mode == POWERCHECK_MODE_BLOCK_UPDOWN){
		pwrd_mode = mode;
		fd = open("/dev/power_detect",O_RDWR | O_NOCTTY);		
		if (fd < 0){
			ret = -ERR_NOFILE;		//没有此路径
			goto err;
		} 
	}else{
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}		
	
	//设置掉电上电检测的硬件管脚
	ret = ioctl(fd, PWRD_SET_IO, CFG_POWERCHECK_IO + PINBASE);
	if (ret < 0){
		ret = -ERR_SYS; 	
		goto err;
	} 
	//设置掉电处理模式
	ret = ioctl(fd, PWRD_SET_MODE, pwrd_mode );
	if (ret < 0){
		ret = -ERR_SYS; 	
		goto err;
	} 
	
	pwrd_count = 1;
	ret = 0;
err:
	return ret;
}

/******************************************************************************
 * 函数:	powercheck_setfasync
 * 功能:	设置异步通知
 * 参数:	p 函数指针,回调函数
 * 返回:	0：成功
 			-ESYS： 错误
 * 说明:	无
 ******************************************************************************/
int powercheck_setfasync (void (*p)())
{
	int ret = -1;
	int oflags;
	//关联信号和处理函数
	signal(SIGIO, p);

	//设置设备文件的拥有者为本进程
	ret = fcntl(fd, F_SETOWN, getpid());
	if(ret < 0){
		perror("setown\n");		
	} 
	//设置设备支持异步通知模式
	oflags = fcntl(fd, F_GETFL);
	ret = fcntl(fd, F_SETFL, oflags | FASYNC);
	if(ret < 0){
		perror("setfl\n");		
	} 
	return ret;
}

/******************************************************************************
*	函数:	powercheck_setwaittime
*	功能:	阻塞模式下设置阻塞时间
*	参数:	timeout：阻塞时间（单位是100ms，为0表示永久阻塞）
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_NOINIT  	-	模块未初始化
*	说明:	无
 ******************************************************************************/
int powercheck_setwaittime (u16 timeout)
{
	int ret = -1;
	pwrd_timeout = 10*timeout;	//内部外部延时单位转换
	ret = ioctl(fd, PWRD_TIMEOUT, pwrd_timeout );
	if (ret < 0){
		return -ERR_SYS;		
	} 
	return 0;
}

/******************************************************************************
*	函数:	powercheck_getwaittime
*	功能:	读取当前设置的阻塞时间
*	参数:	timeout：阻塞时间（单位是100ms，为0表示永久阻塞）（数据传出）
*	返回:	0				-	成功
			-ERR_NOINIT		-	模块未初始化
*	说明:	无
 ******************************************************************************/
int powercheck_getwaittime (u16 *timeout)
{
	*timeout = pwrd_timeout;	
	return 0;
}

/******************************************************************************
*	函数:	powercheck_check
*	功能:	检测当前电源状态
*	参数:	无
*	返回:	0				-	掉电状态
			1				-	上电状态
			-ESYS			-	系统错误
			-ERR_NOINIT		-	模块未初始化
*	说明:	无
 ******************************************************************************/
int powercheck_check ()
{
	int ret = -1;
	u8 buf;
	if(pwrd_count == 0){
		ret = -ERR_NOINIT;	
		goto err;
	}
	ret = read(fd, &buf, 0); 
	if(ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	ret = buf;
err:		
	return ret;
}

/******************************************************************************
*	函数:	powercheck_close
*	功能:	关闭掉电检测模块
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	模块未初始化
*	说明:	无
 ******************************************************************************/
int powercheck_close ()
{
	int ret = -1;
	if(pwrd_count == 0)
		return -ERR_NOINIT;
	ret = close(fd);
	if(ret < 0)
		return -ERR_SYS;
	pwrd_count = 0;
	
	return ret;

}
#endif
