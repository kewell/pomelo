/**
* mdbcur.h -- 当前表计数据
* 
* 
* 创建时间: 2010-5-10
* 最后修改时间: 2010-5-10
*/

#ifndef _MDB_CUR_H
#define _MDB_CUR_H

/*
抄表分为三种:
即时抄表: 响应主站命令即时抄表
分析抄表: 按读表时间间隔读取需要分析及计算的表项
冻结抄表:按冻结时间间隔读表并存库

有三个表数据库：（于RAM中)
分析库：储存测量点需分析的数据，一般为HEX码, mdbana_t
当前库：储存测量点当前的数据（即按读表间隔更新的数据库), 
        一般为BCD码(按照国标格式), mdbcur_t
冻结库：储存冻结的数据, 一般为BCD码(按照国标格式), mdbfrz_t

485类国标表读取方法:
即时抄表: 立刻下发命令至表计并读取,更新当前库,读取当前库
分析抄表: 按读表时间间隔读取数据，然后更新分析库/当前库
冻结抄表: 读取即时库，然后更新冻结库并保存至FLASH中数据库

脉冲/交流采样读取方法:
即时抄表: 读取当前库
分析抄表: 按一分钟间隔读取数据，然后更新分析库/当前库
冻结抄表: 读取即时库，然后更新冻结库并保存至FLASH中数据库

485进口表读取方法
即时抄表: 读取当前库
分析抄表: 按读表时间间隔读取数据，然后更新分析库/当前库
冻结抄表: 读取即时库，然后更新冻结库并保存至FLASH中数据库
*/

#include "cenmet/mdb/mdbconf.h"

#define FLAG_MDBEMPTY	0xee  //无效数据

