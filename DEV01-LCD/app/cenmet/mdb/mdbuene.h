/**
* mdbuene.h -- 用电量
* 
* 
* 创建时间: 2010-5-13
* 最后修改时间: 2010-5-13
*/

#ifndef _MDB_UENE_H
#define _MDB_UENE_H

#include "cenmet/mdb/mdbconf.h"

typedef struct {
	unsigned int enepa_15m;    // 15分钟用电量, 0.001kWh
	unsigned int enepi_15m;  //0.01kvarh
	unsigned int enena_15m;
	unsigned int eneni_15m;

	unsigned int enepa_1m;    // 1分钟用电量
	unsigned int enepi_1m;
	unsigned int enena_1m;
	unsigned int eneni_1m;
} mdbuene_imm_t;

typedef struct {
	//F41~F44
	unsigned int enepa_day[MAXNUM_FEEPRD+1];     //当日正向有功电能量, 0.001kWh
	unsigned int enepi_day[MAXNUM_FEEPRD+1];     //当日正向无功电能量, 0.01kvarh
	unsigned int enena_day[MAXNUM_FEEPRD+1];     //当日反向有功电能量, 0.001kWh
	unsigned int eneni_day[MAXNUM_FEEPRD+1];     //当日反向无功电能量, 0.01kvarh

	//F45~F48
	unsigned int enepa_mon[MAXNUM_FEEPRD+1];     //当月正向有功电能量, 0.001kWh
	unsigned int enepi_mon[MAXNUM_FEEPRD+1];     //当月正向无功电能量, 0.01kvarh
	unsigned int enena_mon[MAXNUM_FEEPRD+1];     //当月反向有功电能量, 0.001kWh
	unsigned int eneni_mon[MAXNUM_FEEPRD+1];     //当月反向无功电能量, 0.01kvarh

	unsigned int enepa_bak[MAXNUM_FEEPRD+1];     //备份正向有功电能量, 0.001kWh
	unsigned int enepi_bak[MAXNUM_FEEPRD+1];     //备份正向无功电能量, 0.01kvarh
	unsigned int enena_bak[MAXNUM_FEEPRD+1];     //备份反向有功电能量, 0.001kWh
	unsigned int eneni_bak[MAXNUM_FEEPRD+1];     //备份反向无功电能量, 0.01kvarh

	unsigned int uenepa;
	unsigned int uenepi;
	unsigned int uenena;
	unsigned int ueneni;
} mdbuene_t;

#ifndef DEFINE_MDBUENE
extern const mdbuene_imm_t MdbUseEneImm[MAX_CENMETP];
extern const mdbuene_t MdbUseEne[MAX_CENMETP];
#endif

void MdbUseEneInit(void);
void ResetMdbUseEne(int flag);

void UpdateMdbUseEne();
void UpdateMdbUseEne15Min(unsigned short mid);
void ClearMdbUseEne15Min(unsigned short mid);

int ReadMdbUseEne(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len);

#endif /*_MDB_UENE_H*/

