/**
* gprs_dev.h -- GPRS模块管理
* 
* 
* 创建时间: 2010-5-19
* 最后修改时间: 2010-5-19
*/

#ifndef _GPRS_DEV_H
#define _GPRS_DEV_H

#define GPRSMODULE_MC35    0x00

#define MAXSIZE_CMDBUF    256
extern unsigned int GprsCmdBuffer[MAXSIZE_CMDBUF/4];
#define GPRS_CMDBUFFER   ((unsigned char *)GprsCmdBuffer)

#define GSTAT_SIG_UNKNOWN    99

#define GSTAT_DIAL_OFF    0
#define GSTAT_DIAL_ON    1

#define GSTAT_LINE_OFF    0
#define GSTAT_LINE_ON    1

#define GSTAT_DEV_OK    0
#define GSTAT_DEV_RESOK    1
#define GSTAT_DEV_DEVERR    2
#define GSTAT_DEV_SIMERR    3
#define GSTAT_DEV_NOSIG    4

extern unsigned char GprsDevSigState;
extern unsigned char GprsDevDailState;
extern unsigned char GprsDevLineState;
extern unsigned char GprsDevModuleState;
extern unsigned char GprsDevRingFlag;

void GprsDevInit(void);
void GprsGetName(char *name);
int GprsRestart(void);
int GprsCheck(void);
int GprsDail(void);
int GprsDailOff(void);
int GprsConnect(int sel);
void GprsDisConnect(void);
int GprsRecvSms(unsigned char *buf, char *srcphone);
int GprsSendSms(const unsigned char *msg, int msglen, const char *destphone);
void GprsRing(void);
int GprsLineState(void);
void GprsDectectLine(unsigned char uc);
int GprsGetChar(unsigned char *buf);
int GprsRawSend(const unsigned char *buf, int len);

#endif /*_GPRS_DEV_H*/

