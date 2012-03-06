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
    文件名     	 ：  c-qd-gpio-outputset-outputget.c
    描述       		 ：  本文件用于平台库gpio_outputset,gpio_outputget函数的测试
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
#include "../include/timer.h"

#define 	MAX_PIN		32*3

int main()
{
	int ret, ret1,ret2;
	u8 io,set_val,get_val;
	ret1 = -1;
	ret2 = -1;
	u8 mode,od,pu;

	//环境初始化
	inittest();
	gpio_init();

	//测试用例1
	io = 2;

	mode = GPIO_OUT;
	od = GPIO_ODD;
	pu = GPIO_PUD;

	ret = gpio_set(io, mode, od, pu);
	p_err(ret);
	set_val = 0;	//设置I/O口输出低电平
	ret1 = gpio_output_set(io, set_val);
	p_err(ret1);
	ret2 = gpio_output_get(io, &get_val);
	p_err(ret2);
	assert((ret1 == 0) && (ret2 == 0) && (get_val == 0),"gpio_output_set,gpio_output_get 1 error");

	//测试用例2
	set_val = 1;	//设置I/O口输出高电平
	ret1 = gpio_output_set(io, set_val);
	p_err(ret1);
	ret2 = gpio_output_get(io, &get_val);
	p_err(ret2);
	assert((ret1 == 0) && (ret2 == 0) && (get_val == 1),"gpio_output_set,gpio_output_get 2 error");

	//测试用例3
	io = 0;
	ret = gpio_set(io, mode, od, pu);
	p_err(ret);
	set_val = 0;	//设置I/O口输出低电平
	ret1 = gpio_output_set(io, set_val);
	p_err(ret1);
	ret2 = gpio_output_get(io, &get_val);
	p_err(ret2);
	assert((ret1 == 0) && (ret2 == 0) && (get_val == 0),"gpio_output_set,gpio_output_get 3 error");


	//测试用例5
	io = 2;
	set_val = 2;
	ret1 = gpio_output_set(io, set_val);
	assert(ret1 == -ERR_INVAL,"gpio_output_set 5 error");

	//测试用例8
	gpio_close();
	set_val = 0;
	io = 2;
	gpio_set(io, mode, od, pu);
	ret1 = gpio_output_set(io, set_val);
	assert(ret1 == -ERR_NOINIT,"gpio_output_set 8 error");

	finaltest();
	return 0;

}






