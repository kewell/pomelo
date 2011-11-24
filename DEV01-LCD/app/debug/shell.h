/**
* shell.h -- 命令行调试
* 
* 
* 创建时间: 2010-5-26
* 最后修改时间: 2010-5-26
*/

#ifndef _DEBUG_SHELL_H
#define _DEBUG_SHELL_H

typedef int (*shell_func)(int argc, char *argv[]);
shell_func FindShellFunc(const char *command);

int ShellParseArg(const char *line, char *argv[], int argmax);

#endif /*_DEBUG_SHELL_H*/

