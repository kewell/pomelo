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
	文件		：  relay.c
	描述		：  本文件定义了继电器模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/
//库配置头文件
#include "private/config.h"
	
//模块启用开关
#ifdef CFG_RELAY_MODULE
//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//open 标志	
#include <sys/ioctl.h>	//ioctl
#include <string.h> 	//memset
#include <unistd.h>		//close
#include <pthread.h>	//pthread库函数

//提供给用户的头文件
#include "include/relay.h"
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

#define PINBASE 32						//传入到驱动的io口地址基址
#define MAX_RELAY_CHN 8					//最大继电器数量

static pthread_mutex_t mutex;			//互斥锁

static u8 	relay_chn[MAX_RELAY_CHN];	//继电器通道对应的物理通道
static u8 	relay_chd[MAX_RELAY_CHN];	//继电器通道对应的双点物理通道
static u8 	relay_type[MAX_RELAY_CHN];	//继电器通道对应的类型
static struct period_arg_t thread_arg[MAX_RELAY_CHN];	//线程传入参数

static pthread_t thread_id[MAX_RELAY_CHN];		//周期动作线程id

static u8 	relay_count = 0;			//模快打开计数
static int 	relay_fd;				//文件描述符
static u8	relay_period[MAX_RELAY_CHN];		//继电器周期动作激活标志
/*************************************************
  API
*************************************************/
/******************************************************************************
*	函数:	relay_act
*	功能:	继电器吸合
*	参数:	id
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与继电器动作之间的联系出错
*	说明:
 ******************************************************************************/
