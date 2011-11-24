/**
* shellcmd.h -- 命令行调试命令
* 
* 
* 创建时间: 2010-5-26
* 最后修改时间: 2010-5-26
*/

#ifndef _DEBUG_SHELLCMD_H
#define _DEBUG_SHELLCMD_H

struct shell_cmd {
	const char *name;
	int (*pfunc)(int argc, char *argv[]);
	const char *info;
};

#define DECLARE_SHELL_CMD(cmdname, func, helpinfo) \
	static struct shell_cmd _shellcmd_##func \
	__attribute__((__used__)) \
	__attribute__((section("_shell_cmd"), unused)) \
	= {cmdname, func, helpinfo}

#endif /*_DEBUG_SHELLCMD_H*/

