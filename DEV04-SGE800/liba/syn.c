/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/******************************************************************************
	项目名称	：SGE800计量智能终端平台
	文件		：syn.c
	描述		：本文件实现了同步模块中的API函数
	版本		：0.1
	作者		：孙锐
	创建日期	：2009.12
******************************************************************************/


//库配置头文件
#include "private/config.h"

//模块启用开关
#ifdef CFG_SYN_MODULE

//调试头文件
#include "private/debug.h"

//驱动调用头文件

//C库头文件
#include <stdio.h>						//printf
#include <string.h>						//bzero
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>

//提供给用户的头文件
#include "include/syn.h"
#include "include/error.h"


/*************************************************
  静态全局变量定义
*************************************************/
static struct{
	u8 state;                   //0表示未被使用，1表示已使用
	pthread_mutex_t mutex;		
}mutex_info[CFG_MUTEX_MAX];

static struct {
	u8 state;                   //0表示未被使用，1表示已使用
	sem_t sem;					
} sem_info[CFG_SEM_MAX];

static struct {
	u8 state;                   //0表示未被使用，1表示已使用
	pthread_rwlock_t rwlp;		       
}lock_info[CFG_RWLOCK_MAX];


/*************************************************
  API函数实现
*************************************************/
/******************************************************************************
*	函数:	syn_init
*	功能:	初始化同步模块
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误

*	说明:	初始化数据库环境和数据库变量。
******************************************************************************/
int syn_init(void)
{
	int i;
	for (i=0; i<CFG_MUTEX_MAX; i++)    
	{
		mutex_info[i].state = 0;               //初始化互斥锁使用状态
	}
	
	for (i=0; i<CFG_SEM_MAX; i++)    
	{
		sem_info[i].state = 0;                //初始化信号使用状态
	}
	
	for (i=0; i<CFG_RWLOCK_MAX; i++)    
	{
		lock_info[i].state = 0;               //初始化信号使用状态
	}
	return(0);

}

/******************************************************************************
*	函数:	syn_mutex_init
*	功能:	创建互斥锁
*	参数:	id				-	互斥锁标号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_BUSY		-	此互斥锁已初始化

*	说明:	无
******************************************************************************/
int syn_mutex_init(u8 id)
{
	int ref;
	int ret = 0;
	
	const pthread_mutex_t mutex_zero = PTHREAD_MUTEX_INITIALIZER;
	if ((id < 0) || (id >= CFG_MUTEX_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	
	if (mutex_info[id].state == 1)
	{
		ret = -ERR_BUSY;
		goto error;
	}
	
	memcpy(&mutex_info[id].mutex, &mutex_zero, sizeof(pthread_mutex_t));
	ref = pthread_mutex_init(&mutex_info[id].mutex, NULL);
	if (ref == EBUSY)
	{
		ret = -ERR_BUSY;
		goto error;
	}
	else if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}
	else
	{
		ret = 0;
		mutex_info[id].state = 1;
	}

error:
	return(ret);
}

/******************************************************************************
*	函数:	syn_mutex_lock
*	功能:	锁定互斥锁
*	参数:	id				-	互斥锁标号
			timeout      	-	0为永久等待，0xffff为不等待，其余为等待秒数
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_NOINIT  	-	 此互斥锁未初始化（未使用）
 			-ERR_BUSY   	-	已被其他线程锁定
 			-ERR_TIMEOUT 	-	超时
*	说明:	无
******************************************************************************/
int syn_mutex_lock(u8 id, u16 timeout)
{
	int ref;
	int ret = 0;
	struct timespec to;
	
	if ((id < 0) || (id >= CFG_MUTEX_MAX))      //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	if (mutex_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	
	if (timeout == SYN_RECV_BLOCK){           //永久等待模式
		ref = pthread_mutex_lock(&mutex_info[id].mutex);
		if (ref)
    	{
    		ret = -ERR_SYS;
    		goto error;
    	}
	}
	else if (timeout == SYN_RECV_NONBLOCK)  //不等待模式
	{
		ref = pthread_mutex_trylock(&mutex_info[id].mutex);
		if (ref== EBUSY)
		{
			ret = -ERR_BUSY;
			goto error;
		}
		else if (ref)
		{
			ret = -ERR_SYS;
			goto error;
		}
	}
	else                                      //超时阻塞
	{
    	to.tv_sec = time(NULL) + timeout;     //秒
    	to.tv_nsec = 0;                       //纳秒
    	ref = pthread_mutex_timedlock(&(mutex_info[id].mutex), &to);
		if (ref == ETIMEDOUT)
		{
			ret = -ERR_TIMEOUT;
			goto error;
		}
		else if(ref)
		{
			ret = -ERR_SYS;
			goto error;
		}
    }
error:
	return(ret);
	
}

/******************************************************************************
*	函数:	syn_mutex_unlock
*	功能:	解除互斥锁
*	参数:	id				-	互斥锁标号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_NOINIT  	-	 此互斥锁未初始化（未使用）
*	说明:
******************************************************************************/
int syn_mutex_unlock(u8 id)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_MUTEX_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	if (mutex_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	//解除互斥锁
	ref = pthread_mutex_unlock(&mutex_info[id].mutex);
	if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}

error:
	return(ret);
}

