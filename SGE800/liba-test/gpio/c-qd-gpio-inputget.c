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
    文件名     	 ：  c-qd-gpio-inputget.c
    描述       		 ：  本文件用于平台库gpio_inputget函数的测试，对应除测试用例2以外的测试用例
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
	int ret1,ret2;
	u8 io = 2,get_val;
	ret1 = -1;
	ret2 = -1;

	u8 mode,od,pu;
	mode = GPIO_IN;
	od = GPIO_ODE;
	pu = GPIO_PUE;

	//环境初始化
	inittest();

	gpio_init();
	gpio_set(io, mode, od, pu);

	//测试用例1
	ret1 = gpio_input_get(io, &get_val);
	assert((ret1 == 0) && (get_val == 0),"gpio_input_get 1 error");

	//测试用例2

	//测试用例3
	io = 0;
	gpio_set(io, mode, od, pu);
	ret1 = gpio_input_get(io, &get_val);
	assert((ret1 == 0) && (get_val == 1),"gpio_input_get 3 error");

	//测试用例4

/*	io = MAX_PIN - 1;
	gpio_set(io, mode, od, pu);
	ret1 = gpio_input_get(io, &get_val);
	assert((ret1 == 0) && (get_val == 0),"gpio_input_get 4 error");
*/
	//测试用例5
	io = MAX_PIN;
	ret1 = gpio_input_get(io, &get_val);
	assert(ret1 == -ERR_NODEV,"gpio_input_get 5 error");

	//测试用例6
	gpio_close();
	gpio_init();
	io = 2;
	ret1 = gpio_input_get(io, &get_val);
	assert(ret1 == -ERR_NOCFG,"gpio_input_get 6 error");

	//测试用例7
	gpio_close();
	io = 2;
	ret1 = gpio_input_get(io, &get_val);
	assert(ret1 == -ERR_NOINIT,"gpio_input_get 7 error");

	finaltest();
	return 0;
}