int relay_act(u8 id)
{
	int ret;

	if(relay_type[id] == 1){
		ret = ioctl(relay_fd, OSET, relay_chn[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		if(relay_chd[id] > 0){		//双点模式
			ret = ioctl(relay_fd, OCLR, relay_chd[id]);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
		}
	}else if(relay_type[id] == 0){
		ret = ioctl(relay_fd, OCLR, relay_chn[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		if(relay_chd[id] > 0){		//双点模式
			ret = ioctl(relay_fd, OSET, relay_chd[id]);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
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
*	函数:	relay_disact
*	功能:	继电器断开
*	参数:	id
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与继电器动作之间的联系出错
*	说明:
 ******************************************************************************/
int relay_disact(u8 id)
{
	int ret;

	//配置id对应的继电器号为不动作
	if(relay_type[id] == 1){
		ret = ioctl(relay_fd, OCLR, relay_chn[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		if(relay_chd[id] > 0){		//双点模式
			ret = ioctl(relay_fd, OSET, relay_chd[id]);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
		}
	}else if(relay_type[id] == 0){
		ret = ioctl(relay_fd, OSET, relay_chn[id]);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		if(relay_chd[id] > 0){		//双点模式
			ret = ioctl(relay_fd, OCLR, relay_chd[id]);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
		}
	}else{
		ret = -ERR_CFG;
		goto err;
	}
err:
	return ret;
}
/******************************************************************************
*	函数:	relay_init
*	功能:	继电器模块初始化
*	参数:	无
*	返回:	0				-	成功
			-ERR_CFG		-	配置超限
			-ERR_BUSY		-	已经打开
			-ERR_SYS		-	系统错误
			-ERR_NOFILE		-	没有此路径
 ******************************************************************************/
int relay_init(void)
{
	int ret = -1;
	int i;
	if(CFG_RELAY_NUM > MAX_RELAY_CHN){
		ret = -ERR_CFG; 	//配置超限
		goto err;
	}
	if(relay_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}

	memset(relay_chn, 0, MAX_RELAY_CHN);
	memset(relay_chd, 0, MAX_RELAY_CHN);
	memset(relay_period, 0, MAX_RELAY_CHN);

	//设置通道
#ifdef CFG_RELAY_0
	relay_chn[RELAY0] = CFG_RELAY_0 + PINBASE;
	relay_type[RELAY0] = CFG_RELAY_TYPE0;
#endif
#ifdef CFG_RELAY_1
	relay_chn[RELAY1] = CFG_RELAY_1 + PINBASE;
	relay_type[RELAY1] = CFG_RELAY_TYPE1;
#endif
#ifdef CFG_RELAY_2
	relay_chn[RELAY2] = CFG_RELAY_2 + PINBASE;
	relay_type[RELAY2] = CFG_RELAY_TYPE2;
#endif
#ifdef CFG_RELAY_3
	relay_chn[RELAY3] = CFG_RELAY_3 + PINBASE ;
	relay_type[RELAY3] = CFG_RELAY_TYPE3;
#endif
#ifdef CFG_RELAY_4
	relay_chn[RELAY4] = CFG_RELAY_4 + PINBASE;
	relay_type[RELAY4] = CFG_RELAY_TYPE4;
#endif
#ifdef CFG_RELAY_5
	relay_chn[RELAY5] = CFG_RELAY_5 + PINBASE ;
	relay_type[RELAY5] = CFG_RELAY_TYPE5;
#endif
#ifdef CFG_RELAY_6
	relay_chn[RELAY6] = CFG_RELAY_6 + PINBASE ;
	relay_type[RELAY6] = CFG_RELAY_TYPE6;
#endif
#ifdef CFG_RELAY_7
	relay_chn[RELAY7] = CFG_RELAY_7 + PINBASE ;
	relay_type[RELAY7] = CFG_RELAY_TYPE7;
#endif

//双点判断
#ifdef CFG_RELAY_D0
	relay_chd[RELAY0] = CFG_RELAY_D0 + PINBASE ;
#endif
#ifdef CFG_RELAY_D1
	relay_chd[RELAY1] = CFG_RELAY_D1 + PINBASE ;
#endif
#ifdef CFG_RELAY_D2
	relay_chd[RELAY2] = CFG_RELAY_D2 + PINBASE;
#endif
#ifdef CFG_RELAY_D3
	relay_chd[RELAY3] = CFG_RELAY_D3 + PINBASE ;
#endif
#ifdef CFG_RELAY_D4
	relay_chd[RELAY4] = CFG_RELAY_D4 + PINBASE ;
#endif
#ifdef CFG_RELAY_D5
	relay_chd[RELAY5] = CFG_RELAY_D5 + PINBASE ;
#endif
#ifdef CFG_RELAY_D6
	relay_chd[RELAY6] = CFG_RELAY_D6 + PINBASE ;
#endif
#ifdef CFG_RELAY_D7
	relay_chd[RELAY7] = CFG_RELAY_D7 + PINBASE ;
#endif
	
	//打开驱动程序
	relay_fd = open("/dev/atmel_gpio", O_RDONLY);	//打开外部ADC驱动
	if (relay_fd < 0){
		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}	

	//设置继电器输出默认不动作
	for(i = 0; i < CFG_RELAY_NUM; i ++){
		if(relay_chn[i] > 0){					//配置io口为输出
			ret = relay_disact(i);					//配置不动作
			if(ret < 0){
				goto err;
			}
			ret = ioctl(relay_fd, SETO, relay_chn[i]);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
		}
		if(relay_chd[i] > 0){					//配置双点io口为输出
			ret = ioctl(relay_fd, SETO, relay_chd[i]);
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
	relay_count = 1;
	ret = 0;
err:
	return ret;
}


/******************************************************************************
*	函数:	relay_thread_period
*	功能:	继电器周期动作线程
*	参数:
*	返回:
*	说明:
 ******************************************************************************/
void * relay_thread_period(void * arg)
{
	int ret;
	fd_set rfds;
	struct timeval tv;
	int fd = 1;
	struct period_arg_t *per_arg;

	tv.tv_sec = 0;
	FD_ZERO (&rfds);
	FD_SET (fd, &rfds);

	//memcpy(&per_arg,(struct period_arg_t *)arg,sizeof per_arg);
	per_arg = (struct period_arg_t *)arg;
	relay_period[per_arg->id] = 1;	//设置线程运行标志位，供线程取消用。

	//设置线程属性，收到cancel后立即退出
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	//输出周期为period，持续动作为last的电平
	while(1){
		ret = relay_act(per_arg->id);
		if(ret < 0){
			break;
		}
		tv.tv_usec = per_arg->last*1000;
		select (0, NULL, NULL, NULL, &tv);

		ret = relay_disact(per_arg->id);
		if(ret < 0){
			break;
		}
		tv.tv_usec = (per_arg->period - per_arg->last) * 1000;
		select (0, NULL, NULL, NULL, &tv);

	}
	pthread_exit(0);
}

/******************************************************************************
*	函数:	relay_on
*	功能:	继电器条件动作
*	参数:	id				-	继电器编号
*			delay			-	延时时间，单位为ms，为0表示没有延时
			last			-	持续时间，单位为ms，为0表示没有持续
			period			-	周期时间，单位为ms，为0表示没有周期
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_NOINIT		-	没有初始化
			-ERR_NODEV		-	没有此设备
			-ERR_CFG		-	配置出错
			-ERR_SYS		-	系统错误
*	说明:	3个时间全为0时，表示一直动作, 此处时间精度不高，应>20ms.
 ******************************************************************************/
int relay_on(u8 id, u32 delay, u32 last, u32 period)
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

	if(relay_count == 0){
		return -ERR_NOINIT;
	}
	if((id < 0) || (id >= MAX_RELAY_CHN)){		//范围检查
		return -ERR_NODEV;
	}

	if(relay_chn[id] == 0){
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
		ret = pthread_create(&thread_id[id], NULL, relay_thread_period, (void *)&thread_arg[id]);

		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else{
		//设置继电器动作,延迟last ms，设置继电器不动作
		if(last > 0){
			ret = relay_act(id);
			if(ret < 0){
				goto err;
			}
			tv.tv_sec = 0;
			tv.tv_usec = last*1000;
			select (0, NULL, NULL, NULL, &tv);
			ret = relay_off(id);
			if(ret < 0){
				goto err;
			}
		}
	}

	//其它参数为0时，只动作继电器
	if((delay == 0) && (last == 0) && (period == 0)){
		ret = relay_act(id);
		if(ret < 0){
			goto err;
		}
	}

	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	relay_off
*	功能:	继电器断开
*	参数:	id				-	继电器号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与继电器动作之间的联系出错
*	说明:
 ******************************************************************************/
int relay_off(u8 id)
{
	int ret = -1;

	if(relay_count == 0){
		return -ERR_NOINIT;
	}
	if((id < 0) || (id >= MAX_RELAY_CHN)){		//范围检查
		return -ERR_NODEV;
	}

	if(relay_chn[id] == 0){
		return -ERR_CFG;
	}

	//周期动作，结束动作线程，并等待线程结束
	if(relay_period[id] == 1){
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

		relay_period[id] = 0;
	}

	ret = relay_disact(id);
	if(ret < 0){
		goto err;
	}

	ret = 0;
err:
	return ret;
}	

/******************************************************************************
*	函数:	relay_check
*	功能:	检查继电器状态
*	参数:	id				-	继电器通道号
*	返回:	1				-	动作
			0				-	不动作
			-ERR_TIMEOUT	-	超时
			-ERR_NODEV 		-	无此设备
			-ERR_NOINIT		-	锁没有初始化化
			-ERR_OTHER:		-	其他关于线程互斥锁的错误
			-ERR_SYS		-	系统错误
*	说明:
 ******************************************************************************/
int relay_check(u8 id)
{
	int ret = -1;
	u32 vp = relay_chn[id], vn = relay_chd[id];

	if(relay_count == 0){
			return -ERR_NOINIT;
	}
	if((id < 0) || (id >= MAX_RELAY_CHN)){		//din范围检查
		return -ERR_NODEV;		
	}	
	//获得互斥锁
	if (pthread_mutex_lock (&mutex)) {
		return	-ERR_NOINIT;
	}

	if(relay_chn[id] == 0){
		return -ERR_CFG;
	}
	//获取io口电平
	ret = ioctl(relay_fd, IOGETO, &vp);
	if(ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	if(vp > 1){				//返回值非0、 1报错
		ret = -ERR_SYS;
		goto err;
	}

	if(vn > 0){
		ret = ioctl(relay_fd, IOGETO, &vn);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
			if(vn > 1){
			ret = -ERR_SYS;
			goto err;
		}
	}

	//判断继电器状态

	if(relay_type[id] == 1){
		if(vp == 1){
			ret = 1;
		}else{
			ret = 0;
		}
		if(relay_chd[id] > 0){		//双点模式,当继电器正极为高，负极为低时动作，其它均为不动作
			if((vp == 1) && (vn == 0)){
				ret = 1;
			}else{
				ret = 0;
			}
		}
	}else if(relay_type[id] == 0){
		if(vp == 0){
			ret = 1;
		}else{
			ret = 0;
		}
		if(relay_chd[id] > 0){		//双点模式,当继电器正极为高，负极为低时动作，其它均为不动作
			if((vp == 0) && (vn == 1)){
				ret = 1;
			}else{
				ret = 0;
			}
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
*	函数:	relay_close
*	功能:	继电器模块关闭函数
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没有初始化
			-ERR_OTHER		-	其他关于线程互斥锁的错误
			-ERR_CFG		-	配置IO高低与继电器动作之间的联系出错
*	说明:	无
 ******************************************************************************/
int relay_close(void)
{
	int ret = -1;
	int i;

	if(relay_count == 0){
		return -ERR_NOINIT;
	}
	for(i = 0; i < CFG_RELAY_NUM; i ++){
		if(relay_chn[i] > 0){
			ret = relay_off(i);					//配置不动作
			if(ret < 0){
				return ret;
			}
		}
	}

	ret = close(relay_fd);
	if(ret < 0){
		return -ERR_SYS;	
	}
	relay_count = 0;
	if (pthread_mutex_destroy(&mutex)) {
		ret = -ERR_OTHER;
	}
	ret = 0;

	return ret;
}
#endif
