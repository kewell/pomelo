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
    文件名      ：  c-qd-timer-heartstart-heartwait.c
    描述        ：  本文件用于平台库timer_heart_start,timer_heart_wait函数的测试
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
	int ret1 = -1, ret2 = -1;
	u8 id, mode;
	u32 set_interval = 20;


	//环境初始化
	inittest();

	//测试用例1
	id = 2;
	mode = TIMER_MODE_HEART;
	timer_init(id, mode);
	timer_heart_setconfig(id, set_interval);
	ret1 = timer_heart_start(id);
	ret2 = timer_heart_wait(id);
	assert(((ret1 == 0) && (ret2 == 0))\
			,"time_heart_start,time_heart_wait 1 error");
	timer_close(id);

	//测试用例2
	id = 0;
	mode = TIMER_MODE_HEART;
	timer_init(id, mode);
	timer_heart_setconfig(id, set_interval);
	ret1 = timer_heart_start(id);
	ret2 = timer_heart_wait(id);
	assert(((ret1 == 0) && (ret2 == 0))\
				,"time_heart_start,time_heart_wait 2 error");
	timer_close(id);

	//测试用例3
	id = MAX_TIMER - 1;
	mode = TIMER_MODE_HEART;
	timer_init(id, mode);
	timer_heart_setconfig(id, set_interval);
	ret1 = timer_heart_start(id);
	ret2 = timer_heart_wait(id);
	assert(((ret1 == 0) && (ret2 == 0))\
				,"time_heart_start,time_heart_wait 3 error");
	timer_close(id);

	//测试用例4
	id = 7;
	mode = TIMER_MODE_HEART;
	timer_init(id, mode);
	timer_heart_setconfig(id, set_interval);
	ret1 = timer_heart_start(id);
	assert(ret1 == -ERR_INVAL, "time_heart_start,time_heart_wait 4 error");
	timer_close(id);

	//测试用例5
	id = 2;
	mode = TIMER_MODE_HEART;
	timer_heart_setconfig(id, set_interval);
	ret1 = timer_heart_start(id);
	assert(ret1 == -ERR_NOINIT, "time_heart_start,time_heart_wait 5 error");
	timer_close(id);

	//测试用例6
	id = 2;
	mode = TIMER_MODE_PWM;
	timer_init(id, mode);
	timer_heart_setconfig(id, set_interval);
	ret1 = timer_heart_start(id);
	assert(ret1 == -ERR_NOFUN, "time_heart_start,time_heart_wait 6 error");
	timer_close(id);

	finaltest();
	return 0;
}
