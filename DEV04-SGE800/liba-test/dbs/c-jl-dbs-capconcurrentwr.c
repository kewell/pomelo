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
		assert((ret == 0)&&((data_r == 2)||(data_r == 102)), "dbs_read error");
		printf("******************************%d\n",data_r);
	}
}

void thread_wr1(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("01write 102\n");
	}
}
void thread_wr2(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("02write 2\n");
	}
}
void thread_wr3(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("03write 102\n");
	}
}
void thread_wr4(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("04write 2\n");
	}
}
void thread_wr5(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("05write 102\n");
	}
}
void thread_wr6(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("06write 2\n");
	}
}
void thread_wr7(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("07write 102\n");
	}
}
void thread_wr8(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("08write 2\n");
	}
}
void thread_wr9(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("09write 102\n");
	}
}
void thread_wr10(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("10write 2\n");
	}
}

void thread_wr11(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("11write 102\n");
	}
}
void thread_wr12(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("12write 2\n");
	}
}
void thread_wr13(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("13write 102\n");
	}
}
void thread_wr14(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("14write 2\n");
	}
}
void thread_wr15(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("15write 102\n");
	}
}
void thread_wr16(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("16write 2\n");
	}
}
void thread_wr17(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("17write 102\n");
	}
}
void thread_wr18(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("18write 2\n");
	}
}
void thread_wr19(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 102;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write1 error");
		printf("19write 102\n");
	}
}
void thread_wr20(void)
{
	int ret,b;
	int key;
	int data;
	dbs_set_t set;
	key = 100;
	data = 2;
	set.whence =  DBS_SEEK_SET;
	set.offset = 2;

	for(b=0; b<MAXNUM/100; b++){
		key = 100;
		ret = dbs_write(0, 4, &key, &set, 4, &data);
		assert(ret==0, "dbs_write2 error");
		printf("20write 2\n");
	}
}







int main()
{
	int ret,i;
	u8 idrd, idwr[21];
	int key;
	int data;

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


	for(i=1;i<=20;i++){
		idwr[i]=i;
	}
	idrd = 1;
	ret = thread_create(idrd, (void *)thread_rd, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);

	idwr[1]=22;
	ret = thread_create(idwr[1], (void *)thread_wr1, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[2], (void *)thread_wr2, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[3], (void *)thread_wr3, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[4], (void *)thread_wr4, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[5], (void *)thread_wr5, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[6], (void *)thread_wr6, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[7], (void *)thread_wr7, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[8], (void *)thread_wr8, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[9], (void *)thread_wr9, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[10], (void *)thread_wr10, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[11], (void *)thread_wr11, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[12], (void *)thread_wr12, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[13], (void *)thread_wr13, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[14], (void *)thread_wr14, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[15], (void *)thread_wr15, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[16], (void *)thread_wr16, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[17], (void *)thread_wr17, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[18], (void *)thread_wr18, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[19], (void *)thread_wr19, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);
	ret = thread_create(idwr[20], (void *)thread_wr20, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);



	sleep(50);
	finaltest();
	return 0;
}

