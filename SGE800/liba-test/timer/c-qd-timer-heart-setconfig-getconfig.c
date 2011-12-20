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
    文件名      ：  c-qd-timer-heart-setconfig-getconfig.c
    描述        ：  本文件用于平台库timer-heart-setconfig,timer-heart-getconfig函数的测试
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
	int ret = -1, ret1 = -1, ret2 = -1;
	u8 id, mode;
	id = 2;
	u32 set_interval, get_interval;
	mode = TIMER_MODE_HEART;

	//环境初始化
	inittest();

	//测试用例1
	ret = timer_init(id, mode);
	if(ret != 0){
		printf("timer_init error\r\n");
	}
	set_interval = 40;
	ret1 = timer_heart_setconfig(id, set_interval);
	ret2 = timer_heart_getconfig(id, &get_interval);
	assert(((ret1 == 0) && (ret2 == 0) && (get_interval == 40))\
			,"time_heart_setconfig,time_heart_getconfig 1 error");
	timer_close(id);

	//测试用例2
	timer_init(id, mode);
	set_interval = 125;
	ret1 = timer_heart_setconfig(id, set_interval);
	ret2 = timer_heart_getconfig(id, &get_interval);
	assert(((ret1 == 0) && (ret2 == 0) && (get_interval == 125))\
			,"time_heart_setconfig,time_heart_getconfig 2 error");
	timer_close(id);

	//测试用例3
	id = 0;
	timer_init(id, mode);
	set_interval = 80;
	ret1 = timer_heart_setconfig(id, set_interval);
	ret2 = timer_heart_getconfig(id, &get_interval);
	assert(((ret1 == 0) && (ret2 == 0) && (get_interval == set_interval))\
			,"time_heart_setconfig,time_heart_getconfig 3 error");
	timer_close(id);

	//测试用例4
	id = MAX_TIMER - 1;
	timer_init(id, mode);
	set_interval = 250;
	ret1 = timer_heart_setconfig(id, set_interval);
	ret2 = timer_heart_getconfig(id, &get_interval);
	assert(((ret1 == 0) && (ret2 == 0) && (get_interval == set_interval))\
			,"time_heart_setconfig,time_heart_getconfig 4 error");
	timer_close(id);

	//测试用例5
	id = 2;
	timer_init(id, mode);
	set_interval = 20;
	ret1 = timer_heart_setconfig(id, set_interval);
	ret2 = timer_heart_getconfig(id, &get_interval);
	assert(((ret1 == 0) && (ret2 == 0) && (get_interval == set_interval))\
			,"time_heart_setconfig,time_heart_getconfig 5 error");
	timer_close(id);

	//测试用例6
	id = 2;
	timer_init(id, mode);
	set_interval = 1000;
	ret1 = timer_heart_setconfig(id, set_interval);
	perror("aaaaaa");
	ret2 = timer_heart_getconfig(id, &get_interval);
	assert(((ret1 == 0) && (ret2 == 0) && (get_interval == set_interval))\
			,"time_heart_setconfig,time_heart_getconfig 6 error");
	printf("ret1 = %d\r\n",ret1);
	printf("ret2 = %d\r\n",ret2);
	printf("get_interval = %d\r\n",get_interval);
	timer_close(id);

	//测试用例7
	id = 7;
	timer_init(id, mode);
	set_interval = 40;
	ret1 = timer_heart_setconfig(id, set_interval);
	ret2 = timer_heart_getconfig(id, &get_interval);
	assert(((ret1 == -ERR_INVAL) && (ret2 == -ERR_INVAL))\
			,"time_heart_setconfig,time_heart_getconfig 7 error");
	timer_close(id);

	//测试用例8
	id = 2;
	timer_init(id, mode);
	set_interval = 2;
	ret1 = timer_heart_setconfig(id, set_interval);
	assert(ret1 == -ERR_INVAL, "time_heart_setconfig,time_heart_getconfig 8 error");
	timer_close(id);

	//测试用例9
	id = 2;
	timer_init(id, mode);
	set_interval = 126;
	ret1 = timer_heart_setconfig(id, set_interval);
	assert(ret1 == -ERR_INVAL, "time_heart_setconfig,time_heart_getconfig 9 error");
	timer_close(id);

	//测试用例10
	id = 2;
	timer_init(id ,mode);
	set_interval = 1002;
	ret1 = timer_heart_setconfig(id, set_interval);
	assert(ret1 == -ERR_INVAL, "time_heart_setconfig,time_heart_getconfig 10 error");
	timer_close(id);

	//测试用例11
/*	id = 2;
	timer_init(id, mode);
	set_interval = 40;
	ret1 = timer_heart_setconfig(id, set_interval);
	assert(ret1 == -ERR_NOINIT, "time_heart_setconfig,time_heart_getconfig 11 error");
	timer_close(id);
*/
	//测试用例12
	timer_close(id);
	id = 2;
	mode = TIMER_MODE_PWM;
	ret = timer_init(id, mode);
	set_interval = 40;
	ret1 = timer_heart_setconfig(id, set_interval);
	assert(ret1 == -ERR_NOFUN, "time_heart_setconfig,time_heart_getconfig 12 error");

	finaltest();
	return 0;
}
