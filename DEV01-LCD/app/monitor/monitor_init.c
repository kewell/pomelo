/**
* mdb.c -- 监测模块初始化
* 
* 
* 创建时间: 2010-5-15
* 最后修改时间: 2010-5-15
*/

#include <stdio.h>

#include "include/debug.h"
#include "ad_calib.h"

extern int AlarmInit(void);
extern int RunStateInit(void);
extern int MonitorTaskInit(void);

/**
* @brief 监测模块初始化
* @return 成功0, 否则失败
*/
DECLARE_INIT_FUNC(MonitorInit);
int MonitorInit(void)
{
	#if 1
	printf("monitor init...\n");

	//RunStateInit();
	AlarmInit();

	//LoadAdCalib();

	MonitorTaskInit();

	SET_INIT_FLAG(MonitorInit);
	#endif
	return 0;
}
