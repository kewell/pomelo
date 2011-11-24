/**
* mdbstic.c -- 统计数据函数
* 
* 
* 创建时间: 2010-5-14
* 最后修改时间: 2010-5-14
*/

#define DEFINE_MDBSTIC

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFINE_MDBSTIC

#include "include/debug.h"
#include "include/sys/timeal.h"
#include "include/param/capconf.h"
#include "mdbana.h"
#include "include/lib/bcd.h"
#include "include/lib/align.h"
#include "include/param/metp.h"
#include "include/param/meter.h"
#include "include/param/hardware.h"
#include "mdbstic.h"
#include "mdbtgrp.h"

mdbstic_t MdbStic;

#define STICPWRMIN_DEFAULT    0x7fffffff

/**
* @brief 将日统计清空(换日)
*/
void MdbSticEmptyDay(void)
{
	unsigned short mid;
	sysclock_t clock;

	memset(MdbStic.metp_day, 0, sizeof(metpstic_t)*MAX_CENMETP);
	memset(&MdbStic.term_day, 0, sizeof(termstic_t));

	for(mid=0; mid<MAX_CENMETP; mid++) {
		MdbStic.metp_day[mid].volmin[0] = 10000;
		MdbStic.metp_day[mid].volmin[1] = 10000;
		MdbStic.metp_day[mid].volmin[2] = 10000;
		MdbStic.metp_day[mid].load_min = 7999;
	}

	for(mid=0; mid<MAX_TGRP; mid++) {
		MdbStic.tgrp_day[mid].pwrmin = STICPWRMIN_DEFAULT;
	}

	SysClockReadCurrent(&clock);
	MdbStic.day = clock.day;
}

/**
* @brief 将月统计清空(换月)
*/
void MdbSticEmptyMonth(void)
{
	unsigned short mid;
	sysclock_t clock;

	memset(MdbStic.metp_mon, 0, sizeof(metpstic_t)*MAX_CENMETP);
	memset(&MdbStic.term_mon, 0, sizeof(termstic_t));

	for(mid=0; mid<MAX_CENMETP; mid++) {
		MdbStic.metp_mon[mid].volmin[0] = 10000;
		MdbStic.metp_mon[mid].volmin[1] = 10000;
		MdbStic.metp_mon[mid].volmin[2] = 10000;
		MdbStic.metp_mon[mid].load_min = 7999;
	}
	
	for(mid=0; mid<MAX_TGRP; mid++) {
		MdbStic.tgrp_mon[mid].pwrmin = STICPWRMIN_DEFAULT;
	}
	
	SysClockReadCurrent(&clock);
	MdbStic.year = clock.year;
	MdbStic.mon = clock.month;
}

static void SetSticTime(stictime_t *ptime, const sysclock_t *pclk)
{
	ptime->mon = pclk->month;
	ptime->day = pclk->day;
	ptime->hour = pclk->hour;
	ptime->min = pclk->minute;

	HexToBcd(&ptime->min, 4);
}

#define MAX_DMWIN    16
#define SIZE_DMWIN    15
typedef struct {
	int list[MAX_DMWIN];
	int sum;
	unsigned char num;
	unsigned char head;
	unsigned char tail;
} dmcal_t;
typedef struct {
	dmcal_t dm[4];
} dmcal_all_t;
static dmcal_all_t DmCal[MAX_CENMETP];

