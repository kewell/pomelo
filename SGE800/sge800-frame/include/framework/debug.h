/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：debug.h
	描述		：本文件定义了业务平台框架的调试头文件
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.12
******************************************************************************/

#ifndef _DEBUG_H
#define _DEBUG_H

#include "config.h"
#include <stdio.h>

#ifdef CFG_FRAMEWORK_DEBUG
#define	PRINTF(x...)			printf(x)
#else
#define PRINTF(x...)
#endif

#ifdef CFG_MODULE_DEBUG
#define	MPRINTF(x...)			printf(x)
#else
#define MPRINTF(x...)
#endif

#endif
