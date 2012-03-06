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
    文件名     	 ：  c-qd-adc-read.c
    描述       		 ：  本文件用于平台库adc_read函数的测试
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
	int ret;
	u16 set_timeout = 12;
	u8 id;
	u16 result;

	//环境初始化
	inittest();
	ret = adc_init();
	if(ret != 0){
		printf("adc_init() error\r\n");
	}
	ret = adc_setwaittime(set_timeout);
	if(ret != 0){
		printf("adc_init() error\r\n");
	}
	//测试用例1
	id = 2;
	printf("executing adc_read()\r\n");
	ret = adc_read(id,&result);
	printf("result = 0x%x\r\n",result);
	assert(ret == 0,"adc_read 1 error");

	//测试用例2
/*	id = 0;
	ret = adc_read(id,&result);
	printf("result = 0x%x\r\n",result);
	assert(ret == 0,"adc_read 2 error");
*/
	//测试用例3
/*	id = 3;
	ret = adc_read(id,&result);
	printf("result = 0x%x\r\n",result);
	assert(ret == 0,"adc_read 3 error");
*/
	//测试用例4
/*	id = 10;
	ret = adc_read(id,&result);
	assert(ret == -ERR_NODEV,"adc_read 4 error");
*/
	//测试用例5
/*	id = 10;
	ret = adc_read(id,&result);
	assert(ret == 0,"adc_read 5 error");
*/
	//测试用例6
/*	id = 2;
	adc_close();
	ret = adc_read(id,&result);
	assert(ret == -ERR_NOINIT,"adc_read 6 error");
*/
	finaltest();
	return 0;
}


