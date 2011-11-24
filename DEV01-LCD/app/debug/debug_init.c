/**
* debug_init.c -- 调试模块初始化
* 
* 
* 创建时间: 2010-5-26
* 最后修改时间: 2010-5-26
*/

#include <stdio.h>
#include "include/debug.h"
#include "include/startarg.h"

extern int EthShellInit(void);
extern int ShellCmdInit(void);
extern int TtyShellStart(void);
extern int PipeShellInit(void);
extern int LoadDebugStatistics(void);
extern int SvrShellInit(void);

DECLARE_INIT_FUNC(DebugInit);
int DebugInit(void)
{
	printf("debug init...\n");

	ShellCmdInit();

	if(EthShellInit()) return 1;
	if(PipeShellInit()) return 1;
	if(SvrShellInit()) return 1;

	if(GetStartArg('b', NULL, 0)) //前台运行
	{ 
		if(!GetStartArg('s', NULL, 0)) //启动命令行界面
		{ 
			TtyShellStart();
		}
	}

	LoadDebugStatistics();

	SET_INIT_FLAG(DebugInit);

	return 0;
}

