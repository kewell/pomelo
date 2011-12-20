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
    文件名         ：  c-tx-comport-close.c
    描述	       ：  本文件用于平台库comport_close函数的测试
    版本              ：  0.1
    作者              ：  孙锐
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>			//exit
#include <unistd.h>			//sleep
#include <db.h>

//平台库头文件
#include "../include/comport.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

int main()
{
	int ret;
	inittest();

	//测试环境的初始化
	u8 port1 = 5;
	u8 mode1 = COMPORT_MODE_NORMAL;
	ret = comport_init(port1, mode1);    //普通串口
	if(ret < 0){
		printf("port1 init error!\n");
		goto error;
	}

	u8 port2 = 1;
	u8 mode2 = COMPORT_MODE_485;
	ret = comport_init(port2, mode2);    //485串口
	if(ret < 0){
		printf("port2 init error!\n");
		goto error;
	}

	port1 = 0;
	ret = comport_init(port1, mode1);    //普通串口
	if(ret < 0){
		printf("port1 init error!\n");
		goto error;
	}

	port1 = 6;
	ret = comport_init(port1, mode1);    //普通串口
	if(ret < 0){
		printf("port1 init error!\n");
		goto error;
	}


	/***************测试用例1（正常功能）*****************/
	port1 = 5;
	ret = comport_close(port1);
	assert(ret==0,"test1:comport close error");
	p_err(ret);

	/***************测试用例2（正常功能）*****************/
	ret = comport_close(port2);
	assert(ret==0,"test2:comport close error");
	p_err(ret);

	/***************测试用例3（边界）*****************/
	port1 = 0;
	ret = comport_close(port1);
	assert(ret==0,"test3:comport close error");

	/***************测试用例4（边界）*****************/
	port1 = 6;
	ret = comport_close(port1);
	assert(ret==0,"test4:comport close error");

	/***************测试用例5（错误）*****************/
	port1 = 10;
	ret = comport_close(port1);
	assert(ret==-ERR_INVAL,"test5:comport close error");

	/***************测试用例6（错误）*****************/
	port1 = 2;
	ret = comport_close(port1);
	assert(ret==-ERR_NOINIT,"test6:comport close error");


	finaltest();
error:
	exit(0);
}
