/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：aaa.h
	描述		：测试业务模块-平台时间管理
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.11
******************************************************************************/

#ifndef _AAA_H
#define _AAA_H

#include "framework/base.h"

struct aaaModule
{
	struct BASE base;
	int tmhdr_4s;
	int tmhdr_9s;
	int tmhdr_clock;
};

extern struct BASEFT aaa_ft;

#endif
