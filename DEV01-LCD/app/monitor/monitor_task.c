/**
* monitor_task.c -- 监测模块任务
* 
* 
* 创建时间: 2010-6-12
* 最后修改时间: 2010-6-12
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "uplink/svrcomm.h"
#include "uplink/uplink_dl.h"
#include "include/debug.h"
#include "include/lib/align.h"
#include "include/sys/schedule.h"
#include "include/sys/gpio.h"
#include "include/sys/sigin.h"
#include "include/sys/reset.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/task.h"
#include "include/sys/cycsave.h"
#include "include/lib/bcd.h"
#include "include/monitor/alarm.h"
#include "include/monitor/runstate.h"
#include "include/param/hardware.h"
#include "include/param/mix.h"
#include "include/cenmet/dbclear.h"
#include "cenmet/mdb/mdbstic.h"
#include "ad_calib.h"
#include "include/plcmet/plmdb.h"

#if 0
static int TimerIdPowerOff = -1;


/**
* @brief 停电定时器(超过3分钟未来电就断电)
*/
static int CTimerPowerOff(unsigned long arg)
{
	DebugPrint(0, "set power off.\r\n");
	SysPowerDown();

	return 1;
}
#endif

#if 0
/**
* @brief 市电状态监测
* @param pbuf 告警缓存区指针
*/
static void PowerMonitor(alarm_t *pbuf)
{
	int stat = GpioGetValue(GPIO_POWER_STATUS);

	#if 1
	if(stat) {  //power off
		if(0 == RunState.pwroff) {
			runstate_t *prun = RunStateModify();
			sysclock_t clock;

			PrintLog(LOGTYPE_ALARM, "power off!!\n");
			Sleep(200);

			prun->pwroff = 1;

			memset((unsigned char *)pbuf, 0, sizeof(alarm_t));
			pbuf->erc = 14;
			pbuf->len = 5;
			//记录停电时间	
			SysClockReadCurrent(&clock);
			prun->timepoweroff[0] = pbuf->min = clock.minute;
			prun->timepoweroff[1] = pbuf->hour = clock.hour;
			prun->timepoweroff[2] = pbuf->day = clock.day;
			prun->timepoweroff[3] = pbuf->mon = clock.month;
			prun->timepoweroff[4] = pbuf->year = clock.year;
			HexToBcd(&pbuf->min, 5);
			MakeAlarm(ALMFLAG_ABNOR, pbuf);
			
			if(TimerIdPowerOff >= 0) {
				SysStopCTimer(TimerIdPowerOff);
				TimerIdPowerOff = -1;
			}
			TimerIdPowerOff = SysAddCTimer(180, CTimerPowerOff, 0);
		}
		else {
			if(TimerIdPowerOff < 0)
				TimerIdPowerOff = SysAddCTimer(180, CTimerPowerOff, 0);
		}
	}
	else {  //power on
		if(RunState.pwroff) {
			sysclock_t clock;

			RunStateModify()->pwroff = 0;
			if(TimerIdPowerOff >= 0) {
				SysStopCTimer(TimerIdPowerOff);
				TimerIdPowerOff = -1;
			}

			memset((unsigned char *)pbuf, 0, sizeof(alarm_t));
			pbuf->erc = 14;
			pbuf->len = 5;
			//记录上次停电时间			
			pbuf->min = RunState.timepoweroff[0];
			pbuf->hour = RunState.timepoweroff[1];
			pbuf->day = RunState.timepoweroff[2];
			pbuf->mon = RunState.timepoweroff[3];
			pbuf->year = RunState.timepoweroff[4];
			HexToBcd(&pbuf->min, 5);

			SysClockReadCurrent(&clock);
			pbuf->data[0] = clock.minute;
			pbuf->data[1] = clock.hour;
			pbuf->data[2] = clock.day;
			pbuf->data[3] = clock.month;
			pbuf->data[4] = clock.year;
			HexToBcd(pbuf->data, 5);
			MakeAlarm(ALMFLAG_NORMAL, pbuf);
		}
	}
	#else
	runstate_t *prun = RunStateModify();

	prun->pwroff = 0;
	#endif
}
#endif

#if 0
/**
* @brief 遥信状态监测
* @param pbuf 告警缓存区指针
*/
static void SignalMonitor(alarm_t *pbuf)
{
	unsigned char stat = SiginReadState();
	unsigned char mask, i, chg;
	runstate_t *prun = RunStateModify();

	chg = 0;

	for(i=0,mask=1; i<SIGIN_NUM; i++,mask<<=1) {
		if(ParaHardw.isig.flagattr&mask) {  //a型触点		
			if(stat&mask) stat &= ~mask;
			else stat |= mask;
		}
	}

#if 0
	stat |= (data[0]&0x04)<<4;  //端子盖开关
	stat |= (data[0]&0x02)<<4;  //中盖开关
#endif

	for(i=0,mask=1; i<SIGIN_NUM; i++,mask<<=1) {
		if(0 == (ParaHardw.isig.flagin&mask)) continue;

		if((stat&mask) != (RunState.isig_stat&mask)) {
			chg |= mask;
			prun->isig_chg |= mask;
		}
	}

	prun->isig_stat &= ~SIGIN_MASK;
	prun->isig_stat |= (stat&SIGIN_MASK);

	if(chg) {
		memset((unsigned char *)pbuf, 0, sizeof(alarm_t));
		pbuf->erc = 4;
		pbuf->len = 2;
		pbuf->data[0] = chg;
		pbuf->data[1] = RunState.isig_stat;
		MakeAlarm(ALMFLAG_ABNOR, pbuf);
	}
}

