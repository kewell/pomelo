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
    文件名      ：  c-jl-dbs-write.c
    描述        ：  本文件用于平台库dbs_write函数的测试
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
	int data,data_r;
	u16 data_size;
	dbs_set_t set;

	//环境的初始化
	inittest();
	ret = dbs_init();
	p_err(ret);

	//测试用例7- 错误，不打开数据库
	key = 100;
	data = 255;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_write( 0, 4, &key, &set, 4, &data);
	assert(ret == -ERR_NOFILE, "dbs_write Use Case 7 error");

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
	key = 100;
	data = 103;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;
	ret = dbs_write(0, 4, &key, &set, 4, &data);
	ret_t = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert((ret==0)&&(data_r == data), "dbs_write Use Case 1 error");

	//测试用例2
	key = 100;
	data = 255;
	set.whence =  DBS_SEEK_END;
	set.offset = 1;
	ret = dbs_write(0, 4, &key, &set, 4, &data);
	ret_t = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert((ret==0)&&(data_r == data), "dbs_write Use Case 2 error");

	//测试用例3- 边界
	key = 100;
	data = 255;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_write( 16, 4, &key, &set, 4, &data);
	assert(ret == -ERR_INVAL, "dbs_write Use Case 3 error");

	//测试用例4- 边界
	key = 100;
	data = 255;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_write( 0, 8+1, &key, &set, 4, &data);
	assert(ret == -ERR_INVAL, "dbs_write Use Case 4 error");

	//测试用例5- 边界
	key = 100;
	data = 255;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_write( 0, 4, &key, &set, 128+1, &data);
	assert(ret == -ERR_NODISK, "dbs_write Use Case 5 error");

	//测试用例6- 边界
	key = 100;
	data = 255;
	set.whence = 2;
	set.offset = 0;
	ret = dbs_write( 0, 4, &key, &set, 4, &data);
	assert(ret == -ERR_INVAL, "dbs_write Use Case 6 error");

	//测试用例14- 覆盖
	key = 200;
	data = 255;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_write( 0, 4, &key, &set, 4, &data);
	assert(ret == 0, "dbs_write Use Case 14 error");

	//测试用例15- 覆盖
	key = 100;
	data = 101;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_write( 0, 4, &key, &set, 4, &data);
	ret_t = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert((ret==0)&&(data_r == data), "dbs_write Use Case 15 error");

	//测试用例16- 覆盖
	key = 100;
	data = 102;
	set.whence = DBS_SEEK_SET;
	set.offset = 1;
	ret = dbs_write( 0, 4, &key, &set, 4, &data);
	ret_t = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert((ret==0)&&(data_r == data), "dbs_write Use Case 16 error");

	//测试用例17- 覆盖
	key = 100;
	data = 205;
	set.whence = DBS_SEEK_END;
	set.offset = 0;
	ret = dbs_write( 0, 4, &key, &set, 4, &data);
	ret_t = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert((ret==0)&&(data_r == data), "dbs_write Use Case 17 error");

	//测试用例18- 覆盖
	key = 100;
	data = 202;
	set.whence = DBS_SEEK_END;
	set.offset = 3;
	ret = dbs_write( 0, 4, &key, &set, 4, &data);
	ret_t = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
	assert((ret==0)&&(data_r == data), "dbs_write Use Case 18 error");

	//测试用例8-错误，只读打开数据库
	ret = dbs_close(0);
	p_err(ret);
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_RD);
	p_err(ret);

	key = 100;
	data = 255;
	set.whence = DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_write( 0, 4, &key, &set, 4, &data);
	assert(ret == -ERR_NOFUN, "dbs_write Use Case 8 error");


	ret = dbs_close(0);		//关闭并移除数据库文件
	p_err(ret);
	ret = dbs_remove(0);
	p_err(ret);

	finaltest();

	exit(0);
}

