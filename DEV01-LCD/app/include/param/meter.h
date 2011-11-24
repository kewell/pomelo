/**
* meter.h -- 表计参数头文件
* 
* 
* 创建时间: 2010-5-7
* 最后修改时间: 2010-5-7
*/

#ifndef _PARAM_METER_H
#define _PARAM_METER_H

#include "include/param/capconf.h"

/*
约定:
前8个表只能用作多功能表计
后面的只能用作载波表计
测量点号与电表号相等
*/

//F10, 终端电能表/交流采样配置参数
#define METTYPE_DL645			1
#define METTYPE_ACSAMP		2
#define METTYPE_DL645_2007		30
#define METTYPE_PLC			31

#define METBAUD_MASK		0xe0
#define METPORT_MASK		0x1f
#define CENMETPORT_MASK	0x02

typedef struct {
	unsigned short index;  //表计索引号,储存用, 其他地方不用
	unsigned short metp_id;    //所属测量点号, 1~2040, 0表示无效
	unsigned char portcfg;    //通信速率及端口号
	unsigned char proto;    //通信规约类型, 1-DL645-1997, 2-交流采样,30-DL645-2007
	unsigned char addr[6];    //通信地址
	unsigned char pwd[6];    //通信密码
	unsigned char prdnum;    //电能费率个数
	unsigned char intdotnum;    //有功电能示值整数位和小数位个数
	unsigned char owneraddr[6];    //所属采集器地址
	unsigned char userclass;    //用户分类号
	unsigned char metclass;    //电表分类号
} para_meter_t;

#ifndef DEFINE_PARAMETER
extern const para_meter_t ParaMeter[MAX_METER];
#endif

#endif /*_PARAM_METER_H*/

