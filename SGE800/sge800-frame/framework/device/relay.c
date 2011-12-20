/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  relay.c
	描述		：  本文件定义了继电器设备的操作接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
//库配置头文件
#include "framework/config.h"
	
//模块启用开关
#ifdef CFG_RELAY_DEVICE
//C库头文件
#include <sys/select.h>		//select

	
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
#define MAX_RELAY_CHN 8						//最大继电器数量
struct period_arg_t{
	u8 id;
	u32 last;
	u32 period;
};


static u8 	relay_count = 0;				//模快打开计数
static pthread_mutex_t mutex;				//查询relay状态互斥锁

static struct {
	u8 chn;					//relay通道对应的物理通道
	u8 chd;					//继电器通道对应的双点物理通道
	u8 type;
	u8 period;				//relay周期动作激活标志
	u8 cancel;				//线程取消标志
	pthread_mutex_t lock;
	pthread_t thread_id;	//周期动作线程id
	struct period_arg_t thread_arg;
}relay[CFG_RELAY_NUM];

/*************************************************
  API
*************************************************/
/******************************************************************************
*	函数:	relay_act
*	功能:	relay动作
*	参数:	id
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与relay吸合断开之间的联系出错
*	说明:
 ******************************************************************************/
static int relay_act(u8 id)
{
	int ret;

	if(relay[id].type == 1){
		ret = gpio_output_set(relay[id].chn, 1);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		if(relay[id].chd > 0){		//双点模式
			ret = gpio_output_set(relay[id].chd, 0);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
		}
	}else if(relay[id].type == 0){
		ret = gpio_output_set(relay[id].chn, 0);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		if(relay[id].chd > 0){		//双点模式
			ret = gpio_output_set(relay[id].chd, 1);
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
*	功能:	relay不动作
*	参数:	id
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与relay动作之间的联系出错
*	说明:
 ******************************************************************************/
static int relay_disact(u8 id)
{
	int ret;

	if(relay[id].type == 1){
		ret = gpio_output_set(relay[id].chn, 0);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		if(relay[id].chd > 0){		//双点模式
			ret = gpio_output_set(relay[id].chd, 1);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
		}
	}else if(relay[id].type == 0){
		ret = gpio_output_set(relay[id].chn, 1);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		if(relay[id].chd > 0){		//双点模式
			ret = gpio_output_set(relay[id].chd, 0);
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
*	功能:	继电器设备初始化
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

	for(i=0;i<CFG_RELAY_NUM;i++ ){
		relay[i].chn = 0;
		relay[i].chd = 0;
		relay[i].period = 0;
		relay[i].cancel = 0;
		pthread_mutex_init(&relay[i].lock, NULL);
	}

//设置通道
#ifdef CFG_RELAY_0
	relay[0].chn = CFG_RELAY_0;
	relay[0].type = CFG_RELAY_TYPE0;
#endif
#ifdef CFG_RELAY_1
	relay[1].chn = CFG_RELAY_1;
	relay[1].type = CFG_RELAY_TYPE1;
#endif
#ifdef CFG_RELAY_2
	relay[2].chn = CFG_RELAY_2;
	relay[2].type= CFG_RELAY_TYPE2;
#endif
#ifdef CFG_RELAY_3
	relay[3].chn = CFG_RELAY_3  ;
	relay[3].type = CFG_RELAY_TYPE3;
#endif
#ifdef CFG_RELAY_4
	relay[4].chn = CFG_RELAY_4;
	relay[4].type = CFG_RELAY_TYPE4;
#endif
#ifdef CFG_RELAY_5
	relay[5].chn = CFG_RELAY_5;
	relay[5].type = CFG_RELAY_TYPE5;
#endif
#ifdef CFG_RELAY_6
	relay[6].chn = CFG_RELAY_6 ;
	relay[6].type= CFG_RELAY_TYPE6;
#endif
#ifdef CFG_RELAY_7
	relay[7].chn = CFG_RELAY_7;
	relay[7].type = CFG_RELAY_TYPE7;
#endif

//双点判断
#ifdef CFG_RELAY_D0
	relay[0].chd = CFG_RELAY_D0;
#endif
#ifdef CFG_RELAY_D1
	relay[1].chd = CFG_RELAY_D1;
#endif
#ifdef CFG_RELAY_D2
	relay[2].chd = CFG_RELAY_D2;
#endif
#ifdef CFG_RELAY_D3
	relay[3].chd = CFG_RELAY_D3 ;
#endif
#ifdef CFG_RELAY_D4
	relay[4].chd = CFG_RELAY_D4;
#endif
#ifdef CFG_RELAY_D5
	relay[5].chd = CFG_RELAY_D5;
#endif
#ifdef CFG_RELAY_D6
	relay[6].chd = CFG_RELAY_D6 ;
#endif
#ifdef CFG_RELAY_D7
	relay[7].chd = CFG_RELAY_D7;
#endif
	//打开gpio库
	ret = gpio_init();
	if((ret < 0) && (ret != -ERR_BUSY)){
		goto err;
	}

	//设置继电器输出默认不动作
	for(i = 0; i < CFG_RELAY_NUM; i ++){
		if(relay[i].chn > 0){					//配置io口为输出
			ret = relay_disact(i);			//配置不动作
			if(ret < 0){
				goto err;
			}
			ret = gpio_set(relay[i].chn, GPIO_OUT, GPIO_ODD, GPIO_PUD);
			if(ret < 0){
				ret = -ERR_SYS;
				goto err;
			}
			if(relay[i].chd > 0){					//配置双点io口为输出
				ret = gpio_set(relay[i].chd, GPIO_OUT, GPIO_ODD, GPIO_PUD);
				if(ret < 0){
					ret = -ERR_SYS;
					goto err;
				}
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
*	功能:	relay周期动作线程
*	参数:
*	返回:
*	说明:
 ******************************************************************************/
static void * relay_thread_period(void * arg)
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

	relay[per_arg->id].period = 1;	//设置线程运行标志位，供线程取消用。
	pthread_mutex_lock(&relay[per_arg->id].lock);
	relay[per_arg->id].cancel = 0;
	pthread_mutex_unlock(&relay[per_arg->id].lock);

	//设置线程属性，收到cancel后立即退出
//	ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//	printf("%s,%s,%d;thread %d ;ret = %d\n",__FILE__ ,__FUNCTION__,__LINE__,per_arg->id, ret);
//	ret = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED ,NULL);
//	printf("thread %d ;ret = %d\n",per_arg->id, ret);

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

		if(relay[per_arg->id].cancel == 1){
			pthread_mutex_lock(&relay[per_arg->id].lock);
			relay[per_arg->id].cancel = 0;
			pthread_mutex_unlock(&relay[per_arg->id].lock);
			pthread_exit(0);
		}

		tv.tv_usec = (per_arg->period - per_arg->last) * 1000;
		select (0, NULL, NULL, NULL, &tv);

	}
	pthread_exit(0);
}

/******************************************************************************
*	函数:	relay_on
*	功能:	relay条件动作
*	参数:	id				-	relay编号
*			delay			-	延时时间，单位为ms，为0表示没有延时
			last			-	持续时间，单位为ms，为0表示没有持续
			period			-	周期时间，单位为ms，为0表示没有周期
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_NOINIT		-	没有初始化
			-ERR_NODEV		-	没有此设备
			-ERR_CFG		-	配置出错
			-ERR_SYS		-	系统错误
					2		-	已经周期动作，请先停止relay，再操作
*	说明:	3个时间全为0时，表示一直动作, 此处时间精度不高，应>20ms.
 ******************************************************************************/
int relay_on(u8 id, u32 delay, u32 last, u32 period)
{
	int ret = -1;

	fd_set rfds;					//select延时
	struct timeval tv;
	int fd = 1;

	if(relay[id].period == 1){
		ret = 2;			//已经周期闪烁，先熄灭闪烁的relay再操作
		goto err;
	}

	relay[id].thread_arg.id = id;
	relay[id].thread_arg.last = last;
	relay[id].thread_arg.period = period;

    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);

	if(relay_count == 0){
		return -ERR_NOINIT;
	}
	if((id < 0) || (id >= MAX_RELAY_CHN)){		//范围检查
		return -ERR_NODEV;
	}

	if(relay[id].chn == 0){
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

//		ret = pthread_create(&relay[id].thread_id, NULL, relay_thread_period, (void *)&relay[id].thread_arg);
		ret = thread_create_base(&relay[id].thread_id, relay_thread_period, (void *)&relay[id].thread_arg, THREAD_MODE_REALTIME,80);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else{
		//设置relay动作,延迟last ms，设置relay灭
		if(last > 0){
			ret = relay_act(id);
			if(ret < 0){
				goto err;
			}
			tv.tv_sec = 0;
			tv.tv_usec = last*1000;
			select (0, NULL, NULL, NULL, &tv);
			ret = relay_disact(id);
			if(ret < 0){
				goto err;
			}
		}
	}

	//其它参数为0时，只动作relay
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
*	功能:	relay恢复
*	参数:	id				-	relay号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_CFG		-	配置IO高低与relay动作之间的联系出错
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

	if(relay[id].chn == 0){
		return -ERR_CFG;
	}

	//周期动作，结束动作线程，并等待线程结束
	if(relay[id].period == 1){
//		ret = pthread_cancel(relay[id].thread_id);
//		if(ret < 0){
//			ret = -ERR_SYS;
//			goto err;
//		}
//		ret = pthread_kill(relay[id].thread_id, SIGKILL);
//		if(ret < 0){
//			ret = -ERR_SYS;
//			goto err;
//		}
//		ret = pthread_join(relay[id].thread_id,NULL);
//		if(ret < 0){
//			ret = -ERR_SYS;
//			goto err;
//		}
		pthread_mutex_lock(&relay[id].lock);
		relay[id].cancel = 1;
		pthread_mutex_unlock(&relay[id].lock);
		relay[id].period = 0;

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
*	功能:	检查relay状态
*	参数:	id				-	relay通道号
*	返回:	1				-	亮
			0				-	灭
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

	u8 vp = 0xff;

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

	if(relay[id].chn == 0){
		return -ERR_CFG;
	}

	//获取io口电平
	ret = gpio_output_get(relay[id].chn, &vp);
	if(ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	if(vp > 1){				//返回值非0、 1报错
		ret = -ERR_SYS;
		goto err;
	}

	//判断relay状态
	if(relay[id].type == 1){
		if(vp == 1){
			ret = 1;
		}else{
			ret = 0;
		}
	}else if(relay[id].type== 0){
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
*	函数:	relay_close
*	功能:	relay模块关闭函数
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没有初始化
			-ERR_OTHER		-	其他关于线程互斥锁的错误
			-ERR_CFG		-	配置IO高低与relay亮灭之间的联系出错
*	说明:	无
 ******************************************************************************/
//int relay_close(void)
//{
//	int ret = -1;
//	int i;
//
//	if(relay_count == 0){
//		return -ERR_NOINIT;
//	}
//	for(i = 0; i < CFG_RELAY_NUM; i ++){
//		if(relay[i].chn > 0){
//			ret = relay_off(i);					//配置不动作
//			if(ret < 0){
//				return ret;
//			}
//		}
//	}
//
////	ret = close(relay_fd);
//	if(ret < 0){
//		return -ERR_SYS;
//	}
//	relay_count = 0;
//	if (pthread_mutex_destroy(&mutex)) {
//		ret = -ERR_OTHER;
//	}
//	ret = 0;
//
//	return ret;
//}
#endif
