/**
* pipeshell.c -- 管道命令行调试
* 
* 
* 创建时间: 2010-5-26
* 最后修改时间: 2010-5-26
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/task.h"
#include "shell.h"

#define PIPE_R		"/tmp/pipe_r"
#define PIPE_W		"/tmp/pipe_w"

#define SHELLARG_NUM	12
static char CmdLineArgBuf[SHELLARG_NUM][128];
static char *CmdLineArgV[SHELLARG_NUM];

static int FdPipeRead = -1;
static int FdPipeWrite = -1;
static int PipeLogonOk = 0;
static char LogonString[] = "logon 7865ae31\n";

static int TimerIdPipeShell = -1;
static int CTimerPipeShell(unsigned long arg)
{
	TimerIdPipeShell = -1;
	SetLogType(0);
	SetLogInterface(0);
	PipeLogonOk = 0;
	return 1;
}

static void PipeReadLine(char *buf, int maxlen)
{
	int count;

	if(FdPipeRead < 0) return;

	while(1) 
	{
		count = read(FdPipeRead, buf, maxlen-1);

		if(count <= 0 || count > (maxlen-1)) 
		{
			Sleep(50);
			continue;
		}

		buf[count] = 0;
		break;
	}
}

/**
* @brief 管道命令行任务
*/
static void *PipeShellTask(void *arg)
{
	static char command[256];

	shell_func pfunc;
	int argc;

	while(1) {
		PipeReadLine(command, 256);

		if(!PipeLogonOk) {
			if(0 == strcmp(command, LogonString)) {
				PipeLogonOk = 1;
				SetLogInterface(2);

				if(TimerIdPipeShell < 0) TimerIdPipeShell = SysAddCTimer(60, CTimerPipeShell, 0);
				else SysClearCTimer(TimerIdPipeShell);

				PrintLog(0, "Logon OK\n");
			}
		}
		else {
			argc = ShellParseArg(command, CmdLineArgV, SHELLARG_NUM);
			if(argc > 0) {
				pfunc = FindShellFunc(CmdLineArgV[0]);
				if(NULL != pfunc) {
					SetLogInterface(2);

					if(TimerIdPipeShell < 0) TimerIdPipeShell = SysAddCTimer(60, CTimerPipeShell, 0);
					else SysClearCTimer(TimerIdPipeShell);

					//Sleep(5);
					(*pfunc)(argc, CmdLineArgV);
				}
			}
		}
	}

	return 0;
}

DECLARE_INIT_FUNC(PipeShellInit);
int PipeShellInit(void)
{
	int argc;

	for(argc=0; argc<SHELLARG_NUM; argc++) {
		CmdLineArgV[argc] = CmdLineArgBuf[argc];
	}

	remove(PIPE_W);
	remove(PIPE_R);

	if(mkfifo(PIPE_W, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) {
		printf("can not make fifo write\n");
		return 1;
	}

	if(mkfifo(PIPE_R, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) {
		printf("can not make fifo read\n");
		return 1;
	}

	FdPipeRead = open(PIPE_R, O_RDWR);
	if(FdPipeRead < 0) {
		printf("can not open %s\n", PIPE_R);
		return 1;
	}

	FdPipeWrite = open(PIPE_W, O_RDWR);
	if(FdPipeWrite < 0) {
		printf("can not open %s\n", PIPE_W);
		return 1;
	}

	SysCreateTask(PipeShellTask, NULL);

	SET_INIT_FLAG(PipeShellInit);

	return 0;
}

static char PipeShellPrintBuf[1536];
char *GetPipeShellBuffer(void)
{
	return PipeShellPrintBuf;
}

void PipeShellPrint(const char *str)
{
	if(FdPipeWrite < 0) return;

	write(FdPipeWrite, str, strlen(str));
}

void PipeShellQuit(void)
{
	if(TimerIdPipeShell >= 0) {
		SysStopCTimer(TimerIdPipeShell);
		TimerIdPipeShell = -1;
	}

	PipeLogonOk = 0;

	return;
}


