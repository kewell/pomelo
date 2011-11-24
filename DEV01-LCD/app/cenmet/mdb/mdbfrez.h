/**
* mdbfrez.h -- 曲线类历史数据结构
* 
* 
* 创建时间: 2010-5-12
* 最后修改时间: 2010-5-12
*/

#ifndef _MDBFREZ_H
#define _MDBFREZ_H

#include "include/sys/timeal.h"

typedef struct {
	unsigned char tpwra[2];   //总加有功功率
	unsigned char tpwri[2];    //总加无功功率
	unsigned char tenea[4];     //总加有功电能量
	unsigned char tenei[4];    //总加无功电能量
} db_tgrpcurve_t;


typedef struct {//二类数据
	unsigned char pwra[12];   //有功功率, SABC, 0.0001kW
	unsigned char pwri[12];    //无功功率, SABC, 0.0001kvar
	unsigned char pwrf[8];    //功率因数, SABC, 0.001
	unsigned char vol[6];    //电压, ABC, 0.1V
	unsigned char amp[8];    //电流, ABC, 0.01A

	unsigned char uenepa[4];    //正向有功总电能, 0.0001kWh
	unsigned char uenepi[4];    //正向无功总电能, 0.0001kvarh
	unsigned char uenena[4];    //反向有功总电能, 0.0001kWh
	unsigned char ueneni[4];    //反向无功总电能, 0.0001kvarh

	unsigned char enepa[4];    //正向有功总电能示值, 0.01kWh
	unsigned char enepi[4];    //正向无功总电能示值, 0.01kvarh
	unsigned char enena[4];    //反向有功总电能示值, 0.01kWh
	unsigned char eneni[4];    //反向无功总电能示值, 0.01kvarh

	unsigned char vol_arc[6];  //电压相位角, 度
	unsigned char amp_arc[6];  //电流相位角, 度

	unsigned char enepi1[4];  //一象限无功总电能示值, 0.01kvarh
	unsigned char enepi4[4];  //四象限无功总电能示值, 0.01kvarh
	unsigned char eneni2[4];  //二象限无功总电能示值, 0.01kvarh
	unsigned char eneni3[4];  //三象限无功总电能示值, 0.01kvarh
} db_metcurve_t;

void SetUTimeFrez(utime_t utime);
utime_t GetUTimeFrez(void);

void UpdateMdbFrez(void);
int ReadMdbFrez(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len);

#endif /*_MDBFREZ_H*/


