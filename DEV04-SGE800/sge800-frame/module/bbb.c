/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：bbb.c
	描述		：测试业务模块-状态灯
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.12
******************************************************************************/

//C库头文件

//基础平台头文件
#include "sge_core/rtc.h"

//业务平台头文件
#include "framework/debug.h"
#include "framework/framework.h"
#include "framework/systime.h"
#include "framework/message.h"
#include "framework/device/led.h"

//业务模块头文件
#include "bbb.h"

static int flag = 0;

int bbb_initmodel(struct BASE * this)
{
	int ret = 0;
	struct bbbModule *obj;

	obj = (struct bbbModule *)this;
//注册模块
	ret = module_register(this);
	if (ret) {
		goto error;
	}
//状态灯模块初始化
	ret = led_init();
	if (ret) {
		goto error;
	}
//注册1秒周期定时
	ret = systime_register_period(this, 1, 1);
	if (ret < 0) {
		goto error;
	}
	else {
		obj->tmhdr_1s = ret;
	}

	ret = 0;
error:
	return ret;
}

int bbb_initdata(struct BASE * this)
{
	//...;
	return 0;
}

int bbb_msghandle(struct BASE * this, message_t *msg)
{
	struct bbbModule *obj;

	obj = (struct bbbModule *)this;

	switch (msg->type) {
	case MSG_TIME:
		if (msg->lpara == obj->tmhdr_1s) {
			if (flag) {
				led_on(LED_TXDJL, 0, 0, 0);
				led_on(LED_WARN, 0, 0, 0);
				led_off(LED_RXDJL);
				led_off(LED_RUN);
				flag = 0;
			}
			else {
				led_on(LED_RXDJL, 0, 0, 0);
				led_on(LED_RUN, 0, 0, 0);
				led_off(LED_TXDJL);
				led_off(LED_WARN);
				flag = 1;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

struct BASEFT bbb_ft = {bbb_initmodel, bbb_initdata, bbb_msghandle};
