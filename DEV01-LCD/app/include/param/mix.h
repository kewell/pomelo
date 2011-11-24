/**
* mix.h -- 综合参数头文件
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#ifndef _PARAM_MIX_H
#define _PARAM_MIX_H

#include "include/param/commport.h"
#include "include/param/capconf.h"

//F35 终端台区集中抄表重点户设置
typedef struct {
	unsigned char num;    //<=20
	unsigned char reserv;
	unsigned short metid[MAX_IMPORTANT_USER];  //重点户的电能表/交流采样装置序号
} cfg_impuser_t;

//F37 终端级联通信参数
#define MAX_CASCADE		3
typedef struct {
	unsigned char port;  //终端级联通信端口号
	unsigned char frame;   //终端级联通信控制字
	unsigned char timeout;  //接收等待报文超时时间, 100ms
	unsigned char timeout_byte;  //接收等待字节超时时间, 10ms
	unsigned char retry;  //级联方(主动站)接收失败重发次数
	unsigned char cycle;  //级联巡测周期, minute
	unsigned char flag;  //级联/被级联标志, 0-主动站, 1-被级联方
	unsigned char num;  //被级联/级联的终端个数
	unsigned char addr[MAX_CASCADE*4];  //级联的终端地址
} cfg_cascade_t;

//F21 终端电能量费率时段和费率数
typedef struct {
	unsigned char period[48];
	unsigned char fenum;
	unsigned char reserv[3];
} cfg_feprd_t;

//F59 电能表异常判别阈值设定
typedef struct {
	unsigned char diff;  //电能量超差阈值
	unsigned char fly;  //电能表飞走阈值
	unsigned char stop;  //电能表停走阈值
	unsigned char time;  //电能表校时阈值
} cfg_metabnor_t;

//F60 谐波限值
typedef struct {
	unsigned short vol_sum;  //总畸变电压含有率上限, 0.1%
	unsigned short vol_odd;  //奇次谐波电压含有率上限
	unsigned short vol_even;  //偶次谐波电压含有率上限
	unsigned short vol_2;  //2次谐波电压含有率上限
	unsigned short vol_4;
	unsigned short vol_6;
	unsigned short vol_8;
	unsigned short vol_10;
	unsigned short vol_12;
	unsigned short vol_14;
	unsigned short vol_16;
	unsigned short vol_18;
	unsigned short vol_3;  //3次谐波电压含有率上限
	unsigned short vol_5;
	unsigned short vol_7;
	unsigned short vol_9;
	unsigned short vol_11;
	unsigned short vol_13;
	unsigned short vol_15;
	unsigned short vol_17;
	unsigned short vol_19;
	unsigned short amp_sum;  //总畸变电流有效值上限, 0.01A
	unsigned short amp_2;  //2次谐波电流有效值上限
	unsigned short amp_4;
	unsigned short amp_6;
	unsigned short amp_8;
	unsigned short amp_10;
	unsigned short amp_12;
	unsigned short amp_14;
	unsigned short amp_16;
	unsigned short amp_18;
	unsigned short amp_3;  //3次谐电流波有效值上限
	unsigned short amp_5;
	unsigned short amp_7;
	unsigned short amp_9;
	unsigned short amp_11;
	unsigned short amp_13;
	unsigned short amp_15;
	unsigned short amp_17;
	unsigned short amp_19;
} cfg_syntony_t;

typedef struct {
	unsigned char bactsend;  //禁止主动上报, 1-禁止,0-不禁止
	unsigned char bactcomm;  //禁止与主站通话, 1-禁止,0-不禁止
	unsigned char reserv[3];
	cfg_feprd_t feprd;
	cfg_impuser_t impuser;
	unsigned int upflow_max;  //F36 终端上行通信流量门限设置 byte(月通信流量) 0表示不需控制
	cfg_cascade_t cascade;
	cfg_metabnor_t metabnor;
	cfg_syntony_t syntony;
} para_mix_t;

typedef struct {
	para_commport_t commport[MAX_COMMPORT];
	para_mix_t mix;
} save_mix_t;

#ifndef DEFINE_PARAMIX
extern const save_mix_t ParaMixSave;
#define ParaMix		(ParaMixSave.mix)
#endif

#endif /*_PARAM_MIX_H*/

