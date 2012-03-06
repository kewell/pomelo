/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：aaa.c
	描述		：测试业务模块-平台时间管理
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.11
******************************************************************************/

//C库头文件

//基础平台头文件
#include "sge_core/rtc.h"

//业务平台头文件
#include "framework/debug.h"
#include "framework/framework.h"
#include "framework/systime.h"
#include "framework/message.h"

//业务模块头文件
#include "enum.h"
#include "aaa.h"



int aaa_initmodel(struct BASE * this)
{
	int ret = 0;
	struct aaaModule *obj;
	st_ymdhmsw_t time;

	obj = (struct aaaModule *)this;
//注册模块
	ret = module_register(this);
	if (ret) {
		goto error;
	}
//设置时间
/*
	time.year = 10;
	time.mon = 12;
	time.day = 3;
	time.hour = 16;
	time.min = 33;
	time.sec = 00;
	time.wday = 5;
	ret = systime_set(&time);
	if (ret < 0) {
		goto error;
	}
	ret = rtc_settime(&time);
	if (ret < 0) {
		goto error;
	}
*/

//注册4秒周期定时
	ret = systime_register_period(this, 1, 4);
	if (ret < 0) {
		goto error;
	}
	else {
		obj->tmhdr_4s = ret;
	}
//注册9秒倒计时定时
	ret = systime_register_timer(this, 9, 3);
	if (ret < 0) {
		goto error;
	}
	else {
		obj->tmhdr_9s = ret;
	}
//注册绝对时间到达
	systime_get (&time);
	time.min += 1;
	ret = systime_register_clock(this, (st_ymdhms_t *)&time);
	if (ret < 0) {
		goto error;
	}
	else {
		obj->tmhdr_clock = ret;
	}

	ret = 0;
error:
	return ret;
}

int aaa_initdata(struct BASE * this)
{
	//...;
	return 0;
}

int aaa_msghandle(struct BASE * this, message_t *msg)
{
	struct aaaModule *obj;
	st_ymdhmsw_t time;

	obj = (struct aaaModule *)this;

	switch (msg->type) {
	case MSG_TIME:
//		if (msg->lpara == obj->tmhdr_4s) {
//			MPRINTF("%s:4s timer!\n", __FILE__);
//		}
//		else if (msg->lpara == obj->tmhdr_9s) {
//			MPRINTF("%s:9s timer!\n", __FILE__);
//		}
//		else if (msg->lpara == obj->tmhdr_clock) {
//			MPRINTF("%s:clock timer!\n", __FILE__);
//		}
//		systime_get (&time);
//		MPRINTF ("%s:time--%02d:%02d:%02d:%02d:%02d:%02d\n", __FILE__, time.year, time.mon, time.day, time.hour, time.min, time.sec);
		break;
	default:
		break;
	}
	return 0;
}

struct BASEFT aaa_ft = {aaa_initmodel, aaa_initdata, aaa_msghandle};