/**
* @brief 计算分相需量
* @param mid 测量点号, 0~
* @param pclock 当前时间
*/
static void MdbSticCalDm(unsigned short mid, const sysclock_t *pclk)
{
	dmcal_t *pcal;
	int i;
	int avr;

	for(i=0; i<4; i++) {
		pcal = &DmCal[mid].dm[i];
		avr = MdbAnalyze[mid].pwra[i];
		pcal->list[pcal->tail] = avr;
		pcal->tail++;
		if(pcal->tail >= MAX_DMWIN) pcal->tail = 0;

		pcal->sum += avr;
		pcal->num++;

		if(pcal->num < 10) continue;

		avr = pcal->sum / pcal->num;
		while(pcal->num >= SIZE_DMWIN) {
			pcal->sum -= pcal->list[pcal->head];
			pcal->head++;
			if(pcal->head >= MAX_DMWIN) pcal->head = 0;
			pcal->num -= 1;
		}

		if(avr < 0) avr *= -1;
		if(avr > MdbStic.metp_day[mid].dmmax[i]) {
			MdbStic.metp_day[mid].dmmax[i] = avr;
			SetSticTime(&MdbStic.metp_day[mid].dmtime[i], pclk);
		}

		if(avr > MdbStic.metp_mon[mid].dmmax[i]) {
			MdbStic.metp_mon[mid].dmmax[i] = avr;
			SetSticTime(&MdbStic.metp_mon[mid].dmtime[i], pclk);
		}
	}
}

/**
* @brief 计算功率统计值
* @param mid 测量点号, 0~
* @param pclock 当前时间
*/
static void UpdateSticPower(unsigned short mid, const sysclock_t *pclk)
{
	metpstic_t *pmday, *pmmon;
	unsigned int ul;
	int i;
	unsigned short us;

	pmday = &MdbStic.metp_day[mid];
	pmmon = &MdbStic.metp_mon[mid];

	for(i=0; i<4; i++) {
		if(MdbAnalyze[mid].pwra[i] < 0) ul = MdbAnalyze[mid].pwra[i] * -1;
		else ul = MdbAnalyze[mid].pwra[i];
		if(ul > pmday->pwramax[i]) {
			pmday->pwramax[i] = ul;
			SetSticTime(&pmday->pwramax_time[i], pclk);
		}

		if(ul > pmmon->pwramax[i]) {
			pmmon->pwramax[i] = ul;
			SetSticTime(&pmmon->pwramax_time[i], pclk);
		}

		if(0 == mrd_firstfail(mid, (MBMSK_PWRAS<<i))) {
			if(0 == MdbAnalyze[mid].pwra[i]) {
				pmday->pwrzero_time[i]++;
				pmmon->pwrzero_time[i]++;
			}
		}
	}

	ul = MdbAnalyze[mid].pwrv;
	if(ul > ParaCenMetp[mid].limit.pwr_over) {
		pmday->pwrvov_time++;
		pmmon->pwrvov_time++;
	}

	if(ul > ParaCenMetp[mid].limit.pwr_limit) {
		pmday->pwrvup_time++;
		pmmon->pwrvup_time++;
	}

	if(0 == mrd_firstfail(mid, MBMSK_PWRF)) {
		if(MdbAnalyze[mid].pwrf < 0) us = MdbAnalyze[mid].pwrf * -1;
		else us = MdbAnalyze[mid].pwrf;

		if(us < ParaCenMetp[mid].pwrf.limit1) {
			pmday->pwrf_time[0]++;
			pmmon->pwrf_time[0]++;
		}
		else if(us >= ParaCenMetp[mid].pwrf.limit2) {
			pmday->pwrf_time[2]++;
			pmmon->pwrf_time[2]++;
		}
		else {
			pmday->pwrf_time[1]++;
			pmmon->pwrf_time[1]++;
		}
	}
}

