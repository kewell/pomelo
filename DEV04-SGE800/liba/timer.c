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
	文件		：  adc.c
	描述		：  本文件定义了定时器模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.01
******************************************************************************/
//库配置头文件
#include "private/config.h"
	
//模块启用开关
#ifdef CFG_TIMER_MODULE
	
//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//open 标志	
#include <sys/ioctl.h>	//ioctl
#include <string.h> 	//memset
#include <unistd.h>		//close
#include <pthread.h>	//pthread库函数
	
//提供给用户的头文件
#include "include/timer.h"
#include "include/error.h"
	
//驱动调用头文件
#include "private/drvlib/timerlib.h"
#include "private/debug.h"
	
/*************************************************
  静态全局变量及宏定义
*************************************************/
#define MAX_TIMER	 6						//最大定时器数量
#define TIMER_PRECISION_HIGH_S SCK32KIHZ	//高精度低频时钟源
#define TIMER_PRECISION_LOW_S MCKD8		//低精度高频时钟源
static struct {
	u8 	count;				//模快打开计数
	int fd;					//文件描述符
	int fun;				//定时器功能

	
	u32	heart_val;			//定时器心跳周期

	u16	pwm_freq;			//pwm频率
	u8	pwm_fz;				//pwm占空比分子
	u8	pwm_fm;				//pwm占空比分母

	pthread_mutex_t mutex;	//互斥锁
}timer[MAX_TIMER]={{0}};//模块打开计数初始化为0
/*************************************************
  API
*************************************************/

/******************************************************************************
*	函数:	timer_init
*	功能:	定时器模块初始化
*	参数:	id				-	定时器通道号
			mode			-	定时器操作模式
*	返回:	0				-	成功
			-ERR_NODEV		-	无此设备
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数无效
			-ERR_BUSY		-	已经打开
			-ERR_NOFILE		-	没有此路径
*	说明:	定时器工作模式包括周期定时、PWM波、频率测量。
 ******************************************************************************/
