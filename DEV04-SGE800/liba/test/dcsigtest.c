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
    文件名      ：  dcsigtest.c
    描述        ：  本文件用于调试和测试平台库dcsig
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.01													 
******************************************************************************/


//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//平台库头文件
#include "../include/dcsig.h"

int main()
{
	int ret;	
	char func;
	int pin;
	u32 result;
	u16 timeout;

	ret = dcsig_init();
	if(ret < 0 ){
		printf("init error %d!\n",ret);
		goto error;
	}
	
	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  dcsig lib test		|\n");
	printf("| 	  Write on 2010.04.22 ROY		|\n");
	printf("+----------------------------------------+\n"); 
help:
	printf("\n Please select the function to test ( Main Menu):\n ");
	printf("\n  h: help ");
	printf("\n  s: set timeout value");
	printf("\n  g: get timeout value");
	printf("\n  r: get dcsig result");
	printf("\n  c: close dcsig modoul");
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
		case 'r':	//本测试程序，获取ad转换结果
		
			printf("please input channel 0~8 \n  4 refm\n  5 mid\n  6 refp\n  7 read cfr\n  8 read FIFO\n");
			// 4 for test voltage=refm 
			// 5 for test voltage=(refm+refp)/2
			// 6 for test voltage=refp
			// 7 read cfr
			// 8 read FIFO
			scanf("%d",&pin);
			ret = dcsig_read(pin,&result);
			if(ret < 0 )
				goto help;
			printf("channel %d dcsig result is %d\n",pin,result);
			break;		
		case 's':	
			printf("please input wait time value \n");
			scanf("%d",&pin);
			ret = dcsig_setwaittime(pin);
			if(ret < 0 )
				goto help;
			break;
		case 'g':				
			ret = dcsig_getwaittime(&timeout);
			if(ret < 0 )
				goto help;
			printf("wait time is %d\n",timeout);
			break;
		case 'c':
			dcsig_close();
			exit(1);						   
			break;	
		
		default: 
			dcsig_close();
		 	exit(1); 
		}
	} 
	   

error:
	printf ("system error\n");
	dcsig_close();
	return 0;
}