/******************************************************************************
*	函数:	syn_mutex_destroy
*	功能:	销毁互斥锁
*	参数:	id				-	互斥锁标号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_NOINIT  	-	 此互斥锁未初始化（未使用）
 			-ERR_BUSY  		-	被锁定不能销毁
*	说明:	无
******************************************************************************/
int syn_mutex_destroy(u8 id)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_MUTEX_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	
	if (mutex_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	//销毁互斥锁
	ref = pthread_mutex_destroy(&mutex_info[id].mutex);
	if (ref == EBUSY )
	{
		ret = -ERR_BUSY;
		goto error;
	}
	else if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}
	else
	{
		ret = 0;
		mutex_info[id].state = 0;
	}

error:
	return(ret);
}


/***************************信号量************************************************/

/******************************************************************************
*	函数:	syn_sem_init
*	功能:	初始化信号量
*	参数:	id				-	信号量标号
			value			-	信号量初值
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_BUSY  		-	 此信号量已初始化
*	说明:	无
******************************************************************************/
int syn_sem_init(u8 id, int value)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_SEM_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	
	if (sem_info[id].state == 1)
	{
		ret = -ERR_BUSY;
		goto error;
	}
	
	ref = sem_init(&sem_info[id].sem, 0, value);
	if (ref == EINVAL)
	{
		ret = -ERR_INVAL;
		goto error;
	}
	else if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}
	else
	{
		ret = 0;
		sem_info[id].state = 1;
	}
error:
	return (ret);	
}

/******************************************************************************
*	函数:	syn_sem_wait
*	功能:	信号量减一
*	参数:	id				-	信号量标号
			timeout    		-	0为永久等待，0xffff为不等待，其余为等待秒数
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_BUSY  		-	 信号量已为锁定状态，不能立刻锁定
*	说明:	无
******************************************************************************/
int syn_sem_wait(u8 id, u16 timeout)
{
	int ref;
	int ret = 0;
	struct timespec to;
	int errnum;

	if ((id < 0) || (id >= CFG_SEM_MAX))        //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	if (sem_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	
	if (timeout == SYN_RECV_BLOCK){           //永久等待模式
		ref = sem_wait(&sem_info[id].sem);
		if (ref)
    	{
    		ret = -ERR_SYS;
    		goto error;
    	}
	}
	else if (timeout == SYN_RECV_NONBLOCK)  //不等待模式
	{
		ref = sem_trywait(&sem_info[id].sem);
		if (ref < 0)
		{
			errnum = errno;
			if (errnum == EAGAIN)                  //返回错误
			{
				ret = -ERR_BUSY;
				goto error;
			}
			else
			{
				ret = -ERR_SYS;
				goto error;
			}
		}
	}
	else                                      //超时阻塞
	{
    	to.tv_sec = time(NULL) + timeout;     //秒
    	to.tv_nsec = 0;                       //纳秒
    	ref = sem_timedwait(&(sem_info[id].sem), &to);
    	if (ref < 0)
		{
			errnum = errno;
			if (errnum == ETIMEDOUT)
			{
				ret = -ERR_TIMEOUT;
				goto error;
			}
			else
			{
				ret = -ERR_SYS;
				goto error;
			}
		}
}
error:
	return(ret);
}

/******************************************************************************
*	函数:	syn_sem_post
*	功能:	信号量加一，同时发出信号唤醒等待
*	参数:	id				-	信号量标号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_NOINIT		-	此信号量未初始化
*	说明:	无
******************************************************************************/

int syn_sem_post(u8 id)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_SEM_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	
	if (sem_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	//增加信号
	ref = sem_post(&sem_info[id].sem);
	if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}
	
error:
	return(ret);
}

/******************************************************************************
*	函数:	syn_sem_getvalue
*	功能:	获得信号量的值
*	参数:	id				-	信号量标号
			value      		-	信号量值（返回）
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_NOINIT		-	此信号量未初始化
*	说明:	无
******************************************************************************/
int syn_sem_getvalue(u8 id, int *value)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_SEM_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	if (sem_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	ref = sem_getvalue(&sem_info[id].sem, value);
	if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}
	
error:
	return (ret);
	
}

