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
	文件		：  key.c
	描述		：  本文件定义了状态灯模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/
//库配置头文件
#include "private/config.h"
	
//模块启用开关
#ifdef CFG_KEY_MODULE
//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//open 标志	
#include <sys/ioctl.h>		//ioctl
#include <string.h> 		//memset
#include <unistd.h>		//close

//提供给用户的头文件
#include "include/key.h"
#include "include/gpio.h"
#include "include/error.h"
	
//驱动调用头文件
#include "private/debug.h"
	
/*************************************************
  静态全局变量及宏定义
*************************************************/
typedef struct {
	u8	status;				//key值状态
	u8	level;				//当前io口电平
	u8	chn;				//key通道对应的io口物理通道
	u8	type;				//IO高低与按键是否按下之间的联系，1-高按下，0-低释放
}key_info_t;

#define KEYSTAT_UP		0				//当前按键释放
#define KEYSTAT_DOWN	1				//当前按键按下
#define KEYSTAT_LONG	2				//当前按键长按
#define PINBASE 		32				//传入到驱动的io口地址基址

#define MAX_KEY_CHN 16					//最大key数量

static u8 	key_count = 0;				//模快打开计数
static key_info_t	key[CFG_KEY_NUM];

/*************************************************
  API
*************************************************/

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

	if(key_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}

	//设置通道
#ifdef CFG_KEY_1
	key[KEY1].chn = CFG_KEY_1 ;
	key[KEY1].type = CFG_KEY_TYPE1;
#endif
#ifdef CFG_KEY_2
	key[KEY2].chn = CFG_KEY_2 ;
	key[KEY2].type = CFG_KEY_TYPE2;
#endif
#ifdef CFG_KEY_3
	key[KEY3].chn = CFG_KEY_3  ;
	key[KEY3].type = CFG_KEY_TYPE3;
#endif
#ifdef CFG_KEY_4
	key[KEY4].chn = CFG_KEY_4 ;
	key[KEY4].type = CFG_KEY_TYPE4;
#endif
#ifdef CFG_KEY_5
	key[KEY5].chn = CFG_KEY_5  ;
	key[KEY5].type = CFG_KEY_TYPE5;
#endif
#ifdef CFG_KEY_6
	key[KEY6].chn = CFG_KEY_6  ;
	key[KEY6].type = CFG_KEY_TYPE6;
#endif
#ifdef CFG_KEY_7
	key[KEY7].chn = CFG_KEY_7  ;
	key[KEY7].type = CFG_KEY_TYPE7;
#endif
#ifdef CFG_KEY_8
	key[KEY8].chn = CFG_KEY_8  ;
	key[KEY8].type = CFG_KEY_TYPE8;
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
		key[i].status = KEYSTAT_UP;
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
int key_get(u8 timeout)
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
int key_close(void)
{
	int ret = -1;

	if(key_count == 0){
		return -ERR_NOINIT;
	}

	ret = gpio_close();
	if(ret < 0){
		return ret;
	}
	key_count = 0;
	ret = 0;
	return ret;
}
#endif
