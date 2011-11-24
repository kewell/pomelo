/**
* syslock.c -- 系统复位函数
* 
* 
* 创建时间: 2010-5-5
* 最后修改时间: 2010-5-15
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

#include "include/sys/reset.h"
#include "include/sys/cycsave.h"
#include "include/sys/syslock.h"
#include "include/sys/gpio.h"
#include "include/sys/schedule.h"

extern void EnableFeedWatchdog(int flag);

void SysRestart(void)
{
	SysCycleSave(1);
	SysLockHalt();

	EnableFeedWatchdog(0);
	reboot(LINUX_REBOOT_CMD_RESTART);

	Sleep(500);
	exit(0);
}

void SysPowerDown(void)
{
	SysCycleSave(1);
	SysLockHalt();

	EnableFeedWatchdog(0);
	GpioSetValue(GPIO_POWER_12V, 0);
	GpioSetValue(GPIO_POWER_BAT, 0);

	Sleep(500);
	reboot(LINUX_REBOOT_CMD_RESTART);

	Sleep(500);
	exit(0);
}
