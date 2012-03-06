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
    文件名      ：  c-qd-timer-pwmsetconfi-pwmgetconfig.c
    描述        ：  本文件用于平台库timer_pwm_setconfig,timer_pwm_getconfig函数的测试
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
	u8 set_fz, set_fm;
	u8 get_fz, get_fm;
	u16 set_freq, get_freq;
	u16 freq1;


	//环境初始化
	inittest();
	mode = TIMER_MODE_PWM;

	//测试用例1
	id = 2;
	freq1 = 10;
	set_freq = freq1;
	set_fz = 3;
	set_fm = 4;
	timer_init(id, mode);
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	ret2 = timer_pwm_getconfig(id, &get_freq, &get_fz, &get_fm);

	assert(((ret1 == 0) \
			&& (ret2 == 0) \
			&& (get_freq == set_freq) \
			&& (get_fz == set_fz) \
			&& (get_fm == set_fm)) \
			,"timer_pwm_setconfig,time_pwm_getconfig 1 error");
	timer_close(id);

	//测试用例2
	id = 0;
	freq1 = 10;
	set_freq = freq1;
	set_fz = 3;
	set_fm = 4;
	timer_init(id, mode);
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	ret2 = timer_pwm_getconfig(id, &get_freq, &get_fz, &get_fm);

	assert(((ret1 == 0) \
			&& (ret2 == 0) \
			&& (get_freq == set_freq) \
			&& (get_fz == set_fz) \
			&& (get_fm == set_fm)) \
			,"timer_pwm_setconfig,time_pwm_getconfig 2 error");
	timer_close(id);

	//测试用例3
	id = MAX_TIMER - 1;
	freq1 = 10;
	set_freq = freq1;
	set_fz = 3;
	set_fm = 4;
	timer_init(id, mode);
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	ret2 = timer_pwm_getconfig(id, &get_freq, &get_fz, &get_fm);

	assert(((ret1 == 0) \
			&& (ret2 == 0) \
			&& (get_freq == set_freq) \
			&& (get_fz == set_fz) \
			&& (get_fm == set_fm)) \
			,"timer_pwm_setconfig,time_pwm_getconfig 3 error");
	timer_close(id);

	//测试用例4
	id = MAX_TIMER - 1;
	freq1 = 10;
	set_freq = freq1;
	set_fz = 3;
	set_fm = 4;
	timer_init(id, mode);
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	ret2 = timer_pwm_getconfig(id, &get_freq, &get_fz, &get_fm);

	assert(((ret1 == 0) \
			&& (ret2 == 0) \
			&& (get_freq == set_freq) \
			&& (get_fz == set_fz) \
			&& (get_fm == set_fm)) \
			,"timer_pwm_setconfig,time_pwm_getconfig 4 error");
	timer_close(id);

	//测试用例5
	id = MAX_TIMER;
	freq1 = 10;
	set_freq = freq1;
	set_fz = 3;
	set_fm = 4;
	timer_init(id, mode);
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	assert(ret1 == -ERR_INVAL,"timer_pwm_setconfig,time_pwm_getconfig 5 error");
	timer_close(id);

	//测试用例6
	id = 2;
	freq1 = 10;
	set_freq = freq1;
	set_fz = 0;
	set_fm = 4;
	timer_init(id, mode);
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	assert(ret1 == -ERR_INVAL,"timer_pwm_setconfig,time_pwm_getconfig 6 error");
	timer_close(id);

	//测试用例7
	id = 2;
	set_freq = 0;
	set_fz = 3;
	set_fm = 4;
	timer_init(id, mode);
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	assert(ret1 == -ERR_INVAL,"timer_pwm_setconfig,time_pwm_getconfig 7 error");
	printf("ret1 = %d\r\n",ret1);
	timer_close(id);

	//测试用例8
	id = 2;
	freq1 = 10;
	set_freq = freq1;
	set_fz = 3;
	set_fm = 4;
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	assert(ret1 == -ERR_NOINIT,"timer_pwm_setconfig,time_pwm_getconfig 8 error");


	//测试用例9
	mode = TIMER_MODE_HEART;
	id = 2;
	freq1 = 10;
	set_freq = freq1;
	set_fz = 3;
	set_fm = 4;
	timer_init(id, mode);
	ret1 = timer_pwm_setconfig(id, set_freq, set_fz, set_fm);
	assert(ret1 == -ERR_NOFUN,"timer_pwm_setconfig,time_pwm_getconfig 9 error");

	finaltest();
	return 0;
}
