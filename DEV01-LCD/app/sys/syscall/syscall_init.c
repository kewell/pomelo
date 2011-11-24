/**
* syscall_init.c -- 系统调用子模块初始化
* 
* 
* 创建时间: 2008-3-31
* 最后修改时间: 2010-3-31
*/
#include <stdio.h>

#include "include/debug.h"

extern int SysTimerInit(void);
extern int SysTimeInit(void);

/**
* @brief 系统调用子模块初始化函数
* @return 返回0表示成功, 否则失败
*/
DECLARE_INIT_FUNC(SysCallInit);
int SysCallInit(void)
{
	printf("  SysCall init...\n");

	if(SysTimeInit()) return 1;
	if(SysTimerInit()) return 1;

	SET_INIT_FLAG(SysCallInit);

	return 0;
}

