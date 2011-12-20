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
    文件名      ：  c-jl-dbs-read_bulk.c
    描述        ：  本文件用于平台库dbs_read_bulk函数的测试
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include <db.h>
#include <string.h>
//平台库头文件
#include "../include/dbs.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

int main()
{
	int ret,ret_t;
	int key;
	int data;
	char data_r[50];
#pragma pack(1)
	struct {
		u16 len;
		int data;
	}data_bulk[8];
#pragma pack()
	u16 data_size;
	dbs_set_t set, from, to;

	//环境的初始化
	inittest();
	ret = dbs_init();
	p_err(ret);

	//测试用例7-错误，不打开数据库
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 3;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 24, &data_size, data_r );
	assert(ret == -ERR_NOFILE, "dbs_readbulk Use Case 7 error");

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
	to.offset = 3;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 24, &data_size, data_r );
	memcpy(&data_bulk,data_r,24);
//	for(i=0; i<4; i++){
//		printf("%d;",data_bulk[i].len);
//		printf("%d    ",data_bulk[i].data);
//	}
	assert((ret==0)&&
			(data_bulk[0].data == 1)&&
			(data_bulk[1].data == 2)&&
			(data_bulk[2].data == 3)&&
			(data_bulk[3].data == 4)&&
			(data_size == 24), "dbs_readbulk Use Case 1 error");

	//测试用例2-边界
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 3;
	ret = dbs_read_bulk(16, 4, &key, &from, &to, 24, &data_size, data_r );
	assert(ret == -ERR_INVAL, "dbs_readbulk Use Case 2 error");

	//测试用例3-边界
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 3;
	ret = dbs_read_bulk(0, 9, &key, &from, &to, 24, &data_size, data_r );
	assert(ret == -ERR_INVAL, "dbs_readbulk Use Case 3 error");

	//测试用例4-边界
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 3;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 129, &data_size, data_r );
	assert(ret == -ERR_INVAL, "dbs_readbulk Use Case 4 error");

	//测试用例5-边界
	key = 100;
	from.whence = 2;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 3;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 24, &data_size, data_r );
	assert(ret == -ERR_INVAL, "dbs_readbulk Use Case 5 error");

	//测试用例6-边界
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = 2;
	to.offset = 3;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 24, &data_size, data_r );
	assert(ret == -ERR_INVAL, "dbs_readbulk Use Case 6 error");

	//测试用例8-错误
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 7;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 48, &data_size, data_r );
	assert(ret == -ERR_NORECORD, "dbs_readbulk Use Case 8 error");

	//测试用例13-错误
	key = 100;
	from.whence = DBS_SEEK_END;
	from.offset = 6;
	to.whence = DBS_SEEK_SET;
	to.offset = 2;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 30, &data_size, data_r );
	assert(ret == -ERR_NORECORD, "dbs_readbulk Use Case 13 error");

	//测试用例14-错误
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 6;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 42, &data_size, data_r );
	assert(ret == -ERR_NORECORD, "dbs_readbulk Use Case 14 error");

	//测试用例16-覆盖，无此键值
	key = 200;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 3;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 24, &data_size, data_r );
	assert(ret == -ERR_NORECORD, "dbs_readbulk Use Case 16 error");

	//测试用例17-覆盖
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 4;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 30, &data_size, data_r );
	memcpy(&data_bulk,data_r,30);
	assert((ret==0)&&
			(data_bulk[0].data == 1)&&
			(data_bulk[1].data == 2)&&
			(data_bulk[2].data == 3)&&
			(data_bulk[3].data == 4)&&
			(data_bulk[4].data == 5)&&
			(data_size == 30), "dbs_readbulk Use Case 17 error");

	//测试用例18-覆盖
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_END;
	to.offset = 0;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 30, &data_size, data_r );
	memcpy(&data_bulk,data_r,30);
	assert((ret==0)&&
			(data_bulk[0].data == 1)&&
			(data_bulk[1].data == 2)&&
			(data_bulk[2].data == 3)&&
			(data_bulk[3].data == 4)&&
			(data_bulk[4].data == 5)&&
			(data_size == 30),  "dbs_readbulk Use Case 18 error");

	//测试用例19-覆盖
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_END;
	to.offset = 6;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 42, &data_size, data_r );
	assert(ret == -ERR_NORECORD, "dbs_readbulk Use Case 19 error");

	//测试用例20-覆盖
	key = 100;
	from.whence = DBS_SEEK_END;
	from.offset = 4;
	to.whence = DBS_SEEK_END;
	to.offset = 0;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 30, &data_size, data_r );
	assert(ret == -ERR_INVAL, "dbs_readbulk Use Case 20 error");

	//测试用例21-覆盖-反向读
	key = 100;
	from.whence = DBS_SEEK_END;
	from.offset = 0;
	to.whence = DBS_SEEK_END;
	to.offset = 4;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 30, &data_size, data_r );
	memset(&data_bulk[0],0,8*sizeof data_bulk[0]);
	memcpy(&data_bulk,data_r,30);
	assert((ret==0)&&
			(data_bulk[0].data == 5)&&
			(data_bulk[1].data == 4)&&
			(data_bulk[2].data == 3)&&
			(data_bulk[3].data == 2)&&
			(data_bulk[4].data == 1)&&
			(data_size == 30), "dbs_readbulk Use Case 21 error");

	//测试用例22-覆盖
	key = 100;
	from.whence = DBS_SEEK_END;
	from.offset = 4;
	to.whence = DBS_SEEK_SET;
	to.offset = 0;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 6, &data_size, data_r );
	memset(&data_bulk[0],0,8*sizeof data_bulk[0]);
	memcpy(&data_bulk,data_r,6);
	assert((ret==0)&&
			(data_bulk[0].data == 1)&&
			(data_size == 6), "dbs_readbulk Use Case 22 error");

	//测试用例23-覆盖
	key = 100;
	from.whence = DBS_SEEK_END;
	from.offset = 4;
	to.whence = DBS_SEEK_SET;
	to.offset = 4;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 30, &data_size, data_r );
	assert(ret == -ERR_INVAL, "dbs_readbulk Use Case 23 error");

	//测试用例24-覆盖
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 0;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 6, &data_size, data_r );
	memset(&data_bulk[0],0,8*sizeof data_bulk[0]);
	memcpy(&data_bulk,data_r,6);
	assert((ret == 0)&&
			(data_bulk[0].data == 1)&&
			(data_size == 6), "dbs_readbulk Use Case 24 error");

	//测试用例25-覆盖
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 1;
	to.whence = DBS_SEEK_SET;
	to.offset = 4;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 24, &data_size, data_r );
	memset(&data_bulk[0],0,8*sizeof data_bulk[0]);
	memcpy(&data_bulk,data_r,24);
	assert((ret==0)&&
			(data_bulk[0].data == 2)&&
			(data_bulk[1].data == 3)&&
			(data_bulk[2].data == 4)&&
			(data_bulk[3].data == 5)&&
			(data_size == 24), "dbs_readbulk Use Case 25 error");

	//测试用例26-覆盖
	key = 100;
	from.whence = DBS_SEEK_END;
	from.offset = 2;
	to.whence = DBS_SEEK_END;
	to.offset = 2;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 6, &data_size, data_r );
	memset(&data_bulk[0],0,8*sizeof data_bulk[0]);
	memcpy(&data_bulk,data_r,24);
	assert((ret==0)&&
			(data_bulk[0].data == 3)&&
			(data_size == 6), "dbs_readbulk Use Case 26 error");

	//测试用例27-覆盖
	key = 100;
	from.whence = DBS_SEEK_SET;
	from.offset = 1;
	to.whence = DBS_SEEK_END;
	to.offset = 0;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 24, &data_size, data_r );
	memset(&data_bulk[0],0,8*sizeof data_bulk[0]);
	memcpy(&data_bulk,data_r,24);
	assert((ret==0)&&
			(data_bulk[0].data == 2)&&
			(data_bulk[1].data == 3)&&
			(data_bulk[2].data == 4)&&
			(data_bulk[3].data == 5)&&
			(data_size == 24), "dbs_readbulk Use Case 27 error");

	//测试用例28-覆盖
	key = 100;
	from.whence = DBS_SEEK_END;
	from.offset = 0;
	to.whence = DBS_SEEK_SET;
	to.offset = 0;
	ret = dbs_read_bulk(0, 4, &key, &from, &to, 30, &data_size, data_r );
	memset(&data_bulk[0],0,8*sizeof data_bulk[0]);
	memcpy(&data_bulk,data_r,30);
	assert((ret==0)&&
			(data_bulk[0].data == 5)&&
			(data_bulk[1].data == 4)&&
			(data_bulk[2].data == 3)&&
			(data_bulk[3].data == 2)&&
			(data_bulk[4].data == 1)&&
			(data_size == 30), "dbs_readbulk Use Case 28 error");


	ret = dbs_close(0);		//关闭并移除数据库文件
	p_err(ret);
	ret = dbs_remove(0);
	p_err(ret);

	finaltest();

	exit(0);
}

