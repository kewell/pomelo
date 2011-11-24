/**
* timer.h -- 定时器操作接口头文件
* 
* 
* 创建时间: 2008-5-16
* 最后修改时间: 2010-3-31
*/

#ifndef _SYS_TIMER_H
#define _SYS_TIMER_H

/*
* 绝对定时器处理函数格式
* arg为增加定时器时指定的参数
* time为处理时的当前时间
*/
typedef void (*rtimerproc_t)(unsigned long arg, utime_t time);

/*
* 相对定时器处理函数格式
* arg为增加定时器时指定的参数
* 函数返回0时表示继续执行定时器
* 返回非零值表示停止该定时器的执行
*/
typedef int (*ctimerproc_t)(unsigned long arg);

/**
* @brief 设置快速任务
* @param routine 快速任务入口函数
*/
void SysSetFastRoutine(void (*routine)(void));

/**
* @brief 添加一个相对定时器
* @param dev 定时器执行间隔(以秒为单位)
* @param proc 定时器处理函数
* @param arg 定时器处理函数
*/
int SysAddCTimer(int dev, ctimerproc_t proc, unsigned long arg);
/**
* @brief 停止相对定时器
* @param id 定时器id
*/
void SysStopCTimer(int id);
/**
* @brief 清除相对定时器计数器,重新开始计算
* @param id 定时器id
*/
void SysClearCTimer(int id);

//时钟定时器配置数据结构
typedef struct {
	utime_t curtime;  //当前时间
	unsigned char bonce;  //是否只执行一次
	unsigned char tdev;  //执行时间间隔数值
	unsigned char tmod;  //执行事件间格单位(参见utime_add)
	sysclock_t basetime;  //执行基准时间
	                                 //如时间间隔为1小时,基准时间的分为5,则每天的0:05, 1:05, 2:05, ...执行
} rtimer_conf_t;

/**
* @brief 重新计算时钟定时器
* @param id 定时器id
*/
void SysRecalRTimer(int id);
/**
* @brief 重新计算所有时钟定时器
*/
void SysRecalAllRTimer(void);
/**
* @brief 添加一个时钟定时器
* @param pconfig 定时器配配置变量指针
* @param proc 定时器处理函数
* @param arg 处理函数参数
* @return 成功时返回定时器id, 否则返回-1
*/
int SysAddRTimer(const rtimer_conf_t *pconf, rtimerproc_t proc, unsigned long arg);
/**
* @brief 停止一个时钟定时器
* @param id 定时器id
*/
void SysStopRTimer(int id);

#endif /*_SYS_TIMER_H*/
