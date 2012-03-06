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
	文件		：  led.c
	描述		：  本文件定义了状态灯模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/
//库配置头文件
#include "private/config.h"
	
//模块启用开关
#ifdef CFG_LED_MODULE
//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//open 标志	
#include <sys/ioctl.h>	//ioctl
#include <string.h> 	//memset
#include <unistd.h>		//close
#include <pthread.h>	//pthread库函数

//提供给用户的头文件
#include "include/led.h"
#include "include/error.h"
	
//驱动调用头文件
#include "private/drvlib/gpiolib.h"
#include "private/debug.h"
	
/*************************************************
  静态全局变量及宏定义
*************************************************/

struct period_arg_t{
	u8 id;
	u32 last;
	u32 period;
};

#define PINBASE 32							//传入到驱动的io口地址基址
#define MAX_LED_CHN 8						//最大led数量

static pthread_mutex_t mutex;				//互斥锁

static u8 	led_chn[CFG_LED_NUM];			//led通道对应的物理通道
static u8 	led_type[CFG_LED_NUM];
static pthread_t thread_id[CFG_LED_NUM];	//周期动作线程id
static struct period_arg_t thread_arg[CFG_LED_NUM];

static u8 	led_count = 0;					//模快打开计数
static int 	led_fd;						//文件描述符
static  u8	led_period[CFG_LED_NUM];		//led周期动作激活标志
/*************************************************
  API
*************************************************/
/******************************************************************************
*	函数:	led_act
*	功能:	led点亮
*	参数:	id
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与led亮灭之间的联系出错
*	说明:
 ******************************************************************************/
