/**
* driver_init.c -- 驱动初始化函数
* 
* 
* 创建时间: 2010-4-24
* 最后修改时间: 2010-5-6
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/debug.h"
#include "include/sys/schedule.h"

extern int GpioInit(void);
extern int Rs485Init(void);
extern int SiginInit(void);
extern int AdcInit(void);

/**
* @brief 驱动接口子模块初始化函数
* @return 0成功, 否则失败
*/
DECLARE_INIT_FUNC(DriverInit);
int DriverInit(void)
{
	printf("  Driver init...\n");

	system("rmmod /conf/net/ohci-hcd.ko");
	Sleep(10);
	system("insmod /conf/net/ohci-hcd.ko");
	if(GpioInit()) return 1;
	if(SiginInit()) return 1;

	SET_INIT_FLAG(DriverInit);

	return 0;
}

/**
* @brief 驱动接口子模块初始化函数(高级部分)
* @return 0成功, 否则失败
*/
DECLARE_INIT_FUNC(DriverHighInit);
int DriverHighInit(void)
{
	printf("  Driver high init...\n");
	if(Rs485Init()) return 1;
	//if(AdcInit()) return 1;

	SET_INIT_FLAG(DriverHighInit);
	return 0;
}