//F25~F40,当前库
typedef struct {
	unsigned char rdtime[5];    //抄表时间, 分时日月年

	//F25, 58
	unsigned char pwra[12];   //有功功率, SABC, 0.0001kW
	unsigned char pwri[12];    //无功功率, SABC, 0.0001kvar
	unsigned char pwrf[8];    //功率因数, SABC, 0.001
	unsigned char vol[6];    //电压, ABC, 0.1V
	unsigned char amp[8];    //电流, ABCN, 0.01A
	unsigned char pwrv[12];  //视在功率, SABC, 0.0001kVA
	
	//F26, 52
	unsigned char volbr_cnt[8];    //断相次数,SABC
	unsigned char volbr_times[12];  //断相累计时间,SABC, min
	unsigned char volbr_timestart[16];    //最近一次断相起始时间,SABC,分时日月
	unsigned char volbr_timeend[16];    //最近一次断相结束时间,SABC,分时日月
	
	//F27, 55
	unsigned char met_clock[6];    //电表日历时钟,秒分时日月年
	unsigned char bat_runtime[4];  //电池工作时间, 分钟
	unsigned char prog_cnt[3];    //编程总次数
	unsigned char prog_time[6];  //最近一次编程时间,秒分时日月年
	unsigned char metclr_cnt[3];  //电表清零总次数
	unsigned char metclr_time[6];  //最近一次清零发生时刻, 秒分时日月年
	unsigned char dmnclr_cnt[3];  //需量清零总次数
	unsigned char dmnclr_time[6];  //最近一次清零发生时刻, 秒分时日月年
	unsigned char evclr_cnt[3];  //事件清零总次数
	unsigned char evclr_time[6];  //最近一次清零发生时刻, 秒分时日月年
	unsigned char chktime_cnt[3];  //校时总次数
	unsigned char chktime_time[6];  //最近一次校时发生时刻, 秒分时日月年

	//F28, 28
	unsigned char flagchg_state[2*7];  //电表运行状态字变位标志
	unsigned char met_runstate[2*7];  //电表运行状态字

	//F29, 50
	unsigned char enecopper[5*MAXNUM_METPRD];  //当前铜损有功总电能示值, 0.0001kWh
	unsigned char eneiron[5*MAXNUM_METPRD];  //当前铁损有功总电能示值, 0.0001kWh

	//F30, 50
	unsigned char enecopper_lm[5*MAXNUM_METPRD];  //上一结算日铜损有功总电能补偿量 0.0001kWh
	unsigned char eneiron_lm[5*MAXNUM_METPRD];  //上一结算日铁损有功总电能补偿量 0.0001kWh

	//F31, 54
	unsigned char enepa_a[5];  //当前A相正向有功电能示值, 0.0001KWh
	unsigned char enena_a[5];  //当前A相反向有功电能示值, 0.0001kWh
	unsigned char enepi_a[4];  //当前A相组合无功1电能示值, 0.01kvarh
	unsigned char eneni_a[4];  //当前A相组合无功2电能示值, 0.01kvarh
	unsigned char enepa_b[5];  //当前B相正向有功电能示值, 0.0001KWh
	unsigned char enena_b[5];  //当前B相反向有功电能示值, 0.0001kWh
	unsigned char enepi_b[4];  //当前B相组合无功1电能示值, 0.01kvarh
	unsigned char eneni_b[4];  //当前B相组合无功2电能示值, 0.01kvarh
	unsigned char enepa_c[5];  //当前C相正向有功电能示值, 0.0001KWh
	unsigned char enena_c[5];  //当前C相反向有功电能示值, 0.0001kWh
	unsigned char enepi_c[4];  //当前C相组合无功1电能示值, 0.01kvarh
	unsigned char eneni_c[4];  //当前C相组合无功2电能示值, 0.01kvarh

	//F32, 54
	unsigned char enepa_a_lm[5];  //上一结算日A相正向有功电能示值, 0.0001KWh
	unsigned char enena_a_lm[5];  //上一结算日A相反向有功电能示值, 0.0001kWh
	unsigned char enepi_a_lm[4];  //上一结算日A相组合无功1电能示值, 0.01kvarh
	unsigned char eneni_a_lm[4];  //上一结算日A相组合无功2电能示值, 0.01kvarh
	unsigned char enepa_b_lm[5];  //上一结算日B相正向有功电能示值, 0.0001KWh
	unsigned char enena_b_lm[5];  //上一结算日B相反向有功电能示值, 0.0001kWh
	unsigned char enepi_b_lm[4];  //上一结算日B相组合无功1电能示值, 0.01kvarh
	unsigned char eneni_b_lm[4];  //上一结算日B相组合无功2电能示值, 0.01kvarh
	unsigned char enepa_c_lm[5];  //上一结算日C相正向有功电能示值, 0.0001KWh
	unsigned char enena_c_lm[5];  //上一结算日C相反向有功电能示值, 0.0001kWh
	unsigned char enepi_c_lm[4];  //上一结算日C相组合无功1电能示值, 0.01kvarh
	unsigned char eneni_c_lm[4];  //上一结算日C相组合无功2电能示值, 0.01kvarh
	

	//F33, 85
	unsigned char enepa[5*MAXNUM_METPRD];  //正向有功电能, 0~4, 0.0001kWh
	unsigned char enepi[4*MAXNUM_METPRD];  //正向无功电能, 0~4, 0.01kvarh
	unsigned char enepi1[4*MAXNUM_METPRD];  //正向无功一象限电能, 0~4, 0.01kvarh
	unsigned char enepi4[4*MAXNUM_METPRD];  //正向无功四象限电能, 0~4, 0.01kvarh

	//F34, 85
	unsigned char enena[5*MAXNUM_METPRD];  //反向有功电能, 0~4, 0.0001kWh
	unsigned char eneni[4*MAXNUM_METPRD];  //反向无功电能, 0~4, 0.01kvarh
	unsigned char eneni2[4*MAXNUM_METPRD];  //反向无功二象限电能, 0~4, 0.01kvarh
	unsigned char eneni3[4*MAXNUM_METPRD];  //反向无功三象限电能, 0~4, 0.01kvarh

	//F35, 70
	unsigned char dmnpa[3*MAXNUM_METPRD];  //当月正向有功最大需量,0~4,0.0001kW
	unsigned char dmntpa[4*MAXNUM_METPRD];  //当月正向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnpi[3*MAXNUM_METPRD];  //当月正向无功最大需量,0~4,0.0001kvar
	unsigned char dmntpi[4*MAXNUM_METPRD];  //当月正向无功最大需量发生时间,0~4,分时日月
	
	//F36, 70
	unsigned char dmnna[3*MAXNUM_METPRD];  //当月反向有功最大需量,0~4,0.0001kW
	unsigned char dmntna[4*MAXNUM_METPRD];  //当月反向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnni[3*MAXNUM_METPRD];  //当月反向无功最大需量,0~4,0.0001kvar
	unsigned char dmntni[4*MAXNUM_METPRD];  //当月反向无功最大需量发生时间,0~4,分时日月
	
	//F37, 85
	unsigned char enepa_lm[5*MAXNUM_METPRD];  //上月正向有功电能, 0~4, 0.0001kWh
	unsigned char enepi_lm[4*MAXNUM_METPRD];  //上月正向无功电能, 0~4, 0.01kvarh
	unsigned char enepi1_lm[4*MAXNUM_METPRD];  //上月正向无功一象限电能, 0~4, 0.01kvarh
	unsigned char enepi4_lm[4*MAXNUM_METPRD];  //上月正向无功四象限电能, 0~4, 0.01kvarh

	//F38, 85
	unsigned char enena_lm[5*MAXNUM_METPRD];  //上月反向有功电能, 0~4, 0.0001kWh
	unsigned char eneni_lm[4*MAXNUM_METPRD];  //上月反向无功电能, 0~4, 0.01kvarh
	unsigned char eneni2_lm[4*MAXNUM_METPRD];  //上月反向无功二象限电能, 0~4, 0.01kvarh
	unsigned char eneni3_lm[4*MAXNUM_METPRD];  //上月反向无功三象限电能, 0~4, 0.01kvarh
	
	//F39, 70
	unsigned char dmnpa_lm[3*MAXNUM_METPRD];  //上月正向有功最大需量,0~4,0.0001kW
	unsigned char dmntpa_lm[4*MAXNUM_METPRD];  //上月正向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnpi_lm[3*MAXNUM_METPRD];  //上月正向无功最大需量,0~4,0.0001kvar
	unsigned char dmntpi_lm[4*MAXNUM_METPRD];  //上月正向无功最大需量发生时间,0~4,分时日月
	
	//F40, 70
	unsigned char dmnna_lm[3*MAXNUM_METPRD];  //上月反向有功最大需量,0~4,0.0001kW
	unsigned char dmntna_lm[4*MAXNUM_METPRD];  //上月反向有功最大需量发生时间,0~4,分时日月
	unsigned char dmnni_lm[3*MAXNUM_METPRD];  //上月反向无功最大需量,0~4,0.0001kvar
	unsigned char dmntni_lm[4*MAXNUM_METPRD];  //上月反向无功最大需量发生时间,0~4,分时日月

	//F49, 12
	unsigned char phase_arc[12];  //当前电压、电流相位角, 0.1度

	//F57, 217
	unsigned char syntony_value[216+1]; //当前A、B、C三相电压、电流2~N次谐波有效值

	//F58, 229
	unsigned char syntony_rating[222+1];  //当前A、B、C三相电压、电流2~N次谐波含有率

	unsigned short cnt_comerr;  //通信错误
	unsigned char flag_comerr;
} mdbcur_t;

#define UPCURFLAG_ERROR    0x80
#define UPCURFLAG_645      0x00
#define UPCURFLAG_GB      0x01
#define UPCURFLAG_MASK    0x0f

#ifndef DEFINE_MDBCURRENT
extern const mdbcur_t MdbCurrent[MAX_CENMETP];
#endif
#define mdbcur(mid)		(MdbCurrent[mid])
void UpdateMdbCurrent(unsigned short metpid, unsigned int itemid, const unsigned char *buf, int len, unsigned char flag);
int ReadMdbCurrent(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len);
void UpdateMdbCurRdTime(unsigned short mid);


#endif /*_MDB_CUR_H*/

