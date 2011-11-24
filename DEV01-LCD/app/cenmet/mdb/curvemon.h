/**
* curvemon.h -- 月冻结类历史数据结构
* 
* 
* 创建时间: 2010-5-12
* 最后修改时间: 2010-5-12
*/

#ifndef _CURVE_MON_H
#define _CURVE_MON_H

#include "cenmet/mdb/mdbconf.h"

typedef struct {//二类数据
	unsigned char rdtime[5];    //抄表时间, 分时日月年
	unsigned char fenum;  //费率数

	//F17
	unsigned char enepa[5*MAXNUM_METPRD];  //正向有功电能, 0~4, 0.0001kWh
	unsigned char enepi[4*MAXNUM_METPRD];  //正向无功电能, 0~4, 0.01kvarh
	unsigned char enepi1[4*MAXNUM_METPRD];  //正向无功一象限电能, 0~4, 0.01kvarh
	unsigned char enepi4[4*MAXNUM_METPRD];  //正向无功四象限电能, 0~4, 0.01kvarh
	//F18
	unsigned char enena[5*MAXNUM_METPRD];  //反向有功电能, 0~4, 0.0001kWh
	unsigned char eneni[4*MAXNUM_METPRD];  //反向无功电能, 0~4, 0.01kvarh
	unsigned char eneni2[4*MAXNUM_METPRD];  //反向无功二象限电能, 0~4, 0.01kvarh
	unsigned char eneni3[4*MAXNUM_METPRD];  //反向无功三象限电能, 0~4, 0.01kvarh
	//F19
	unsigned char dmnpa[3*MAXNUM_METPRD];  //当月正向有功最大需量,0~4,0.0001kW
	unsigned char dmntpa[4*MAXNUM_METPRD];  //当月正向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnpi[3*MAXNUM_METPRD];  //当月正向无功最大需量,0~4,0.0001kvar
	unsigned char dmntpi[4*MAXNUM_METPRD];  //当月正向无功最大需量发生时间,0~4,分时日月
	//F20
	unsigned char dmnna[3*MAXNUM_METPRD];  //当月反向有功最大需量,0~4,0.0001kW
	unsigned char dmntna[4*MAXNUM_METPRD];  //当月反向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnni[3*MAXNUM_METPRD];  //当月反向无功最大需量,0~4,0.0001kvar
	unsigned char dmntni[4*MAXNUM_METPRD];  //当月反向无功最大需量发生时间,0~4,分时日月
	//F21
	unsigned char enepa_day[4*MAXNUM_METPRD];     //当月正向有功电能量, 0.0001kWh
	//F22
	unsigned char enepi_day[4*MAXNUM_METPRD];     //当月正向无功电能量, 0.0001kvarh
	//F23
	unsigned char enena_day[4*MAXNUM_METPRD];     //当月反向有功电能量, 0.0001kWh
	//F24
	unsigned char eneni_day[4*MAXNUM_METPRD];     //当月反向无功电能量, 0.0001kvarh
	//F157
	unsigned char enepa_abc[5*3];	//ABC相正向有功电能示值, 0.0001kWh
	//F158
	unsigned char enepi_abc[4*3];	//ABC相正向无功电能示值, 0.01kvarh
	//F159
	unsigned char enena_abc[5*3];	//ABC相反向有功电能示值, 0.0001kWh
	//F160
	unsigned char eneni_abc[4*3];	//ABC相正向无功电能示值, 0.01kvarh
} db_metmon_t;

typedef struct {//二类数据
	//F33
	unsigned char pwra_max[6*4];    //三相最大有功功率及发生时间, kW, 分时日
	unsigned char pwra_zero[2*4];    //三相有功功率为零时间, min
	//F34
	unsigned char dm_max[6*4];    //三相最大需量及发生时间, kW, 分时日
	//F35
	unsigned char vol_extime[10*3];   //电压越限累计时间, min
	unsigned char vol_maxmin[10*3];   //电压最大值最小值及发生时间V, 分时日
	unsigned char vol_avg[2*3];   //平均电压, V
	//F36
	unsigned char amp_unb[2];   //电流不平衡越限累计时间, min
	unsigned char vol_unb[2];   //电压不平衡越限累计时间, min
	unsigned char ampunb_max[6];  //电流不平衡最大值及发生时间, %, 分时日月
	unsigned char volunb_max[6];  //电压不平衡最大值及发生时间, %, 分时日月
	//F37
	unsigned char amp_extime[4*3];    //电流越限累计时间, min
	unsigned char zamp_extime[2];    //零序电流越限累计时间, min
	unsigned char amp_max[5*4];    //电流最大值及发生时间, A, 分时日
	//F38
	unsigned char pwrv_extime[4];    //视在功率越限累计时间, min
	//F39
	unsigned char load[12];   //月负载率统计, %, 分时日月
	//F44
	unsigned char pwrf_ptime[6];   //功率因数区段累计时间
} db_msticmon_t;

typedef struct {//二类数据
	//F51
	unsigned char pwr_time[2];    //终端月供电时间, min
	unsigned char rest_cnt[2];    //终端月复位次数
	//F52
	unsigned char sw_time[4];	 //跳闸累计次数
	//F54
	unsigned char comm_bytes[4];  //终端与主站月通信流量, 字节
} db_termmon_t;

typedef struct {
	//F60
	unsigned char pwra_time[5*2];   //最大最小功率及发生时间
	unsigned char pwra_zero[2];    //有功功率为0时间, min
	//F65
	unsigned char pcuene[6];    //超功率定值月累计时间及累计电量
	//F66
	unsigned char ecuene[6];    //超月电量定值月累计时间及累计电量

	//F61
	unsigned char fenum61;
	unsigned char enepa[4*MAXNUM_METPRD];    //有功电量
	//F62
	unsigned char fenum62;
	unsigned char enepi[4*MAXNUM_METPRD];    //无功电量

	unsigned char reserv[2];
} db_tgrpmon_t;

#endif /*_CURVE_MON_H*/

