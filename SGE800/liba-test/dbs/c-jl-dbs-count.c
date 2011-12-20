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
    文件名      ：  c-jl-dbs-count.c
    描述        ：  本文件用于平台库dbs_count函数的测试
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
#include "../include/xj_assert.h"
#include "../include/error.h"

int main()
{
	int ret,ret_t;
	int key;
	int data;
	dbs_set_t set;
	u32 num;
	//环境的初始化
	inittest();
	ret = dbs_init();

	//测试用例6-错误，不打开
	ret = dbs_count(0, 4, &key, &num);
	assert(ret == -ERR_NOFILE, "dbs_count Use Case 6 error");


	//环境的初始化
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_RW);
	p_err(ret);

	key = 100;
	data = 1;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);
	p_err(ret);

	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	key = 100;
	data = 3;
	set.whence =  DBS_SEEK_SET;
	set.offset = 1;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	key = 100;
	data = 4;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret_t);

	key = 100;
	data = 5;
	set.whence =  DBS_SEEK_SET;
	set.offset = 3;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret_t);

	//测试用例1
	ret = dbs_count(0, 4, &key, &num);
	assert((ret==0)&&(num == 5), "dbs_count Use Case 1 error");

	//测试用例2-边界
	ret = dbs_count(16, 4, &key, &num);
	assert(ret == -ERR_INVAL, "dbs_count Use Case 2 error");

	//测试用例3-边界
	ret = dbs_count(0, 8+1, &key, &num);
	assert(ret == -ERR_INVAL, "dbs_count Use Case 3 error");

	//测试用例7-错误，无此关键字
	key = 300;
	ret = dbs_count(0, 4, &key, &num);
	assert(ret == -ERR_NORECORD, "dbs_count Use Case 7 error");

	ret = dbs_close(0);		//关闭并移除数据库文件
	p_err(ret);
	ret = dbs_remove(0);
	p_err(ret);
	finaltest();

	exit(0);
}

