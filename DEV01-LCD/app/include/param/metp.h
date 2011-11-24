/**
* metp.h -- 测量点参数头文件
* 
* 
* 创建时间: 2010-5-7
* 最后修改时间: 2010-5-7
*/

#ifndef _PARAM_METP_H
#define _PARAM_METP_H

#include "include/param/capconf.h"

#define PWRTYPE_3X3W	1
#define PWRTYPE_3X4W	2
#define PWRTYPE_1X		3

//F25, 测量点基本参数
typedef struct {
	unsigned short pt;    //电压互感器倍率
	unsigned short ct;    //电流互感器倍率
	unsigned short vol_rating;    //额定电压, 0.1V
	unsigned short amp_rating;    //额定电流, 0.01A
	unsigned int pwr_rating;   //额定负荷, 0.0001kVA
	unsigned char pwrtype;    //电源接线方式, 
	                             // 1=三相三线,2=三相四线,3=单相
	unsigned char pwrphase;   //单相表接线相, 0-unknown, 1-A, 2-B, 3-C
} cfg_mpbase_t;

//F26, 测量点限值参数
typedef struct {
	unsigned short volok_up;    //电压合格上限, 0.1V
	unsigned short volok_low;    //电压合格下限, 0.1V
	unsigned short vol_lack;    //电压断相门限, 0.1V
	unsigned short vol_over;    //过压门限, 0.1V
	unsigned short vol_less;    //欠压门限, 0.1V

	unsigned short amp_over;    //过流门限, 0.01A
	unsigned short amp_limit;    //额定电流门限, 0.01A
	unsigned short zamp_limit;    //零序电流上限, 0.01A

	unsigned int pwr_over;    //视在功率上上限, 0.0001kVA
	unsigned int pwr_limit;    //视在功率上限, 0.0001kVA

	unsigned short vol_unb;    //三相电压不平衡限值, 0.1%
	unsigned short amp_unb;    //三相电流不平衡限值, 0.1%

	unsigned char time_volover;   //越限持续时间, 分
	unsigned char time_volless;
	unsigned char time_ampover;
	unsigned char time_amplimit;
	unsigned char time_zamp;
	unsigned char time_pwrover;
	unsigned char time_pwrlimit;
	unsigned char time_volunb;
	unsigned char time_ampunb;
	unsigned char time_volless_2;    //连续失压时间限值, min

	unsigned int restore_pwrover;
	unsigned int restore_pwrlimit;
	unsigned short restore_volover;  //过压越限恢复电压, 0.1V, 以下相同
	unsigned short restore_volless;
	unsigned short restore_ampover;
	unsigned short restore_amplimit;
	unsigned short restore_zamp;
	unsigned short restore_volunb;
	unsigned short restore_ampunb;
} cfg_mplimit_t;

//F27, 测量点铜损, 铁损参数
typedef struct {
	unsigned short Ra;  //A相电阻
	unsigned short Xa;  //A相电抗
	unsigned short Ga;  //A相电导
	unsigned short Ba;  //A相电纳

	unsigned short Rb;  //B相电阻
	unsigned short Xb;  //B相电抗
	unsigned short Gb;  //B相电导
	unsigned short Bb;  //B相电纳

	unsigned short Rc;  //C相电阻
	unsigned short Xc;  //C相电抗
	unsigned short Gc;  //C相电导
	unsigned short Bc;  //C相电纳
} cfg_mpcopper_t;

//F28, 测量点功率因数分段限值
typedef struct {
// 3个区段, A<limit1<=B<limit2<=C
	unsigned short limit1;    //分段限值1, 0.1%
	unsigned short limit2;
} cfg_mppwrf_t;

//F29, 终端当地电能表显示号
typedef struct {
	unsigned char no[12];
} cfg_mpdisplay_t;

//多功能测量点参数
typedef struct {
	unsigned short mid;   //use for save
	unsigned char stopped;   //F30, 终端台区集中抄表停抄/投抄设置
	unsigned char reserv;
	cfg_mpbase_t base;
	cfg_mplimit_t limit;
	cfg_mpcopper_t copper;
	cfg_mppwrf_t pwrf;
	unsigned char displayno[12];   //F29, 终端当地电能表显示号
} para_cenmetp_t;

//载波测量点参数
typedef struct {
	unsigned short mid;   //use for save
	unsigned char phase;   //单相表接线相, 0-unknown, 1-A, 2-B, 3-C
	unsigned char stopped;   //F30, 终端台区集中抄表停抄/投抄设置
	unsigned char displayno[12];   //F29, 终端当地电能表显示号
} para_plcmetp_t;

//F31 载波从节点附属节点地址
#define MAX_CHILDEND	20
typedef struct {
	unsigned short mid;   //use for save
	unsigned char reserv;
	unsigned char num;
	unsigned char addr[MAX_CHILDEND*6];
} para_childend_t;

//测量点到各个电表的映射
#define METP_NONE		0
#define METP_METER		1
#define METP_PULSE		2

typedef struct {
	unsigned char type;   //类型, 0-无效, 1-电表, 2-脉冲
	unsigned char reserv;
	unsigned short metid;   //表号-对应电表号,脉冲号等等
} metp_map_t;


#ifndef DEFINE_PARAMETP
extern const para_cenmetp_t ParaCenMetp[MAX_CENMETP];
extern const para_plcmetp_t ParaPlcMetp[MAX_METP];
extern const para_childend_t ParaChildEnd[MAX_METP];
extern const metp_map_t CenMetpMap[MAX_CENMETP];
#endif

void MappingCenMetp(void);

#define metp_pwrtype(mid)    (ParaCenMetp[mid].base.pwrtype)
#define EMPTY_CENMETP(mid)		(CenMetpMap[mid].type == METP_NONE)

#endif /*_PARAM_METP_H*/

