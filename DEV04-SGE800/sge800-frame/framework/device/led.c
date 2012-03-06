/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  led.c
	描述		：  本文件定义了状态灯设备的操作接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
//库配置头文件
#include "framework/config.h"
	
//模块启用开关
#ifdef CFG_LED_DEVICE
//C库头文件
//#include <stdio.h>
//#include <fcntl.h> 		//open 标志
//#include <sys/ioctl.h>	//ioctl
//#include <string.h> 	//memset
#include <sys/select.h>		//select
//#include <signal.h>
	
//基础平台头文件
#include "sge_core/gpio.h"
#include "sge_core/error.h"
#include "sge_core/pinio.h"
#include "sge_core/thread.h"

//业务平台头文件
#include "framework/device/led.h"
#include "framework/message.h"
#include "framework/base.h"
	
/*************************************************
  静态全局变量及宏定义
*************************************************/

struct period_arg_t{
	u8 id;
	u32 last;
	u32 period;
};


#define MAX_LED_CHN 8						//最大led数量
static u8 	led_count = 0;					//模快打开计数
static pthread_mutex_t mutex;				//查询led状态互斥锁

static struct {
	u8 chn;					//led通道对应的物理通道
	u8 type;
	u8 period;				//led周期动作激活标志
	u8 cancel;				//线程取消标志
	pthread_mutex_t lock;
	pthread_t thread_id;	//周期动作线程id
	struct period_arg_t thread_arg;
}led[CFG_LED_NUM];

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
static int led_act(u8 id)
{
	int ret;

	if(led[id].type == 1){
		ret = gpio_output_set(led[id].chn, 1);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else if(led[id].type == 0){
		ret = gpio_output_set(led[id].chn, 0);
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
static int led_disact(u8 id)
{
	int ret;

	if(led[id].type == 1){
		ret = gpio_output_set(led[id].chn, 0);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else if(led[id].type == 0){
		ret = gpio_output_set(led[id].chn, 1);
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
*	功能:	状态灯设备初始化
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
	if(CFG_LED_NUM > MAX_LED_CHN){
		ret = -ERR_CFG; 	//配置超限
		goto err;
	}
	if(led_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}

	for(i=0;i<CFG_LED_NUM;i++ ){
		led[i].chn = 0;
		led[i].period = 0;
		led[i].cancel = 0;
		pthread_mutex_init(&led[i].lock, NULL);
	}

//设置通道
#ifdef CFG_LED_0
	led[0].chn = CFG_LED_0;
	led[0].type = CFG_LED_TYPE0;
#endif
#ifdef CFG_LED_1
	led[1].chn = CFG_LED_1;
	led[1].type = CFG_LED_TYPE1;
#endif
#ifdef CFG_LED_2
	led[2].chn = CFG_LED_2;
	led[2].type= CFG_LED_TYPE2;
#endif
#ifdef CFG_LED_3
	led[3].chn = CFG_LED_3  ;
	led[3].type = CFG_LED_TYPE3;
#endif
#ifdef CFG_LED_4
	led[4].chn = CFG_LED_4;
	led[4].type = CFG_LED_TYPE4;
#endif
#ifdef CFG_LED_5
	led[5].chn = CFG_LED_5;
	led[5].type = CFG_LED_TYPE5;
#endif
#ifdef CFG_LED_6
	led[6].chn = CFG_LED_6 ;
	led[6].type= CFG_LED_TYPE6;
#endif
#ifdef CFG_LED_7
	led[7].chn = CFG_LED_7;
	led[7].type = CFG_LED_TYPE7;
#endif

	//打开gpio库
	ret = gpio_init();
	if((ret < 0) && (ret != -ERR_BUSY)){
		goto err;
	}

	//设置继电器输出默认不动作
	for(i = 0; i < CFG_LED_NUM; i ++){
		if(led[i].chn > 0){					//配置io口为输出
			ret = led_disact(i);			//配置不动作
			if(ret < 0){
				goto err;
			}
			ret = gpio_set(led[i].chn, GPIO_OUT, GPIO_ODD, GPIO_PUD);
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
static void * led_thread_period(void * arg)
{
	int ret = -1;
	fd_set rfds;
	struct timeval tv;
	int fd = 1;
	struct period_arg_t *per_arg;

	tv.tv_sec = 0;
	FD_ZERO (&rfds);
	FD_SET (fd, &rfds);

	per_arg = (struct period_arg_t *)arg;

	led[per_arg->id].period = 1;	//设置线程运行标志位，供线程取消用。
	pthread_mutex_lock(&led[per_arg->id].lock);
	led[per_arg->id].cancel = 0;
	pthread_mutex_unlock(&led[per_arg->id].lock);

	//设置线程属性，收到cancel后立即退出
//	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//	printf("%s,%s,%d;thread %d ;ret = %d\n",__FILE__ ,__FUNCTION__,__LINE__,per_arg->id, ret);
//	ret = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED ,NULL);
//	printf("thread %d ;ret = %d\n",per_arg->id, ret);

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

		if(led[per_arg->id].cancel == 1){
			pthread_mutex_lock(&led[per_arg->id].lock);
			led[per_arg->id].cancel = 0;
			pthread_mutex_unlock(&led[per_arg->id].lock);
			pthread_exit(0);
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
					2		-	已经周期闪烁，先熄灭闪烁的led再操作
*	说明:	3个时间全为0时，表示一直亮, 此处时间精度不高，应>20ms.
 ******************************************************************************/
int led_on(u8 id, u32 delay, u32 last, u32 period)
{
	int ret = -1;

	fd_set rfds;					//select延时
	struct timeval tv;
	int fd = 1;

	if(led[id].period == 1){
		ret = 2;			//已经周期闪烁，先熄灭闪烁的led再操作
		goto err;
	}

	led[id].thread_arg.id = id;
	led[id].thread_arg.last = last;
	led[id].thread_arg.period = period;

    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);

	if(led_count == 0){
		return -ERR_NOINIT;
	}
	if((id < 0) || (id >= MAX_LED_CHN)){		//范围检查
		return -ERR_NODEV;
	}

	if(led[id].chn == 0){
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

//		ret = pthread_create(&led[id].thread_id, NULL, led_thread_period, (void *)&led[id].thread_arg);
		ret = thread_create_base(&led[id].thread_id, led_thread_period, (void *)&led[id].thread_arg, THREAD_MODE_REALTIME,80);
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
			ret = led_disact(id);
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

	if(led[id].chn == 0){
		return -ERR_CFG;
	}

	//周期动作，结束动作线程，并等待线程结束
	if(led[id].period == 1){
//		ret = pthread_cancel(led[id].thread_id);
//		if(ret < 0){
//			ret = -ERR_SYS;
//			goto err;
//		}
//		ret = pthread_kill(led[id].thread_id, SIGKILL);
//		if(ret < 0){
//			ret = -ERR_SYS;
//			goto err;
//		}
//		ret = pthread_join(led[id].thread_id,NULL);
//		if(ret < 0){
//			ret = -ERR_SYS;
//			goto err;
//		}
		pthread_mutex_lock(&led[id].lock);
		led[id].cancel = 1;
		pthread_mutex_unlock(&led[id].lock);
		led[id].period = 0;

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

	u8 vp = 0xff;

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

	if(led[id].chn == 0){
		return -ERR_CFG;
	}

	//获取io口电平
	ret = gpio_output_get(led[id].chn, &vp);
	if(ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	if(vp > 1){				//返回值非0、 1报错
		ret = -ERR_SYS;
		goto err;
	}

	//判断led状态
	if(led[id].type == 1){
		if(vp == 1){
			ret = 1;
		}else{
			ret = 0;
		}
	}else if(led[id].type== 0){
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
//int led_close(void)
//{
//	int ret = -1;
//	int i;
//
//	if(led_count == 0){
//		return -ERR_NOINIT;
//	}
//	for(i = 0; i < CFG_LED_NUM; i ++){
//		if(led[i].chn > 0){
//			ret = led_off(i);					//配置不动作
//			if(ret < 0){
//				return ret;
//			}
//		}
//	}
//
////	ret = close(led_fd);
//	if(ret < 0){
//		return -ERR_SYS;
//	}
//	led_count = 0;
//	if (pthread_mutex_destroy(&mutex)) {
//		ret = -ERR_OTHER;
//	}
//	ret = 0;
//
//	return ret;
//}
#endif
