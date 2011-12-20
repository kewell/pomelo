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
    文件名         ：  c-xt-thread-setpriority-getpriority.c
    描述	       ：  本文件用于平台库thread-setpriority-getpriority函数的测试
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

u8 test_prio=0;
void thread_test1(void)
{
	int i;
	printf("thread_test1\n");
	if (test_prio== 0){
		test_prio = 1;
	}
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
	if (test_prio== 0){
		test_prio = 2;
	}
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
	int ret,ret1;
	//环境的初始化
	inittest();
	u8 id1,id2;
	ret = thread_init();                //初始化模块环境
	if(ret){
		printf("thread init error!\n");
		goto error;
	}

	id1 = 2;
	u8 test_num = 20;
	u8 prio = 5;
	ret = thread_create(id1, (void *)thread_test2, &test_num, THREAD_MODE_REALTIME, prio);   //建立实时线程
	if(ret){
		printf("thread create error!\n");
		goto error;
	}
	/*********测试用例1**************/
	u8 prio_set = 6;
	u8 prio_get;
	ret = thread_setpriority(id1, prio_set);    //设置优先级
	ret1 = thread_getpriority(id1, &prio_get);  //获取优先级
	assert((ret==0)&&(ret1==0)&&(prio_set==prio_get),"test1:thread set/get prior error!");

	/********测试用例2*************/
	id1 = 5;
	prio = 3;
	ret = thread_create(id1, (void *)thread_test1, NULL, THREAD_MODE_REALTIME, prio);       //建立实时线程
	if (ret<0){
		printf("test2:create thread fail!\n");
	}
	//判断是哪个线程先执行，本例应该是thread_test2线程先执行
	assert(test_prio=2,"test2:prio test fail!");

	/*********测试用例3**************/
	id2 = 3;
	ret = thread_create(id2, (void *)thread_test2, &test_num, THREAD_MODE_NORMAL, 0);   //建立普通线程，带参数
	if (ret<0){
		printf("test3:create thread fail!\n");
		goto error;
	}
	ret1 = thread_getpriority(id2, &prio_get);  //获取优先级
	assert((ret1==0)&&(0==prio_get),"test3:thread set/get prior error!");


	/*********测试用例4**************/
	id1 = 2;
	prio_set = 1;
	ret = thread_setpriority(id1, prio_set);    //设置优先级
	ret1 = thread_getpriority(id1, &prio_get);  //获取优先级
	assert((ret==0)&&(ret1==0)&&(prio_set==prio_get),"test4:thread set/get prior error!");

	/*********测试用例5**************/
	prio_set = 99;
	ret = thread_setpriority(id1, prio_set);    //设置优先级
	ret1 = thread_getpriority(id1, &prio_get);  //获取优先级
	assert((ret==0)&&(ret1==0)&&(prio_set==prio_get),"test5:thread set/get prior error!");

	/*********测试用例6**************/
	id2 = 31;
	prio = 5;
	ret = thread_create(id2, (void *)thread_test2, &test_num, THREAD_MODE_REALTIME, prio);   //建立普通线程，带参数
	if (ret<0){
		printf("test6:create thread fail!\n");
		goto error;
	}
	prio_set = 1;
	ret = thread_setpriority(id2, prio_set);    //设置优先级
	ret1 = thread_getpriority(id2, &prio_get);  //获取优先级
	assert((ret==0)&&(ret1==0)&&(prio_set==prio_get),"test6:thread set/get prior error!");

	/*********测试用例7**************/
	ret = thread_setpriority(32, prio_set);    //设置优先级
	ret1 = thread_getpriority(32,&prio_get);  //获取优先级
	assert(((ret==-ERR_INVAL)&&(ret1==-ERR_INVAL)),"test7:thread set/get prior error!");

	/*********测试用例8**************/
	id1 = 2;
	prio_set = 0;
	ret = thread_setpriority(id1, prio_set);    //设置优先级
	assert(ret=-ERR_INVAL,"test8:thread set/get prior error!");

	/*********测试用例9**************/
	id1 = 2;
	prio_set = 100;
	ret = thread_setpriority(id1, prio_set);    //设置优先级
	assert(ret=-ERR_INVAL,"test9:thread set/get prior error!");

	/*********测试用例10**************/
	id2 = 10;
	ret = thread_setpriority(id2, prio_set);    //设置优先级
	ret1 = thread_getpriority(id2, &prio_get);  //获取优先级
	assert(((ret==-ERR_NODEV)&&(ret1==-ERR_NODEV)),"test10:thread set/get prior error!");

	/*********测试用例11**************/
	id2 = 3;
	prio_set = 1;
	ret = thread_setpriority(id2, prio_set);    //普通线程设置优先级
	assert(ret=-ERR_INVAL,"test9:thread set/get prior error!");

	/*********测试用例12**************/
	id2 = 0;
	prio_set = 10;
	ret = thread_setpriority(id2, prio_set);    //设置主线程优先级
	ret1 = thread_getpriority(id2, &prio_get);  //获取优先级
	assert(((ret==0)&&(ret1==0)&&(prio_get==10)),"test12:thread set/get prior error!");

	while(1){

	}



	finaltest();
error:
	exit(0);
}

