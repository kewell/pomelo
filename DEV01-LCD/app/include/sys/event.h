/**
* event.h -- 事件接口头文件
* 
* 
* 创建时间: 2010-3-30
* 最后修改时间: 2010-3-30
*/

#ifndef _SYS_EVENT_H
#define _SYS_EVENT_H

#include <unistd.h>
#include <pthread.h>

/**
* 每个事件变量可以包含32个事件,
* 由一个32bit整数表示, 每bit代表一个事件,
* 该bit置1表示该事件发生,置0表示未发生
* 每个事件的含义自定义
*/

///事件控制变量结构
typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned long event;
} sys_event_t;

/**
* @brief 初始化事件控制变量
* @param pctrl 事件控制变量指针
*/
void SysInitEvent(sys_event_t *pctrl);

/**
* @brief 等待事件
* @param pctrl 事件控制变量指针
* @param bwait 等待时是否挂起, 
*          如果为0, 则不管有没有收到想接收的事件, 函数都将直接返回
*          如果为1, 则如果没有收到想接收的事件, 任务将挂起, 直到收到
* @param waitmask 需要等待的事件
* @param pevent 接收到的事件
*/
void SysWaitEvent(sys_event_t *pctrl, int bwait, unsigned long waitmask, unsigned long *pevent);

/**
* @brief 发送事件
* @param pctrl 事件控制变量指针
* @param event 需要发送的事件
*/
void SysSendEvent(sys_event_t *pctrl, unsigned long event);

#endif /*_SYS_EVENT_H*/
