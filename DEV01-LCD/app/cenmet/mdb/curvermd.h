/**
* curveday.h -- 抄表日冻结类历史数据结构
* 
* 
* 创建时间: 2010-5-12
* 最后修改时间: 2010-5-12
*/

#ifndef _CURVE_RMD_H
#define _CURVE_RMD_H

#include "cenmet/mdb/mdbconf.h"
	
typedef struct {//二类数据
	unsigned char rdtime[5];	 //抄表时间, 分时日月年
	unsigned char fenum;

	//F9
	unsigned char enepa[5*MAXNUM_METPRD];  //正向有功电能, 0~4, 0.0001kWh
	unsigned char enepi[4*MAXNUM_METPRD];  //正向无功电能, 0~4, 0.01kvarh
	unsigned char enepi1[4*MAXNUM_METPRD];	//正向无功一象限电能, 0~4, 0.01kvarh
	unsigned char enepi4[4*MAXNUM_METPRD];	//正向无功四象限电能, 0~4, 0.01kvarh
	//F10
	unsigned char enena[5*MAXNUM_METPRD];  //反向有功电能, 0~4, 0.0001kWh
	unsigned char eneni[4*MAXNUM_METPRD];  //反向无功电能, 0~4, 0.01kvarh
	unsigned char eneni2[4*MAXNUM_METPRD];	//反向无功二象限电能, 0~4, 0.01kvarh
	unsigned char eneni3[4*MAXNUM_METPRD];	//反向无功三象限电能, 0~4, 0.01kvarh
	//F11
	unsigned char dmnpa[3*MAXNUM_METPRD];  //当月正向有功最大需量,0~4,0.0001kW
	unsigned char dmntpa[4*MAXNUM_METPRD];	//当月正向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnpi[3*MAXNUM_METPRD];  //当月正向无功最大需量,0~4,0.0001kvar
	unsigned char dmntpi[4*MAXNUM_METPRD];	//当月正向无功最大需量发生时间,0~4,分时日月
	//F12
	unsigned char dmnna[3*MAXNUM_METPRD];  //当月反向有功最大需量,0~4,0.0001kW
	unsigned char dmntna[4*MAXNUM_METPRD];	//当月反向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnni[3*MAXNUM_METPRD];  //当月反向无功最大需量,0~4,0.0001kvar
	unsigned char dmntni[4*MAXNUM_METPRD];	//当月反向无功最大需量发生时间,0~4,分时日月
} db_metrmd_t;
	
#endif /*_CURVE_RMD_H*/
	
