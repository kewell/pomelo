/**
* capconf.h -- 容量配置
* 
* 
* 创建时间: 2010-5-10
* 最后修改时间: 2010-5-10
*/

#ifndef _PARAM_CAPCONF_H
#define _PARAM_CAPCONF_H

//#define MAX_METER    2040   //最大电表/交流采样数
#define MAX_METER    (2040)  //最大电表/交流采样数

#define MAX_METP		MAX_METER   //最大测量点数
//#define MAX_CENMETP		8     //最大多功能测量点数, 载波表计的测量点号只能大于这个
#define MAX_CENMETP		2     //最大多功能测量点数, 载波表计的测量点号只能大于这个
#define MAX_PLCMET		(MAX_METP-MAX_CENMETP)  //最大载波表数
#define PLC_BASEMETP	MAX_CENMETP  //载波表起始测量点号(表号)
//#define PLC_BASEMETP	3  //载波表起始测量点号(表号)
//#define PLC_BASEMETP	2  //载波表起始测量点号(表号)
//#define PLC_BASEMETP	0  //载波表起始测量点号(表号)

#define MAX_IMPORTANT_USER		20   //最大重点用户表数


#define MAX_PULSE	2  //脉冲最大数
#define MAX_ISIG	2  //状态量输入最大数
#define MAX_TGRP	8  //总加组最大数
#define MAX_DIFFA	2  //有功总电能量差动最大数

#define MAX_DTASK_CLS1		16   // 1类任务最大数
#define MAX_DTASK_CLS2		32  // 2类任务最大数

#endif /*_PARAM_CAPCONF_H*/

