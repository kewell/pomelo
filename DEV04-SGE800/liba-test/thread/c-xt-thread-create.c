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
    文件名         ：  c-tx-thread-create.c
    描述	       ：  本文件用于平台库thread_create函数的测试
    版本              ：  0.1
    作者              ：  孙锐
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>			//exit
#include <unistd.h>			//sleep
#include <db.h>

//平台库头文件
#include "../include/thread.h"
#include "../include/xj_assert.h"
#include "../include/error.h"


void thread_test1(void)
{
	int i;
	//printf("thread_test1\n");
	for(i=1;i<100;i++)
	{
	}
	while(1){
		sleep(1);
	}

}
void thread_test2(u8 *num)
{
	int i;
	int n;
	for(i=1;i<100;i++)
	{
		n++;
	}
	printf("num = %d\n",*num);
	while(1){
		sleep(1);
	}

}

int main()
{
	int ret;
	//环境的初始化
	inittest();
	u8 id1,id2;

	/*********测试用例10**************/
	id1 = 2;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);             //建立普通线程
	assert(ret==-ERR_NOINIT,"test10:thread create error!");

	ret = thread_init();                //初始化模块环境
	if(ret){
		printf("thread init error!\n");
		goto error;
	}

	/*********测试用例1**************/
	id1 = 2;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);             //建立普通线程
	assert(ret==0,"test1:thread create error!");

	/*********测试用例2**************/
	id2 = 3;
	u8 test_num = 10;
	ret = thread_create(id2, (void *)thread_test2, &test_num, THREAD_MODE_NORMAL, 0);   //建立普通线程，带参数
	assert(ret==0,"test1:thread create error!");

	/*********测试用例3**************/
	id1 = 5;
	u8 prio = 3;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_REALTIME, prio);       //建立实时线程
	assert(ret==0,"test3:thread create error!");

	/*********测试用例4**************/
	id2 = 6;
	prio = 5;
	ret = thread_create(id2, (void *)thread_test2, &test_num, THREAD_MODE_REALTIME, prio);   //建立实时线程，带参数
	assert(ret==0,"test4:thread create error!");

	/*********测试用例5**************/
	id1 = 1;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);
	assert(ret==0,"test5:thread create error!");

	/*********测试用例6**************/
	id2 = 31;
	ret = thread_create(id2, (void *)thread_test2, &test_num, THREAD_MODE_NORMAL, 0);
	assert(ret==0,"test6:thread create error!");

	/*********测试用例7**************/
	id1 = 7;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_REALTIME, 1);
	assert(ret==0,"test7:thread create error!");

	/*********测试用例8**************/
	id1 = 8;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_REALTIME, 99);
	assert(ret==0,"test8:thread create error!");

	/*********测试用例9**************/
	id2 = 9;
	ret = thread_create(32, (void *)thread_test2, &test_num, THREAD_MODE_NORMAL, 0);   //建立普通线程，带参数
	assert(ret==-ERR_INVAL,"test9:thread create error!");

	/*********测试用例11**************/
	id2 = 8;
	ret = thread_create(id2, (void *)thread_test2, &test_num, THREAD_MODE_NORMAL, 0);   //建立同一普通线程，带参数
	assert(ret==-ERR_BUSY,"test11:thread create error!");

	/*********测试用例12**************/
	id2 = 10;
	ret = thread_create(id2, (void *)thread_test2, &test_num, 8, 0);   //建立普通线程，带参数
	assert(ret==-ERR_INVAL,"test12:thread create error!");

	/*********测试用例13**************/
	id1 = 11;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 1);             //建立普通线程
	assert(ret==-ERR_INVAL,"test13:thread create error!");

	/*********测试用例14**************/
	id1 = 12;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_REALTIME, 0);             //建立普通线程
	assert(ret==-ERR_INVAL,"test14:thread create error!");

	sleep(5);
	finaltest();

error:
	exit(0);
}