/******************************************************************************
*	函数:	syn_sem_destroy
*	功能:	销毁信号量
*	参数:	id				-	信号量标号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_NOINIT		-	此信号量未初始化
*	说明:	无
******************************************************************************/
int syn_sem_destroy(u8 id)
{
	int ref;
	int ret = 0;
	int errnum;
	
	if ((id < 0) || (id >= CFG_SEM_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	
	if (sem_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	//销毁互斥锁
	ref = sem_destroy(&sem_info[id].sem);
	if (ref < 0)
	{
		errnum = errno;
		if (errnum == EINVAL )
		{
			ret = -ERR_NOINIT;
			goto error;
		}
		else
		{
			ret = -ERR_SYS;
			goto error;
		}
	}
	else
	{
		mutex_info[id].state = 0;
	}

error:
	return(ret);
}


/***********************读写锁************************************/
/******************************************************************************
*	函数:	syn_rwlock_init
*	功能:	读写锁初始化
*	参数:	id				-	读写锁标号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_BUSY		-	此读写锁已初始化
*	说明:	无
******************************************************************************/
int syn_rwlock_init(u8 id)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_RWLOCK_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	
	if (lock_info[id].state == 1)
	{
		ret = -ERR_BUSY;
		goto error;
	}
	ref = pthread_rwlock_init(&lock_info[id].rwlp, NULL);
	if (ref == EBUSY)
	{
		ret = -ERR_BUSY;
		goto error;
	}
	else if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}
	else
	{
		ret = 0;
		lock_info[id].state = 1;
	}

error:
	return(ret);
}

/******************************************************************************
*	函数:	syn_rwlock_rd
*	功能:	获取读锁
*	参数:	id				-	读写锁标号
			timeout     	-	0:永久等待；0xffff表示不等待，不支持超时时间
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_BUSY		-	已被其他线程锁定
			-ERR_NOINIT  	-	此读写锁未初始化（未使用）
			-ERR_TIMEOUT	-	超时
*	说明:	无
******************************************************************************/
int syn_rwlock_rd(u8 id, u8 timeout)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_RWLOCK_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	if (lock_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	
	if (timeout == SYN_RECV_BLOCK){           //永久等待模式
		ref = pthread_rwlock_rdlock(&lock_info[id].rwlp);
		if (ref)
    	{
    		ret = -ERR_SYS;
    		goto error;
    	}
	}
	else if (timeout == SYN_RECV_NONBLOCK)  //不等待模式
	{
		ref = pthread_rwlock_tryrdlock(&lock_info[id].rwlp);
		if (ref == EBUSY)          //返回错误
		{
			ret = -ERR_BUSY;
			goto error;
		}
		else if (ref)
		{
			ret = -ERR_SYS;
			goto error;
		}
	}
	else                                      //超时阻塞
	{
    	ret = -ERR_INVAL;
    	goto error;
    }
error:
	return(ret);
	
}

/******************************************************************************
*	函数:	syn_rwlock_wr
*	功能:	获取写锁
*	参数:	id				-	读写锁标号
			timeout     	-	0:永久等待；0xffff表示不等待，不支持超时时间
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_BUSY		-	已被其他线程锁定
			-ERR_NOINIT  	-	此读写锁未初始化（未使用）
			-ERR_TIMEOUT	-	超时
*	说明:	无
******************************************************************************/
int syn_rwlock_wr(u8 id, u8 timeout)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_RWLOCK_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	if (lock_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	
	if (timeout == SYN_RECV_BLOCK){           //永久等待模式
		ref = pthread_rwlock_wrlock(&lock_info[id].rwlp);
		if (ref)
    	{
    		ret = -ERR_SYS;
    		goto error;
    	}
	}
	else if (timeout == SYN_RECV_NONBLOCK)  //不等待模式
	{
		ref = pthread_rwlock_trywrlock(&lock_info[id].rwlp);
		if (ref == EBUSY)                  //返回错误
		{
			ret = -ERR_BUSY;
			goto error;
		}
		else if (ref)
		{
			ret = -ERR_SYS;
			goto error;
		}
	}
	else                                      //超时阻塞
	{
    	ret = -ERR_INVAL;
    	goto error;
    }
error:
	return(ret);
}

/******************************************************************************
*	函数:	syn_rwlock_unlock
*	功能:	解除锁
*	参数:	id				-	读写锁标号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_NOINIT  	-	此读写锁未初始化（未使用）
*	说明:	无
******************************************************************************/
int syn_rwlock_unlock(u8 id)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_RWLOCK_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	if (lock_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	//解除互斥锁
	ref = pthread_rwlock_unlock(&lock_info[id].rwlp);
	if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}
error:
	return(ret);
}

/******************************************************************************
*	函数:	syn_rwlock_destroy
*	功能:	销毁互斥锁
*	参数:	id				-	读写锁标号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
			-ERR_NOINIT  	-	此读写锁未初始化（未使用）
			-ERR_BUSY 		-	被锁定不能销毁
*	说明:	无
******************************************************************************/
int syn_rwlock_destroy(u8 id)
{
	int ref;
	int ret = 0;
	
	if ((id < 0) || (id >= CFG_RWLOCK_MAX))     //判断接口
	{
		ret = -ERR_INVAL;
		goto error;
	}
	
	if (lock_info[id].state == 0)
	{
		ret = -ERR_NOINIT;
		goto error;
	}
	//销毁互斥锁
	ref = pthread_rwlock_destroy(&lock_info[id].rwlp);
	if (ref == EBUSY )
	{
		ret = -ERR_BUSY;
		goto error;
	}
	else if (ref)
	{
		ret = -ERR_SYS;
		goto error;
	}
	else
	{
		ret = 0;
		lock_info[id].state = 0;
	}

error:
	return(ret);
}

#endif      /* CFG_SYN_MODULE */
