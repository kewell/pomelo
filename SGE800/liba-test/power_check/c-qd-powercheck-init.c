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
    文件名      ：  c-qd-powercheck-init.c
    描述        ：  本文件用于平台库powercheck_init函数的测试
    版本        ：  0.1
    作者        ：  左伟杰
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

//测试程序头文件
#include "../include/powercheck.h"

int main()
{
	int ret1 = -1, ret2 = -1, ret3 = -1, ret4 = -1;
	u8 mode;
	//环境初始化
	inittest();

	//测试用例1
	mode = POWERCHECK_MODE_NOBLOCK;
	ret1 = powercheck_init(mode);
	assert(ret1 == 0,"powercheck_init 1 error");
	powercheck_close();

	//测试用例2
	mode = POWERCHECK_MODE_BLOCK_UP;
	ret2 = powercheck_init(mode);
	assert(ret2 == 0,"powercheck_init 2 error");
	powercheck_close();

	//测试用例3
	mode = POWERCHECK_MODE_BLOCK_DOWN;
	ret3 = powercheck_init(mode);
	assert(ret3 == 0,"powercheck_init 3 error");
	powercheck_close();

	//测试用例4
	mode = POWERCHECK_MODE_BLOCK_UPDOWN;
	ret4 = powercheck_init(mode);
	assert(ret4 == 0,"powercheck_init 4 error");
	powercheck_close();

	// 测试用例5
	mode = POWERCHECK_MODE_NOBLOCK;
	ret4 = powercheck_init(mode);
	assert(ret4 == -ERR_NOFILE,"powercheck_init 5 error");
	powercheck_close();

	// 测试用例6
	mode = POWERCHECK_MODE_NOBLOCK;
	ret4 = powercheck_init(mode);
	ret4 = powercheck_init(mode);
	assert(ret4 == -ERR_BUSY,"powercheck_init 6 error");
	powercheck_close();

	// 测试用例7
	mode = 13;
	ret4 = powercheck_init(mode);
	assert(ret4 == -ERR_INVAL,"powercheck_init 7 error");
	powercheck_close();

	finaltest();
	return 0;
}

