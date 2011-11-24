/**
* time.c -- 时钟操作接口
* 
* 
* 创建时间: 2008-5-16
* 最后修改时间: 2010-4-24
*/


#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/sys/mutex.h"
#include "include/sys/gpio.h"

static utime_t UTimeCurrent;

///系统时间结构(读取读写,防止中断)
typedef union {
	unsigned long u[2];
	sysclock_t c;
} sysclock_rwctl_t;

static sysclock_rwctl_t ClockCurrent;

static sysclock_t ClockSysStart;

//static sys_rwlock_t ClockRwLock;

/**
* @brief 读取系统时间
* @param pclock 读取的时间值变量指针
* @return 成功时返回0, 否则返回非零值
*/
int SysClockRead(sysclock_t *pclock)
{
	time_t timep;
	struct tm timec;
	sysclock_rwctl_t rwclock;

	time(&timep);
	timec = *localtime(&timep);

	//SysReadLockRwLock(&ClockRwLock);

	pclock->year = rwclock.c.year = timec.tm_year - 100;
	pclock->month = rwclock.c.month = timec.tm_mon + 1;
	pclock->day = rwclock.c.day = timec.tm_mday;
	pclock->hour = rwclock.c.hour = timec.tm_hour;
	pclock->minute = rwclock.c.minute = timec.tm_min;
	pclock->second = rwclock.c.second = timec.tm_sec;
	pclock->week = rwclock.c.week = timec.tm_wday;


	ClockCurrent.u[0] = rwclock.u[0];
	ClockCurrent.u[1] = rwclock.u[1];

	UTimeCurrent = SysClockToUTime(&(ClockCurrent.c));

	//SysUnLockRwLock(&ClockRwLock);
	//printf("SysClockRead\n");
	return 0;
}

/**
* @brief 设置系统时间
* @param pclock 设置的时间值变量指针
* @return 成功时返回0, 否则返回非零值
*/
int SysClockSet(const sysclock_t *pclock)
{
	time_t timep;
	struct tm timec;
	struct timeval tval;
	struct timezone tzone;
	sysclock_rwctl_t rwclock;

	gettimeofday(&tval, &tzone);

	timec.tm_year = (int)pclock->year + 100;
	timec.tm_mon = pclock->month - 1;
	timec.tm_mday = pclock->day;
	timec.tm_hour = pclock->hour;
	timec.tm_min = pclock->minute;
	timec.tm_sec = pclock->second;
	timep = mktime(&timec);
	tval.tv_sec = timep;
	tval.tv_usec = 0;
	settimeofday(&tval, &tzone);

	//SysWriteLockRwLock(&ClockRwLock);

	UTimeCurrent = SysClockToUTime(pclock);
	UTimeToSysClock(UTimeCurrent, &(rwclock.c));
	ClockCurrent.u[0] = rwclock.u[0];
	ClockCurrent.u[1] = rwclock.u[1];

	//SysUnLockRwLock(&ClockRwLock);

	return 0;
}

static const int DaysBeforeMonth[12] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/*
* @brief 将系统时间转换为utime_t格式
* @param ptime 系统时间值变量指针
* @return 成功时返回对应的utime_t时间, 否则返回-1
*/
utime_t SysClockToUTime(const sysclock_t *ptime)
{
	int year, day;
	int cal;
	//unsigned char i, j;
	
	year = (int)(ptime->year)&0xff;
	if(year != 0) day = ((year-1)>>2) + 1;
	else day = 0;
	cal = year *365 + day; //days
	day = cal;

	if(ptime->month > 2 && 0 == (ptime->year&0x03)) day += 1;

	/*j = ptime->month;
	for(i=1; i<j; i++) {
		if(2 == i) {
			if(0 == (ptime->year & 0x03)) day += 29;
			else day += 28;
		}
		else if(i < 8) {
			if(i&0x01) day += 31;
			else day += 30;
		}
		else {
			if(i&0x01) day += 30;
			else day += 31;
		}
	}*/
	if(ptime->month != 0 && ptime->month < 13) {
		day += DaysBeforeMonth[ptime->month - 1];
	}

	if(0 != ptime->day) day += ptime->day - 1;

	cal = day*1440;

	day = ptime->hour;
	day *= 60;
	cal += day;

	cal += ptime->minute;
	cal *= 60;

	cal += ptime->second;

	return (utime_t)cal;
}

