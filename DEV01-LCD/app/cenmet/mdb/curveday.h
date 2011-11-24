/**
* curveday.h -- 日冻结类历史数据结构
* 
* 
* 创建时间: 2010-5-12
* 最后修改时间: 2010-5-12
*/

#ifndef _CURVE_DAY_H
#define _CURVE_DAY_H

#include "cenmet/mdb/mdbconf.h"

typedef struct {//二类数据
	unsigned char rdtime[5];    //抄表时间, 分时日月年
	unsigned char fenum;

	//F1
	unsigned char enepa[5*MAXNUM_METPRD];  //正向有功电能, 0~4, 0.0001kWh
	unsigned char enepi[4*MAXNUM_METPRD];  //正向无功电能, 0~4, 0.01kvarh
	unsigned char enepi1[4*MAXNUM_METPRD];  //正向无功一象限电能, 0~4, 0.01kvarh
	unsigned char enepi4[4*MAXNUM_METPRD];  //正向无功四象限电能, 0~4, 0.01kvarh
	//F2
	unsigned char enena[5*MAXNUM_METPRD];  //反向有功电能, 0~4, 0.0001kWh
	unsigned char eneni[4*MAXNUM_METPRD];  //反向无功电能, 0~4, 0.01kvarh
	unsigned char eneni2[4*MAXNUM_METPRD];  //反向无功二象限电能, 0~4, 0.01kvarh
	unsigned char eneni3[4*MAXNUM_METPRD];  //反向无功三象限电能, 0~4, 0.01kvarh
	//F3
	unsigned char dmnpa[3*MAXNUM_METPRD];  //当月正向有功最大需量,0~4,0.0001kW
	unsigned char dmntpa[4*MAXNUM_METPRD];  //当月正向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnpi[3*MAXNUM_METPRD];  //当月正向无功最大需量,0~4,0.0001kvar
	unsigned char dmntpi[4*MAXNUM_METPRD];  //当月正向无功最大需量发生时间,0~4,分时日月
	//F4
	unsigned char dmnna[3*MAXNUM_METPRD];  //当月反向有功最大需量,0~4,0.0001kW
	unsigned char dmntna[4*MAXNUM_METPRD];  //当月反向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnni[3*MAXNUM_METPRD];  //当月反向无功最大需量,0~4,0.0001kvar
	unsigned char dmntni[4*MAXNUM_METPRD];  //当月反向无功最大需量发生时间,0~4,分时日月
	//F5
	unsigned char enepa_day[4*MAXNUM_METPRD];     //当日正向有功电能量, 0.0001kWh
	//F6
	unsigned char enepi_day[4*MAXNUM_METPRD];     //当日正向无功电能量, 0.0001kvarh
	//F7
	unsigned char enena_day[4*MAXNUM_METPRD];     //当日反向有功电能量, 0.0001kWh
	//F8
	unsigned char eneni_day[4*MAXNUM_METPRD];     //当日反向无功电能量, 0.0001kvarh
	//F153
	unsigned char enepa_abc[5*3];	//ABC相正向有功电能示值, 0.0001kWh
	//F154
	unsigned char enepi_abc[4*3];	//ABC相正向无功电能示值, 0.01kvarh
	//F155
	unsigned char enena_abc[5*3];	//ABC相反向有功电能示值, 0.0001kWh
	//F156
	unsigned char eneni_abc[4*3];	//ABC相正向无功电能示值, 0.01kvarh
} db_metday_t;

typedef struct {//二类数据
	//F25
	unsigned char pwra_max[6*4];    //三相最大有功功率及发生时间, kW, 分时日
	unsigned char pwra_zero[2*4];    //三相有功功率为零时间, min
	//F26
	unsigned char dm_max[6*4];    //三相最大需量及发生时间, kW, 分时日
	//F27
	unsigned char vol_extime[10*3];   //电压越限累计时间, min
	unsigned char vol_maxmin[10*3];   //电压最大值最小值及发生时间V, 分时日
	unsigned char vol_avg[2*3];   //平均电压, V
	//F28
	unsigned char amp_unb[2];   //电流不平衡越限累计时间, min
	unsigned char vol_unb[2];   //电流不平衡越限累计时间, min
	unsigned char ampunb_max[5];  //电流不平衡最大值及发生时间, %, 分时日
	unsigned char volunb_max[5];  //电压不平衡最大值及发生时间, %, 分时日
	//F29
	unsigned char amp_extime[4*3];    //电流越限累计时间, min
	unsigned char zamp_extime[2];    //零序电流越限累计时间, min
	unsigned char amp_max[5*4];    //电流最大值及发生时间, A, 分时日
	//F30
	unsigned char pwrv_extime[4];    //视在功率越限累计时间, min
	//F31
	unsigned char load[10];   //日负载率统计, %, 分时日
	//F43
	unsigned char pwrf_ptime[6];   //功率因数区段累计时间
	//F32
	unsigned char rdtime[5];
	unsigned char volbr_cnt[8];    //断相次数,SABC
	unsigned char volbr_times[12];  //断相累计时间,SABC, min
	unsigned char volbr_timestart[16];    //最近一次断相起始时间,SABC,分时日月
	unsigned char volbr_timeend[16];    //最近一次断相结束时间,SABC,分时日月

	unsigned char reserv;
} db_msticday_t;

typedef struct {//二类数据
	//F49
	unsigned char pwr_time[2];    //终端日供电时间, min
	unsigned char rest_cnt[2];    //终端日复位次数
	//F50
	unsigned char sw_time[4];	 //跳闸累计次数
	//F53
	unsigned char comm_bytes[4];  //终端与主站日通信流量, 字节
} db_termday_t;//终端日数据

typedef struct {
	//F57
	unsigned char pwra_time[5*2];   //最大最小功率及发生时间
	unsigned char pwra_zero[2];    //有功功率为0时间, min
	//F58
	unsigned char fenum58;
	unsigned char enepa[4*MAXNUM_METPRD];    //有功电量
	//F59
	unsigned char fenum59;
	unsigned char enepi[4*MAXNUM_METPRD];    //无功电量

	unsigned char reserv[2];
} db_tgrpday_t;

#endif /*_CURVE_DAY_H*/

