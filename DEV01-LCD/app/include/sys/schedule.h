#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include <unistd.h>

/**
* @brief 任务睡眠一段时间
* @param ticks -- 睡眠的事件, 以10ms为单位
*/
#define Sleep(ticks)    usleep((unsigned long)(ticks)*10000)

#endif /*_SCHEDULE_H*/
