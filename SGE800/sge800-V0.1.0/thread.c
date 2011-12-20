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
	文件		：thread.c
	描述		：本文件实现了线程操作模块中的API函数
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2009.12
******************************************************************************/
//库配置头文件
#include "private/config.h"

//模块启用开关
#ifdef CFG_THREAD_MODULE

//调试头文件
#include "private/debug.h"

//驱动调用头文件

//C库头文件
#include <stdio.h>                      //printf
#include <pthread.h>                    //pthread


//提供给用户的头文件
#include "include/thread.h"
#include "include/error.h"


/*************************************************
  静态全局变量定义
*************************************************/
static struct {
    u8 prio;                    //0表示普通线程，1-99表示实时线程优先级。
    pthread_t tid;              //0表示未建立，[0]=0表示线程模块未初始化
} thread_info[CFG_THREAD_MAX];

/*************************************************
  API函数实现
*************************************************/
/******************************************************************************
*	函数:	thread_create
*	功能:	建立线程
*	参数:	id				-	线程号
 			function		-	线程函数
 			arg				-	线程函数参数
 			mode			-	线程模式（普通和实时）
 			prio			-	优先级（1-99）
*	返回:	0				-	成功
 			-ERR_NOINIT		-	模块未初始化
 			-ERR_INVAL		-	参数不正确
 			-ERR_BUSY		-	线程已存在
 			-ERR_NOMEM		-	内存不足
*	说明:	只有实时模式支持优先级，99为最高优先级。普通模式优先级固定为0。
 ******************************************************************************/
int thread_create(u8 id, void *(*function)(void *), void *arg, u8 mode, u8 prio)
{
    int ret = 0;
    pthread_t tid;

    if (id > CFG_THREAD_MAX - 1) {
        ret = -ERR_INVAL;
        goto error;
    }
    if (0 != thread_info[id].tid) {
        ret = -ERR_BUSY;
        goto error;
    }

    ret = thread_create_base (&tid, function, arg, mode, prio);
    if (ret) {
    	goto error;
    }
//保存线程信息
    thread_info[id].prio = prio;
    thread_info[id].tid = tid;

error:
    return ret;
}

/******************************************************************************
*	函数:	thread_cancel
*	功能:	终止线程
*	参数:	id				-	线程号
*	返回:	0				-	成功
 			-ERR_NODEV		-	无此线程
 			-ERR_INVAL		-	参数错误
*	说明:
 ******************************************************************************/
int thread_cancel(u8 id)
{
    int ret = 0;

    if (id > CFG_THREAD_MAX - 1) {
        ret = -ERR_INVAL;
        goto error;
    }
    if (0 == thread_info[id].tid) {
        ret = -ERR_NODEV;
        goto error;
    }
//终止线程
    ret = pthread_cancel (thread_info[id].tid);
    if (ret) {
        ret = -ERR_NODEV;
        goto error;
    }
//更新线程信息
    thread_info[id].prio = 0;
    thread_info[id].tid = 0;

error:
    return ret;
}

/******************************************************************************
*	函数:	thread_setpriority
*	功能:	设置线程优先级
*	参数:	id				-	线程号
 			prio			-	优先级
*	返回:	0				-	成功
 			-ERR_NODEV		-	无此线程
 			-ERR_INVAL		-	参数错误
*	说明:	优先级为1-99，99为最高优先级。
 ******************************************************************************/
int thread_setpriority(u8 id, u8 prio)
{
    int ret = 0;

    if (id > CFG_THREAD_MAX - 1) {
        ret = -ERR_INVAL;
        goto error;
    }
    if (0 == thread_info[id].tid) {
        ret = -ERR_NODEV;
        goto error;
    }
//设置优先级
    ret = thread_setpriority_base(thread_info[id].tid, prio);
    if (ret) {
    	goto error;
    }
//更新线程信息
    thread_info[id].prio = prio;

error:
    return ret;
}

/******************************************************************************
*	函数:	thread_getpriority
*	功能:	获取优先级
*	参数:	id				-	线程号
 			prio			-	优先级（数据传出）
*	返回:	0				-	成功
 			-ERR_NODEV		-	无此线程
 			-ERR_INVAL		-	参数错误
*	说明:	实时线程优先级为1-99，99为最高优先级；普通线程固定为0。
 ******************************************************************************/
