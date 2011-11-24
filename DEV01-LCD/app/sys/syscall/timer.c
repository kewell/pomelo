/**
* timer.c -- 定时器操作接口
* 
* 
* 创建时间: 2008-5-16
* 最后修改时间: 2010-6-6
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "sys/sys_config.h"
#include "include/sys/task.h"
#include "include/sys/schedule.h"
#include "include/sys/mutex.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/debug.h"
#include "include/startarg.h"
#include "include/sys/gpio.h"
#include "include/debug/shellcmd.h"
#include "include/sys/gpio.h"

//时钟定时器控制变量结构
typedef struct st_rtman {
	struct st_rtman *next;
	struct st_rtman *prev;
	int id;
	rtimerproc_t proc;
	unsigned long arg;
	utime_t tstart;
	utime_t tbase;
	unsigned char dev;
	unsigned char mod;
	unsigned char bonce;
	unsigned char flag;
	utime_t basetime;
} rtman_t;

//相对定时器控制变量结构
typedef struct st_ctman {
	struct st_ctman *next;
	struct st_ctman *prev;
	int id;
	ctimerproc_t proc;
	unsigned long arg;
	unsigned long dev;
	unsigned long cnt;
	unsigned char flag;
} ctman_t;

static void CTimerProc(void);
static void RTimerProc(utime_t timec);

//快速执行任务指针(200ms执行一次)
static void (*FastRountine)(void) = NULL;
//系统自启动以来运行的秒数
static int SysJiffies;

static int FeedWatchdog = 1;
int SysTimerTask_watchdog = 0;
/**
* @brief 使能或禁止喂狗
* @param flag 0-禁止, 1-使能
*/
void EnableFeedWatchdog(int flag)
{
	FeedWatchdog = flag;
}



/**
* @brief 系统定时器任务
* @param arg 启动参数
*/
static void *SysTimerTask(void *arg)
{
	sysclock_t clock;
	int countRtimer;

	//int bakJiffies;
	//int fd;


	SysClockRead(&clock);
	/*
	fd = open("/dev/exwdg", O_RDWR);
	if(fd < 0) {
		printf("can not open exwdg!\n");
		return 0;
	}

	write(fd, (char *)&clock, sizeof(clock));  //feed watchdog
	if(0 == (int)arg) ioctl(fd, 1, 1); //enable watchdog
	read(fd, (char *)&SysJiffies, sizeof(SysJiffies));
	bakJiffies = SysJiffies;
	*/

	
	countRtimer = 0;
	SysTimerTask_watchdog = 0;

	
	//if(watchdog_fd>0)
	//{
		//system("killall -9 watchdog");
	//}
	
	while(1) 
	{
		//if(NULL != FastRountine) (*FastRountine)();

		//read(fd, (char *)&SysJiffies, sizeof(SysJiffies));
		//if(SysJiffies != bakJiffies) 
		//{
		//	if(FeedWatchdog) write(fd, (char *)&clock, sizeof(clock));  //feed watchdog
			//CTimerProc();
		//	bakJiffies = SysJiffies;
		//}

		countRtimer++;
		
		if(countRtimer >= 4)   // 0.8s读一次linux系统时间
		{
			countRtimer = 0;
			SysClockRead(&clock);
			RTimerProc(UTimeReadCurrent());
			CTimerProc();
		}
		if(SysTimerTask_watchdog>30)
		{
			SysTimerTask_watchdog = 0;
		}
		
		Sleep(20);
	}
	

	return 0;
}

/**
* @brief 设置快速任务
* @param routine 快速任务入口函数
*/
void SysSetFastRoutine(void (*routine)(void))
{
	FastRountine = routine;
}

/**
* @brief 读取系统自启动以来的运行秒数
* @return 系统自启动以来的运行秒数
*/
int SysReadJiffies(void)
{
	return SysJiffies;
}

#define TMAN_EMPTY   0
#define TMAN_VALID   1

//时钟定时器控制变量区
static rtman_t RTimerList;
static rtman_t RTimerBuffer[MAX_RTIMER];
//相对定时器控制变量区
static ctman_t CTimerList;
static ctman_t CTimerBuffer[MAX_CTIMER];
//同步定时器访问的互斥变量
static sys_mutex_t CTimerMutex, RTimerMutex;
//自增ID,用来防止ID重复
static int CTimerIdAddup = 0;
static int RTimerIdAddup = 0;

