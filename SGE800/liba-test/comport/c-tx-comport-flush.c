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
    文件名         ：  c-tx-comport-flush.c
    描述	       ：  本文件用于平台库comport_flush函数的测试
    版本              ：  0.1
    作者              ：  孙锐
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>			//exit
#include <unistd.h>			//sleep
#include <db.h>
#include <string.h>

//平台库头文件
#include "../include/comport.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

int main()
{
	int ret,ref;
	inittest();

	//测试环境的初始化
	u8 port1 = 2;
	u8 mode = COMPORT_MODE_NORMAL;
	ret = comport_init(port1, mode);    //普通串口
	if(ret < 0){
		printf("init error!\n");
		goto error;
	}
	u8 port2 = 5;
	mode = COMPORT_MODE_NORMAL;
	ret = comport_init(port2, mode);
	if(ret < 0){
		printf("init error!\n");
		goto error;
	}
	comport_config_t fig = {COMPORT_VERIFY_NO, 8,  1, 20, 9600, COMPORT_RTSCTS_DISABLE};  //阻塞方式
	ret = comport_setconfig (port1, &fig);
	if(ret<0){
		printf("port1 setconfig error!\n");
		goto error;
	}
	ret = comport_setconfig (port2, &fig);
	if(ret<0){
		printf("port2 setconfig error!\n");
		goto error;
	}

	/***************测试用例1*****************/
	u8 buf[6]={6,8,5,0,1,6};    //先port1发送数据到port2
	ret = comport_send(port1, buf, 6);
	if(ret < 0){
		printf("test1:send error!\n");
		goto error;
	}
	sleep(3);    				//等待写入缓冲
	u8 buf_rev[20];
	memset(buf_rev,0,20);
	ret = comport_flush(port2, COMPORT_FLUSH_ALL);   //清空port2发送接收缓冲
	ref = comport_recv(port2, buf_rev, 6);
	assert((ret==0) && (ref<0),"test1:flush error");

	/***************测试用例2*****************/
	ret = comport_send(port1, buf, 6);
	if(ret < 0){
		printf("test2:send error!\n");
		goto error;
	}
	sleep(3);
	ret = comport_flush(port2, COMPORT_FLUSH_RD);   //清空port2接收缓冲
	ref = comport_recv(port2, buf_rev, 6);
	assert((ret==0) && (ref<0),"test2:flush error");

	/***************测试用例3*****************/
	ret = comport_send(port1, buf, 6);
	if(ret < 0){
		printf("test3:send error!\n");
		goto error;
	}
	sleep(3);
	ret = comport_flush(port1, COMPORT_FLUSH_WR);   //清空port1发送缓冲
	assert(ret==0,"test3:flush error");

	/***************测试用例4(边界测试)*****************/
	port1 = 0;
	mode = COMPORT_MODE_NORMAL;
	ret = comport_init(port1, mode);
	if(ret < 0){
		printf("test4:init error!\n");
		goto error;
	}
	ret = comport_flush(port1, COMPORT_FLUSH_ALL);   //清空port1发送接收缓冲
	assert(ret==0,"test4:flush error");

	/***************测试用例5(边界测试)*****************/
	port1 = 6;
	mode = COMPORT_MODE_NORMAL;
	ret = comport_init(port1, mode);
	if(ret < 0){
		printf("test5:init error!\n");
		goto error;
	}
	ret = comport_flush(port1, COMPORT_FLUSH_ALL);   //清空port1发送接收缓冲
	assert(ret==0,"test5:flush error");

	/***************测试用例6(错误测试)*****************/
	port1 = 10;                                      //接口错误
	ret = comport_flush(port1, 12);
	assert(ret== -ERR_INVAL,"test6:flush error");

	/***************测试用例7(错误测试)*****************/
	port1 = 2;
	ret = comport_flush(port1, 12);                  //mode接口错误
	assert(ret== -ERR_INVAL,"test7:flush error");

	/***************测试用例8(错误测试)*****************/
	port1 = 4;
	ret = comport_flush(port1, COMPORT_FLUSH_RD);   //清空未打开串口
	assert(ret== -ERR_NOINIT,"test8:flush error");


	finaltest();
error:
	exit(0);
}
