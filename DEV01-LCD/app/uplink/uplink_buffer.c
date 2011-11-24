/**
* uplink_buffer.c -- 上行通信缓存区定义
* 
* 
* 创建时间: 2010-5-18
* 最后修改时间: 2010-5-18
*/

#include <stdio.h>

#include "include/debug.h"
#include "uplink_pkt.h"
#include "uplink_dl.h"

static int empty_rawsend(const unsigned char *buf, int len)
{
	return 0;
}

static int empty_getchar(unsigned char *buf)
{
	return 0;
}

static int linestat_empty(void)
{
	return 1;
}

extern int SerialGetChar(unsigned char *buf);
extern int SerialRawSend(const unsigned char *buf, int len);

extern int IrGetChar(unsigned char *buf);
extern int IrRawSend(const unsigned char *buf, int len);

extern int EthSvrGetChar(unsigned char *buf);
extern int EthSvrRawSend(const unsigned char *buf, int len);

extern int EtherGetChar(unsigned char *buf);
extern int EtherRawSend(const unsigned char *buf, int len);

extern int GprsGetChar(unsigned char * buf);
extern int GprsRawSend(const unsigned char * buf,int len);

extern int CascadeGetChar(unsigned char *buf);
int CascadeRawSend(const unsigned char *buf, int len);

extern int GprsLineState(void);

#define MAXLEN_SERIALITF_BUF	1152
#define MAXLEN_ETHSVRITF_BUF	1152
#define MAXLEN_GPRSITF_BUF		1152
#define MAXLEN_SMSITF_BUF		1152
#define MAXLEN_IRITF_BUF		1152
#define MAXLEN_CASCADE_BUF		1152

static unsigned int g_gprsitf_rcvbuf[MAXLEN_GPRSITF_BUF/4];
static unsigned int g_gprsitf_sndbuf[MAXLEN_GPRSITF_BUF/4];

//static unsigned int g_gprssrvitf_rcvbuf[MAXLEN_GPRSITF_BUF/4];
//static unsigned int g_gprssrvitf_sndbuf[MAXLEN_GPRSITF_BUF/4];

static unsigned int g_serialitf_rcvbuf[MAXLEN_SERIALITF_BUF/4];
static unsigned int g_serialitf_sndbuf[MAXLEN_SERIALITF_BUF/4];
static unsigned int g_ethsvritf_rcvbuf[MAXLEN_ETHSVRITF_BUF/4];
static unsigned int g_ethsvritf_sndbuf[MAXLEN_ETHSVRITF_BUF/4];
static unsigned int g_smsitf_rcvbuf[MAXLEN_SMSITF_BUF/4];
static unsigned int g_smsitf_sndbuf[MAXLEN_SMSITF_BUF/4];
static unsigned int g_iritf_rcvbuf[MAXLEN_IRITF_BUF/4];
static unsigned int g_iritf_sndbuf[MAXLEN_IRITF_BUF/4];
static unsigned int g_casditf_rcvbuf[MAXLEN_CASCADE_BUF/4];
static unsigned int g_casditf_sndbuf[MAXLEN_CASCADE_BUF/4];


const uplinkitf_t UplinkInterface[UPLINKITF_NUM] = {

	{(unsigned char *)g_serialitf_rcvbuf, (unsigned char *)g_serialitf_sndbuf, 
		SerialRawSend, SerialGetChar, linestat_empty, 5, 
		1024, 1024, 512, 0},

	{(unsigned char *)g_ethsvritf_rcvbuf, (unsigned char *)g_ethsvritf_sndbuf, 
		EthSvrRawSend, EthSvrGetChar, linestat_empty, 5, 
		1024, 1024, 1024, 0},

	{(unsigned char *)g_gprsitf_rcvbuf, (unsigned char *)g_gprsitf_sndbuf, 
		GprsRawSend, GprsGetChar, GprsLineState, 50, 
		1024, 1024, 512, 0},

	{(unsigned char *)g_gprsitf_rcvbuf, (unsigned char *)g_gprsitf_sndbuf, 
		EtherRawSend, EtherGetChar, linestat_empty, 6, 
		1024, 1024, 1024, 0},

	{(unsigned char *)g_smsitf_rcvbuf, (unsigned char *)g_smsitf_sndbuf, 
		//smsitf_rawsend, smsitf_getchar, linestat_empty, 50, 
		empty_rawsend, empty_getchar, linestat_empty, 5, 
		127, 127, 127, UPLINKATTR_NOECHO},

	{(unsigned char *)g_iritf_rcvbuf, (unsigned char *)g_iritf_sndbuf, 
		IrRawSend, IrGetChar, linestat_empty, 20, 
		1024, 1024, 512, 0},

	{(unsigned char *)g_casditf_rcvbuf, (unsigned char *)g_casditf_sndbuf, 
		CascadeRawSend, CascadeGetChar, linestat_empty, 10, 
		1024, 1024, 512, 0},

	//{(unsigned char *)g_gprssrvitf_rcvbuf, (unsigned char *)g_gprssrvitf_sndbuf, 
	//GprsSrvRawSend, GprsSrvGetChar, GprsLineState, 50, 
	//1024, 1024, 512, 0},	
};

