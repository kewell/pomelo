/**
* statistic.c -- 统计量接口
* 
* 
* 创建时间: 2010-7-16
* 最后修改时间: 2010-7-16
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/environment.h"
#include "include/debug.h"
#include "include/debug/statistic.h"
#include "include/sys/xin.h"
#include "include/sys/cycsave.h"
#include "include/debug/shellcmd.h"

#define STATISTIC_SAVEPATH		IMPDATA_PATH

DECLARE_STATISTIC(0, dummy, 0);
DECLARE_STATISTIC(STATISGROUP_DOWNLINK, dummy_dwnlink, 0);

static const char *NameSave[STATISGROUP_MAX] = {
	"stat_sys.xin",
	"stat_uplink.xin",
	"stat_dwnlink.xin",
};

extern struct debug_statistic_t __start__statistic[];
extern struct debug_statistic_t __stop__statistic[];

/**
* @brief 保存统计量
*/
static void SaveStatistics(void)
{
	unsigned int i;
	int group;
	XINREF pf;
	char filename[64];

	for(group=1; group<=STATISGROUP_MAX; group++) {
		sprintf(filename, "%s%s", STATISTIC_SAVEPATH, NameSave[group-1]);
		pf = XinOpen(filename, 'w');
		if(NULL == pf) {
			ErrorLog("can not open %s for write\n", filename);
			continue;
		}

		for(i=0; __start__statistic+i<__stop__statistic; i++) {
			if(__start__statistic[i].group == group)
				XinWriteInt(pf, __start__statistic[i].name, __start__statistic[i].count, 0);
		}

		XinClose(pf);
	}
}
DECLARE_CYCLE_SAVE(SaveStatistics, 0);

/**
* @brief 载入保存的统计量
*/
DECLARE_INIT_FUNC(LoadDebugStatistics);
int LoadDebugStatistics(void)
{
	unsigned int i;
	int group;
	XINREF pf;
	char filename[64];

	printf("  load statistics...\n");

	for(group=1; group<=STATISGROUP_MAX; group++) {
		sprintf(filename, "%s%s", STATISTIC_SAVEPATH, NameSave[group-1]);
		pf = XinOpen(filename, 'r');
		if(NULL == pf) continue;

		for(i=0; __start__statistic+i<__stop__statistic; i++) {
			if(__start__statistic[i].group == group)
				__start__statistic[i].count = XinReadInt(pf, __start__statistic[i].name, __start__statistic[i].count);
		}

		XinClose(pf);
	}

	SET_INIT_FLAG(LoadDebugStatistics);
	return 0;
}

int shell_printstatistics(int argc, char *argv[])
{
	int group;
	unsigned int i;

	if(2 != argc) {
		PrintLog(0, "usage: statistics group\n");
		return 1;
	}

	group = atoi(argv[1]);

	for(i=0; __start__statistic+i<__stop__statistic; i++) {
		if(group && __start__statistic[i].group == group) {
			PrintLog(0, "%s = %d\n", __start__statistic[i].name, __start__statistic[i].count);
		}
	}

	PrintLog(0, "end\n");
	return 0;
}
DECLARE_SHELL_CMD("statistics", shell_printstatistics, "打印统计量");

