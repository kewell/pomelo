/**
* svrcomm.h -- 服务器通信
* 
* 
* 创建时间: 2010-5-19
* 最后修改时间: 2010-5-19
*/

#ifndef _SVRCOMM_H
#define _SVRCOMM_H
#include "uplink/uplink_pkt.h"

void SvrCommPeekEvent(unsigned long waitmask, unsigned long *pevent);
int SvrNoteProc(unsigned char itf);

void SvrMessageProc(unsigned char itf);
int SvrEchoSend(unsigned char itf, uplink_pkt_t *pkt);
void SvrEchoNoData(unsigned char itf, uplink_pkt_t *pkt);
int IsSvrMsgAfn(unsigned char afn);

#define LINESTAT_OFF    0
#define LINESTAT_ON    1
extern int SvrCommLineState;

extern unsigned char SvrCommInterface;

extern unsigned char enable_comm_with_station_flag;

int SvrCommHaveTask(void);

#define SOFTCHG_DATAINIT	0x01
#define SOFTCHG_VERSION		0x02
void SetSoftChange(unsigned char flag);

void SvrSendFileInfo(unsigned char itf);

#endif /*_SVRCOMM_H*/

