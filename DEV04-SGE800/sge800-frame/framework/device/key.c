/******************************************************************************
	项目名称	：  SGE800计量智能终端业务平台
	文件		：  key.c
	描述		：  本文件定义了按键模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
//库配置头文件
#include "framework/config.h"
	
//模块启用开关
#ifdef CFG_KEY_DEVICE

//C库头文件
#include <sys/select.h>

//基础平台头文件
#include "sge_core/gpio.h"
#include "sge_core/error.h"
#include "sge_core/pinio.h"
#include "sge_core/thread.h"

//业务平台头文件
#include "framework/device/key.h"
#include "framework/message.h"
#include "framework/base.h"
	
/*************************************************
  静态全局变量及宏定义
*************************************************/
typedef struct {
	u8	status;				//key值状态
	u8	level;				//当前io口电平
	u8	chn;				//key通道对应的io口物理通道
	u8	type;				//IO高低与按键是否按下之间的联系，1-高按下，0-低释放
	u8	value;				//键值
	u8	msg_type;			//消息类型
}key_info_t;

#define KEYSTAT_UP		0				//当前按键释放
#define KEYSTAT_DOWN	1				//当前按键按下
#define KEYSTAT_LONG	2				//当前按键长按

#define NON_BLOCK		0

#define MAX_KEY_CHN 16					//最大key数量

static u8 	key_count = 0;				//模快打开计数
static key_info_t	key[MAX_KEY_CHN ];
static int key_get(u8 timeout);

/*************************************************
  API
*************************************************/
/******************************************************************************
*	函数:	thread_key_scan
*	功能:	扫描按键输入线程
*	参数:
*	返回:
*	说明:
 ******************************************************************************/
static void * thread_key_scan(void * arg)
{
	int ret=-1;
	message_t msg_key ;
	//输出周期为period，持续动作为last的电平
	while(1){
		ret = key_get(NON_BLOCK);
		if(ret > 0){
			if(ret < 100){
				msg_key.wpara 	= 	key[ret].value;
				msg_key.type	=	key[ret].msg_type;
				message_dispatch(&msg_key);
			}else{
				ret -= 100;
				msg_key.wpara 	= 	key[ret].value + 100;
				msg_key.type	=	key[ret].msg_type;
				message_dispatch(&msg_key);
			}
		}
	}
	pthread_exit(0);
}
/******************************************************************************
*	函数:	key_init
*	功能:	按键模块初始化
*	参数:	无
*	返回:	0				-	成功
			-ERR_BUSY		-	已经打开
	说明:本函数出错，大部分返回gpio.h中gpio_init及gpio_set函数的错误
 ******************************************************************************/
