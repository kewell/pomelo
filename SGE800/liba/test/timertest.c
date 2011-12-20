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
    文件名      ：  timertest.c
    描述        ：  本文件用于调试和测试平台库timer
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2009.12													 
******************************************************************************/


//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//平台库头文件
#include "../include/timer.h"

int main()
{
	int ret;	
	char func;
	int pin;
	int mode;
	u32 interval;
	int tmp=0;
	
	printf("please input pin 0~5 \n");
	scanf("%d",&pin);
	printf("please input mode 1-heart, 2-measure,3-PWM  \n");
	scanf("%d",&mode);
	ret = timer_init(pin, mode);
	if(ret < 0 ){
		printf("init error %d!\n",ret);
		goto error;
	}
	
	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  timer lib test		|\n");
	printf("| 	  Write on 2010.01.05 ROY		|\n");
	printf("+----------------------------------------+\n"); 
help:
	printf("\n Please select the function to test ( Main Menu):\n ");
	printf("\n  h: help ");
	printf("\n  t: config timer heart mode");
	printf("\n  p: config timer pwm mode");
	printf("\n  m: config timer measure mode");
	printf("\n  s: start heart mode");
	printf("\n  c: close timer modoul");	
	printf("\n ");
	printf("ret = %d \n", ret);
	while(1){
	 printf("please select function \n");
	 scanf("%s",&func);
	 switch(func)
	 {
		 case 'h':	//本测试程序帮助   
			goto help;
			break;			
		case 't':	//本测试程序，配置心跳模式
			printf("please input val  \n");
			scanf("%d",&interval);

			ret = timer_heart_setconfig(pin,interval);
			if(ret < 0 )
				goto help;
			//获取配置
			ret = timer_heart_getconfig(pin,&interval);			
			if(ret < 0 )
				goto help;
			printf("get interval = %d\n",interval);
			break;	
		case 's':	//本测试程序，测试心跳

			tmp = 0;
			ret = timer_heart_start(pin);
			if(ret < 0 )
				goto help;
			while(1){				
				ret = timer_heart_wait(pin);
				if(ret < 0 )
					goto help;
				if(++tmp == 10){
					printf("heart is %d\n",interval);
					tmp = 0;
				}
			}
			break;	
		
		case 'c':
			timer_close(pin);
			exit(1);						   
			break;	
		
		default: 
		  timer_close(pin);
		  exit(1); 
		}
	} 
	   

error:
	printf ("system error\n");	
	return 0;
}