/**
* @brief 添加一个相对定时器
* @param dev 定时器执行间隔(以秒为单位)
* @param proc 定时器处理函数
* @param arg 定时器处理函数
*/
int SysAddCTimer(int dev, ctimerproc_t proc, unsigned long arg)
{
	int i;
	ctman_t *p;

	AssertLogReturn(dev<=0, -1, "invalid dev(%d)\n", dev);

	SysLockMutex(&CTimerMutex);

	for(i=0; i<MAX_CTIMER; i++) {
		if(TMAN_EMPTY == CTimerBuffer[i].flag) {
			p = &(CTimerBuffer[i]);
			p->flag = TMAN_VALID;
			p->proc = proc;
			p->arg = arg;
			p->cnt = 0;
			p->dev = dev;
			p->id = (CTimerIdAddup & 0xffff) | (i << 16);
			CTimerIdAddup++;

			p->next = &CTimerList;
			p->prev = CTimerList.prev;
			CTimerList.prev->next = &CTimerBuffer[i];
			CTimerList.prev = &CTimerBuffer[i];

			SysUnlockMutex(&CTimerMutex);
			return p->id;
		}
	}

	SysUnlockMutex(&CTimerMutex);

	ErrorLog("CTimer full\n");
	return -1;
}

/**
* @brief 停止相对定时器
* @param id 定时器id
*/
void SysStopCTimer(int id)
{
	ctman_t *p;
	int offset;

	offset = id>>16;
	if((offset < 0) || (offset >= MAX_CTIMER)) {
		ErrorLog("invalid id(%d)\n", id);
		return;
	}

	p = &CTimerBuffer[offset];

	SysLockMutex(&CTimerMutex);

	if((p->id == id) && (TMAN_EMPTY != p->flag)) {
		p->flag = TMAN_EMPTY;
		p->prev->next = p->next;
		p->next->prev = p->prev;
	}

	SysUnlockMutex(&CTimerMutex);
}

/**
* @brief 停止相对定时器(指针形式)
* @param p 定时器控制变量指针
*/
static void StopCTimerP(ctman_t *p)
{
	if(TMAN_EMPTY != p->flag) {
		p->flag = TMAN_EMPTY;
		p->prev->next = p->next;
		p->next->prev = p->prev;
	}
}

/**
* @brief 清除相对定时器计数器,重新开始计算
* @param id 定时器id
*/
void SysClearCTimer(int id)
{
	ctman_t *p;
	int offset;

	offset = id>>16;
	if((offset < 0) || (offset >= MAX_CTIMER)) {
		ErrorLog("invalid id(%d)\n", id);
		return;
	}

	p = &CTimerBuffer[offset];

	SysLockMutex(&CTimerMutex);

	if((p->id == id) && (TMAN_EMPTY != p->flag)) {
		p->cnt = 0;
	}

	SysUnlockMutex(&CTimerMutex);
}

/**
* @brief 相对定时器处理函数
*/
static void CTimerProc(void)
{
	ctimerproc_t func;
	ctman_t *p, *p1;
	int rc;

	SysLockMutex(&CTimerMutex);

	p = CTimerList.next;
	while(p != &CTimerList) {
		p1 = p->next;

		p->cnt++;
		if(p->cnt >= p->dev) {
			p->cnt = 0;
			func = p->proc;
			rc = (*func)(p->arg);
			if(rc) StopCTimerP(p);
		}

		p = p1;
	}

	SysUnlockMutex(&CTimerMutex);
}

/**
* @brief 根据配置计算基准时间
* @param p 时钟定时器控制变量指针
* @return 基准时间
*/
static utime_t RTimerGetBase(unsigned char mod, const sysclock_t *baseclock)
{
	utime_t rtn;
	sysclock_t clock;

	memcpy(&clock, baseclock, sizeof(clock));

	clock.year = clock.month = 0;
	switch(mod) {
	case UTIMEDEV_MINUTE:
		//clock.minute = 0;
	case UTIMEDEV_HOUR:
		clock.hour = 0;
	case UTIMEDEV_DAY:
		clock.day = 0;
		break;
	default: //month
		if(clock.day) clock.day -= 1;
		break;
	}

	rtn = 0;
	if(0 != clock.day) rtn += ((utime_t)(clock.day)&0xff)*1440*60;
	if(0 != clock.hour) rtn += ((utime_t)(clock.hour)&0xff)*60*60;
	if(0 != clock.minute) rtn += ((utime_t)(clock.minute)&0xff)*60;

	return rtn;
}