int thread_getpriority(u8 id, u8 *prio)
{
    int ret = 0;

    if (id > CFG_THREAD_MAX - 1) {
        ret = -ERR_INVAL;
        goto error;
    }
    if (0 == thread_info[id].tid) {
        ret = -ERR_NODEV;
        goto error;
    }

    *prio = thread_info[id].prio;

error:
    return ret;
}

/******************************************************************************
*	函数:	thread_gettid_fromid
*	功能:	根据内部线程号得到系统线程号
*	参数:	id				-	线程号
 			prio			-	优先级（数据传出）
*	返回:	0				-	此内部线程号未用
			>0				-	系统线程号
 			-ERR_INVAL		-	参数错误
*	说明:
 ******************************************************************************/
int thread_gettid_fromid(u8 id)
{
    int ret = 0;

    if (id > CFG_THREAD_MAX - 1) {
        ret = -ERR_INVAL;
        goto error;
    }
    ret = thread_info[id].tid;
error:
    return ret;
}

/******************************************************************************
*	函数:	thread_create_base
*	功能:	建立线程
*	参数:	tid				-	系统线程号
 			function		-	线程函数
 			arg				-	线程函数参数
 			mode			-	线程模式（普通和实时）
 			prio			-	优先级（1-99）
*	返回:	0				-	成功
 			-ERR_INVAL		-	参数不正确
 			-ERR_NOMEM		-	内存不足
*	说明:	只有实时模式支持优先级，99为最高优先级。普通模式优先级固定为0。
 ******************************************************************************/
int thread_create_base(pthread_t *tid, void *(*function)(void *), void *arg, u8 mode, u8 prio)
{
    int ret = 0;
    pthread_attr_t attr;
    struct sched_param param;

//初始化线程属性
    ret = pthread_attr_init(&attr);
    if (ret) {
        ret = -ERR_NOMEM;
        goto error;
    }
//设置线程为分离状态
    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret) {
        ret = -ERR_INVAL;
        goto error;
    }
//设置优先级
    if (THREAD_MODE_NORMAL == mode) {
        if (0 != prio) {
            ret = -ERR_INVAL;
            goto error;
        }
    }
    else if (THREAD_MODE_REALTIME == mode) {
        if ((prio < 1) || (prio > 99)) {			//1是实时线程最低优先级，99是最高优先级
            ret = -ERR_INVAL;
            goto error;
        }
        ret = pthread_attr_setschedpolicy(&attr, SCHED_RR);
        if (ret) {
            ret = -ERR_INVAL;
            goto error;
        }
        param.sched_priority = prio;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) {
            ret = -ERR_INVAL;
            goto error;
        }
    }
    else {
        ret = -ERR_INVAL;
        goto error;
    }
//建立线程
    ret = pthread_create (tid, &attr, function, arg);
    if (ret) {
        ret = -ERR_NOMEM;
        goto error;
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	thread_setpriority_base
*	功能:	设置线程优先级
*	参数:	tid				-	系统线程号
 			prio			-	优先级
*	返回:	0				-	成功
 			-ERR_INVAL		-	参数错误
*	说明:	优先级为1-99，99为最高优先级。
 ******************************************************************************/
int thread_setpriority_base(pthread_t tid, u8 prio)
{
    int ret = 0;
    struct sched_param param;

    //参数有效性判断
	if ((prio < 1) || (prio > 99)) {			//1是实时线程最低优先级，99是最高优先级
		ret = -ERR_INVAL;
		goto error;
	}
	//设置优先级
	param.sched_priority = prio;
	ret = pthread_setschedparam(tid, SCHED_RR, &param);
	if(ret){
		ret = -ERR_INVAL;
		goto error;
	}
error:
    return ret;
}

/******************************************************************************
*	函数:	thread_gettid_base
*	功能:	得到当前系统线程号
*	参数:	void
*	返回:	*				-	系统线程号
*	说明:
 ******************************************************************************/
int thread_gettid_base(void)
{
    return pthread_self();
}

#endif /* CFG_THREAD_MODULE */
