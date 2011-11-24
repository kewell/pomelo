/**
* mdbsave.c -- 状态数据保存
* 
* 
* 创建时间: 2010-5-14
* 最后修改时间: 2010-5-14
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFINE_MDBTGRP

#include "include/debug.h"
#include "include/sys/bin.h"
#include "include/sys/syslock.h"
#include "include/sys/timeal.h"
#include "include/sys/cycsave.h"
#include "include/param/capconf.h"
#include "include/monitor/runstate.h"
#include "mdbconf.h"
#include "dbconfig.h"
#include "mdbuene.h"
#include "mdbstic.h"
#include "mdbtgrp.h"

static int LockIdMdbSave = -1;

#define UENE_MAGIC		0x21190514
#define STIC_MAGIC		0x2ab90514
#define TGRP_MAGIC		0x31023987

extern mdbtgrp_t MdbTGrp[MAX_TGRP];

/**
* @brief 清除MDB储存
*/
void ClearMdbSave(void)
{
	MdbSticInit();
	MdbUseEneInit();
}

/**
* @brief MDB储存初始化
* @return 成功0, 否则失败
*/
DECLARE_INIT_FUNC(MdbSaveInit);
int MdbSaveInit(void)
{
	MdbSticInit();
	MdbUseEneInit();
	MdbTGrpInit();
	
	DebugPrint(0, "  load uene.dat...");
	if(ReadBinFile(DBASE_SAVEPATH "uene.dat", UENE_MAGIC, (unsigned char *)MdbUseEne, sizeof(MdbUseEne)) > 0) {
		DebugPrint(0, "ok\n");
	}
	else {
		DebugPrint(0, "fail\n");
	}

	DebugPrint(0, "  load stic.dat...");
	if(ReadBinFile(DBASE_SAVEPATH "stic.dat", STIC_MAGIC, (unsigned char *)&MdbStic, sizeof(MdbStic)) > 0) {
		DebugPrint(0, "ok\n");
	}
	else {
		DebugPrint(0, "fail\n");
	}

	DebugPrint(0, "  load tgrp.dat...");
	
	if(ReadBinFile(DBASE_SAVEPATH "tgrp.dat", TGRP_MAGIC, (unsigned char *)&MdbTGrp, sizeof(MdbTGrp)) > 0) {
		DebugPrint(0, "ok\n");
	}
	else {
		DebugPrint(0, "fail\n");
	}
	UpdateSticResetCount();

	LockIdMdbSave = RegisterSysLock();

	SET_INIT_FLAG(MdbSaveInit);

	return 0;
}

/**
* @brief 保存用电量数据库
*/
void SaveMdbUene(void)
{
	PrintLog(0, "SaveMdbUene...\r\n");
	LockSysLock(LockIdMdbSave);
	SaveBinFile(DBASE_SAVEPATH "uene.dat", UENE_MAGIC, (unsigned char *)MdbUseEne, sizeof(MdbUseEne));
	UnlockSysLock(LockIdMdbSave);
}

/**
* @brief 保存统计数据库
*/
void SaveMdbStic(void)
{
	PrintLog(0, "SaveMdbStic...\r\n");
	LockSysLock(LockIdMdbSave);
	SaveBinFile(DBASE_SAVEPATH "stic.dat", STIC_MAGIC, (unsigned char *)&MdbStic, sizeof(MdbStic));
	UnlockSysLock(LockIdMdbSave);
}

void SaveMdbTGrp(void)
{
	LockSysLock(LockIdMdbSave);
	SaveBinFile(DBASE_SAVEPATH "tgrp.dat", TGRP_MAGIC, (unsigned char *)&MdbTGrp, sizeof(MdbTGrp));
	UnlockSysLock(LockIdMdbSave);
}

/**
* @brief 保存数据
*/
void SaveMdb(void)
{
	PrintLog(0, "SaveMdb...\r\n");
	SaveMdbUene();
	//SaveMdbStic();
	//SaveMdbTGrp();
}
DECLARE_CYCLE_SAVE(SaveMdb, 0);

void ResetMdbDay(void)
{
	unsigned char tid;
	
	MdbSticEmptyDay();

	ResetMdbUseEne(0);
	
	for(tid=0; tid<MAX_TGRP; tid++) {
		memset(MdbTGrp[tid].enea_day, 0, sizeof(int)*(MAXNUM_FEEPRD+1)*2);
	}

}

void ResetMdbMonth(void)
{
	unsigned char tid;
	
	MdbSticEmptyMonth();

	ResetMdbUseEne(1);
	
	for(tid=0; tid<MAX_TGRP; tid++) {
		memset(MdbTGrp[tid].enea_mon, 0, sizeof(int)*(MAXNUM_FEEPRD+1)*2);
	}

	RunStateModify()->commflow= 0;
}

