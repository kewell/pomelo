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
    文件名      ：  gpiotest.c
    描述        ：  本文件用于调试和测试平台库gpio
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2009.12													 
******************************************************************************/


//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit

#include <unistd.h>		//sleep

//平台库头文件
#include "../include/gpio.h"
#include "../include/pinio.h"

u32 pin;
static struct io_attr{
	u8 active;	//记录gpio的激活状态
	u8 mode;	//工作模式（1输入或0输出）
	u8 od;		//OD门使能标志，1表示使能
	u8 pu;		//上拉电阻使能标志，1表示使能
}gpio;

/******************************************************************************
 * 函数:	thread1
 * 功能:	线程，向一个io口发出脉冲
 * 参数:	 			
 * 返回:	
 * 说明:	无
 ******************************************************************************/

void* thread1 (void *arg) {
	int ret;
	while(1){
		ret = gpio_output_set(pin,1);
		if(ret < 0 )
			goto error;
		usleep(10000);
		ret = gpio_output_set(pin,0);
		if(ret < 0 )
			goto error;
		usleep(10000);		
			} 
error:	
	return NULL;
}

int main()
{
	int ret;
	u32 tmp;
	u32 val;
	u8 oval;		

	pthread_t tid;	
	char func;	
	ret = gpio_init();
	if(ret < 0 )
		goto error;
	
	
	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  GPIO lib test		|\n");
	printf("| 	  Write on 2009.12.24 ROY		|\n");
	printf("+----------------------------------------+\n"); 
help:
	printf("\n Please select the function to test ( Main Menu):\n ");
	printf("\n  h: help ");
	printf("\n  s: set gpio");
	printf("\n  o: set output val");
	printf("\n  g: get output val");
	printf("\n  i: get input val ");  
	printf("\n  t: test output pulse");	
	printf("\n  c: close gpio");	
	printf("\n ");

	for(;;){
	 printf("please select function \n");
	 scanf("%s",&func);
	 switch(func)
	 {
		 case 'h':	//本测试程序帮助   
			goto help;
			break;

		 case 's':	//本测试程序，设置io口属性
		 	do{
			 	printf("please input pin 0~95 \n");
				scanf("%d",&pin);
		 	}while((pin < 0) | (pin > 95));		//设置io口引脚

			do{
			 	printf("please input pin 0 output,1 input \n");
				scanf("%d",&tmp);
				
		 	}while((tmp!= 0) && (tmp != 1))	;	//设置io口功能
			gpio.mode = tmp;
			do{
			 	printf("please input pin 0 od gate disable, 1 od gate enable\n");
				scanf("%d",&tmp);
		 	}while((tmp!= 0) && (tmp != 1))	;	//设置io口od们
		 	gpio.od = tmp;

			do{
			 	printf("please input pin 0 pull up res disable, 1 pull up res enable\n");
				scanf("%d",&tmp);
		 	}while((tmp != 0) && (tmp != 1));		//设置io口pull up res
			gpio.pu = tmp;
			
			ret = gpio_set(pin, gpio.mode, gpio.od, gpio.pu);			
			if(ret < 0 )
				goto error;
							
			break;

		 case 'o':	//本测试程序，设置io口输出的值
		 	do{
			 	printf("please input val 0 or 1? \n");
				scanf("%d",&val);
		 	}while((val!=0) && (val !=1));
			ret = gpio_output_set(pin,val);
			if(ret < 0 )
				goto error;				  
			break;
			
		case 'g':	//本测试程序，获取io口输出的值
			
			ret = gpio_output_get(pin,&oval);
			if(ret < 0 )
				goto error;
			printf("output val is %x\n",oval);
			break;	
			
		case 'i':	//本测试程序，获取io口输入的值
			
			ret = gpio_input_get(pin,&oval);
			if(ret < 0 )
				goto help;
			printf("input val is %x\n",oval);
			break;	
		case 't':	//本测试程序，io口输出脉冲
			//建立thread1测试线程
			pthread_create (&tid, NULL, thread1, NULL);			
			printf("please chech out the %d pin output\n",pin);
			break; 
		
		case 'c':
			gpio_close();
			exit(1);						   
			break;	
		
		default: 
			  gpio_close();
			  exit(1); 
		}
	} 
	   

error:
	printf ("system error\n");
	gpio_close();
	return 0;
}