/**
* @brief 将utime_t格式转换为系统时间
* @param utime 需转换utime_t时间变量
* @param ptime 转换后的系统时间值变量指针
*/
void UTimeToSysClock(utime_t utime, sysclock_t *ptime)
{
	unsigned char uc;
	int max;

	ptime->second = utime%60;
	utime /= 60;

	ptime->minute = utime%60;
	utime /= 60;

	ptime->hour = utime%24;
	utime /= 24;

	ptime->week = (unsigned char)((utime+6)%7);  // 2000.1.1 is saturday

	max = utime/1461;  //four years
	ptime->year = (unsigned char)(max<<2);
	//utime %= 1461;
	utime -= max*1461;
	uc = 0;
	max = 366;
	while(utime >= max) {
		utime -= max;
		max = 365;
		uc += 1;
	}
	ptime->year += uc;

	uc=1;
	max = 31;
	while(utime >= max) {
		uc++;
		utime -= max;

		if(2 == uc) {
			if(0 == (ptime->year&0x03)) max = 29;
			else max = 28;
		}
		else if(uc < 8) {
			if(uc&0x01) max = 31;
			else max = 30;
		}
		else {
			if(uc&0x01) max = 30;
			else max = 31;
		}
	}
	ptime->month = uc;

	ptime->day = utime + 1;
}

/**
* @brief 比较2个系统时间的差异
* @param ptime1 第一个系统时间变量指针
* @param ptime2 第二个系统时间变量指针
* @return 
*   2个时间之间的差异, 以秒为单位
*   <0 表示ptime1早于ptime2
*   =0 表示2个时间相同
*   >0 表示ptime1晚于ptime2
*/
int SysClockDifference(const sysclock_t *ptime1, const sysclock_t *ptime2)
{
	utime_t cal1, cal2;
	int diff;

	cal1 = SysClockToUTime(ptime1);
	cal2 = SysClockToUTime(ptime2);

	diff = (int)cal1 - (int)cal2;

	return diff;
}

/**
* @brief 在原有时间上增加一段时间
* @param time 原始时间变量
* @param mod 增加的时间数值
* @param dev 增加的时间单位
*     UTIMEDEV_MINUTE -- 分
*     UTIMEDEV_HOUR -- 小时
*     UTIMEDEV_DAY -- 天
*     UTIMEDEV_MONTH -- 月
* @return 增加后的时间值
*/
utime_t UTimeAdd(utime_t time, int mod, int dev)
{
	utime_t rtn = time;
	int addup;

	if(mod < UTIMEDEV_MONTH) {
		if(UTIMEDEV_MINUTE== mod) addup = 60;  //minute
		else if(UTIMEDEV_HOUR == mod) addup = 3600;   //hour
		else addup = 86400;   //day

		addup *= dev;
	}
	else { //month
		sysclock_t clk;
		int i;

		addup = 0;
		UTimeToSysClock(time, &clk);

		for(i=0; i<dev;i++) {
			if(2 == clk.month) {
				if(0 == (clk.year&0x03)) addup += 29*1440*60;
				else addup += 28*1440*60;
			}
			else if(clk.month < 8) {
				if(clk.month&0x01) addup += 31*1440*60;
				else addup += 30*1440*60;
			}
			else {
				if(clk.month&0x01) addup += 30*1440*60;
				else addup += 31*1440*60;
			}

			clk.month++;
			if(clk.month > 12) {
				clk.month = 1;
				clk.year++;
			}
		}
	}

	rtn += addup;
	return(rtn);
}

/**
* @brief 得到当前系统时间
*   这个函数返回定时器任务定时更新的当前时间缓存值, 因此要远快于SysClockRead()
* @param pclock 储存时间的变量指针
*/
void SysClockReadCurrent(sysclock_t *pclock)
{
	sysclock_rwctl_t rwclock, rwclock2;

	//SysReadLockRwLock(&ClockRwLock);
	do {
		rwclock2.u[0] = ClockCurrent.u[0];
		rwclock2.u[1] = ClockCurrent.u[1];
		rwclock.u[0] = ClockCurrent.u[0];
		rwclock.u[1] = ClockCurrent.u[1];
	} while(rwclock2.u[0] != rwclock.u[0] || rwclock2.c.minute != rwclock.c.minute);

	pclock->year = rwclock.c.year;
	pclock->month = rwclock.c.month;
	pclock->day = rwclock.c.day;
	pclock->hour = rwclock.c.hour;
	pclock->minute = rwclock.c.minute;
	pclock->second = rwclock.c.second;
	pclock->week = rwclock.c.week;

	//SysUnLockRwLock(&ClockRwLock);
}

/**
* @brief 得到当前系统时间(utime_t格式)
*   这个函数返回定时器任务定时更新的当前时间缓存值, 因此要远快于SysClockRead()
* @return 当前系统时间(utime_t格式)
*/
utime_t UTimeReadCurrent(void)
{
	return UTimeCurrent;
}

