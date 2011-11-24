/**
* main.c -- 应用软件主程序入口
* 
* 
* 创建时间: 2010-3-31
* 最后修改时间: 2010-3-31
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h>
#include <stdlib.h>
#include "include/em9x60_drivers.h"
#include "uplink/ppp/ppp_api.h"
#include "include/lcd_api.h"
#include "include/debug.h"
#include "include/startarg.h"
#include "include/sys/init.h"
#include "include/version.h"
#include "include/sys/task.h"
#include "include/sys/schedule.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/gpio.h"
#include "include/sys/uart.h"


#ifdef OPEN_DEBUGPRINT
static const char VersionDebugInfo[] = "debug";
#else
#ifdef OPEN_ASSERTLOG
static const char VersionDebugInfo[] = "test";
#else
static const char VersionDebugInfo[] = "release";
#endif
#endif


/**
* @brief 启动函数
*/
static void startup(void)
{
	if(SysInit()) {
		printf("system init failed!! quit\n");
		//return;
	}
	if(DebugInit()) {
		printf("debug init failed!! quit\n");
		//return;
	}
	if(ParamInit()) {
		printf("param init failed!! quit\n");
		//return;
	}

	ScreenInit();
	SvrCommInit();

	PlcInit();

	CenMetInit();
	MonitorInit();

	{
		sysclock_t clock;

		if(SysClockRead(&clock)) printf("read system clock failed.\n");
		else printf("current time = %s\n", SysClockFormat(&clock));
	}

	SysWaitTaskEnd();
}

/**
* 启动参数:
* '-b': 后台运行
* '-s': 启动命令行界面
* '-d': 不喂狗, 调试用
* '-rXXX': tty使用raw模式, XXX为串口波特率
* '-pXXX': tty使用raw模式,并且启动串口接收任务, XXX为串口波特率
*          -b, -s, -r , -p 不能同时有效
*/

static void ParseArgs(int argc, char *argv[])
{
	int i;

	for(i=1; i<argc; i++) {
		if('-' != argv[i][0] || 0 == argv[i][1]) continue;

		SetStartArg(argv[i]+1);
	}
}

/*
static void SmallCpyTest(void)
{
	unsigned int testbuf[512];
	int i,count;
	unsigned int max;
	unsigned char *pbuf = (unsigned char *)testbuf;

	for(i=0; i<512; i++) testbuf[i] = i;

	//smallcpy(pbuf+300, pbuf, 126);
	for(i=0,count=0; i<3000000; i++) {
		smallcpy(pbuf, pbuf+1, 126);
		pbuf++;
		count++;
		if(count >= 1800) {
			pbuf = (unsigned char *)testbuf;
			count = 0;
		}
	}

	for(i=0,max=0; i<512; i++) {
		if(testbuf[i] > max) max = testbuf[i];
	}
	printf("max=%d\n", max);
}
*/

/**
* @brief 主程序入口
* @param argc 参数数目
* @param argv 参数列表
* @return 0表示成功, 否则失败
*/
int main(int argc, char *argv[])
{
	printf("\n\n");

	printf("\033[1;32m" "武汉盛帆电子股份有限公司 载波集中器\n");	
	printf("Version %d.%02d.%d (%s).\n",
		VERSION_MAJOR, VERSION_MINOR, VERSION_PROJECT, STRING_PROJECT);
	printf("Relase Date: 20%02d-%d-%d %d:%d %s \033[0m\n\n", 
		_mk_year, _mk_month, _mk_day, _mk_hour, _mk_minute, VersionDebugInfo);

	//SmallCpyTest();
	//return 0;
	//Sleep(12000);
	Sleep(100*5);
	//Sleep(2000);
	ParseArgs(argc, argv);

	//if(!GetStartArg('b', NULL, 0)) 
	//if(0) 	
	if(1) 
	{ //后台运行
		if(fork() == 0) 
		{
			startup();
		}
	}
	else 
	{
		startup();
	}

	return 0;
}
