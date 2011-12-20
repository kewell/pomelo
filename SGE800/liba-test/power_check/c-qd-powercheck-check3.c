
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
    文件名      ：  c-qd-powercheck-check.c
    描述        ：  本文件用于平台库powercheck_check函数的测试用例3
    版本        ：  0.1
    作者        ：  左伟杰
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include <db.h>

//平台库头文件
#include "../include/dbs.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

//测试程序头文件
#include "../include/powercheck.h"
#include "../include/gpio.h"

int main()
{
	/*power_check相关变量*/
	int ret = -1;
	u8 mode;
	u16 set_timeout = 1000;

	/*I/O口相关变量*/
	u8 io_mode, io_od, io_pu;
	u8 io_clearval = 0;
	u8 io_setval = 1;						//I/O口输出高
	u8 io_trigger = 35;						//pb3触发掉电处理


	//环境初始化
	inittest();

	//测试用例1
	io_mode = GPIO_OUT;
	io_od = GPIO_ODD;
	io_pu = GPIO_PUD;

	ret = gpio_init();											//初始化I/O口
	if(ret != 0){
		printf("gpio_init error\r\n");
	}

	ret = gpio_set(io_trigger, io_mode, io_od, io_pu);			//设置pb0为输出口
	if(ret != 0){
		printf("gpio_set error\r\n");
	}
	/*掉电检测时初始化为高，上电检测时初始化为低*/
//	gpio_output_set(io_trigger, io_clearval);					//PB3口输出低，模拟为掉电
	gpio_output_set(io_trigger, io_setval);						//PB3口输出低，模拟为上电



	//测试用例3
	/*掉电监测，函数返回值为0*/
	mode = POWERCHECK_MODE_BLOCK_UPDOWN;
	powercheck_init(mode);
	powercheck_setwaittime(set_timeout);
	ret = powercheck_check();
	assert(ret == 0,"powercheck_check 3 error");
	powercheck_close();

	/*上电检测，函数返回值为1*/
/*
	mode = POWERCHECK_MODE_BLOCK_UPDOWN;
	powercheck_init(mode);
	powercheck_setwaittime(set_timeout);
	ret = powercheck_check();
	assert(ret == 1,"powercheck_check 3 error");
	printf("ret = %d\r\n",ret3);
	powercheck_close();
*/
	finaltest();
	return 0;
}


