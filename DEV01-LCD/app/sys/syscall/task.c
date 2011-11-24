/**
* task.c -- 任务系统接口
* 
* 
* 创建时间: 2008-5-16
* 最后修改时间: 2010-3-30
*/

#include <unistd.h>
#include <pthread.h>

#include "sys/sys_config.h"

static pthread_t ThreadIds[MAX_PTHREAD];
static int ThreadNumbers = 0;//模块内的静态变量，其它模块不能访问

int SysCreateTask(void *(*routine)(void *), void *arg)
{
	if(ThreadNumbers >= MAX_PTHREAD) return 1;

	pthread_create(&ThreadIds[ThreadNumbers], NULL, routine, arg);
	ThreadNumbers++;

	return 0;
}

void SysWaitTaskEnd(void)
{
	int i;

	for(i=0; i<ThreadNumbers; i++)
	{
		pthread_join(ThreadIds[i], NULL);
	}
}