/**
* @brief 计算电压统计值
* @param mid 测量点号, 0~
* @param pclock 当前时间
*/
static void UpdateSticVoltage(unsigned short mid, const sysclock_t *pclk)
{
	metpstic_t *pmday, *pmmon;
	unsigned int mask;
	unsigned short us;
	int i;
	unsigned char bok;

	//if(METP_METER != ParaCenMetp[mid].base.attr) return;

	pmday = &MdbStic.metp_day[mid];
	pmmon = &MdbStic.metp_mon[mid];

	for(i=0; i<3; i++) {
		bok = 1;
		us = MdbAnalyze[mid].vol[i];

		if(us > ParaCenMetp[mid].limit.vol_over) {
			bok = 0;
			pmday->vol_overtime[i]++;
			pmmon->vol_overtime[i]++;
		}

		if(us > ParaCenMetp[mid].limit.volok_up) {
			bok = 0;
			pmday->vol_uptime[i]++;
			pmmon->vol_uptime[i]++;
		}

		if(us > pmday->volmax[i]) {
			pmday->volmax[i] = us;
			SetSticTime(&pmday->volmax_time[i], pclk);
		}

		if(us > pmmon->volmax[i]) {
			pmmon->volmax[i] = us;
			SetSticTime(&pmmon->volmax_time[i], pclk);
		}

		mask = MBMSK_VOLA;
		mask <<= i;

		if(0 == mrd_firstfail(mid, mask)) {
			if(us < ParaCenMetp[mid].limit.vol_less) {
				bok = 0;
				pmday->vol_lesstime[i]++;
				pmmon->vol_lesstime[i]++;
			}

			if(us < ParaCenMetp[mid].limit.volok_low) {
				bok = 0;
				pmday->vol_lowtime[i]++;
				pmmon->vol_lowtime[i]++;
			}

			if(us < pmday->volmin[i]) {
				pmday->volmin[i] = us;
				SetSticTime(&pmday->volmin_time[i], pclk);
			}

			if(us < pmmon->volmin[i]) {
				pmmon->volmin[i] = us;
				SetSticTime(&pmmon->volmin_time[i], pclk);
			}

			pmday->volsum[i] += us;
			pmday->volsnum[i]++;

		}

		if(bok) {
			pmday->vol_oktime[i]++;
			pmmon->vol_oktime[i]++;
		}
	}
}

/**
* @brief 计算不平衡统计值
* @param mid 测量点号, 0~
* @param pclock 当前时间
*/
static void UpdateSticUnb(unsigned short mid, const sysclock_t *pclk)
{
	unsigned short unb;
	

	//if(METP_METER != ParaCenMetp[mid].base.attr) return;

	unb = MdbAnalyze[mid].vol_unb;

	if(unb > ParaCenMetp[mid].limit.vol_unb) {
		MdbStic.metp_day[mid].volunb_time++;
		MdbStic.metp_mon[mid].volunb_time++;
	}

	if(unb > MdbStic.metp_day[mid].volunb_max) {
		MdbStic.metp_day[mid].volunb_max = unb;
		SetSticTime(&MdbStic.metp_day[mid].volunbmax_time, pclk);
	}

	if(unb > MdbStic.metp_mon[mid].volunb_max) {
		MdbStic.metp_mon[mid].volunb_max = unb;
		SetSticTime(&MdbStic.metp_mon[mid].volunbmax_time, pclk);
	}

	unb = MdbAnalyze[mid].amp_unb;

	if(unb > ParaCenMetp[mid].limit.amp_unb) {
		MdbStic.metp_day[mid].ampunb_time++;
		MdbStic.metp_mon[mid].ampunb_time++;
	}

	if(unb > MdbStic.metp_day[mid].ampunb_max) {
		MdbStic.metp_day[mid].ampunb_max = unb;
		SetSticTime(&MdbStic.metp_day[mid].ampunbmax_time, pclk);
	}

	if(unb > MdbStic.metp_mon[mid].ampunb_max) {
		MdbStic.metp_mon[mid].ampunb_max = unb;
		SetSticTime(&MdbStic.metp_mon[mid].ampunbmax_time, pclk);
	}
}