/**
* @brief 重新计算定时器出发时间
* @param p 时钟定时器控制变量指针
* @param curtime 当前时间
*/
static void RecalRTimerP(rtman_t *p, utime_t curtime)
{
	sysclock_t clock;

	if(UTIMEDEV_MINUTE == p->dev) {
		if(0 == p->mod) p->dev = 15;
		else p->dev = 1;
	}
	if(p->mod > UTIMEDEV_MONTH) p->mod = UTIMEDEV_HOUR;

	if(p->bonce) return;

	UTimeToSysClock(curtime, &clock);

	switch(p->mod) {
	case UTIMEDEV_MONTH:
		clock.month = 1;
	case UTIMEDEV_DAY:
		clock.day = 1;
	case UTIMEDEV_HOUR:
		clock.hour = 0;
	case UTIMEDEV_MINUTE:
		clock.minute = 0;
		clock.second = 0;
		break;
	default:
		return;
	}

	p->tstart = SysClockToUTime(&clock);
	p->tbase = p->tstart;
	//p->tstart += RTimerGetBase(p);
	p->tstart += p->basetime;
	while(curtime >= p->tstart) {
		p->tbase = UTimeAdd(p->tbase, p->mod, p->dev);
		p->tstart = p->tbase + p->basetime;
		//p->tstart += RTimerGetBase(p);
		//p->tstart += p->basetime;
	}

	//DebugPrint(LOGTYPE_SHORT, "rtimer start: %s\n", UTimeFormat(p->tstart));

	//printf("rcal timer: %s\n", ascii_utime(p->tstart));
	//print_utime(p->tstart, "recal rtimer");
}

/**
* @brief 重新计算时钟定时器
* @param id 定时器id
*/
void SysRecalRTimer(int id)
{
	rtman_t *p;
	int offset;

	offset = id>>16;
	if((offset < 0) || (offset >= MAX_RTIMER)) return;

	p = &RTimerBuffer[offset];

	SysLockMutex(&RTimerMutex);

	if((p->id == id) && (TMAN_EMPTY != p->flag)) {
		RecalRTimerP(p, UTimeReadCurrent());
	}

	SysUnlockMutex(&RTimerMutex);
}

/**
* @brief 重新计算所有时钟定时器
*/
void SysRecalAllRTimer(void)
{
	rtman_t *p;

	DebugPrint(LOGTYPE_ALARM, "recall all rtimer...\n");

	SysLockMutex(&RTimerMutex);

	p = RTimerList.next;
	while(p != &RTimerList) {
		if(TMAN_EMPTY != p->flag) {
			RecalRTimerP(p, UTimeReadCurrent());
		}

		p = p->next;
	}

	SysUnlockMutex(&RTimerMutex);
}

/**
* @brief 添加一个时钟定时器
* @param pconfig 定时器配配置变量指针
* @param proc 定时器处理函数
* @param arg 处理函数参数
* @return 成功时返回定时器id, 否则返回-1
*/
int SysAddRTimer(const rtimer_conf_t *pconf, rtimerproc_t proc, unsigned long arg)
{
	int i;
	rtman_t *p;

	if((0 == pconf->tdev) || (pconf->tmod > UTIMEDEV_MONTH)) {
		ErrorLog("invalid conf(%d, %d)\n", pconf->tdev, pconf->tmod);
		return -1;
	}

	//print_logo(0, "add rtimer: %d, %d\r\n", pconf->tdev, pconf->tmod);

	SysLockMutex(&RTimerMutex);

	for(i=0; i<MAX_RTIMER; i++) {
		if(TMAN_EMPTY == RTimerBuffer[i].flag) {
			p = &(RTimerBuffer[i]);
			p->flag = TMAN_VALID;
			p->proc = proc;
			p->arg = arg;
			p->bonce = pconf->bonce;
			if(pconf->bonce) {
				p->dev = 1;
				p->mod = 1;
				p->tstart = pconf->curtime;
			}
			else {
				p->dev = pconf->tdev;
				p->mod = pconf->tmod;
				//p->basetime = pconf->basetime;
				p->basetime = RTimerGetBase(pconf->tmod, &pconf->basetime);
				RecalRTimerP(p, pconf->curtime);
			}
			p->id = (RTimerIdAddup & 0xffff) | (i << 16);
			RTimerIdAddup++;

			p->next = &RTimerList;
			p->prev = RTimerList.prev;
			RTimerList.prev->next = &RTimerBuffer[i];
			RTimerList.prev = &RTimerBuffer[i];

			SysUnlockMutex(&RTimerMutex);
			return p->id;
		}
	}

	SysUnlockMutex(&RTimerMutex);

	ErrorLog("RTimer full\n");
	return -1;
}

