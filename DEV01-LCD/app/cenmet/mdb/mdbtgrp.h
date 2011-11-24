#ifndef _MDB_TGRP_H
#define _MDB_TGRP_H

#include "cenmet/mdb/mdbconf.h"
#include "include/lib/datatype_gb.h"

typedef struct {
	int pwra;    //当前总加有功功率, 0.01kW
	int pwri;    //当前总加无功功率, 0.01kvar

	int uenea_15min;   // 15 分钟用电量, kWh
	int uenei_15min;   // 15 分钟用电量, kvarh

	int uenea_1min;   // 1分钟用电量, kWh
	int uenei_1min;   // 1 分钟用电量, kvarh

	int uenea;    //有功用电量(冻结用)
	int uenei;    //无功用电量(冻结用)
} mdbtgrp_imm_t;

typedef struct {
	int enea_day[MAXNUM_FEEPRD+1];    //当日总加有功电能量, kWh
	int enei_day[MAXNUM_FEEPRD+1];    //当日总加无功电能量, kvarh
	int enea_mon[MAXNUM_FEEPRD+1];     //当月总加有功电能量, kWh
	int enei_mon[MAXNUM_FEEPRD+1];      //当月总加无功电能量, kvarh

	int enea_res;   //剩下补足有功电量, 0.001kWh
	int enei_res;   //剩下补足无功电量, 0.01kvarh

	gene_t ene_bec;   //终端当前剩余电量/费
	int pwr_flt;   //当前功率下浮控后有功功率冻结值
} mdbtgrp_t;

#ifndef DEFINE_MDBTGRP
extern const mdbtgrp_imm_t MdbTGrpImm[MAX_TGRP];
#define mdbtgrp_imm(tid)	(MdbTGrpImm[tid])
extern const mdbtgrp_t MdbTGrp[MAX_TGRP];
#define mdbtgrp(tid)		(MdbTGrp[tid])
#endif

int MdbTGrpInit(void);

void UpdateMdbTGrpPower(void);
void UpdateTGrpEne(void);
void MdbTGrpUEneFrezEnd(void);
void AddMdbTGrpUEne15Min(void);
void ClearMdbTGrp15Minute(void);
void SetMdbTGrpPwrflt(unsigned char tid, int pwr);

int ReadMdbTGrp(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len);

#endif /*_MDB_TGRP_H*/

