/**
* event.h -- 任务接口头文件
* 
* 
* 创建时间: 2010-3-30
* 最后修改时间: 2010-3-31
*/

#ifndef _SYS_TASK_H
#define _SYS_TASK_H

#include <unistd.h>

/**
* @brief 创建一个任务(线程)
* @param routine 任务起始执行函数
* @param arg 成功返回0, 失败返回非零值
*/
int SysCreateTask(void *(*routine)(void *), void *arg);

/**
* @brief 等待所有任务(线程)结束,主进程调用
*/
void SysWaitTaskEnd(void);

extern int SysTimerTask_watchdog;
extern int EthShellTask_watchdog;
extern int PlFrezTask_watchdog;
extern int PlcTask_watchdog;
extern int GprsActiveTask_watchdog;
extern int PlcTask_watchdog;
extern int PlFrezTask_watchdog;
#endif /*_SYS_TASK_H*/
