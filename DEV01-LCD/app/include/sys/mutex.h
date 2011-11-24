/**
* mutex.c -- 互斥操作函数头文件
* 
* 
* 创建时间: 2008-5-16
* 最后修改时间: 2010-4-23
*/

#ifndef _SYS_MUTEX_H
#define _SYS_MUTEX_H

#include <unistd.h>
#include <pthread.h>

///互斥控制变量类型
#define sys_mutex_t 		pthread_mutex_t

/**
* @brief 初始化互斥控制变量
* @param p 互斥控制变量指针
*/
#define SysInitMutex(p)		pthread_mutex_init(p, NULL)

/**
* @brief 锁住互斥控制变量
*     如果该变量没有被锁, 则锁住该变量
*     如果该变量被锁, 则任务挂起, 直到锁住变量的任务释放
* @param p 互斥控制变量指针
*/
#define SysLockMutex(p)		pthread_mutex_lock(p)

/**
* @brief 释放互斥控制变量
* @param p 互斥控制变量指针
*/
#define SysUnlockMutex(p)	pthread_mutex_unlock(p)

#if 0  /*目前版本pthread不支持读写锁*/
///读写锁控制变量类型
#define sys_rwlock_t		pthread_rwlock_t

/**
* @brief 初始化读写锁控制变量
* @param p 读写锁控制变量指针
*/
#define SysInitRwLock(p)	pthread_rwlock_init(p, NULL)

/**
* @brief 锁定读写锁(以读为目的)
* @param p 读写锁控制变量指针
*/
#define SysReadLockRwLock(p)	pthread_rwlock_rdlock(p)

/**
* @brief 锁定读写锁(以写为目的)
* @param p 读写锁控制变量指针
*/
#define SysWriteLockRwLock(p)	pthread_rwlock_wrlock(p)

/**
* @brief 解锁读写锁
* @param p 读写锁控制变量指针
*/
#define SysUnLockRwLock(p)		pthread_rwlock_unlock(p)
#else
#endif

#endif /*_SYS_MUTEX_H*/

