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
    文件名      ：  c-jl-dbs_open.c
    描述        ：  本文件用于平台库dbs_open函数的测试
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include <db.h>

//平台库头文件
#include "../include/dbs.h"
#include "../include/error.h"
#include "../include/xj_assert.h"

int main()
{
	int ret;
	//环境的初始化,确保DBS_POS_RAM等指定目录没有.db文件
	inittest();

	//测试用例5- 错误，未初始化环境
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_RW);
	assert(ret == -ERR_NOINIT,"dbs_open Use Case 5 error");

	//环境的初始化
	ret = dbs_init();
	p_err(ret);

	//测试用例1-正常
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_RW);
	assert(ret == 0,"dbs_open Use Case 1 error");
	p_err(ret);

	//测试用例6-错误，重复打开
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_RW);
	assert(ret == -ERR_BUSY,"dbs_open Use Case 6 error");

	ret = dbs_close(0);
	p_err(ret);

	//测试用例8-错误，重复创建文件
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_CREAT);
	assert(ret == -ERR_BUSY,"dbs_open Use Case 8 error");

	ret = dbs_remove(0);
	p_err(ret);

	//测试用例2-边界
	ret = dbs_open(16, DBS_POS_RAM, DBS_MODE_RW);
	assert(ret == -ERR_INVAL,"dbs_open Use Case 2 error");

	//测试用例3-边界
	ret = dbs_open(0, DBS_POS_RAM, 4);
	assert(ret == -ERR_INVAL,"dbs_open Use Case 3 error");


	//测试用例4-边界
	ret = dbs_open(0, 6, DBS_MODE_RW);
	assert(ret == -ERR_INVAL,"dbs_open Use Case 4 error");

	//测试用例7-错误，没有创建文件
	ret = dbs_open(1, DBS_POS_RAM, DBS_MODE_OPEN);
	assert(ret == -ERR_NOFILE,"dbs_open Use Case 7 error");

	//测试用例9-覆盖
	ret = dbs_open(2, DBS_POS_FLASH_CODE, DBS_MODE_RW);
	assert(ret == 0,"dbs_open Use Case 9 error");
	p_err(ret);

	ret = dbs_close(2);
	p_err(ret);
	ret = dbs_remove(2);
	p_err(ret);

	//测试用例11-覆盖
	ret = dbs_open(0, DBS_POS_SD, DBS_MODE_RW);
	assert(ret == 0,"dbs_open Use Case 11 error");
	p_err(ret);

	ret = dbs_close(0);
	p_err(ret);
	ret = dbs_remove(0);
	p_err(ret);

	//测试用例12-覆盖
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_CREAT);
	p_err(ret);
	ret = dbs_close(0);
	p_err(ret);
	ret = dbs_open(0, DBS_POS_FLASH_CODE, DBS_MODE_CREAT);
	assert(ret == -ERR_INVAL,"dbs_open Use Case 12 error");

	ret = dbs_remove(0);
	p_err(ret);

	//测试用例13-覆盖
	ret = dbs_open(2, DBS_POS_RAM, DBS_MODE_RD);
	assert(ret == -ERR_NOFILE,"dbs_open Use Case 13 error");

	//测试用例14-覆盖
	ret = dbs_open(10, DBS_POS_RAM, DBS_MODE_CREAT);
	assert(ret == 0 ,"dbs_open Use Case 14 error");


	ret = dbs_close(10);
	p_err(ret);
	ret = dbs_remove(10);
	p_err(ret);

	//测试统计
	finaltest();
	exit(0);
}

