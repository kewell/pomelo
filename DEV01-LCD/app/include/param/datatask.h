/**
* datatask.h -- 任务参数头文件
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#ifndef _PARAM_DATATASK_H
#define _PARAM_DATATASK_H

#include "include/param/capconf.h"

//F65 定时上报1类数据任务设置
//F66 定时上报2类数据任务设置

#define MAX_TASK_DUID		64

typedef struct {
	unsigned char da[2];    //信息点
	unsigned char dt[2];    //信息类
} task_duid_t;

#define TASKTYPE_CLS1    1
#define TASKTYPE_CLS2    2

typedef struct {
	unsigned char valid;
	unsigned char dev_snd;   //定时上报周期
	unsigned char mod_snd;  //0~3依次表示分、时、日、月
	unsigned char base_year;  //上报基准时间: 年
	unsigned char base_month;
	unsigned char base_day;
	unsigned char base_hour;
	unsigned char base_minute;
	unsigned char base_second;
	unsigned char base_week;
	unsigned char freq; //曲线数据抽取倍率
	unsigned char num;
	task_duid_t duid[MAX_TASK_DUID];
} para_task_t;

typedef struct {
	para_task_t cls1[MAX_DTASK_CLS1];
	para_task_t cls2[MAX_DTASK_CLS2];
} save_dtask_t;

#ifndef DEFINE_PARATASK
extern const save_dtask_t ParaTask;
#define ParaTaskCls1	(ParaTask.cls1)
#define ParaTaskCls2	(ParaTask.cls2)
#endif

#endif /*_PARAM_DATATASK_H*/

