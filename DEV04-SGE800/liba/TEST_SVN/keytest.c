/*****************************************************************************/
/*许继电气股份有限公司                                     版权：2008-2015   */
/*****************************************************************************/
/* 本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许    */
/* 可不得擅自修改或发布，否则将追究相关的法律责任。                          */
/*                                                                           */
/*                      河南许昌许继股份有限公司                             */
/*                      www.xjgc.com                                         */
/*                      (0374) 321 2924                                      */
/*                                                                           */
/*****************************************************************************/


/******************************************************************************
    项目名称    ：  SGE800计量智能终端平台
    文件名      ：  keytest.c
    描述        ：  本文件用于调试和测试平台库key
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.04
******************************************************************************/


//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include <pthread.h>	//pthread库函数

//平台库头文件
#include "../include/key.h"
#include "../include/gpio.h"
#include "../include/pinio.h"
#define		GPIO_HIGH  PIN_PB1


void p_err(int exp)
{
	//_TestCount ++;
	if (exp != 0)
	{
	//	_ErrorCount ++;
//		printf(msg);
		switch(exp)
		{
			case -1:
				printf(" error: ERR_SYS\n");
				break;
			case -2:
				printf(" error: ERR_NODEV/ERR_NOFILE\n");
				break;
			case -3:
				printf(" error: ERR_TIMEOUT\n");
				break;
			case -4:
				printf(" error: ERR_INVAL\n");
				break;
			case -5:
				printf(" error: ERR_NOFUN\n");
				break;
			case -6:
				printf(" error: ERR_BUSY\n");
				break;
			case -7:
				printf(" error: ERR_NOINIT\n");
				break;
			case -8:
				printf(" error: ERR_NOMEM\n");
				break;
			case -9:
				printf(" error: ERR_NODISK\n");
				break;
			case -10:
				printf(" error: ERR_NORECORD\n");
				break;
			case -11:
				printf(" error: ERR_CFG\n");
				break;
			case -12:
				printf(" error: ERR_NOCFG\n");
				break;
			case -13:
				printf(" error: ERR_DEVUNSTAB\n");
				break;
			case -14:
				printf(" error: ERR_DISCONNECT\n");
				break;
			case -80:
				printf(" error: ERR_OTHER\n");
				break;
			default:
				printf("\n");
				break;
		}

	}
}

//void * thread_period(void *arg)
//{
//	u8 id;
//	fd_set rfds;
//	struct timeval tv;
//	int fd = 1;
//
//	id = *((u8 *)arg);
//	tv.tv_sec = 0;
//	FD_ZERO (&rfds);
//	FD_SET (fd, &rfds);
//
//	//设置线程属性，收到cancel后立即退出
//	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
//
//
//	while(1){
//		printf("key %d persist down\n",(id-100));
//		tv.tv_sec = 1;
//		tv.tv_usec = 0;
//		select (0, NULL, NULL, NULL, &tv);
//	}
//	pthread_exit(0);
//}
int main()
{
	int ret;	
	//pthread_t idkey;
	ret = key_init();
	if(ret < 0 ){
		p_err(ret);
	}

	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  key lib test	 |\n");
	printf("| 	  Write on 2010.04.21 ROY	 |\n");
	printf("+----------------------------------------+\n"); 

	while(1){
		ret = key_get(20);
		printf("ret = %d ",ret);

		if(ret == 0){
			printf("no key \n");;
		}else if(ret < 0){
			p_err(ret);
		}else if(ret < 100){
			printf("key %d down\n",ret);
		}else if(ret < 200){
			printf("key %d persist down\n",ret-100);
			// pthread_create(&idkey, NULL, thread_period, (void *)&ret);
//		}else if(ret > 200){
//			pthread_cancel(idkey);
//			pthread_join(idkey,NULL);
//			printf("key %d persist down over\n",ret-200);
		}
	}
	exit(0);
}

