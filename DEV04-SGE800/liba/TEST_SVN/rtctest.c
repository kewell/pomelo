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
    文件名      ：  rtctest.c
    描述        ：  本文件用于调试和测试平台库rtc
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2009.12													 
******************************************************************************/


//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//平台库头文件
#include "../include/rtc.h"

int main()
{
	int ret;
	rtc_time_t time;
	char func;
	int tmp;
	unsigned char stat;

	ret = rtc_init();
	if(ret < 0 ){
		printf("init error ret = %d!\n",ret);
		goto error;
	}
	
	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  rtc lib test		|\n");
	printf("| 	  Write on 2009.12.28 ROY		|\n");
	printf("+----------------------------------------+\n"); 
help:
	printf("\n Please select the function to test ( Main Menu):\n ");
	printf("\n  h: help ");
	printf("\n  s: set rtc time");
	printf("\n  g: get rtc time");
	printf("\n  u: get rtc status");
	printf("\n  c: close gpio");	
	printf("\n ");

	while(1){
	 printf("please select function \n");
	 scanf("%s",&func);
	 switch(func)
	 {
		 case 'h':	//本测试程序帮助   
			goto help;
			break;

		 case 's':	//本测试程序，设置时间
//		 	do{
			 	printf("please input week 1~7 \n");
				scanf("%d",&tmp);
//		 	}while((tmp < 1) | (tmp > 7));		//设置周
		 	time.wday = tmp;

//			do{
			 	printf("please input year 2000~2255 \n");
				scanf("%d",&tmp);				
//		 	}while((tmp < 2000) && (tmp > 2255));	//设置年功能
		 	tmp -= 2000;
			time.year = tmp;

//			do{
			 	printf("please input month 1~12 \n");
				scanf("%d",&tmp);				
//		 	}while((tmp < 1) && (tmp > 12))	;	//设置月功能
			time.mon = tmp;

//			do{
			 	printf("please input day 1~31 \n");
				scanf("%d",&tmp);				
//		 	}while((tmp < 1) && (tmp > 31))	;	//设置日功能
			time.day = tmp;

//			do{
			 	printf("please input hour 0~23 \n");
				scanf("%d",&tmp);				
//		 	}while((tmp < 0) && (tmp > 23))	;	//设置时功能
			time.hour = tmp;
		
//			do{
			 	printf("please input minute 0~59 \n");
				scanf("%d",&tmp);				
//		 	}while((tmp < 0) && (tmp > 59))	;	//设置分功能
			time.min = tmp;

//			do{
			 	printf("please input second 0~59 \n");
				scanf("%d",&tmp);				
//		 	}while((tmp < 0) && (tmp > 59))	;	//设置秒功能
			time.sec = tmp;
			
			
			ret = rtc_settime(&time);			
			if(ret < 0 ){
				printf("set error!!!\n\n");
				goto help;
			}		
			break;
	
			
		case 'g':	//本测试程序，获取io口输出的值
			
			ret =  rtc_gettime (&time);
			if(ret < 0 )
				goto error;
			printf("get time is: %d.%d.%d %d:%d:%d %d\n",
					2000+time.year,time.mon,time.day,
					time.hour,time.min,time.sec,time.wday);
			break;

		case 'u':	//本测试程序，获取io口输出的值

			ret =  rtc_getstat (&stat);
			if(ret < 0 )
				goto error;
			printf("get status is: %d\n",stat);
			break;
		case 'c':
			rtc_close();
			exit(1);						   
			break;	
		
		default: 
		  rtc_close();
		  exit(1); 
		}
	} 
	   

error:
	printf ("system error\n");
	rtc_close();
	return 0;
}

