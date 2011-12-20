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
    文件名      ：  c-xt-syn-mutexlock.c
    描述        ：  本文件用于平台库syn_mutex_lock函数的全部用例测试
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
#include "../include/syn.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

void thread_uc4(void)
{
	int ret;
	ret = syn_mutex_lock(2,2);
	assert(ret == -ERR_TIMEOUT, "syn_mutex_lock Use Case 4 error");

}

void thread_uc7(void)
{
	int ret;
	ret = syn_mutex_lock(2,SYN_RECV_NONBLOCK);
	assert(ret == -ERR_BUSY, "syn_mutex_lock Use Case 7 error");

}

int main()
{
	int ret;
	u8 id4,id7;
	//环境初始化
	inittest();
	ret = syn_init();
	p_err(ret);
	ret = syn_mutex_init(2);
	p_err(ret);
	ret = thread_init();
	p_err(ret);

	//测试用例1
	ret = syn_mutex_lock(2,2);
	assert(ret == 0, "syn_mutex_lock Use Case 1 error");

	//测试用例2-边界
	ret = syn_mutex_lock(64,10);
	assert(ret == -ERR_INVAL, "syn_mutex_lock Use Case 2 error");

	//测试用例3-错误，未初始化
	ret = syn_mutex_lock(4,10);
	assert(ret == -ERR_NOINIT, "syn_mutex_lock Use Case 3 error");

	//测试用例4-覆盖，先占有锁
	id4=1;
	ret = thread_create(id4, (void *)thread_uc4, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);

	sleep(2);

	//测试用例5-覆盖，获得锁
	syn_mutex_unlock(2);
	p_err(ret);
	ret = syn_mutex_lock(2,SYN_RECV_BLOCK);
	assert(ret == 0, "syn_mutex_lock Use Case 5 error");

	//测试用例6-覆盖，获得锁
	syn_mutex_unlock(2);
	p_err(ret);
	ret = syn_mutex_lock(2,SYN_RECV_NONBLOCK);
	assert(ret == 0, "syn_mutex_lock Use Case 6 error");

	//测试用例7-覆盖，先占有锁
	id7=2;
	ret = thread_create(id7, (void *)thread_uc7, NULL, THREAD_MODE_NORMAL, 0);
	p_err(ret);

	sleep(2);
	finaltest();
	return 0;
}

