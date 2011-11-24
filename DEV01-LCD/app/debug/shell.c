/**
* shell.c -- 命令行调试
* 
* 
* 创建时间: 2010-5-26
* 最后修改时间: 2010-5-26
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "shell.h"
#include "include/debug/shellcmd.h"

extern const struct shell_cmd __start__shell_cmd[];
extern const struct shell_cmd __stop__shell_cmd[];

#define INODE_NUM		5
#define HANSHNODE_NUM	26
#define INDEX_SIZE		(HANSHNODE_NUM*INODE_NUM+INODE_NUM)
static const struct shell_cmd *cmd_index[INDEX_SIZE];

#define ERROR_PROMPT	"\033[1;31m!error! \033[0m"

DECLARE_INIT_FUNC(ShellCmdInit);
int ShellCmdInit(void)
{
	const struct shell_cmd *plist;
	int i;

	for(i=0; i<INDEX_SIZE; i++) cmd_index[i] = NULL;

	for(plist=__start__shell_cmd; plist<__stop__shell_cmd; plist++) {
		if(plist->name[0] < 'a' || plist->name[0] > 'z') {
			printf(ERROR_PROMPT "command %s invalid\n", plist->name);
			continue;
		}

		i = (int)(plist->name[0] - 'a')&0xff;
		i *= INODE_NUM;
		for(; i<INDEX_SIZE; i++) {
			if(NULL == cmd_index[i]) {
				cmd_index[i] = plist;
				break;
			}
		}
		if(i >= INDEX_SIZE) printf(ERROR_PROMPT "can not index command %s\n", plist->name);
	}

	SET_INIT_FLAG(ShellCmdInit);
	return 0;
}

shell_func FindShellFunc(const char *command)
{
	int i;

	if(command[0] < 'a' || command[0] > 'z') return NULL;

	i = (int)(command[0] - 'a')&0xff;
	i *= INODE_NUM;
	for(; i<INDEX_SIZE; i++) {
		if(NULL == cmd_index[i]) return NULL;

		if(0 == strcmp(cmd_index[i]->name, command)) {
			return cmd_index[i]->pfunc;
		}
	}

	return NULL;
}

int ShellParseArg(const char *line, char *argv[], int argmax)
{
	int rtn = 0;
	char *p = argv[0];
	int arglen;

	*p = 0;
	arglen = 0;
	for(; 0!=*line; line++) {
		if(' ' == *line) {
			if(arglen) {
			   *p = 0;
			   rtn++;
			   if(rtn >= argmax) return rtn;
			   p = argv[rtn];
			   *p = 0;
			   arglen = 0;
			}
		}
		else if('\r' == *line || '\n' == *line) break;
		else {
			*p++ = *line;
			arglen++;
			if(arglen >= 128) {
			   p = argv[rtn];
			   *p = 0;
			   arglen = 0;
			}
		}
	}

	if(arglen) {
		*p = 0;
		rtn++;
	}

	return rtn;
}

static int shell_help(int argc, char *argv[])
{
	int i;

	for(i=0; i<INDEX_SIZE; i++) {
		if(NULL != cmd_index[i]) PrintLog(0, "%s: \t%s\n", cmd_index[i]->name, cmd_index[i]->info);
	}

	return 0;
}
DECLARE_SHELL_CMD("help",shell_help, "help");

static int shell_empty(int argc, char *argv[])
{
	return 0;
}
DECLARE_SHELL_CMD("link", shell_empty, "empty command");


