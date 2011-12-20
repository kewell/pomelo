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
    文件名      ：  c-xt-syn-semgetvalue.c
    描述        ：  本文件用于平台库syn_sem_getvalue函数的全部用例测试
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
	int value;
	//环境初始化
	inittest();
	ret = syn_init();
	p_err(ret);
	ret = syn_sem_init(2,1);
	p_err(ret);

	//测试用例1
	ret = syn_sem_getvalue(2,&value);
	assert((ret == 0)&&(value == 1),"syn_sem_getvalue Use Case 1 error");

	//测试用例2
	ret = syn_sem_getvalue(64,&value);
	assert(ret == -ERR_INVAL,"syn_sem_getvalue Use Case 2 error");

	//测试用例3
	ret = syn_sem_getvalue(4,&value);
	assert(ret == -ERR_NOINIT,"syn_sem_getvalue Use Case 3 error");

	finaltest();
	return 0;
}

