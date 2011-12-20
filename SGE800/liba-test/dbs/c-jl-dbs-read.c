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
    文件名      ：  c-jl-dbs-read.c
    描述        ：  本文件用于平台库dbs_read函数的测试
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
	int ret;
	int key;
	int data,data_r;
	u16 data_size;
	dbs_set_t set;

	//环境的初始化
	inittest();
	ret = dbs_init();
	p_err(ret);

	//测试用例6-错误，不打开数据库
	key = 100;
	data = 255;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_read(0,4, &key,&set,8,&data_size,&data_r);
	assert(ret == -ERR_NOFILE, "dbs_read Use Case 6 error");

	//环境的初始化-读写方式，ram中打开
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_RW);
	p_err(ret);

	key = 100;
	data = 0;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);

	data = 1;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);

	data = 2;
	set.offset = 1;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);

	data = 3;
	set.offset = 2;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);

	//测试用例1-正常
	key = 100;
	set.whence = DBS_SEEK_SET;
	set.offset = 2;
	ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert((ret==0)&&(data_r == 2), "dbs_read Use Case 1 error");

	//测试用例2-边界
	key = 100;
	set.whence = DBS_SEEK_SET;
	set.offset = 2;
	ret = dbs_read(16, 4, &key, &set, 8, &data_size, &data_r);
	assert(ret == -ERR_INVAL, "dbs_read Use Case 2 error");

	//测试用例3-边界
	key = 100;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_read(0, 9, &key, &set, 8, &data_size, &data_r);
	assert(ret == -ERR_INVAL, "dbs_read Use Case 3 error");

	//测试用例4-边界
	key = 100;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_read(0, 4, &key, &set, 129, &data_size, &data_r);
	assert(ret == -ERR_INVAL, "dbs_read Use Case 4 error");

	//测试用例5-边界
	key = 100;
	set.whence = 2;
	set.offset = 0;
	ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert(ret == -ERR_INVAL, "dbs_read Use Case 5 error");

	//测试用例7-错误
	key = 100;
	set.whence = DBS_SEEK_END;
	set.offset = 6;
	ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert(ret == -ERR_NORECORD, "dbs_read Use Case 7 error");

	//测试用例12-覆盖
	key = 200;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert(ret == -ERR_NORECORD, "dbs_read Use Case 12 error");

	//测试用例13-覆盖
	key = 100;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert(ret == 0, "dbs_read Use Case 13 error");

	//测试用例14-覆盖
	key = 100;
	set.whence = DBS_SEEK_SET;
	set.offset = 3;
	ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert(ret == 0, "dbs_read Use Case 14 error");

	//测试用例15-覆盖
	key = 100;
	set.whence = DBS_SEEK_END;
	set.offset = 0;
	ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert(ret == 0, "dbs_read Use Case 15 error");

	//测试用例16-覆盖
	key = 100;
	set.whence = DBS_SEEK_END;
	set.offset = 3;
	ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert(ret == 0, "dbs_read Use Case 16 error");

	ret = dbs_close(0);		//关闭并移除数据库文件
	p_err(ret);
	ret = dbs_remove(0);
	p_err(ret);

	finaltest();

	exit(0);
}

