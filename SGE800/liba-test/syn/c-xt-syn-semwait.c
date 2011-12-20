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
    文件名      ：  c-xt-syn-semwait.c
    描述        ：  本文件用于平台库syn_sem_wait函数的全部用例测试
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//平台库头文件
#include "../include/syn.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

int main()
{
	int ret;

	//环境初始化
	inittest();
	ret = syn_init();
	p_err(ret);
	ret = syn_sem_init(2,3);
	p_err(ret);
	//测试用例1
	ret = syn_sem_wait(2,10);
	assert(ret == 0,"syn_sem_wait Use Case 1 error");

	//测试用例2-边界
	ret = syn_sem_wait(64,10);
	assert(ret == -ERR_INVAL,"syn_sem_wait Use Case 2 error");

	//测试用例3-错误
	ret = syn_sem_wait(4,10);
	assert(ret == -ERR_NOINIT,"syn_sem_wait Use Case 3 error");

	//测试用例5
	ret = syn_sem_wait(2,SYN_RECV_BLOCK);
	assert(ret == 0,"syn_sem_wait Use Case 5 error");

	//测试用例6
	ret = syn_sem_wait(2,SYN_RECV_NONBLOCK);
	assert(ret == 0,"syn_sem_wait Use Case 6 error");

	//测试用例4
	ret = syn_sem_wait(2,2);
	assert(ret == -ERR_TIMEOUT,"syn_sem_wait Use Case 4 error");

	//测试用例7
	ret = syn_sem_wait(2,SYN_RECV_NONBLOCK);
	assert(ret == -ERR_BUSY,"syn_sem_wait Use Case 7 error");

	finaltest();
	exit(0);
}

