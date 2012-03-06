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
    文件名      ：  ledtest.c
    描述        ：  本文件用于调试和测试平台库adc
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.04
******************************************************************************/


//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//平台库头文件
#include "../include/led.h"
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
int main()
{
	int ret;	
	char func;
	int pin;
	u32 delay,last,period;

	ret = led_init();
	if(ret < 0 ){
		p_err(ret);
		goto error;
	}
	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  led lib test		|\n");
	printf("| 	  Write on 2010.04.13 ROY		|\n");
	printf("+----------------------------------------+\n"); 
help:
	printf("\n Please select the function to test ( Main Menu):\n ");
	printf("\n  h: help ");
	printf("\n  a: active led");
	printf("\n  d: deactive led");
	printf("\n  g: get led status");
	printf("\n  c: close led modoul");
	printf("\n ");

	while(1){
	 printf("please select function ");
	 scanf("%s",&func);
	 printf("\n ");
	 switch(func)
	 {
		 case 'h':	//本测试程序帮助   
			goto help;
			break;			
		case 'a':	//本测试程序，点亮led
			printf("please input id 0~7 \n");
			scanf("%d",&pin);
			printf("please input delay \n");
			scanf("%u",&delay);
			printf("please input last \n");
			scanf("%u",&last);
			printf("please input period \n");
			scanf("%u",&period);
			ret = led_on(pin,delay,last,period);
			if(ret < 0 ){
				p_err(ret);
				goto help;
			}
			printf("active led done\n");
			break;		
		case 'd':		//led熄灭
			printf("please input id \n");
			scanf("%d",&pin);
			ret = led_off(pin);
			if(ret < 0 ){
				p_err(ret);
				goto help;
			}
			break;
		case 'g':	//获取led状态 1-亮，0-灭
			printf("please input id \n");
			scanf("%d",&pin);
			ret = led_check(pin);
			if(ret < 0 ){
				p_err(ret);
				goto help;
			}
			printf("led status is %d\n",ret);
			break;
		case 'c':
			led_close();
			exit(1);						   
			break;	
		
		default: 
			led_close();
		 	exit(1); 
		}
	} 
	   

error:
	printf ("system error\n");
	led_close();
	return 0;
}

