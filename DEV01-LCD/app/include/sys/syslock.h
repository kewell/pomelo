/**
* syslock.c -- 系统资源锁接口函数头文件
* 
* 
* 创建时间: 2010-5-5
* 最后修改时间: 2010-5-5
*/

#ifndef _SYSLOCK_H
#define _SYSLOCK_H

/**
* @brief 注册一个系统资源锁
* @return 成功返回资源锁id, 否则返回-1
*/
int RegisterSysLock(void);
/**
* @brief 锁住一个系统资源锁
* @param id 系统资源锁id
*/
void LockSysLock(int id);
/**
* @brief 解锁一个系统资源锁
* @param id 系统资源锁id
*/
void UnlockSysLock(int id);

/**
* @brief 禁止所有注册了系统资源锁的操作
*/
void SysLockHalt(void);

#endif /*_SYSLOCK_H*/

