#ifndef _PLTASK_H
#define _PLTASK_H

#include "include/sys/timeal.h"

#define	PROTO_DL1997		0x01
#define	PROTO_DL2007		0x02

#define	PROTO_DL1997_376_1		1
#define	PROTO_DL2007_376_1		30
#define	PROTO_PLC_376_1			31
#define	ONLY_97_METER			1
#define	ONLY_07_METER			2
#define	DOUBLE_METER			3


#define	FRAME_HEAD		0x68
#define	MAX_FRAME_LENTH		256
#define	MIN_FRAME_LENTH		15
#define	COMM_MODE		0x1F
#define	DIR_CTL			0x80
#define	FRM_CTL		0x40
#define	RUTER_CHECK		0x10

#define 	METEV_1MIN_CHECK_RUTER_RUN_STAT			0x00000040   
#define 	METEV_SET_RUTER_WORK_MOD					0x00000080   
#define 	METEV_PLC_POINT_READ							0x00000100   
#define 	METEV_STOP_PLC_POINT_READ					0x00000200   
#define 	SHELL_MAX_QUERY_TIMES						1

#define	PLC_EVENT	METEV_1MIN_CHECK_RUTER_RUN_STAT|METEV_SET_RUTER_WORK_MOD |METEV_PLC_POINT_READ	|METEV_STOP_PLC_POINT_READ	

#define 	RUTER_RUN_STAT			0x02
#define 	MAX_METER_CNT			2040
#define 	HAVE_READ					0xAA


typedef struct{
	unsigned char head;
	unsigned char len1;
	unsigned char len2;
	unsigned char ctl;
	unsigned char mes[6];
	unsigned char afn;
	unsigned char fn[2];	
	unsigned char data[256];	
}plc_packet_t;


typedef struct{
	unsigned char head;
	unsigned char len1;
	unsigned char len2;
	unsigned char ctl;
	unsigned char mes[6];
	unsigned char afn;
	unsigned char fn[2];	
	unsigned char run_stat;
	unsigned char node_cnt[2];
	unsigned char have_read_node_cnt[2];
	unsigned char fwd_read_node_cnt[2];
	unsigned char work_swch;	
	unsigned char comm_spch[2];	
	unsigned char fwd_cnt_1;	
	unsigned char fwd_cnt_2;	
	unsigned char fwd_cnt_3;	
	unsigned char ph_stp_1;	
	unsigned char ph_stp_2;	
	unsigned char ph_stp_3;	
}AFN10_FN4_t;

typedef struct{
	unsigned char meter_index[2];
	unsigned char proto;
	unsigned char len;
	unsigned char data[256];	
}pak_645_t;



typedef struct{
	unsigned char read_stat;
	unsigned short meter_index;
	unsigned short met_id;
	unsigned char portcfg;
	unsigned char meter_proto;
	unsigned char meter_addr[6];
	unsigned char meter_ene[20];
	unsigned short readtime;   //–° ±*60+∑÷÷”
}meter_ene_buf_t;

typedef struct{
	unsigned short meter_index;
	unsigned char meter_proto;
	unsigned char meter_addr[6];
}jzq_meter_t;

typedef struct{
	unsigned char node_type;
	unsigned char node_addr[6];
	unsigned short mac_addr;
	unsigned char forwd_lev;
	unsigned short forwd_mac_addr[7];
}pl_node_info_t;

typedef struct{
	unsigned char node_addr[6];
	unsigned short mac_addr;
}pl_cjq_info_t;

typedef struct{
	unsigned short read_meter_succ_cnt;
	unsigned char read_meter_prd;
}pl_read_meter_stic_t;


extern pl_node_info_t pl_node_info[MAX_METER_CNT];
extern meter_ene_buf_t meter_ene_buffer[MAX_METER_CNT];
extern pl_cjq_info_t pl_cjq_info[1024];
extern pl_read_meter_stic_t pl_read_meter_stic[31];

extern unsigned short meter_total_cnt;
extern unsigned short cen_meter_cnt;
extern unsigned short imp_meter_cnt;
extern unsigned short imp_meter_read_succ_cnt;
extern unsigned short meter_read_succ_cnt;
extern sysclock_t read_meter_start_time;
extern sysclock_t read_meter_finish_time;
extern unsigned char read_meter_finish;


int check_buf(unsigned char *buf);
int process_add_addr_frame(unsigned char *buf,unsigned char len,jzq_meter_t *jzq_meter);
int point_read(unsigned int itemid,unsigned char *meter_addr,unsigned char *buf);
int check_rcv_frame(unsigned char *buf,int len);
int process_rcv_645_pak(unsigned char *buf,unsigned char len,unsigned char *meter_addr,unsigned char *ene);
int make_read_645_pkt(const unsigned char *addr, unsigned int itemid, unsigned char *buf);
unsigned short get_meter_total_cnt();
unsigned short get_cen_meter_total_cnt();
unsigned short get_imp_meter_total_cnt();
unsigned short get_read_meter_succ_cnt();
unsigned short get_imp_meter_read_succ_cnt();
void go_to_active_register(void);
void go_to_active_register_init(void);
void go_to_set_ruter_work_mode(void);
void clear_read_meter_stat(void);
void stop_point_read(void);
void go_to_point_read(void);
int check_read_meter_succ_cnt(void);
#endif