/**
* @brief 计算电流统计值
* @param mid 测量点号, 0~
* @param pclock 当前时间
*/
static void UpdateSticAmp(unsigned short mid, sysclock_t *pclk)
{
	metpstic_t *pmday, *pmmon;
	unsigned short us;
	int i;

	//if(METP_METER != ParaCenMetp[mid].base.attr) return;

	pmday = &MdbStic.metp_day[mid];
	pmmon = &MdbStic.metp_mon[mid];

	for(i=0; i<3; i++) {
		if(MdbAnalyze[mid].amp[i] < 0) us = MdbAnalyze[mid].amp[i] * -1;
		else us = MdbAnalyze[mid].amp[i];

		if(us > ParaCenMetp[mid].limit.amp_over) {
			pmday->ampover_time[i]++;
			pmmon->ampover_time[i]++;
		}

		if(us > ParaCenMetp[mid].limit.amp_limit) {
			pmday->ampup_time[i]++;
			pmmon->ampup_time[i]++;
		}

		if(us > pmmon->ampmax[i]) {
			pmday->ampmax[i] = us;
			SetSticTime(&pmday->ampmax_time[i], pclk);
		}

		if(us > pmmon->ampmax[i]) {
			pmmon->ampmax[i] = us;
			SetSticTime(&pmmon->ampmax_time[i], pclk);
		}
	}
}

/**
* @brief 计算负荷统计值
* @param mid 测量点号, 0~
* @param pclock 当前时间
*/
static void UpdateSticLoad(unsigned short mid, sysclock_t *pclk)
{
	int i;

	if(0 == mrd_fail(mid, MBMSK_PWRAS) && 0 != ParaCenMetp[mid].base.pwr_rating) {
		i = (MdbAnalyze[mid].pwrv * 1000) / ParaCenMetp[mid].base.pwr_rating;

		if(i > MdbStic.metp_day[mid].load_max) {
			MdbStic.metp_day[mid].load_max = i;
			SetSticTime(&MdbStic.metp_day[mid].loadmax_time, pclk);
		}

		if(i < MdbStic.metp_day[mid].load_min) {
			MdbStic.metp_day[mid].load_min = i;
			SetSticTime(&MdbStic.metp_day[mid].loadmin_time, pclk);
		}
	}
}

/**
* @brief 更新总加组统计数据
*/
static void UpdateMdbSticTGrp(void)
{
	tgrpstic_day_t *pday;
	tgrpstic_mon_t *pmon;
	unsigned char tid;
	sysclock_t clk;
	int pwr;
	unsigned long ul;

	SysClockReadCurrent(&clk);

	for(tid=0; tid<MAX_TGRP; tid++) {
		if(EMPTY_TGRP(tid)) continue;

		pday = &MdbStic.tgrp_day[tid];
		pmon = &MdbStic.tgrp_mon[tid];

		pwr = MdbTGrpImm[tid].pwra;
		ul = abs(pwr);

		if(ul > abs(pday->pwrmax)) {
			pday->pwrmax = pwr;
			SetSticTime(&pday->pwrmax_time, &clk);
		}

		if(ul < abs(pday->pwrmin)) {
			pday->pwrmin = pwr;
			SetSticTime(&pday->pwrmin_time, &clk);
		}

		if(ul > abs(pmon->pwrmax)) {
			pmon->pwrmax = pwr;
			SetSticTime(&pmon->pwrmax_time, &clk);
		}

		if(ul < abs(pmon->pwrmin)) {
			pmon->pwrmin = pwr;
			SetSticTime(&pmon->pwrmin_time, &clk);
		}

		if(0 == pwr) {
			pday->pwrzero_time++;
			pmon->pwrzero_time++;
		}
	}
}

/**
* @brief 更新测量点统计数据
*/
static void UpdateMdbSticMetp(void)
{
	unsigned short mid;
	sysclock_t clk;

	SysClockReadCurrent(&clk);

	for(mid=0; mid<MAX_CENMETP; mid++) {
		if(EMPTY_CENMETP(mid)) continue;

		UpdateSticPower(mid, &clk);
		MdbSticCalDm(mid, &clk);
		UpdateSticVoltage(mid, &clk);
		UpdateSticAmp(mid, &clk);
		UpdateSticUnb(mid, &clk);
		UpdateSticLoad(mid, &clk);
	}
}

