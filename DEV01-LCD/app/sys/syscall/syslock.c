/**
* syslock.c -- 系统资源锁接口函数
* 在某些操作前需要禁止文件操作, 如掉电
* 
* 
* 创建时间: 2010-5-5
* 最后修改时间: 2010-5-5
*/

#include "include/debug.h"
#include "include/sys/mutex.h"
#include "sys/sys_config.h"

static int SysLockNum = 0;
static sys_mutex_t SysLock[MAX_SYSLOCK];

/**
* @brief 注册一个系统资源锁
* @return 成功返回资源锁id, 否则返回-1
*/
int RegisterSysLock(void)
{
	int i;

	AssertLogReturn(SysLockNum>=MAX_SYSLOCK, -1, "syslock too much\n");

	i = SysLockNum;
	SysInitMutex(&SysLock[i]);
	SysLockNum++;

	return(i);
}

/**
* @brief 锁住一个系统资源锁
* @param id 系统资源锁id
*/
void LockSysLock(int id)
{
	
	if((id < 0) || (id >= SysLockNum)) {
		ErrorLog("invalid id(%d)\n", id);
		return;
	}

	SysLockMutex(&SysLock[id]);
}

/**
* @brief 解锁一个系统资源锁
* @param id 系统资源锁id
*/
void UnlockSysLock(int id)
{
	if((id < 0) || (id >= SysLockNum)) {
		ErrorLog("invalid id(%d)\n", id);
		return;
	}

	SysUnlockMutex(&SysLock[id]);
}

/**
* @brief 禁止所有注册了系统资源锁的操作
*/
void SysLockHalt(void)
{
	int i;

	for(i=0; i<SysLockNum; i++) {
		SysLockMutex(&SysLock[i]);
	}
}

