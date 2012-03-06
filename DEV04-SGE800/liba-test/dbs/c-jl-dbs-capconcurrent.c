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
    文件名      ：  c-jl-dbs-capconcurrent.c
    描述        ：  本文件用于平台库数据库并发性能测试
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//平台库头文件
#include "../include/thread.h"
#include "../include/dbs.h"
#include "../include/xj_assert.h"
#include "../include/error.h"
#define MAXNUM 10000
void thread_rd(void)
{
	int ret,a;
	int key;
	int data_r;
	u16 data_size;
	dbs_set_t set;

	key = 100;
	set.whence = DBS_SEEK_SET;
	set.offset = 2;

	for(a=0; a<MAXNUM; a++){
		ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
		assert((ret == 0)&&(data_r == 2), "dbs_read error");
		//printf("read\n");
	}
}

//void thread_wr(void)
//{
//	int ret,b;
//	int key;
//	int data;
//	dbs_set_t set;
//	key = 100;
//	data = 103;
//	set.whence =  DBS_SEEK_SET;
//	set.offset = 0;
//
//	for(b=0; b<MAXNUM; b++){
//		key = 100;
//		data = b;
//		ret = dbs_insert(0, 4, &key, &set, DBS_SEEK_FORWARD, 4, &data);
//		assert(ret==0, "dbs_write error");
//		//printf("write\n");
//	}
//}

int main()
{
	int ret,i;
	u8 idrd;
	int key;
	int data,data_r;
	u16 data_size;
	dbs_set_t set;

	//环境初始化
	inittest();
	ret = thread_init();
	p_err(ret);

	//环境的初始化-读写方式，ram中打开
	inittest();
	ret = dbs_init();
	p_err(ret);

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


	//测试用例
	idrd=1;
	ret = thread_create(idrd, (void *)thread_rd, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);

//	//测试用例
//	idwr=2;
//	ret = thread_create(idwr, (void *)thread_wr, NULL, THREAD_MODE_NORMAL, 0);
//	p_err(ret);
	key = 100;
	set.whence = DBS_SEEK_SET;
	set.offset = 2;
	for(i=0; i<MAXNUM; i++){
		ret = dbs_read(0, 4, &key, &set, 8, &data_size, &data_r);
		assert((ret == 0)&&(data_r == 2), "dbs_read Main error");
		//printf("read Main\n");
	}


	sleep(5);
	finaltest();
	return 0;
}

