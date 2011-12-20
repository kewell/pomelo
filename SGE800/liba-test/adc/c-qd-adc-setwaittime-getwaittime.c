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
    文件名     	 ：  c-qd-adc-setwaittime.c
    描述       		 ：  本文件用于平台库adc_setwaittime、adc_getwaittime函数的测试
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
#include "../include/adc.h"

int main()
{
	int ret1,ret2;
	u16 set_timeout;
	u16 get_timeout;

	//环境初始化
	inittest();
	adc_init();

	//测试用例1
	set_timeout = 12;
	ret1 = adc_setwaittime(set_timeout);
	ret2 = adc_getwaittime(&get_timeout);
	assert(((ret1 == 0) && (ret1 == 0) && (set_timeout == get_timeout)),\
			"adc_setwaittime,adc_getwaittime 1 error");

	//测试用例2
	set_timeout = 0xffff;
	ret1 = adc_setwaittime(set_timeout);
	ret2 = adc_getwaittime(&get_timeout);
	assert(((ret1 == 0) && (ret1 == 0) && (set_timeout == get_timeout)),\
			"adc_setwaittime,adc_getwaittime 2 error");

	//测试用例3
	set_timeout = 0;
	ret1 = adc_setwaittime(set_timeout);
	assert(ret1 == -ERR_INVAL,\
			"adc_setwaittime,adc_getwaittime 3 error");

	//测试用例4
	adc_close();
	set_timeout = 12;
	ret1 = adc_setwaittime(set_timeout);
	assert(ret1 == -ERR_NOINIT,\
			"adc_setwaittime,adc_getwaittime 4 error");
	printf("ret1 = %d\r\n",ret1);



	finaltest();
	return 0;
}