int led_act(u8 id)
{
	int ret;

	if(led_type[id] == 1){
		ret = ioctl(led_fd, OSET, led_chn[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else if(led_type[id] == 0){
		ret = ioctl(led_fd, OCLR, led_chn[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else{
		ret = -ERR_CFG;
		goto err;
	}
	ret = 0;
err:
	return ret;
}


/******************************************************************************
*	函数:	led_disact
*	功能:	led熄灭
*	参数:	id
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与led亮灭之间的联系出错
*	说明:
 ******************************************************************************/
int led_disact(u8 id)
{
	int ret;

	if(led_type[id] == 1){
		ret = ioctl(led_fd, OCLR, led_chn[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else if(led_type[id] == 0){
		ret = ioctl(led_fd, OSET, led_chn[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else{
		ret = -ERR_CFG;
		goto err;
	}
err:
	return ret;
}
/******************************************************************************
*	函数:	led_init
*	功能:	AD转换模块初始化
*	参数:	无
*	返回:	0				-	成功
			-ERR_CFG		-	配置超限
			-ERR_BUSY		-	已经打开
			-ERR_SYS		-	系统错误
			-ERR_NOFILE		-	没有此路径
 ******************************************************************************/
int led_init(void)
{
	int ret = -1;
	int i;

	if(led_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}

	memset(led_chn, 0, CFG_LED_NUM);
	memset(led_period, 0, CFG_LED_NUM);

	//设置通道
#ifdef CFG_LED_0
	led_chn[LED0] = CFG_LED_0 + PINBASE;
	led_type[LED0] = CFG_LED_TYPE0;
#endif
#ifdef CFG_LED_1
	led_chn[LED1] = CFG_LED_1 + PINBASE;
	led_type[LED1] = CFG_LED_TYPE1;
#endif
#ifdef CFG_LED_2
	led_chn[LED2] = CFG_LED_2 + PINBASE;
	led_type[LED2] = CFG_LED_TYPE2;
#endif
#ifdef CFG_LED_3
	led_chn[LED3] = CFG_LED_3 + PINBASE ;
	led_type[LED3] = CFG_LED_TYPE3;
#endif
#ifdef CFG_LED_4
	led_chn[LED4] = CFG_LED_4 + PINBASE;
	led_type[LED4] = CFG_LED_TYPE4;
#endif
#ifdef CFG_LED_5
	led_chn[LED5] = CFG_LED_5 + PINBASE ;
	led_type[LED5] = CFG_LED_TYPE5;
#endif
#ifdef CFG_LED_6
	led_chn[LED6] = CFG_LED_6 + PINBASE ;
	led_type[LED6] = CFG_LED_TYPE6;
#endif
#ifdef CFG_LED_7
	led_chn[LED7] = CFG_LED_7 + PINBASE ;
	led_type[LED7] = CFG_LED_TYPE7;
#endif

	//打开驱动程序
	led_fd = open("/dev/atmel_gpio", O_RDONLY);	//打开外部ADC驱动
	if (led_fd < 0){
		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}	

	//设置继电器输出默认不动作
	for(i = 0; i < CFG_LED_NUM; i ++){
		if(led_chn[i] > 0){					//配置io口为输出
			ret = led_disact(i);			//配置不动作
			if(ret < 0){
				goto err;
			}
			ret = ioctl(led_fd, SETO, led_chn[i]);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
		}
	}

	if (pthread_mutex_init(&mutex, NULL)) {	//初始化互斥锁
		ret = -ERR_SYS;
		goto err;
	}
	led_count = 1;
	ret = 0;
err:
	return ret;
}


/******************************************************************************
*	函数:	led_thread_period
*	功能:	led周期亮灭线程
*	参数:
*	返回:
*	说明:
 ******************************************************************************/
void * led_thread_period(void * arg)
{
	int ret;
	fd_set rfds;
	struct timeval tv;
	int fd = 1;
	struct period_arg_t *per_arg;

	tv.tv_sec = 0;
	FD_ZERO (&rfds);
	FD_SET (fd, &rfds);

	per_arg = (struct period_arg_t *)arg;

	led_period[per_arg->id] = 1;	//设置线程运行标志位，供线程取消用。

	//设置线程属性，收到cancel后立即退出
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	//输出周期为period，持续动作为last的电平
	while(1){
		ret = led_act(per_arg->id);
		if(ret < 0){
			break;
		}
		tv.tv_usec = per_arg->last*1000;
		select (0, NULL, NULL, NULL, &tv);

		ret = led_disact(per_arg->id);
		if(ret < 0){
			break;
		}
		tv.tv_usec = (per_arg->period - per_arg->last) * 1000;
		select (0, NULL, NULL, NULL, &tv);

	}
	pthread_exit(0);
}

/******************************************************************************
*	函数:	led_on
*	功能:	led条件亮灭
*	参数:	id				-	led编号
*			delay			-	延时时间，单位为ms，为0表示没有延时
			last			-	持续时间，单位为ms，为0表示没有持续
			period			-	周期时间，单位为ms，为0表示没有周期
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_NOINIT		-	没有初始化
			-ERR_NODEV		-	没有此设备
			-ERR_CFG		-	配置出错
			-ERR_SYS		-	系统错误
*	说明:	3个时间全为0时，表示一直亮, 此处时间精度不高，应>20ms.
 ******************************************************************************/
int led_on(u8 id, u32 delay, u32 last, u32 period)
{
	int ret = -1;

	fd_set rfds;					//select延时
	struct timeval tv;
	int fd = 1;

	thread_arg[id].id = id;
	thread_arg[id].last = last;
	thread_arg[id].period = period;

    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);

	if(led_count == 0){
		return -ERR_NOINIT;
	}
	if((id < 0) || (id >= MAX_LED_CHN)){		//范围检查
		return -ERR_NODEV;
	}

	if(led_chn[id] == 0){
		return -ERR_CFG;
	}

	//延迟delay ms
	if(delay > 0){
		tv.tv_sec = 0;
		tv.tv_usec = delay*1000;
		select (0, NULL, NULL, NULL, &tv);

	}
	//周期动作
	if(period > 0){
		if((last <= 0) || (period <= last) ){
			ret = -ERR_INVAL;
			goto err;
		}

		ret = pthread_create(&thread_id[id], NULL, led_thread_period, (void *)&thread_arg[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else{
		//设置led亮,延迟last ms，设置led灭
		if(last > 0){
			ret = led_act(id);
			if(ret < 0){
				goto err;
			}
			tv.tv_sec = 0;
			tv.tv_usec = last*1000;
			select (0, NULL, NULL, NULL, &tv);
			ret = led_off(id);
			if(ret < 0){
				goto err;
			}
		}
	}

	//其它参数为0时，只亮led
	if((delay == 0) && (last == 0) && (period == 0)){
		ret = led_act(id);
		if(ret < 0){
			goto err;
		}
	}

	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	led_off
*	功能:	led灭
*	参数:	id				-	led号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与led亮灭之间的联系出错
*	说明:
 ******************************************************************************/
int led_off(u8 id)
{
	int ret = -1;

	if(led_count == 0){
		return -ERR_NOINIT;
	}
	if((id < 0) || (id >= MAX_LED_CHN)){		//范围检查
		return -ERR_NODEV;
	}

	if(led_chn[id] == 0){
		return -ERR_CFG;
	}

	//周期动作，结束动作线程，并等待线程结束
	if(led_period[id] == 1){
		ret = pthread_cancel(thread_id[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		ret = pthread_join(thread_id[id],NULL);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		led_period[id] = 0;
	}

	ret = led_disact(id);
	if(ret < 0){
		goto err;
	}

	ret = 0;
err:
	return ret;
}	

/******************************************************************************
*	函数:	led_check
*	功能:	检查led状态
*	参数:	id				-	led通道号
*	返回:	1				-	亮
			0				-	灭
			-ERR_TIMEOUT	-	超时
			-ERR_NODEV 		-	无此设备
			-ERR_NOINIT		-	锁没有初始化化
			-ERR_OTHER:		-	其他关于线程互斥锁的错误
			-ERR_SYS		-	系统错误
*	说明:
 ******************************************************************************/
int led_check(u8 id)
{
	int ret = -1;

	u32 vp = led_chn[id];

	if(led_count == 0){
			return -ERR_NOINIT;
	}
	if((id < 0) || (id >= MAX_LED_CHN)){		//din范围检查
		return -ERR_NODEV;		
	}	
	//获得互斥锁
	if (pthread_mutex_lock (&mutex)) {
		return	-ERR_NOINIT;
	}

	if(led_chn[id] == 0){
		return -ERR_CFG;
	}

	//获取io口电平
	ret = ioctl(led_fd, IOGETO, &vp);
	if(ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	if(vp > 1){				//返回值非0、 1报错
		ret = -ERR_SYS;
		goto err;
	}

	//判断led状态
	if(led_type[id] == 1){
		if(vp == 1){
			ret = 1;
		}else{
			ret = 0;
		}
	}else if(led_type[id] == 0){
		if(vp == 0){
			ret = 1;
		}else{
			ret = 0;
		}
	}else{
		ret = -ERR_CFG;
		goto err;
	}
err:	
	if (pthread_mutex_unlock (&mutex)) {
		ret = -ERR_OTHER;	
	}	
	return ret;
}

/******************************************************************************
*	函数:	led_close
*	功能:	led模块关闭函数
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没有初始化
			-ERR_OTHER		-	其他关于线程互斥锁的错误
			-ERR_CFG		-	配置IO高低与led亮灭之间的联系出错
*	说明:	无
 ******************************************************************************/
int led_close(void)
{
	int ret = -1;
	int i;

	if(led_count == 0){
		return -ERR_NOINIT;
	}
	for(i = 0; i < CFG_LED_NUM; i ++){
		if(led_chn[i] > 0){
			ret = led_off(i);					//配置不动作
			if(ret < 0){
				return ret;
			}
		}
	}

	ret = close(led_fd);
	if(ret < 0){
		return -ERR_SYS;	
	}
	led_count = 0;
	if (pthread_mutex_destroy(&mutex)) {
		ret = -ERR_OTHER;
	}
	ret = 0;

	return ret;
}
#endif