int key_init(void)
{
	int ret = -1;
	int i;
	pthread_t key_id;
	if(key_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}

	//设置通道
#ifdef CFG_KEY_ENTER
	key[6].chn 		= 	CFG_KEY_ENTER ;
	key[6].type	 	= 	CFG_KEY_ENTER_TYPE;
	key[6].value 	= 	KEY_ENTER;
	key[6].msg_type	= 	MSG_KEY;
#endif
#ifdef CFG_KEY_CANCEL
	key[1].chn 		= 	CFG_KEY_CANCEL ;
	key[1].type		= 	CFG_KEY_CANCEL_TYPE;
	key[1].value 	= 	KEY_CANCEL;
	key[1].msg_type	= 	MSG_KEY;
#endif
#ifdef CFG_KEY_LEFT
	key[2].chn		= 	CFG_KEY_LEFT  ;
	key[2].type		= 	CFG_KEY_LEFT_TYPE;
	key[2].value 	= 	KEY_LEFT;
	key[2].msg_type	= 	MSG_KEY;
#endif
#ifdef CFG_KEY_UP
	key[3].chn 		= 	CFG_KEY_UP ;
	key[3].type 	= 	CFG_KEY_UP_TYPE;
	key[3].value 	= 	KEY_UP;
	key[3].msg_type	= 	MSG_KEY;
#endif
#ifdef CFG_KEY_RIGHT
	key[4].chn 		= 	CFG_KEY_RIGHT  ;
	key[4].type 	= 	CFG_KEY_RIGHT_TYPE;
	key[4].value 	= 	KEY_RIGHT;
	key[4].msg_type	= 	MSG_KEY;
#endif
#ifdef CFG_KEY_DOWN
	key[5].chn 		= 	CFG_KEY_DOWN ;
	key[5].type 	= 	CFG_KEY_DOWN_TYPE;
	key[5].value 	= 	KEY_DOWN;
	key[5].msg_type	= 	MSG_KEY;
#endif
#ifdef CFG_KEY_PROGREM
	key[7].chn 		= 	CFG_KEY_PROGREM  ;
	key[7].type 	= 	CFG_KEY_PROGREM_TYPE;
	key[7].value 	=	KEY_PROGREM;
	key[7].msg_type	= 	MSG_SKEY;
#endif

	//打开gpio库
	ret = gpio_init();
	if((ret < 0) && (ret != -ERR_BUSY)){
		goto err;
	}
	for(i = 1; i <= CFG_KEY_NUM; i++){
		ret = gpio_set(key[i].chn,GPIO_IN,0,0);
		if(ret < 0){
			goto err;
		}
		key[i].status = KEYSTAT_UP;			//全部初始化为抬起状态
	}
	//创建按键扫描线程，实时线程，优先级80-90
	ret = thread_create_base(&key_id, thread_key_scan, NULL, THREAD_MODE_REALTIME, 80);
	if((ret < 0) ){
		goto err;
	}

	key_count = 1;
	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	key_get
*	功能:	获取按键有效状态
*	参数:	timeout			-	超时时间（单位为秒,为0表示永久阻塞））
*	返回:	0				-	无键按下
*			0~15			-	对应键号按下
*			100~115			-	对应键号长按
			-ERR_NOINIT		-	没有初始化
*	说明:本函数出错，大部分返回gpio.h中gpio_input_get函数的错误
 ******************************************************************************/
static int key_get(u8 timeout)
{
	int ret = -1;
	int i;
	u32 delay = 0;		//延时时间累积

	fd_set rfds;		//select延时
	struct timeval tv;
	int fd = 1;

    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);

	if(key_count == 0){
		return -ERR_NOINIT;
	}

scan:
	//扫描按键
	for(i = 1; i <= CFG_KEY_NUM; i++){
		ret = gpio_input_get(key[i].chn,&key[i].level);
		if(ret < 0){
			goto err;
		}
	}

	//处理扫描数据,若有几个键同时按下，则返回键号小的值
	for(i = 1; i <= CFG_KEY_NUM; i++){
		switch(key[i].status){

			case KEYSTAT_DOWN:
				if(key[i].level != key[i].type){
					key[i].status = KEYSTAT_UP;	//释放
					if(CFG_KEY_MODE == 0){
						return i;
					}

				}else{
					if(delay >= CFG_KEY_LONG){
						delay = 0;
						key[i].status = KEYSTAT_LONG;
						return (100+i);			//长按开始
					}
				}
				break;

			case KEYSTAT_LONG:
				if(key[i].level != key[i].type){
					key[i].status = KEYSTAT_UP;
					delay = 0;				//长按结束
				}
				if(delay >= CFG_KEY_PER){
					delay = 0;
					return (100+i);			//长按期间
				}
				break;

			case KEYSTAT_UP:
				if(key[i].level == key[i].type){
					key[i].status = KEYSTAT_DOWN;
					if(CFG_KEY_MODE == 1){
						return (i);					//产生按键,按下返回
					}

				}
				break;
			default:
				key[i].status = KEYSTAT_UP;
				break;
		}
	}

	tv.tv_sec = 0;
	tv.tv_usec = CFG_KEY_SCAN_DELAY*1000;				//延时
	select (0, NULL, NULL, NULL, &tv);
	delay += CFG_KEY_SCAN_DELAY;

	if((timeout <= 0) || (delay < timeout*1000)){		//阻塞模式
		goto scan;
	}else{
		ret = 0;		//超时无键按下
	}
err:
	return ret;
}


/******************************************************************************
*	函数:	key_close
*	功能:	按键模块关闭函数
*	参数:	无
*	返回:	0				-	成功
			-ERR_NOINIT		-	没有初始化
*	说明:本函数出错，大部分返回gpio.h中gpio_close函数的错误
 ******************************************************************************/
//static int key_close(void)
//{
//	int ret = -1;
//
//	if(key_count == 0){
//		return -ERR_NOINIT;
//	}
//
//	ret = gpio_close();
//	if(ret < 0){
//		return ret;
//	}
//	key_count = 0;
//	ret = 0;
//	return ret;
//}
#endif