/**
* @brief 时钟电池监测
* @param pbuf 告警缓存区指针
*/
static void ClockBatMonitor(alarm_t *pbuf)
{
	int stat = GpioGetValue(GPIO_BATLOW);

	if(0 == stat && 0 == RunState.battery) { //bat low
		PrintLog(LOGTYPE_ALARM, "clock battery voltage too low!!\n");

		RunStateModify()->battery = 1;

		memset((unsigned char *)pbuf, 0, sizeof(alarm_t));
		pbuf->erc = 13;
		pbuf->len = 3;
		pbuf->data[0] = 0;
		pbuf->data[1] = 0x80;
		pbuf->data[2] = 0x10;
		MakeAlarm(ALMFLAG_ABNOR, pbuf);
	}
}


static void CommFlowsMonitor(alarm_t *pbuf) 
{
	if(ParaMixSave.mix.upflow_max && !RunState.commflow) {
		if(MdbStic.term_mon.comm_bytes > ParaMixSave.mix.upflow_max) {
			PrintLog(LOGTYPE_ALARM, "Month Communication flows over limit!!\n");

			RunStateModify()->commflow = 1;
			
			memset((unsigned char *)pbuf, 0, sizeof(alarm_t));
			pbuf->erc = 32;
			pbuf->len = 8;
			DEPART_LONG(MdbStic.term_mon.comm_bytes, pbuf->data);
			DEPART_LONG(ParaMixSave.mix.upflow_max, &pbuf->data[4]);
			MakeAlarm(ALMFLAG_ABNOR, pbuf);
		}
	}
}
#endif


static unsigned char BatCapacityStatus = 0;
unsigned char BatCapacity(void)
{
	return BatCapacityStatus;
}
#if 0
/**
* @brief 可充电电池监测
*/
static void BatteryMonitor(void)
{
#define VOL_CHARGE		4800
#define VOL_UNCHARGE	5700
#define VOL_BATLOW		3000
#define VOL_BATBAD		5810
#define BATCAP_CHK		((VOL_UNCHARGE-VOL_CHARGE)/8)

	int batv = ReadBatVol();

	if(batv < VOL_BATLOW) {  // 没插电池
		BatCapacityStatus = 0;
		//GpioSetValue(GPIO_BAT_CHARGE, 0);  //停止充电
		if(0 == RunState.batbad) {
			PrintLog(LOGTYPE_ALARM, "battery disconnected!\n");
			RunStateModify()->batbad = 1;
		}
		/*if(RunState.batcharge) {
			PrintLog(LOGTYPE_ALARM, "battery stop charge\n");
			RunStateModify()->batcharge = 0;
		}*/
		return;
	}
	else if(RunState.batbad) {
		PrintLog(LOGTYPE_ALARM, "battery connected!\n");
		RunStateModify()->batbad = 0;
	}

	

	if(batv < VOL_UNCHARGE) {
		GpioSetValue(GPIO_BAT_CHARGE, 1);  //充电
		if(0 == RunState.batcharge) {
			PrintLog(LOGTYPE_ALARM, "battery start charge\n");
			RunStateModify()->batcharge = 1;
		}
	}
	else {
		GpioSetValue(GPIO_BAT_CHARGE, 0);  //停止充电
		if(RunState.batcharge) {
			PrintLog(LOGTYPE_ALARM, "battery stop charge\n");
			RunStateModify()->batcharge = 0;
		}
	}

	if(batv <= (VOL_CHARGE+BATCAP_CHK)) BatCapacityStatus = 0;
	else if(batv >= (VOL_UNCHARGE-BATCAP_CHK)) BatCapacityStatus = 3;
	else if(batv < ((VOL_UNCHARGE+VOL_CHARGE)/2)) BatCapacityStatus = 1;
	else BatCapacityStatus = 2;
}
#endif

/**
* @brief 校准系统时钟
*/
static void SysClockCilibrate(void)
{
	sysclock_t clock;

	if(ReadExternSysClock(&clock)) return;

	SysClockSet(&clock);
}



#define CYCSAVE_TIMEOUT		(7*3500)  // 7 hour
#define CYCSAVE_FIRSTCNT	(3*3500)  // 3 hour
#define DBCLR_TIMEOUT		(16*3500)  // 16 hour
#define DBCLR_FIRSETCNT		(2*3500)  // 2 hour
#define CLKCHK_TIMEOUT		(5*3500)  // 5 hour


