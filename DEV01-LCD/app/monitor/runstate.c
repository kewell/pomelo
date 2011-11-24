#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFINE_RUNSTATE

#include "include/debug.h"
#include "include/sys/syslock.h"
#include "include/sys/flat.h"
#include "include/sys/cycsave.h"
#include "include/monitor/runstate.h"
#include "include/debug/shellcmd.h"

#define FLATID_RUNSTATE		2

runstate_t RunState;

/**
* @brief 获取运行状态修改指针
*/
runstate_t *RunStateModify(void)
{
	return &RunState;
}

/**
* @brief 清除运行状态
*/
void ClearRunState(void)
{
	memset(&RunState, 0, sizeof(RunState));
}

DECLARE_INIT_FUNC(RunStateInit);
int RunStateInit(void)
{
	memset(&RunState, 0, sizeof(RunState));
	DebugPrint(0, "  load runstate(size=%d)...", sizeof(RunState));
	if(ReadFlatFile(FLATID_RUNSTATE, (unsigned char *)&RunState, sizeof(RunState)) > 0) {
		DebugPrint(0, "ok\n");
	}
	else {
		DebugPrint(0, "fail\n");
	}

	SET_INIT_FLAG(RunStateInit);
	return 0;
}

/**
* @brief 保存运行状态
*/
void SaveRunState(void)
{
	WriteFlatFile(FLATID_RUNSTATE, (unsigned char *)&RunState, sizeof(RunState));
}
DECLARE_CYCLE_SAVE(SaveRunState, 0);

int shell_runstate(int argc, char *argv[])
{
	PrintLog(0, "时钟电池: %s\n", RunState.battery?"低":"正常");
	PrintLog(0, "市电状态: %s\n", RunState.pwroff?"无":"有");
	PrintLog(0, "遥信状态: %02X\n", RunState.isig_stat);
	PrintLog(0, "遥信变位: %02X\n", RunState.isig_chg);

	PrintLog(0, "电池充电: %s\n", RunState.batcharge?"充电中":"停止");
	PrintLog(0, "电池连接: %s\n", RunState.batbad?"未连接":"正常");
	PrintLog(0, "终端剔除: %s\n\n", RunState.outgrp?"投入":"解除");

	PrintLog(0, "重要事件: head=%d, cur=%d, snd=%d\n", 
		RunState.alarm.head[0], RunState.alarm.cur[0], RunState.alarm.snd[0]);
	PrintLog(0, "一般事件: head=%d, cur=%d, snd=%d\n", 
		RunState.alarm.head[1], RunState.alarm.cur[1], RunState.alarm.snd[1]);
	PrintLog(0, "事件标志: ");
	PrintHexLog(0, RunState.alarm.stat, 8);
	PrintLog(0, "\n");

	PrintLog(0, "softchg=");
	PrintHexLog(0, RunState.softchg.ver, 8);

	PrintLog(0, "cnt_snderr= %d, %d\n", RunState.cnt_snderr[0], RunState.cnt_snderr[1]);
	PrintLog(0, "flag_acd= %d\n", RunState.flag_acd);

	PrintLog(0, "timepoweroff= ");
	PrintHexLog(0, RunState.timepoweroff, 8);

	PrintLog(0, "malmflag= ");
	PrintHexLog(0, RunState.malmflag, LEN_MALM_STAT);

	PrintLog(0, "\n");
	return 0;
}
DECLARE_SHELL_CMD("runstate", shell_runstate, "显示运行状态变量");

