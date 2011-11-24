/**
* foward.h -- 表计透明转发
* 
* 
* 创建时间: 2010-5-17
* 最后修改时间: 2010-5-17
*/

#ifndef _CENMET_FORWARD_H
#define _CENMET_FORWARD_H

typedef struct {
	unsigned char port;
	unsigned char flag;
	unsigned char timeout1;
	unsigned char timeout2;
	unsigned char len[2];
	unsigned char data[2];
} metfrwd_t;

typedef struct {
	unsigned char port;
	unsigned char len[2];
	unsigned char data[1];
} metfrwd_echo_t;

void CenMetForward(const metfrwd_t *pcmd, metfrwd_echo_t *pecho);


#endif /*_CENMET_FORWARD_H*/

