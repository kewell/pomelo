/**
* mdbtgrp.c -- 总加组当前数据库
* 
* 
* 创建时间: 2010-1-12
* 最后修改时间: 2010-1-12
*/

#include <stdio.h>
#include <string.h>

#define DEFINE_MDBTGRP

#include "include/debug.h"
#include "include/param/capconf.h"
#include "include/param/hardware.h"
#include "include/param/metp.h"
#include "include/param/mix.h"
#include "mdbtgrp.h"
#include "include/sys/timeal.h"
#include "include/lib/bcd.h"
#include "include/monitor/alarm.h"
#include "mdbana.h"
#include "mdbuene.h"

mdbtgrp_imm_t MdbTGrpImm[MAX_TGRP];
mdbtgrp_t MdbTGrp[MAX_TGRP];

/**
* @brief 初始化总加组当前库
* @return 0-成功, 1-失败
*/
DECLARE_INIT_FUNC(MdbTGrpInit);
int MdbTGrpInit(void)
{
	memset(MdbTGrpImm, 0, sizeof(MdbTGrpImm));
	memset(MdbTGrp, 0, sizeof(MdbTGrp));

	SET_INIT_FLAG(MdbTGrpInit);

	return 0;
}

/**
* @brief 更新(计算)总加组功率
*/
void UpdateMdbTGrpPower(void)
{
	unsigned char tid, mid, idx, num;
	int pwra, pwri;
	const unsigned char *pflag;

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		pwra = pwri = 0;

		pflag = ParaTGrp[tid].flag;
		num = ParaTGrp[tid].num;
		if(num > MAX_TGRP_METP) num = MAX_TGRP_METP;

		for(idx=0; idx<num; idx++,pflag++) {
			mid = *pflag&TGRFLG_METP;
			if(mid >= MAX_CENMETP) continue;

			if(*pflag&TGRFLG_OP) {   //del
				pwra -= mdbana(mid).pwra_1;
				pwri -= mdbana(mid).pwri_1;
			}
			else {  //add
				pwra += mdbana(mid).pwra_1;
				pwri += mdbana(mid).pwri_1;
			}
		}

		MdbTGrpImm[tid].pwra = pwra;
		MdbTGrpImm[tid].pwri = pwri;
	}
}

/**
* @brief 更新(计算)总加组电能量
*/
void UpdateTGrpEne(void)
{
	unsigned char tid, mid, idx, num;
	int enea, enei, ia, ii;
	const unsigned char *pflag;
	mdbtgrp_t *pt;
	unsigned int utime;

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		pt = &MdbTGrp[tid];

		enea = enei = 0;

		pflag = ParaTGrp[tid].flag;
		num = ParaTGrp[tid].num;
		if(num > MAX_TGRP_METP) num = MAX_TGRP_METP;

		for(idx=0; idx<num; idx++,pflag++) {
			mid = *pflag&TGRFLG_METP;
			if(mid >= MAX_CENMETP) continue;

			if(*pflag&TGRFLG_DIRECT) {  //反向			
				ia = (int)MdbUseEneImm[mid].enena_1m;
				ii = (int)MdbUseEneImm[mid].eneni_1m;
			}
			else { //正向
				ia = (int)MdbUseEneImm[mid].enepa_1m;
				ii = (int)MdbUseEneImm[mid].enepi_1m;
			}

			ia *= (unsigned int)ParaCenMetp[mid].base.pt*ParaCenMetp[mid].base.ct;
			ii *= (unsigned int)ParaCenMetp[mid].base.pt*ParaCenMetp[mid].base.ct;

			if(*pflag&TGRFLG_OP) {   //del
				enea -= ia;
				enei -= ii;
			}
			else {  //add
				enea += ia;
				enei += ii;
			}
		}

		enea += pt->enea_res;
		enei += pt->enei_res;

		pt->enea_res = enea % 1000;
		pt->enei_res = enei % 100;
		enea /= 1000;
		enei /= 100;

		MdbTGrpImm[tid].uenea_1min = enea;
		MdbTGrpImm[tid].uenei_1min = enei;
		MdbTGrpImm[tid].uenea_15min += enea;
		MdbTGrpImm[tid].uenei_15min += enei;

		pt->enea_day[0] += enea;
		pt->enea_mon[0] += enea;
		pt->enei_day[0] += enei;
		pt->enei_mon[0] += enei;

		utime = UTimeReadCurrent()/(15*60);
		ii = utime % 96;
		ii >>= 1;
		ia = ii>>1;
		ii &= 0x01;
		idx = ParaMix.feprd.period[ia];
		if(ii) idx >>= 4;
		idx &= 0x0f;
		if((idx >= MAXNUM_FEEPRD) || (idx >= ParaMix.feprd.fenum)) idx = 0;
		//@add later: ldbec_cal(tid, idx);
		idx += 1;
		pt->enea_day[idx] += enea;
		pt->enea_mon[idx] += enea;
		pt->enei_day[idx] += enei;
		pt->enei_mon[idx] += enei;
	}
}

