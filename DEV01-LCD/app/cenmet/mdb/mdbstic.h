/**
* mdbstic.h -- 统计数据
* 
* 
* 创建时间: 2010-5-14
* 最后修改时间: 2010-5-14
*/

#ifndef _MDB_STIC_H
#define _MDB_STIC_H

typedef struct {
	unsigned char min;
	unsigned char hour;
	unsigned char day;
	unsigned char mon;
} stictime_t;

typedef struct {
	unsigned int pwramax[4];    //最大有功功率 0.1W
	stictime_t pwramax_time[4];    //最大有功功率发生时间
	unsigned short pwrzero_time[4];    //有功功率为零时间, 分

	unsigned int dmmax[4];   //分相最大需量 0.1W
	stictime_t dmtime[4];   //分相最大需量发生时间

	unsigned short vol_overtime[3];    //过压累计时间
	unsigned short vol_lesstime[3];    //欠压累计时间
	unsigned short vol_uptime[3];    //越上限累计时间
	unsigned short vol_lowtime[3];    //越下限累计时间
	unsigned short vol_oktime[3];    //合格累计时间

	unsigned short volmax[3];
	unsigned short volmin[3];
	stictime_t volmax_time[3];
	stictime_t volmin_time[3];
	unsigned int volsum[3];    //平均电压
	unsigned short volsnum[3];

	unsigned short ampunb_time;   //电流不平衡累计时间
	unsigned short volunb_time;   //电压不平衡累计时间
	unsigned short ampunb_max;    //电流不平衡最大值,%
	unsigned short volunb_max;    //电压不平衡最大值,%
	stictime_t ampunbmax_time;    //电流不平衡最大值发生时间
	stictime_t volunbmax_time;    //电压不平衡最大值发生时间

	unsigned short ampover_time[3];   //过流累计时间
	unsigned short ampup_time[3];   //电流越限累计时间
	unsigned short zampup_time;   //零序电流越限累计时间
	unsigned short ampmax[4];    //电流最大值
	stictime_t ampmax_time[4];   //电流最大值发生时间

	unsigned short pwrvov_time;    //视在功率越上上限累计时间
	unsigned short pwrvup_time;    //视在功率越上限累计时间

	unsigned short pwrf_time[3];   //功率因数区段累计时间

	unsigned short load_max;  //负载率最大值,%
	unsigned short load_min;  //负载率最小值,%
	stictime_t loadmax_time;  //负载率最大值发生时间
	stictime_t loadmin_time;  //负载率最小值发生时间
} metpstic_t;

typedef struct {
	unsigned short pwr_time;   //供电时间
	unsigned short rst_cnt;   //复位次数
	unsigned char sw_cnt[4];   //跳闸累计次数
	unsigned int comm_bytes;  //终端与主站通信流量
} termstic_t;

typedef struct {
	int pwrmax;
	stictime_t pwrmax_time;
	int pwrmin;
	stictime_t pwrmin_time;

	unsigned short pwrzero_time;
} tgrpstic_day_t;

typedef struct {
	int pwrmax;
	stictime_t pwrmax_time;
	int pwrmin;
	stictime_t pwrmin_time;

	unsigned short pwrzero_time;

	//@add later: 未处理超限累计
	unsigned short pcov_time;   //超功率定值月累计时间及累计电量
	unsigned short ecov_time;  //超月电量定值月累计时间及累计电量
	int pcov_ene;
	int ecov_ene;
} tgrpstic_mon_t;

typedef struct {
	metpstic_t metp_day[MAX_CENMETP];
	metpstic_t metp_mon[MAX_CENMETP];

	tgrpstic_day_t tgrp_day[MAX_TGRP];
	tgrpstic_mon_t tgrp_mon[MAX_TGRP];

	termstic_t term_day;
	termstic_t term_mon;

	unsigned char year;
	unsigned char mon;
	unsigned char day;
	unsigned char reserv;
} mdbstic_t;

#ifndef DEFINE_MDBSTIC
extern const mdbstic_t MdbStic;
#endif

void UpdateMdbStic(void);
void MdbSticEmptyDay(void);
void MdbSticEmptyMonth(void);
void MdbSticInit(void);

void UpdateSticResetCount(void);
void UpdateSticComm(unsigned int bytes);

void BakupSticMetp(unsigned char flag, unsigned short mid, unsigned char *buf);
void BakupSticTerm(unsigned char flag, unsigned char *buf);

void BakupSticTGrpDay(unsigned short tid, unsigned char *buf);
void BakupSticTGrpMon(unsigned short tid, unsigned char *buf);

#endif /*_MDB_STIC_H*/

