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
    文件名         ：  c-tx-thread-cancle.c
    描述	       ：  本文件用于平台库thread_cancle函数的测试
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
	printf("thread_test1\n");
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
	printf("thread_test2\n");
	for(i=1;i<100;i++)
	{
		num++;
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
	ret = thread_init();                //初始化模块环境
	if(ret){
		printf("thread init error!\n");
		goto error;
	}

	id1 = 2;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);             //建立普通线程
	if(ret){
		printf("thread1 create error!\n");
		goto error;
	}
	id2 = 3;
	u8 test_num = 10;
	u8 prio = 5;
	ret = thread_create(id2, (void *)thread_test2, &test_num, THREAD_MODE_REALTIME, prio);   //建立实时线程
	if(ret){
		printf("thread1 create error!\n");
		goto error;
	}
	sleep(5);
	/*********测试用例1**************/
	ret = thread_cancel(id1);              //普通线程取消
	assert(ret==0,"test1:thread cancel fail!");

	/*********测试用例2**************/
	ret = thread_cancel(id2);              //实时线程取消
	assert(ret==0,"test2:thread cancel fail!");

	/*********测试用例3**************/
	id1 = 1;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);             //建立普通线程
	if(ret){
		printf("thread1 create error!\n");
		goto error;
	}
	ret = thread_cancel(id1);              //普通线程取消
	assert(ret==0,"test3:thread cancel fail!");

	/*********测试用例4**************/
	id1 = 31;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);             //建立普通线程
	if(ret){
		printf("thread1 create error!\n");
		goto error;
	}
	ret = thread_cancel(id1);              //普通线程取消
	assert(ret==0,"test4:thread cancel fail!");

	/*********测试用例5**************/
	id1 = 32;
	ret = thread_cancel(id1);              //普通线程取消
	assert(ret==-ERR_INVAL,"test5:thread cancel fail!");

	/*********测试用例6**************/
	id1 = 6;
	ret = thread_cancel(id1);              //普通线程取消
	assert(ret==-ERR_NODEV,"test6:thread cancel fail!");

	sleep(5);
	finaltest();

error:
	exit(0);
}