/**
* @brief 累加总加组用电量(15分钟调用一次)
*/
void AddMdbTGrpUEne15Min(void)
{
	unsigned char tid;

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		MdbTGrpImm[tid].uenea += MdbTGrpImm[tid].uenea_15min;
		MdbTGrpImm[tid].uenei += MdbTGrpImm[tid].uenei_15min;
	}
}

/**
* @brief 总加组用电量冻结结束(需清零用电量)
*/
void MdbTGrpUEneFrezEnd(void)
{
	unsigned char tid;

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		MdbTGrpImm[tid].uenea = 0;
		MdbTGrpImm[tid].uenei = 0;
	}
}


/**
* @brief 清除总加组15分钟用电量
*/
void ClearMdbTGrp15Minute(void)
{
	unsigned char tid;

	for(tid=0; tid<MAX_TGRP; tid++) {
		MdbTGrpImm[tid].uenea_15min = 0;
		MdbTGrpImm[tid].uenei_15min = 0;
	}
}

/**
* @brief 设置总加组当前功率下浮控后有功功率冻结值
* @param tid 总加组号
* @param pwr 功率
*/
void SetMdbTGrpPwrflt(unsigned char tid, int pwr)
{
	if(tid >= MAX_TGRP) return;

	MdbTGrp[tid].pwr_flt = pwr;
}

/**
* @brief 读取当前总加组数据
* @param metpid 总加组号, 1~MAX_TGRP
* @param itemid 数据项编号
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度, 失败返回负数, 无此数据项返回-2, 缓存区溢出返回-1
*/
int ReadMdbTGrp(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len)
{
	int i;
	int *paddene;

	if((metpid==0) ||(metpid > MAX_TGRP)) return -2;
	metpid -= 1;

	switch(itemid) {
	case 0x0201://F17
		PowerToGbformat02(MdbTGrpImm[metpid].pwra, buf);
		return 2;
	case 0x0202://F18
		PowerToGbformat02(MdbTGrpImm[metpid].pwri, buf);
		return 2;

	case 0x0204: paddene = MdbTGrp[metpid].enea_day; break;//F19
	case 0x0208: paddene = MdbTGrp[metpid].enei_day; break;//F20
	case 0x0210: paddene = MdbTGrp[metpid].enea_mon; break;//F21
	case 0x0220: paddene = MdbTGrp[metpid].enei_mon; break;//F22

	case 0x0240:
		EneToGbformat03(MdbTGrp[metpid].ene_bec, buf);
		return 4;

	case 0x0280:
		PowerToGbformat02(MdbTGrp[metpid].pwr_flt, buf);
		return 2;

	default: return 1;
	}

	*buf++ = MAXNUM_FEEPRD;

	for(i=0; i<(MAXNUM_FEEPRD+1); i++) {
		ShortEneToGbformat03(paddene[i], buf);
		buf += 4;
	}

	return 21;
}

