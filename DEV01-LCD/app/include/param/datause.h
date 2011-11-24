/**
* datause.h -- 支持数据配置参数头文件
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#ifndef _PARAM_DATAUSE_H
#define _PARAM_DATAUSE_H

//F38, F39 1, 2类数据配置设置 (在终端支持的数据配置内)
#define MAX_USERCLASS	16
#define MAX_METCLASS	16
typedef struct {
	unsigned char num;
	unsigned char fnmask[31];  //F1~F248
} cfg_datafns_t;

typedef struct {
	cfg_datafns_t met[MAX_METCLASS];
} cfg_usercls_data_t;

typedef struct {
	cfg_usercls_data_t datacls1[MAX_USERCLASS];  //F38
	cfg_usercls_data_t datacls2[MAX_USERCLASS];  //F39
} para_datause_t;

#ifndef DEFINE_PARADATAUSE
extern const para_datause_t ParaDataUse;
#define ParaDataUseCls1(userclass, metclass)	(ParaDataUse.datacls1[userclass].met[metclass])
#define ParaDataUseCls2(userclass, metclass)	(ParaDataUse.datacls2[userclass].met[metclass])
#endif

extern const cfg_datafns_t ValidDataCls1_1;
extern const cfg_datafns_t ValidDataCls2_1;
extern const cfg_datafns_t ValidDataCls1_2;
extern const cfg_datafns_t ValidDataCls2_2;

#endif /*_PARAM_DATAUSE_H*/

