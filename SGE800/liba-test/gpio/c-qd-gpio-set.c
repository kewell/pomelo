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
    项目名称   	 ：  SGE800计量智能终端平台
    文件名     	 ：  c-qd-gpio-set.c
    描述       		 ：  本文件用于平台库gpio_set函数的测试
    版本      		  ：  0.1
    作者        	：  左伟杰
    创建日期   	 ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include <db.h>

//平台库头文件
#include "../include/dbs.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

//测试程序头文件
#include "../include/gpio.h"

#define		MAX_PIN 	32*3

int main()
{
	int ret;
	u8 io,mode,od,pu;
	io = 2;
	ret = -1;

	//环境初始化
	inittest();

	gpio_init();

	//测试用例1
	mode = GPIO_OUT;
	od = GPIO_ODD;
	pu = GPIO_PUD;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == 0,"gpio_set 1 error");

	//测试用例2
	mode = GPIO_IN;
	od = GPIO_ODE;
	pu = GPIO_PUE;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == 0,"gpio_set 2 error");

	//测试用例3
	io = 0;
	mode = GPIO_IN;
	od = GPIO_ODE;
	pu = GPIO_PUE;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == 0,"gpio_set 3 error");

	//测试用例4
/*	io = MAX_PIN - 1;
	mode = GPIO_IN;
	od = GPIO_ODE;
	pu = GPIO_PUE;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == 0,"gpio_set 4 error");
*/

	//测试用例5
	io = MAX_PIN;
	mode = GPIO_OUT;
	od = GPIO_ODD;
	pu = GPIO_PUD;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == -ERR_NODEV,"gpio_set 5 error");

	//测试用例6
	io = 2;
	mode = 6;
	od = GPIO_ODD;
	pu = GPIO_PUD;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == -ERR_INVAL,"gpio_set 6 error");

	//测试用例7
	io = 2;
	mode = GPIO_OUT;
	od = 6;
	pu = GPIO_PUD;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == -ERR_INVAL,"gpio_set 7 error");

	//测试用例8
	io = 2;
	mode = GPIO_OUT;
	od = GPIO_ODD;
	pu = 6;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == -ERR_INVAL,"gpio_set 8 error");

	//测试用例9
	gpio_close();
	io = 2;
	mode = GPIO_OUT;
	od = GPIO_ODD;
	pu = GPIO_PUD;
	ret = gpio_set(io, mode, od, pu);
	assert(ret == -ERR_NOINIT,"gpio_set 9 error");
	//printf("ret = %d\r\n",ret);

	finaltest();
	return 0;
}




