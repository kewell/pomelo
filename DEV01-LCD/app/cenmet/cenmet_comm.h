/**
* cenmet_comm.h -- 表计通信
* 
* 
* 创建时间: 2010-5-17
* 最后修改时间: 2010-5-17
*/

#ifndef _CENMET_COMM_H
#define _CENMET_COMM_H


//int CenMetRead(unsigned short metid, unsigned short itemid, unsigned char *buf, int *plen);
int CenMetRead(unsigned short metid, unsigned int itemid, unsigned char *buf, int *plen);
int CenMetReadTime(unsigned short metid, sysclock_t *pclk);

void CenMetProc(void);

void ReadImmMdbCur(unsigned short mid, const unsigned short *pitems, unsigned short num);

int CMetCommInit(void);
int check_cen_meter_rcv_pak(unsigned char *buf,int len);
int RecvPkt_485(unsigned char *buf,int timeout,unsigned char port_num);
void CenMetLock(unsigned char port);
void CenMetUnlock(unsigned char port);

#endif /*_CENMET_COMM_H*/

