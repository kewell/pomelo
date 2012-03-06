/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：bbb.h
	描述		：测试业务模块-状态灯
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.12
******************************************************************************/

#ifndef _BBB_H
#define _BBB_H

#include "framework/base.h"

struct bbbModule
{
	struct BASE base;
	int tmhdr_1s;
};

extern struct BASEFT bbb_ft;

#endif
