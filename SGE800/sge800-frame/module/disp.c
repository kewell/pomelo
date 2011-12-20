/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：disp.c
	描述		：显示示例模块
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.12
******************************************************************************/

//C库头文件

//基础平台头文件
#include "sge_core/rtc.h"
#include "sge_gui/GUI.h"

//业务平台头文件
#include "framework/debug.h"
#include "framework/framework.h"
#include "framework/systime.h"
#include "framework/message.h"
#include "framework/device/key.h"

//业务模块头文件
#include "disp.h"

int disp_initmodel(struct BASE * this)
{
	int ret = 0;
	struct dispModule *obj;

	obj = (struct dispModule *)this;
//注册模块
	ret = module_register(this);
	if (ret) {
		goto error;
	}
//GUI模块初始化
	ret = GUI_Init();
	if (ret) {
		goto error;
	}
//按键模块初始化
	ret = key_init();
	if (ret) {
		goto error;
	}
	LCD_On();
//订阅消息
	ret = message_subscribe(this, MSG_KEY);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this, MSG_SKEY);
	if (ret) {
		goto error;
	}

error:
	return ret;
}

int disp_initdata(struct BASE * this)
{
	//...;
	return 0;
}

int disp_msghandle(struct BASE * this, message_t *msg)
{
	struct dispModule *obj;

	obj = (struct dispModule *)this;

	switch (msg->type) {
	case MSG_KEY:
		LCD_Backlight_On();

		switch (msg->wpara) {
		case KEY_UP:
			GUI_DispStringAt("上", 60,40);
			break;
		case KEY_DOWN:
			GUI_DispStringAt("下", 60,40);
			break;
		case KEY_LEFT:
			GUI_DispStringAt("左", 60,40);
			break;
		case KEY_RIGHT:
			GUI_DispStringAt("右", 60,40);
			break;
		case KEY_ENTER:
			GUI_DispStringAt("前", 60,40);
			break;
		case KEY_CANCEL:
			GUI_DispStringAt("后", 60,40);
			break;
		default:
			break;
		}
		//MPRINTF ("%s:%d\n", __FILE__, msg->wpara);
		break;
	case MSG_SKEY:
		LCD_Backlight_Off();
		//MPRINTF ("%s:%d\n", __FILE__, msg->wpara);
		break;
	default:
		break;
	}
	return 0;
}

struct BASEFT disp_ft = {disp_initmodel, disp_initdata, disp_msghandle};