/**
* @brief 停止一个时钟定时器
* @param id 定时器id
*/
void SysStopRTimer(int id)
{
	rtman_t *p;
	int offset;

	offset = id>>16;
	if((offset < 0) || (offset >= MAX_RTIMER)) return;

	p = &RTimerBuffer[offset];

	SysLockMutex(&RTimerMutex);

	if((p->id == id) && (TMAN_EMPTY != p->flag)) {
		p->flag = TMAN_EMPTY;
		p->prev->next = p->next;
		p->next->prev = p->prev;
	}

	SysUnlockMutex(&RTimerMutex);
}

/**
* @brief 停止一个时钟定时器(指针形式)
* @param p 时钟定时器控制变量指针
*/
static void StopRTimerP(rtman_t *p)
{
	if(TMAN_EMPTY == p->flag) return;

	p->flag = TMAN_EMPTY;
	p->prev->next = p->next;
	p->next->prev = p->prev;
}

/**
* @brief 时钟定时器处理函数
* @param timec 当前时间
*/
static void RTimerProc(utime_t timec)
{
	rtimerproc_t func;
	rtman_t *p, *p1;
	int bstart;
	int comp;

	SysLockMutex(&RTimerMutex);

	p = RTimerList.next;
	while(p != &RTimerList) {
		p1 = p->next;
		bstart = 0;

		if(timec >= p->tstart) {
			bstart = 1;
			if(!(p->bonce)) {
				comp = timec - p->tstart;
				if(comp > 3600) {   //延后执行大于一小时重新计算
					RecalRTimerP(p, timec);
				}
				else {
					do {
						p->tbase = UTimeAdd(p->tbase, p->mod, p->dev);
						p->tstart = p->tbase + p->basetime;
						//p->tstart += RTimerGetBase(p);
					} while(timec >= p->tstart);
				}
			}
		}

		if(bstart) {
			func = p->proc;

			(*func)(p->arg, timec);

			if(p->bonce) StopRTimerP(p);
		}

		p = p1;
	}

	SysUnlockMutex(&RTimerMutex);
}

/**
* @brief 定时器模块初始化函数
* @return 返回0表示成功, 否则失败
*/
DECLARE_INIT_FUNC(SysTimerInit);
int SysTimerInit(void)
{
	int i = 0, arg = 0;
	sysclock_t clock;
	extclock_t extclock;

	for(i=0; i<MAX_RTIMER; i++) RTimerBuffer[i].flag = TMAN_EMPTY;
	RTimerList.next = RTimerList.prev = &RTimerList;

	for(i=0; i<MAX_CTIMER; i++) CTimerBuffer[i].flag = TMAN_EMPTY;
	CTimerList.next = CTimerList.prev = &CTimerList;

	SysInitMutex(&CTimerMutex);
	SysInitMutex(&RTimerMutex);

	//if(!GetStartArg('d', NULL, 0)) arg = 1; //不喂狗, 调试用
	//else arg = 0;

	if(ExtClockRead(&extclock)) {
		printf("!read external clock fail!!\n");
		return 1;
	}
	clock.year = extclock.year;
	clock.month = extclock.month;
	clock.day = extclock.day;
	clock.hour = extclock.hour;
	clock.minute = extclock.minute;
	clock.second = extclock.second;

	SysClockSet(&clock);

	SysCreateTask(SysTimerTask, (void *)arg);


	SET_INIT_FLAG(SysTimerInit);

	return 0;
}

int shell_timerinfo(int argc, char *argv[])
{
	char flag;

	if(2 != argc) {
		PrintLog(0, "timerinfo r/c\n");
		return 1;
	}

	flag = argv[1][0];

	if('c' == flag) {
		ctman_t *p;

		p = CTimerList.next;
		while(p != &CTimerList) {

			PrintLog(0, "ctimer %2d: ", ((unsigned int)p-(unsigned int)CTimerBuffer)/sizeof(ctman_t));
			PrintLog(0, "dev=%ds, count=%d\n", p->dev, p->cnt);

			p = p->next;
		}
	}
	else if('r' == flag) {
		rtman_t *p;

		p = RTimerList.next;
		while(p != &RTimerList) {

			PrintLog(0, "rtimer %2d: ", ((unsigned int)p-(unsigned int)RTimerBuffer)/sizeof(rtman_t));
			PrintLog(0, "dev=%d, mod=%d, once=%d\n", p->dev, p->mod, p->bonce);
			PrintLog(0, "  base=%s, ", UTimeFormat(p->basetime));
			PrintLog(0, "start=%s\n", UTimeFormat(p->tstart));

			p = p->next;
		}
	}
	else {
		PrintLog(0, "invalid arg\n");
		return 1;
	}

	PrintLog(0, "end\n");
	return 0;
}
DECLARE_SHELL_CMD("timerinfo",shell_timerinfo, "显示系统定时器信息");

