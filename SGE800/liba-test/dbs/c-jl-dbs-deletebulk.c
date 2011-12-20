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
    文件名      ：  c-jl-dbs-delete_bulk.c
    描述        ：  本文件用于平台库dbs_delete_bulk函数的测试
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
	dbs_set_t set, from, to;

	//环境的初始化
	inittest();
	ret = dbs_init();
	p_err(ret);

	//测试用例6-错误，不打开数据库
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 2;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == -ERR_NOFILE, "dbs_deletebulk Use Case 6 error");

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
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 4;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == 0, "dbs_deletebulk Use Case 1 error");

	//测试用例2-边界
	key = 100;
	data = 1;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);
	p_err(ret);

	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	data = 3;
	set.whence =  DBS_SEEK_SET;
	set.offset = 1;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 2;
	ret = dbs_delete_bulk(16, 4, &key, &from, &to);
	assert(ret == -ERR_INVAL, "dbs_deletebulk Use Case 2 error");

	//测试用例3-边界
	ret = dbs_delete_bulk(0, 9, &key, &from, &to);
	assert(ret == -ERR_INVAL, "dbs_deletebulk Use Case 3 error");

	//测试用例4-边界
	from.whence = 2;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 2;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == -ERR_INVAL, "dbs_deletebulk Use Case 4 error");

	//测试用例5-边界
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = 2;
	to.offset = 2;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == -ERR_INVAL, "dbs_deletebulk Use Case 5 error");

	//测试用例6-错误
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 3;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == -ERR_NORECORD, "dbs_deletebulk Use Case 6 error");

	//测试用例11-错误
	from.whence = DBS_SEEK_SET;
	from.offset = 2;
	to.whence = DBS_SEEK_SET;
	to.offset = 1;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == -ERR_INVAL, "dbs_deletebulk Use Case 11 error");

	//测试用例12-覆盖
	key = 200;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_END;
	to.offset = 0;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == -ERR_NORECORD, "dbs_deletebulk Use Case 12 error");

	//测试用例13-覆盖
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_END;
	to.offset = 0;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == 0, "dbs_deletebulk Use Case 13 error");

	//测试用例14-覆盖
	key = 100;
	data = 1;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);
	p_err(ret);

	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	data = 3;
	set.whence =  DBS_SEEK_SET;
	set.offset = 1;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	from.whence = DBS_SEEK_END;
	from.offset = 0;
	to.whence = DBS_SEEK_END;
	to.offset = 2;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == 0, "dbs_deletebulk Use Case 14 error");


	//测试用例15-覆盖
	key = 100;
	data = 1;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);
	p_err(ret);

	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	data = 3;
	set.whence =  DBS_SEEK_SET;
	set.offset = 1;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	from.whence = DBS_SEEK_END;
	from.offset = 2;
	to.whence = DBS_SEEK_END;
	to.offset = 0;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == -ERR_INVAL, "dbs_deletebulk Use Case 15 error");

	//测试用例16-覆盖
	from.whence = DBS_SEEK_END;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 0;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == 0, "dbs_deletebulk Use Case 16 error");

	//测试用例17-覆盖
	key = 100;
	data = 1;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);
	p_err(ret);

	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	data = 3;
	set.whence =  DBS_SEEK_SET;
	set.offset = 1;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	from.whence = DBS_SEEK_END;
	from.offset = 2;
	to.whence = DBS_SEEK_SET;
	to.offset = 2;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == -ERR_INVAL, "dbs_deletebulk Use Case 17 error");

	//测试用例18-覆盖
	from.whence = DBS_SEEK_SET;
	from.offset = 1;
	to.whence = DBS_SEEK_SET;
	to.offset = 2;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == 0, "dbs_deletebulk Use Case 18 error");

	//测试用例19-覆盖
	key = 100;
	data = 1;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);
	p_err(ret);

	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	from.whence = DBS_SEEK_END;
	from.offset = 0;
	to.whence = DBS_SEEK_END;
	to.offset = 0;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == 0, "dbs_deletebulk Use Case 19 error");

	//测试用例20-覆盖
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	from.whence = DBS_SEEK_SET;
	from.offset = 1;
	to.whence = DBS_SEEK_END;
	to.offset = 0;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == 0, "dbs_deletebulk Use Case 20 error");

	//测试用例21-覆盖
	key = 100;
	data = 1;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);
	p_err(ret);

	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 0;
	ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_BACKWARD, 4, &data);
	p_err(ret);

	from.whence = DBS_SEEK_END;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 2;
	ret = dbs_delete_bulk(0, 4, &key, &from, &to);
	assert(ret == 0, "dbs_deletebulk Use Case 21 error");

	ret = dbs_close(0);		//关闭并移除数据库文件
	p_err(ret);
	ret = dbs_remove(0);
	p_err(ret);
	finaltest();

	exit(0);
}

