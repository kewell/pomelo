/**
* imet_save.h -- 内部交流采样数据储存
* 
* 
* 创建时间: 2010-6-11
* 最后修改时间: 2010-6-11
*/

#ifndef _IMET_SAVE_H
#define _IMET_SAVE_H

struct imetene_save {
	unsigned char year;
	unsigned char month;
	unsigned char unuse[2];

	unsigned int enepa;  //正向有功电能量, 0.01kWh
	unsigned int enepi;  //正向无功电能量, 0.01kVarh
	unsigned int enepi1;  //一象限无功电能量, 0.01kVarh
	unsigned int enepi4;  //四象限无功电能量, 0.01kVarh
	unsigned int enepaa[3];  //正向ABC相有功电能量, 0.01kWh
	unsigned int enepia[3];  //正向ABC相无功电能量, 0.01kVarh

	unsigned int enena;  //反向有功电能量, 0.01kWh
	unsigned int eneni;  //反向无功电能量, 0.01kVarh
	unsigned int eneni2;  //二象限无功电能量, 0.01kVarh
	unsigned int eneni3;  //三象限无功电能量, 0.01kVarh
	unsigned int enenaa[3];  //反向ABC相有功电能量, 0.01kWh
	unsigned int enenia[3];  //反向ABC相无功电能量, 0.01kVarh
};
extern struct imetene_save IMetEneSave, IMetEneSaveLm;

void UpdateIMetEneLmMdb(void);
void UpdateIMetEneMdb(void);

void SaveIMetEne(void);
int LoadIMetSave(void);

#endif /*_IMET_SAVE_H*/
