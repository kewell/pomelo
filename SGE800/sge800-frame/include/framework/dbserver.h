/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：dbserver.h
	描述		：本文件定义了数据访问模块接口
	版本		：0.1
	作者		：孙锐
	创建日期	：2010.12
******************************************************************************/

#ifndef _DBSERVER_H
#define _DBSERVER_H

#include "base.h"
#include "systime.h"

/*************************************************
  宏定义
*************************************************/



/*************************************************
  结构类型定义
*************************************************/
//数据项枚举定义
typedef enum {
	data111
} data_enum_t;

//参数项枚举定义
typedef enum {
	para111
} para_enum_t;



/*************************************************
  API
*************************************************/
//数据访问模块初始化int dbserver_init(void);

/************************测量点实时数据**************************/
//读 一个测量点 一个实时数据
int read_mp_rt_data (u16 mp, data_enum_t item, s64 *data);
//写 一个测量点 一个实时数据
int write_mp_rt_data (u16 mp, data_enum_t item, s64 *data);
//清 一个测量点 所有实时数据
int clear_mp_rt_data (u16 mp);
//清 所有测量点 所有实时数据
int clear_allmp_rt_data (void);



/************************测量点历史数据**************************/
//读 一个测量点 一个时间 一个日数据
int read_mp_day_data (u16 mp, st_ymd_t *tm, data_enum_t item, s64 *data);
//写 一个测量点 一个时间 一个日数据
int write_mp_day_data (u16 mp, st_ymd_t *tm, data_enum_t item, s64 *data);
//读 一个测量点 一个时间 一个月数据
int read_mp_month_data (u16 mp, st_ym_t *tm, data_enum_t item, s64 *data);
//写 一个测量点 一个时间 一个月数据
int write_mp_month_data (u16 mp, st_ym_t *tm, data_enum_t item, s64 *data);
//读 一个测量点 一个时间 一个曲线数据
int read_mp_curve_data (u16 mp, st_ymdhm_t *tm, data_enum_t item, s64 *data);
//写 一个测量点 一个时间 一个曲线数据
int write_mp_curve_data (u16 mp, st_ymdhm_t *tm, data_enum_t item, s64 *data);
//清 一个测量点 所有历史数据
int clear_mp_history_data (u16 mp);
//清 所有测量点 所有历史数据
int clear_allmp_history_data (void);



/************************直流模拟量实时数据**********************/
//读 一个直流模拟量 一个实时数据
int read_dc_rt_data (u8 chn, s64 *data);
//写 一个直流模拟量 一个实时数据
int write_dc_rt_data (u8 chn, s64 *data);
//清 一个直流模拟量 所有实时数据
int clear_dc_rt_data (u16 mp);
//清 所有直流模拟量 所有实时数据
int clear_alldc_rt_data (void);



/************************直流模拟量历史数据**********************/
//读 一个直流模拟量 一个时间 一个日数据
int read_dc_day_data (u8 chn, st_ymd_t *tm, s64 *data);
//写 一个直流模拟量 一个时间 一个日数据
int write_dc_day_data (u8 chn, st_ymd_t *tm, s64 *data);
//读 一个直流模拟量 一个时间 一个月数据
int read_dc_month_data (u8 chn, st_ym_t *tm, s64 *data);
//写 一个直流模拟量 一个时间 一个月数据
int write_dc_month_data (u8 chn, st_ym_t *tm, s64 *data);
//读 一个直流模拟量 一个时间 一个曲线数据
int read_dc_curve_data (u8 chn, st_ymdhm_t *tm, s64 *data);
//写 一个直流模拟量 一个时间 一个曲线数据
int write_dc_curve_data (u8 chn, st_ymdhm_t *tm, s64 *data);
//清 一个直流模拟量 所有历史数据
int clear_dc_history_data (u16 mp);
//清 所有直流模拟量 所有历史数据
int clear_alldc_history_data (void);



/************************总加组实时数据**************************/
//读 一个总加组 一个实时数据
int read_zj_rt_data (u8 gp, data_enum_t item, s64 *data);
//写 一个总加组 一个实时数据
int write_zj_rt_data (u8 gp, data_enum_t item, s64 *data);
//清 一个总加组 所有实时数据
int clear_zj_rt_data (u16 mp);
//清 所有总加组 所有实时数据
int clear_allzj_rt_data (void);



/************************总加组历史数据**************************/
//读 一个总加组 一个时间 一个日数据
int read_zj_day_data (u8 gp, st_ymd_t *tm, data_enum_t item, s64 *data);
//写 一个总加组 一个时间 一个日数据
int write_zj_day_data (u8 gp, st_ymd_t *tm, data_enum_t item, s64 *data);
//读 一个总加组 一个时间 一个月数据
int read_zj_month_data (u8 gp, st_ym_t *tm, data_enum_t item, s64 *data);
//写 一个总加组 一个时间 一个月数据
int write_zj_month_data (u8 gp, st_ym_t *tm, data_enum_t item, s64 *data);
//读 一个总加组 一个时间 一个曲线数据
int read_zj_curve_data (u8 gp, st_ymdhm_t *tm, data_enum_t item, s64 *data);
//写 一个总加组 一个时间 一个曲线数据
int write_zj_curve_data (u8 gp, st_ymdhm_t *tm, data_enum_t item, s64 *data);
//清 一个总加组 所有历史数据
int clear_zj_history_data (u8 gp);
//清 所有总加组 所有历史数据
int clear_allzj_history_data (void);



/************************事件数据*******************************/

/************************参数*****************************/



#endif
