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
    文件名      ：  c-qd-timer-init.c
    描述        ：  本文件用于平台库timer_init函数的测试
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
#include "../include/timer.h"

int main()
{
	int ret1 = -1, ret2 = -1, ret3 = -1;
	u8 id, mode;
	id = 3;

	//环境初始化
	inittest();

	//测试用例1
	mode = TIMER_MODE_HEART;
	ret1 = timer_init(id, mode);
	assert(ret1 == 0,"timer_init 1 error");
	timer_close(id);

	//测试用例2
	mode = TIMER_MODE_MEASURE;
	ret2 = timer_init(id, mode);
	assert(ret2 == 0,"timer_init 2 error");
	timer_close(id);

	//测试用例3
	mode = TIMER_MODE_PWM;
	ret3 = timer_init(id, mode);
	assert(ret3 == 0,"timer_init 3 error");
	timer_close(id);

	//测试用例4
	id = 0;
	mode = TIMER_MODE_HEART;
	ret1 = timer_init(id, mode);
	assert(ret1 == 0,"timer_init 4 error");
	timer_close(id);

	//测试用例5
	id = 0;
	mode = TIMER_MODE_MEASURE;
	ret1 = timer_init(id, mode);
	assert(ret1 == 0,"timer_init 5 error");
	timer_close(id);

	//测试用例6
	id = 0;
	mode = TIMER_MODE_PWM;
	ret1 = timer_init(id, mode);
	assert(ret1 == 0,"timer_init 6 error");
	timer_close(id);

	//测试用例7
	id = MAX_TIMER - 1;
	mode = TIMER_MODE_HEART;
	ret1 = timer_init(id, mode);
	assert(ret1 == 0,"timer_init 7 error");
	timer_close(id);

	//测试用例8
	id = MAX_TIMER - 1;
	mode = TIMER_MODE_MEASURE;
	ret1 = timer_init(id, mode);
	assert(ret1 == 0,"timer_init 8 error");
	timer_close(id);

	//测试用例9
	id = MAX_TIMER - 1;
	mode = TIMER_MODE_PWM;
	ret1 = timer_init(id, mode);
	assert(ret1 == 0,"timer_init 8 error");
	timer_close(id);

	//测试用例10
	id = 3;
	mode = TIMER_MODE_MEASURE;
	ret1 = timer_init(id, mode);
	assert(ret1 == -ERR_NOFILE,"timer_init 10 error");

	//测试用例11
	id = 3;
	mode = TIMER_MODE_MEASURE;
	ret1 = timer_init(id, mode);
	ret1 = timer_init(id, mode);
	assert(ret1 == -ERR_BUSY,"timer_init 11 error");
	timer_close(id);

	//测试用例12
	id = 3;
	mode = 4;
	ret1 = timer_init(id, mode);
	assert(ret1 == -ERR_INVAL,"timer_init 12 error");
	printf("ret1 = %d\r\n",ret1);
	timer_close(id);

	finaltest();
	return 0;
}
