/**
* errlog.c -- 错误日志管理
* 
* 将发生错误的地方已文件的形式记录
* 
* 创建时间: 2010-4-3
* 最后修改时间: 2010-4-3
*/

#include <stdio.h>
#include <stdarg.h>

#include "include/debug.h"

#define MAX_LINE_LEN	64
#define MAX_LOGO_NUM	512

/*
typedef struct {
	char line[MAX_LINE_LEN];
} errlog_line_t;

static int ErrLogLines = 0;
static errlog_line_t ErrLogBuffer[MAX_LOGO_NUM];
*/

/*void PrintErrorLog(const char *format, ...)
{
	va_list va;

	va_start(va, format);
	vprintf(format, va);
	va_end(va);
}*/

extern const struct _init_flag_t __start__init_flag[];
extern const struct _init_flag_t __stop__init_flag[];

void CheckInitFlag(void)
{
	int i, buinit = 0;

	for(i=0; __start__init_flag+i<__stop__init_flag; i++) {
		if(!(__start__init_flag[i]).flag) {
			printf("\033[1;33m" "!warning: \033[0m" "%s not init\n", (__start__init_flag[i]).name);
			buinit = 1;
		}
	}
	if(buinit) printf("\n");
}
