/**
* plc_stat.h -- 载波通信状态
* 
* 
* 创建时间: 2010-5-21
* 最后修改时间: 2010-5-21
*/

#ifndef _PLC_STAT_H
#define _PLC_STAT_H

#include "include/param/capconf.h"

typedef struct {
	unsigned short metid;
	unsigned char routes;  //中继路由级数
	unsigned char phase;  //载波抄读相位
	unsigned char quality;  //载波信号品质
	unsigned char okflag;  //最近一次抄表成功/失败标志 , 1/0
	unsigned char failcount;  //最近连续失败累计次数
	unsigned char unuse;
	sysclock_t oktime;  //最近一次抄表成功时间
	sysclock_t failtime;  //最近一次抄表失败时间
} plc_state_t;
extern plc_state_t PlcState[MAX_METER];//F170 PARA

//单次通信信息
typedef struct {
	unsigned char routes;  //中继路由级数
	unsigned char phase;  //载波抄读相位
	unsigned char quality;  //载波信号品质
	unsigned short ctime;  //通信时间 100ms
} plc_sinfo_t;

int PlcStateSave(void);

int ReadPlMdbState(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len);

#define MAX_PLCYCSTAT		13
//抄表轮次统计
struct pl_cycstat{
	unsigned short ok_mets;  //抄表成功个数
	unsigned char time_start[6];  //轮次开始时间(BCD)
	unsigned char time_end[6];  //轮次结束时间(BCD)
	unsigned short rd_mets;  //总共抄读表数
};
extern struct pl_cycstat PlCycState[MAX_PLCYCSTAT];

int PlStatisticSave(void);

#endif /*_PLC_STAT_H*/