static char TimeFormatBuffer[32];
static struct timeval TimeValStart;

/**
* @brief 将utime_t时间转换为ascii字符串
* @param time 输入的时间
* @return 时间字符串指针
*/
const char *UTimeFormat(utime_t time)
{
	sysclock_t clock;

	UTimeToSysClock(time, &clock);

	sprintf(TimeFormatBuffer, "20%02d-%d-%d %d:%d:%d", clock.year, clock.month, clock.day,
			clock.hour, clock.minute, clock.second);

	return TimeFormatBuffer;
}

/**
* @brief 将sysclock_t时间转换为ascii字符串
* @param clock 输入的时间
* @return 时间字符串指针
*/
const char *SysClockFormat(const sysclock_t *pclock)
{
	sprintf(TimeFormatBuffer, "20%02d-%d-%d %d:%d:%d", pclock->year, pclock->month, pclock->day,
			pclock->hour, pclock->minute, pclock->second);

	return TimeFormatBuffer;
}

/**
* @brief 开始测量时间
*/
void StartTimeMeasure(void)
{
	gettimeofday(&TimeValStart, (struct timezone *)0);
}

/**
* @brief 停止测量时间
* @return 从开始测量到停止经过的毫秒数
*/
int StopTimeMeasure(void)
{
	struct timeval tval;
	int rtn;

	gettimeofday(&tval, (struct timezone *)0);

	if(tval.tv_usec < TimeValStart.tv_usec) {
		tval.tv_sec--;
		tval.tv_usec += 1000000;
	}
	if(tval.tv_sec < TimeValStart.tv_sec) return 0;

	rtn = tval.tv_sec - TimeValStart.tv_sec;
	rtn *= 1000;
	rtn += (tval.tv_usec - TimeValStart.tv_usec)/1000;

	return rtn;
}

/**
* @brief 停止测量时间(格式化返回)
* @return 从开始测量到停止经过的毫秒数(字符串形式)
*/
const char *StopTimeMeasureFormat(void)
{
	int msec;

	msec = StopTimeMeasure();
	sprintf(TimeFormatBuffer, "%d.%03ds", msec/1000, msec%1000);

	return TimeFormatBuffer;
}

/**
* @brief 获得系统启动时间
* @param clock 时钟变量指针
*/
void GetClockSysStart(sysclock_t *clock)
{
	clock->year = ClockSysStart.year;
	clock->month = ClockSysStart.month;
	clock->day = ClockSysStart.day;
	clock->hour = ClockSysStart.hour;
	clock->minute = ClockSysStart.minute;
	clock->second = ClockSysStart.second;
	clock->week = ClockSysStart.week;
}

/**
* @brief 读取外部时钟
* @param clock 返回时钟变量指针
* @return 返回0表示成功, 否则失败
*/
int ReadExternSysClock(sysclock_t *clock)
{
#define FAILCNT_MAX		300
#define RETRY_MAX		3

	int retry, failcnt;
	extclock_t extclock1, extclock2;

	//SysInitRwLock(&ClockRwLock);

	for(failcnt=0; failcnt<FAILCNT_MAX; failcnt++) {
		for(retry=0; retry<RETRY_MAX; retry++) {
			if(ExtClockRead(&extclock1)) continue;
			if(ExtClockRead(&extclock2)) continue;
			break;
		}
		if(retry >= RETRY_MAX) {
			printf("read ext clock fail\n");
			return 1;
		}

		if(extclock1.year != extclock2.year) continue;
		if(extclock1.month != extclock2.month) continue;
		if(extclock1.day != extclock2.day) continue;
		if(extclock1.hour != extclock2.hour) continue;
		if(extclock1.minute != extclock2.minute) continue;

		break;
	}
	if(failcnt >= FAILCNT_MAX) {
		printf("ext clock time invalid\n");
		return 1;
	}

	clock->year = extclock2.year;
	clock->month = extclock2.month;
	clock->day = extclock2.day;
	clock->hour = extclock2.hour;
	clock->minute = extclock2.minute;
	clock->second = extclock2.second;
	clock->week = extclock2.week;
	//printf("read ext clock succ\n");
	return 0;
}

/**
* @brief 时钟模块初始化函数
* @return 返回0表示成功, 否则失败
*/
DECLARE_INIT_FUNC(SysTimeInit);
int SysTimeInit(void)
{
	//printf("SysTimeInit1\n");
	if(ReadExternSysClock(&ClockSysStart)) return 1;

	SysClockSet(&ClockSysStart);
	//printf("SysTimeInit2\n");
	SET_INIT_FLAG(SysTimeInit);

	return 0;
}