int timer_init (u8 id, u8 mode)
{
	int ret = -1;	
	char *dev[]={"/dev/atmel_tc0","/dev/atmel_tc1","/dev/atmel_tc2",
				"/dev/atmel_tc3","/dev/atmel_tc4","/dev/atmel_tc5"};

	if(id < 0 || id >= MAX_TIMER){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	if(timer[id].count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}
	if((mode == TIMER_MODE_HEART) || (mode == TIMER_MODE_MEASURE) || (mode == TIMER_MODE_PWM)){
		timer[id].fun = mode;
	}else{
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	//初始化互斥锁
	if (pthread_mutex_init(&timer[id].mutex, NULL)) {
		ret = -ERR_SYS;
		goto err;
	}

	//打开定时器驱动
	timer[id].fd = open(dev[id], O_RDONLY);
	if (timer[id].fd < 0){
		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}	

	timer[id].count = 1;
	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	timer_heart_setconfig
*	功能:	定时器心跳模式下配置
*	参数:	id				-	定时器通道号
			interval		-	周期定时的间隔（10ms的倍数或者125ms的倍数）
*	返回:	0				-	成功
			-ERR_NOFUN		-	无此功能
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没初始化
*	说明:	如果在没有关掉定时器时，再次配置延时时间，会报-ERR_SYS错误
 ******************************************************************************/
int timer_heart_setconfig (u8 id, u32 interval)
{
	int ret = -1;
	if(id < 0 || id >= MAX_TIMER){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	if(timer[id].count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	//判断功能有效性
	if(timer[id].fun != TIMER_MODE_HEART){
		ret = -ERR_NOFUN;
		goto err;
	}

	if(interval < 10 || interval > 1000 || ((interval%10 != 0) &&  (interval%125 != 0)) ){//判断参数有效性
		ret = -ERR_INVAL;		
		goto err;
	}
	//设置定时器的时钟			
	ret = ioctl(timer[id].fd, SET_CLOCK, SCK32KIHZ); 
	if (ret < 0){
		ret = -ERR_SYS;
		goto err;
	} 
	//停止定时器
	
	//设置周期
	ret = ioctl(timer[id].fd, SET_DELAY, interval);
	if (ret < 0){		
		ret = -ERR_SYS;
		goto err;
	}
	
	timer[id].heart_val = interval;		
	ret = 0;
err:
	return ret;
}


/******************************************************************************
*	函数:	timer_heart_getconfig
*	功能:	定时器心跳模式下读取配置
*	参数:	id				-	定时器通道号
			interval		-	周期定时的间隔（数据传出指针）
*	返回:	0				-	成功
			-ERR_NOFUN		-	无此功能
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没初始化
			-ERR_NOCFG		-	没有配置
*	说明:	无
 ******************************************************************************/
int timer_heart_getconfig (u8 id, u32 *interval)
{
	int ret = -1;
	if(id < 0 || id >= MAX_TIMER){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	if(timer[id].count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	//判断功能有效性
	if(timer[id].fun != TIMER_MODE_HEART){
		ret = -ERR_NOFUN;
		goto err;
	}
	if(timer[id].heart_val < 10 || timer[id].heart_val > 2000 ||	//判断参数有效性
			((timer[id].heart_val%10 != 0) &&  (timer[id].heart_val != 125)) ){//判断参数有效性
		ret = -ERR_NOCFG;		
		goto err;
	}
	*interval = timer[id].heart_val;
	
	ret = 0;
err:
	return ret;
}


/******************************************************************************
*	函数:	timer_heart_start
*	功能:	周期定时模式下启动周期触发
*	参数:	id				-	定时器通道号
*	返回:	0				-	成功
			-ERR_NOFUN		-	无此功能
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没初始化
			-ERR_NOCFG		-	没有配置
*	说明:	无
 ******************************************************************************/
int timer_heart_start(u8 id)
{
	int ret = -1;
	if(id < 0 || id >= MAX_TIMER){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	if(timer[id].count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	//判断功能有效性
	if(timer[id].fun != TIMER_MODE_HEART){
		ret = -ERR_NOFUN;
		goto err;
	}
	//判断是否已配置
	if(timer[id].heart_val == 0 ){
		ret = -ERR_NOCFG;
		goto err;
	}
	//启动定时器
	ret = ioctl(timer[id].fd, TCSTART, 0);
	if (ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	ret = 0;
err:
	return ret;
	
}

/******************************************************************************
*	函数:	timer_heart_wait
*	功能:	周期定时模式下等待周期触发
*	参数:	id				-	定时器通道号
*	返回:	0				-	成功
			-ERR_NOFUN		-	无此功能
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没初始化
*	说明:	无
 ******************************************************************************/
int timer_heart_wait (u8 id)
{
	int ret = -1, a;
	if(id < 0 || id >= MAX_TIMER){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	if(timer[id].count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	//判断功能有效性
	if(timer[id].fun != TIMER_MODE_HEART){
		ret = -ERR_NOFUN;
		goto err;
	}
		
	ret = read(timer[id].fd, (char *)&a, 2);	//读取定时器
	if (ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	timer_pwm_setconfig
*	功能:	设置或改变PWM功能的频率，占空比等参数
*	参数:	id				-	定时器号
			freq			-	脉宽调制频率
			fz				-	占空比分子
			fm				-	占空比分母
*	返回:	0				-	成功
			-ERR_NOFUN		-	无此功能
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没初始化
			-ERR_OTHER		-	其他关于系统线程锁的错误
*	说明:	当改变频率输出时，各种参数为0时，表示不改变其值
 ******************************************************************************/
int timer_pwm_setconfig (u8 id, u16 freq, u8 fz, u8 fm)
{
	int ret = -1;	
	
	long long arg;
	
	//获得互斥锁
	if (pthread_mutex_lock (&timer[id].mutex)) {
		return	-ERR_NOINIT;		
	}
	if(fz == 0 || fm == 0){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	
	if(id < 0 || id >= MAX_TIMER){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	if(timer[id].count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	//判断功能有效性
	if(timer[id].fun != TIMER_MODE_PWM){
		ret = -ERR_NOFUN;
		goto err;
	}
	//根据频率设置时钟??????
	ret = ioctl(timer[id].fd, SET_CLOCK, SCK32KIHZ); 
	if (ret < 0){
		ret = -ERR_SYS;
		goto err;
	} 

	//停止定时器(调用设置参数系统调用时，自动停止定时器)

	//设置参数
	arg = freq;
	arg = (arg<<32) | ((fz&0xff)<<16) | (fm&0xffff);	//驱动arg参数
	
	ret = ioctl(timer[id].fd, SET_PWM, &arg);
	if (ret < 0){
		ret = -ERR_SYS;
		goto err;
	}

	//启动定时器	
	ret = ioctl(timer[id].fd, TCSTART, 0);
	if (ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	timer[id].pwm_freq = freq;
	timer[id].pwm_fz = fz;
	timer[id].pwm_fm = fm;
	ret = 0;
err:
	if (pthread_mutex_unlock (&timer[id].mutex)) {
		ret = -ERR_OTHER;	
	}
	return ret;
}


/******************************************************************************
*	函数:	timer_pwm_getconfig
*	功能:	获取PWM功能的频率，占空比等参数
*	参数:	id				-	定时器号
			freq			-	脉宽调制频率（数据传出）
			fz				-	占空比分子（数据传出）
			fm				-	占空比分母（数据传出）
*	返回:	0				-	成功
			-ERR_NOFUN		-	无此功能
			-ERR_INVAL		-	参数错误
			-ERR_NOINIT		-	没有初始化
*	说明:	无
 ******************************************************************************/
int timer_pwm_getconfig (u8 id, u16 *freq, u8 *fz, u8 *fm)
{
	int ret = -1;	
	if(id < 0 || id >= MAX_TIMER){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	if(timer[id].count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	//判断功能有效性
	if(timer[id].fun != TIMER_MODE_PWM){
		ret = -ERR_NOFUN;
		goto err;
	}

	*freq = timer[id].pwm_freq ;
	*fz = timer[id].pwm_fz ;
	*fm = timer[id].pwm_fm ;
	
	ret = 0;
err:
	return ret;
}


/******************************************************************************
*	函数:	timer_measure_read
*	功能:	读取频率测量功能的频率
*	参数:	id				-	定时器通道号
			freq			-	读取的频率
*	返回:	0				-	成功
			-ERR_NOFUN		-	无此功能
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没初始化
*	说明:	无
 ******************************************************************************/
int timer_mesure_read(u8 id, u16 *freq)
{
	int ret = -1;	

	if(id < 0 || id >= MAX_TIMER){
		ret = -ERR_INVAL;		//参数无效
		goto err;
	}
	if(timer[id].count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	//判断功能有效性
	if(timer[id].fun != TIMER_MODE_MEASURE){
		ret = -ERR_NOFUN;
		goto err;
	}
	//设置时钟,根据测量频率设置时钟?????????
	
	//读取频率???????

	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	timer_close
*	功能:	关闭定时器
*	参数:	id				-	定时器通道号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化
			-ERR_OTHER		-	其他关于线程锁的错误
*	说明:	无
 ******************************************************************************/
int timer_close (u8 id)
{
	int ret = -1;
	if(id < 0 || id >= MAX_TIMER){
		return  -ERR_INVAL;		//参数无效		
	}
	if(timer[id].count == 0)
		return -ERR_NOINIT;
	ret = close(timer[id].fd);
	if(ret < 0)
		return -ERR_SYS;
	
	timer[id].count = 0;
	//销毁互斥锁	
	if (pthread_mutex_destroy(&timer[id].mutex)) {
		ret = -ERR_OTHER;
	}

	ret = 0;
	return ret;
}


#endif 		//CFG_TIMER_MODULE