int watchdog_fd = 0;
//unsigned char wBuf[ 2 ] = {0x10,0x00};
unsigned char wBuf[ 2 ] = {0x00,0x80};

/**
* @brief 监测任务
*/
static void *MonitorTask(void *arg)
{
	//@change later 温度监测(高温关屏)

	//alarm_t alarmbuf;
	//int countbat = 29;
	//int countcycs = CYCSAVE_TIMEOUT - CYCSAVE_FIRSTCNT;
	int countdbclr = DBCLR_TIMEOUT - DBCLR_FIRSETCNT;
	int countclkchk = 0;
	int countclkread = 0;
	int WatchdogTimer = 0;
	int GPRSLineStatTimer = 0;
	sysclock_t clock;
	
	Sleep(20);


	WatchdogTimer = 0;
	GPRSLineStatTimer = 0;
	//Sleep(1000);
	//Sleep(50);
	watchdog_fd = 0;
	watchdog_fd = open("/dev/watchdog",O_RDWR);
	printf("watchdog_fd = %d\n",watchdog_fd);
	PrintLog(0, "watchdog_fd = %d\n",watchdog_fd);
	
	


	while(1) {
		#if 0
		//PowerMonitor(&alarmbuf);
		//SignalMonitor(&alarmbuf);
		//ClockBatMonitor(&alarmbuf);
		//CommFlowsMonitor(&alarmbuf);
		
		//countbat++;
		//if(countbat >= 30) {
		//	countbat = 0;

		//	BatteryMonitor();
		//}

		countcycs++;
		if(countcycs > CYCSAVE_TIMEOUT) {
			countcycs = 0;
			PrintLog(LOGTYPE_SHORT, "start cycle save...\n");
			SysCycleSave(0);
			DebugPrint(LOGTYPE_SHORT, "end\n");
		}
		#endif

		
		countdbclr++;
		if(countdbclr > DBCLR_TIMEOUT) {//16个小时检测一次过时数据
		//if(countdbclr > 3) {
			countdbclr = 0;
			DbaseClear();
		}

		countclkchk++;
		if(countclkchk > CLKCHK_TIMEOUT) {
			countclkchk = 0;

			SysClockCilibrate();//5个小时校准一次系统时间
		}
		countclkread++;
		if(countclkread> 98 * 6) {
			countclkread = 0;
			ReadExternSysClock(&clock);
		}		
		
		
		WatchdogTimer++;
		if(WatchdogTimer>= 5)	
		{
			WatchdogTimer = 0;
			if(watchdog_fd>0)
			{
				//PrintLog(1,"watchdog_fd = %d\n",watchdog_fd);
				//PrintLog(1,"write(watchdog_fd, wBuf, 2 );\n");
       			write(watchdog_fd, wBuf, 2 );
			}
			/*
			else
			{	
				system("killall -9 watchdog");
				Sleep(100);
				watchdog_fd = open("/dev/watchdog", O_RDWR);
				PrintLog(1, "watchdog_fd = %d\n",watchdog_fd);
				//printf("watchdog_fd = %d\n",watchdog_fd);
				//if(watchdog_fd<=0)
				//{
				//	system("./home/run/watchdog");
				//	PrintLog(0, "system/home/run/watchdog");
				//}
			}
			*/
		}
		SysTimerTask_watchdog++;
		//EthShellTask_watchdog++;
	
		if(SysTimerTask_watchdog>(10 * 60 * 60 * 5))
		{
			SysRestart();
		} 
		//if(EthShellTask_watchdog>(10 * 60 * 5))
		//{
		//	SysRestart();
		//}
		if(SvrCommInterface == UPLINKITF_GPRS)
		{
			GprsActiveTask_watchdog++;
			if(GprsActiveTask_watchdog>(10 * 60 * 60 * 5))
			{
				SysRestart();
			}
			if(LINESTAT_OFF == SvrCommLineState) 
			{
				GPRSLineStatTimer++;
			}
			else if(LINESTAT_ON == SvrCommLineState) 
			{
				GPRSLineStatTimer = 0;
			}
			if(GPRSLineStatTimer>(10 * 60 * 60 * 25))///////25个小时不在线,则重启
			{
				SavePlMdb();
				SysRestart();
			}
		}
		PlcTask_watchdog++;
		if(PlcTask_watchdog>(10 * 60 * 60 * 3))
		{
			SysRestart();
		} 
		PlFrezTask_watchdog++;
		if(PlFrezTask_watchdog>(10 * 60 * 60 * 3))
		{
			SysRestart();
		} 
		
		Sleep(10);
	}
}


DECLARE_INIT_FUNC(MonitorTaskInit);
int MonitorTaskInit(void)
{
	//GpioSetDirect(GPIO_BATLOW, 0);
	//GpioSetDirect(GPIO_BAT_CHARGE, 1);
	//GpioSetValue(GPIO_BAT_CHARGE, RunState.batcharge);

	SysCreateTask(MonitorTask, NULL);

	SET_INIT_FLAG(MonitorTaskInit);
	return 0;
}

