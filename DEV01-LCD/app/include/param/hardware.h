/**
* hardware.h -- 硬件参数
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#ifndef _PARAM_HARDWARE_H
#define _PARAM_HARDWARE_H

#include "include/param/capconf.h"

#define PLSTYP_POSA    0    //正向有功
#define PLSTYP_POSI    1    //正向无功
#define PLSTYP_NEGA    2    //反向有功
#define PLSTYP_NEGI    3    //反向无功
//F11, 终端脉冲配置参数
typedef struct {
	unsigned char metp_id;    //所属测量点号, 1~16
	unsigned char type;    //脉冲属性
	unsigned short consk;    //脉冲常数
} para_pulse_t;

//F12, 状态量输入参数
typedef struct {
	unsigned char flagin;    //接入标志位
	unsigned char flagattr;    //属性标志位
	unsigned char reserv[2];
} para_isig_t;

//F13, 电压/电流模拟量配置参数,保留

#define TGRFLG_OP    0x80
#define TGRFLG_DIRECT    0x40
#define TGRFLG_METP    0x3f
#define MAX_TGRP_METP		15
//F14, 终端总加组配置参数
//注意!!总加组的测量点号从0~3, 而不是1~4
typedef struct {
	unsigned char num;
	unsigned char flag[MAX_TGRP_METP];
} para_tgrp_t;

//F15, 有功总电能量差动越限事件参数设置
typedef struct {
	unsigned char tgrp_con;    //对比总加组号
	unsigned char tgrp_ref;    //参照总加组号
	unsigned char flag;    //标志,
	                      //D0~D1=0-60分钟电量, 1-30分钟电量, 2-15分钟电量
	                      //D7=0-相对对比, =1-绝对对比
	unsigned char percent;    //差动越限相对差值, %
	int absdiff;    //差动越限绝对差值,kWh
} para_diffa_t;

typedef struct {
	para_pulse_t pulse[MAX_PULSE];
	para_isig_t isig;
	para_tgrp_t tgrp[MAX_TGRP];
	para_diffa_t diffa[MAX_DIFFA];
} para_hardw_t;

#ifndef DEFINE_PARAHARDWARE
extern const para_hardw_t ParaHardw;
#define ParaPulse		(ParaHardw.pulse)
#define ParaIsig		(ParaHardw.isig)
#define ParaTGrp		(ParaHardw.tgrp)
#define ParaDiffa		(ParaHardw.diffa)
#endif

#define EMPTY_TGRP(tid)		(ParaHardw.tgrp[tid].num == 0)

#endif /*_PARAM_HARDWARE_H*/