/**
* @brief 更新终端统计数据
*/
static void UpdateMdbSticTerm(void)
{
	MdbStic.term_day.pwr_time++;
	MdbStic.term_mon.pwr_time++;
}

/**
* @brief 更新上行通信流量
* @param bytes 通信流量
*/
void UpdateSticComm(unsigned int bytes)
{
	MdbStic.term_day.comm_bytes += bytes;
	MdbStic.term_mon.comm_bytes += bytes;
}

/**
* @brief 更新复位次数
*/
void UpdateSticResetCount(void)
{
	MdbStic.term_day.rst_cnt += 1;
	MdbStic.term_mon.rst_cnt += 1;
}

/**
* @brief 更新统计数据库
*/
void UpdateMdbStic(void)
{
	UpdateMdbSticMetp();
	UpdateMdbSticTerm();
	UpdateMdbSticTGrp();
}

/**
* @brief 统计数据库初始化
*/
void MdbSticInit(void)
{
	memset(DmCal, 0, sizeof(DmCal));

	MdbSticEmptyDay();
	MdbSticEmptyMonth();
}


/**
* @brief 冻结测量点统计数据(存入冻结数据库中，二类数据)
* @param flag 备份标志, 0-日数据, 1-月数据
* @param mid 测量点号, 0~
* @param buf 缓存区指针
*/
void BakupSticMetp(unsigned char flag, unsigned short mid, unsigned char *buf)
{
	metpstic_t *psrc;
	int i;
	unsigned int ul;

	if(flag) psrc = &MdbStic.metp_mon[mid];
	else psrc = &MdbStic.metp_day[mid];

	for(i=0; i<4; i++) {
		UnsignedToBcd(psrc->pwramax[i], buf, 3); buf += 3;
		smallcpy(buf, &psrc->pwramax_time[i].min, 3); buf += 3;
	}

	for(i=0; i<4; i++) {
		DEPART_SHORT(psrc->pwrzero_time[i], buf); buf += 2;
	}

	for(i=0; i<4; i++) {
		UnsignedToBcd(psrc->dmmax[i], buf, 3); buf += 3;
		memcpy(buf, &psrc->dmtime[i].min, 3); buf += 3;
	}

	for(i=0; i<3; i++) {
		DEPART_SHORT(psrc->vol_overtime[i], buf); buf += 2;
		DEPART_SHORT(psrc->vol_lesstime[i], buf); buf += 2;
		DEPART_SHORT(psrc->vol_uptime[i], buf); buf += 2;
		DEPART_SHORT(psrc->vol_lowtime[i], buf); buf += 2;
		DEPART_SHORT(psrc->vol_oktime[i], buf); buf += 2;
	}

	for(i=0; i<3; i++) {
		UnsignedToBcd(psrc->volmax[i], buf, 2); buf += 2;
		smallcpy(buf, &psrc->volmax_time[i].min, 3); buf += 3;
		UnsignedToBcd(psrc->volmin[i], buf, 2); buf += 2;
		smallcpy(buf, &psrc->volmin_time[i].min, 3); buf += 3;
	}

	for(i=0; i<3; i++) {
		ul = psrc->volsum[i];
		if(psrc->volsnum[i]) ul /= psrc->volsnum[i];
		else ul = 0;

		UnsignedToBcd(ul, buf, 2); buf += 2;
		if(!flag) {
			MdbStic.metp_mon[mid].volsum[i] += ul;
			MdbStic.metp_mon[mid].volsnum[i]++;
		}
	}

	DEPART_SHORT(psrc->ampunb_time, buf); buf += 2;
	DEPART_SHORT(psrc->volunb_time, buf); buf += 2;
	UnsignedToBcd(psrc->ampunb_max, buf, 2); buf += 2;
	smallcpy(buf, &psrc->ampunbmax_time.min, 3); buf += 3;
	UnsignedToBcd(psrc->volunb_max, buf, 2); buf += 2;
	smallcpy(buf, &psrc->volunbmax_time.min, 3); buf += 3;

	for(i=0; i<3; i++) {
		DEPART_SHORT(psrc->ampover_time[i], buf); buf += 2;
		DEPART_SHORT(psrc->ampup_time[i], buf); buf += 2;
	}

	DEPART_SHORT(psrc->zampup_time, buf); buf += 2;

	for(i=0; i<4; i++) {
		UnsignedToBcd(psrc->ampmax[i], buf, 2); buf += 2;
		smallcpy(buf, &psrc->ampmax_time[i].min, 3); buf += 3;
	}

	DEPART_SHORT(psrc->pwrvov_time, buf); buf += 2;
	DEPART_SHORT(psrc->pwrvup_time, buf);

	UnsignedToBcd(psrc->load_max, buf, 2); buf += 2;
	smallcpy(buf, &psrc->loadmax_time.min, 3); buf += 3;
	UnsignedToBcd(psrc->load_min, buf, 2); buf += 2;
	smallcpy(buf, &psrc->loadmin_time.min, 3); buf += 3;

	DEPART_SHORT(psrc->pwrf_time[0], buf); buf += 2;
	DEPART_SHORT(psrc->pwrf_time[1], buf); buf += 2;
	DEPART_SHORT(psrc->pwrf_time[2], buf);
}

