/**
* pldynamic_dl.h -- 动态态路由载波底层通信接口
* 
* 
* 创建时间: 2010-8-25
* 最后修改时间: 2010-8-25
*/

#ifndef _PLDYNAMIC_DL_H
#define _PLDYNAMIC_DL_H

const plc_sinfo_t *GetPlDynamicInfo(void);
int PlDynamicSendPkt(const plc_dest_t *dest, const unsigned char *buf, int len, unsigned char proto);
int PlDynamicRecvPkt(const plc_dest_t *dest, unsigned char *buf, int len, int timeout);

#endif /*_PLDYNAMIC_DL_H*/

