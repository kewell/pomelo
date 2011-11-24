/**
* unique.h -- 自定义参数
* 
* 
* 创建时间: 2010-5-18
* 最后修改时间: 2010-5-18
*/

#ifndef _PARA_UNIQUE_H
#define _PARA_UNIQUE_H

typedef struct {
	unsigned char uplink;   //上行通信接口
	unsigned char unuse[3];
	char cmply[2];
	unsigned char addr_area[2];  //终端地址-区域码
	unsigned char addr_sn[2];   //终端地址-序号

	unsigned char addr_mac[6];  //终端MAC地址

	char manuno[14];  //终端外部生产编号
	char manuno_inner[12];  //终端内部生产编号
	unsigned int gprs_mod_baud;
} para_uni_t;

#ifndef DEFINE_PARAUNI
extern const para_uni_t ParaUni;
#endif

#endif /*_PARA_UNIQUE_H*/