/**
* @brief 冻结终端统计数据(存入冻结数据库中)
* @param flag 备份标志, 0-日数据, 1-月数据
* @param buf 缓存区指针
*/
void BakupSticTerm(unsigned char flag, unsigned char *buf)
{
	termstic_t *psrc;

	if(flag) psrc = &MdbStic.term_mon;
	else psrc = &MdbStic.term_day;

	DEPART_SHORT(psrc->pwr_time, buf); buf += 2;
	DEPART_SHORT(psrc->rst_cnt, buf); buf += 2;
	smallcpy(buf, psrc->sw_cnt, 4);
	smallcpy(buf, &psrc->comm_bytes, 4);
}

/**
* @brief 冻结测量点统计日数据(存入冻结数据库中)
* @param tid 总加组号, 0~
* @param buf 缓存区指针
*/
void BakupSticTGrpDay(unsigned short tid, unsigned char *buf)
{
	tgrpstic_day_t *psrc;
	int i;

	psrc = &MdbStic.tgrp_day[tid];

	PowerToGbformat02(psrc->pwrmax,buf); buf += 2;
	smallcpy(buf, &psrc->pwrmax_time.min, 3); buf += 3;
	if(STICPWRMIN_DEFAULT == psrc->pwrmin) i = 0;
	else i = psrc->pwrmin;
	PowerToGbformat02(i, buf); buf += 2;
	smallcpy(buf, &psrc->pwrmin_time.min, 3); buf += 3;
	DEPART_SHORT(psrc->pwrzero_time, buf);
}

/**
* @brief 备份测量点统计月数据(存入冻结数据库中)
* @param tid 总加组号, 0~
* @param buf 缓存区指针
*/
void BakupSticTGrpMon(unsigned short tid, unsigned char *buf)
{
	tgrpstic_mon_t *psrc;
	int i;

	psrc = &MdbStic.tgrp_mon[tid];

	PowerToGbformat02(psrc->pwrmax, buf); buf += 2;
	smallcpy(buf, &psrc->pwrmax_time.min, 3); buf += 3;
	if(STICPWRMIN_DEFAULT == psrc->pwrmin) i = 0;
	else i = psrc->pwrmin;
	PowerToGbformat02(i, buf); buf += 2;
	smallcpy(buf, &psrc->pwrmin_time.min, 3); buf += 3;
	DEPART_SHORT(psrc->pwrzero_time, buf); buf += 2;

	DEPART_SHORT(psrc->pcov_time, buf); buf += 2;
	ShortEneToGbformat03(psrc->pcov_ene, buf); buf += 4;
	DEPART_SHORT(psrc->ecov_time, buf); buf += 2;
	ShortEneToGbformat03(psrc->ecov_ene, buf);
}


