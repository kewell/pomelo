/**
* plstatic_dl.h -- 静态路由载波底层通信接口
* 
* 
* 创建时间: 2010-5-21
* 最后修改时间: 2010-5-21
*/

#ifndef _PLSTATIC_DL_H
#define _PLSTATIC_DL_H

const plc_sinfo_t *GetPlStaticInfo(void);
int PlStaticSendPkt(const plc_dest_t *dest, const unsigned char *buf, int len, unsigned char proto);
int PlStaticRecvPkt(const plc_dest_t *dest, unsigned char *buf, int len, int timeout);
int PlcRecvPkt(unsigned char *buf,int timeout);

#endif /*_PLSTATIC_DL_H*/

