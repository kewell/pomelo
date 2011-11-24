/**
* imet_save.c -- 内部交流采样数据储存
* 
* 
* 创建时间: 2010-6-11
* 最后修改时间: 2010-6-11
*/

#include <stdio.h>
#include <string.h>

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/sys/flat.h"
#include "include/sys/bin.h"
#include "include/sys/syslock.h"
#include "include/lib/bcd.h"
#include "include/lib/align.h"
#include "imet_save.h"

#define FLATID_IMETENE		0
#define FILE_IMETENE		IMPDATA_PATH "imetene.dat"
#define MAGIC_IMETENE		0x783612c9

struct imetene_save IMetEneSave, IMetEneSaveLm;

/**
* @brief 保存交流采样上月电能量
*/
static void SaveIMetEneLm(void)
{
	SaveBinFile(FILE_IMETENE, MAGIC_IMETENE, (unsigned char *)&IMetEneSaveLm, sizeof(IMetEneSaveLm));
}

/**
* @brief 保存交流采样电能量
*/
void SaveIMetEne(void)
{
	sysclock_t clock;

	SysClockReadCurrent(&clock);

	IMetEneSave.year = clock.year;
	IMetEneSave.month = clock.month;

	WriteFlatFile(FLATID_IMETENE, (unsigned char *)&IMetEneSave, sizeof(IMetEneSave));

	if(IMetEneSaveLm.year != clock.year || IMetEneSaveLm.month != clock.month) {
		PrintLog(LOGTYPE_ALARM, "save imet month...\n");
		memcpy(&IMetEneSaveLm, &IMetEneSave, sizeof(IMetEneSave));
		SaveIMetEneLm();
		UpdateIMetEneLmMdb();

		return;
	}
}

/**
* @brief 交流采样储存初始化
*/
DECLARE_INIT_FUNC(LoadIMetSave);
int LoadIMetSave(void)
{
	DebugPrint(0, "  load imet ene...");
	if(ReadFlatFile(FLATID_IMETENE, (unsigned char *)&IMetEneSave, sizeof(IMetEneSave)) > 0) {
		UpdateIMetEneMdb();
		DebugPrint(0, "ok\n");
	}
	else {
		DebugPrint(0, "fail\n");
	}

	DebugPrint(0, "  load imet ene lm...");
	if(ReadBinFile(FILE_IMETENE, MAGIC_IMETENE, (unsigned char *)&IMetEneSaveLm, sizeof(IMetEneSaveLm)) > 0) {
		UpdateIMetEneLmMdb();
		DebugPrint(0, "ok\n");
	}
	else {
		DebugPrint(0, "fail\n");
	}

	SET_INIT_FLAG(LoadIMetSave);
	return 0;
}

