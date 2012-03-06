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
    文件名      ：  comporttest.c
    描述        ：  本文件用于调试和测试平台库comport
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.01													 
******************************************************************************/


//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//平台库头文件
#include "../include/comport.h"

int main()
{
	int ret=-1;	
	char func;
	char coms;
	int mode;
	int pin;
	u8 buf[50];
	u8 bufs[50]="hello,kitty!";
	int flush_mode;
	comport_config_t com_s={COMPORT_VERIFY_NO,8,1,0,9600,0};
	comport_config_t com_g;
	
	
	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  comport lib test		|\n");
	printf("| 	  Write on 2010.01.06 ROY		|\n");
	printf("+----------------------------------------+\n"); 
help:
	printf("\n Please select the function to test ( Main Menu):\n ");
	printf("\n  h: help ");
	printf("\n  i: init comport");
	printf("\n  s: config comport");
	printf("\n  g: get config of comport");
	printf("\n  r: read  comport");
	printf("\n  w: write single char comport");
	printf("\n  z: write string comport");
	printf("\n  f: flush comport buffer");
	printf("\n  c: close comport modoul");	
	printf("\n  ret = %d\n ",ret);

	while(1){
	 printf("please select function:");
	 scanf("%s",&func);
	 printf("\n");
	 switch(func)
	 {
		 case 'h':	//本测试程序帮助   
			goto help;
			break;			
		case 'r':	//本测试程序，读取串口输入
			
			ret = comport_recv(pin,buf,50);
			if(ret < 0 )
				goto help;
			buf[ret]= '\0';
			printf("read %d byte:%s\n",ret,buf);
			
			break;		
		case 'w':	//单字符写串口
			printf("please input charactor:");
			scanf("%s",&coms);
			printf("\n");
			buf[0]=coms;
			ret = comport_send(pin,buf,1);
			if(ret < 0 )
				goto help;
			printf("send %d byte:%c\n",ret,coms);
			break;
		case 'z':	//字符串写串口

			ret = comport_send(pin,bufs,sizeof bufs);
			if(ret < 0 )
				goto help;
			printf("send %d byte:%c\n",ret,coms);
			break;
		case 'i':	//初始化串口
			printf("please input port 0~7 \n");
			scanf("%d",&pin);
			printf("please input mode 0-232 1-485 \n");
			scanf("%d",&mode);
			ret = comport_init(pin,mode);
			if(ret < 0 ){
				printf("init error %d!\n",ret);
				goto help;
			}			
			printf("init successfully\n");

			break;
		case 's':	//设置串口
			ret = comport_setconfig(pin,&com_s);
			if(ret < 0 )
				goto help;
			printf("config successfully\n");
			break;
		case 'g':	//获取配置参数			
			ret = comport_getconfig(pin,&com_g);
			if(ret < 0 )
				goto help;
			printf("get band is %d\n",com_g.baud);
			break;
		case 'f':	//清空串口输入或输出缓冲区
			printf("please input flush mode: 0 wr,1 r,2 w \n");
			scanf("%d",&flush_mode);
			ret = comport_flush(pin,flush_mode);
			if(ret < 0 )
				goto help;		
			printf("flush successfully\n");
			break;
		case 'c':
			comport_close(pin);
			exit(1);						   
			break;		
		default: 
			comport_close(pin);
		 	exit(1); 
		}
	} 
	   


	printf ("system error\n");
	comport_close(pin);
	return 0;
}

