/**
* gprs_dev.c -- GPRS模块管理
* 
* 
* 创建时间: 2010-5-19
* 最后修改时间: 2010-5-19
*/

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "include/debug.h"
#include "include/lib/align.h"
#include "include/param/term.h"
#include "gprs_dev.h"
#include "uplink/uplink_pkt.h"
#include "uplink/svrcomm.h"
#include "gprs_hardware.h"
#include "gprs_dev.h"
#include "gprs_at.h"
#include "mc37.h"

unsigned int GprsCmdBuffer[MAXSIZE_CMDBUF/4];

unsigned char GprsDevSigState;
unsigned char GprsDevDailState;//1-拨号成功
unsigned char GprsDevLineState;//1-已打开socket
unsigned char GprsDevModuleState = GSTAT_DEV_DEVERR;
unsigned char GprsDevRingFlag;

struct gprsdev_t {
	unsigned short devno;
	unsigned short attr;   // 0=GPRS, 1-CDMA

	int (*dev_init)(void);
	int (*dev_restart)(const char *apn, const char *sms, const char *usrname, const char *pwd);
	int (*dev_check)(void);

	int (*dev_getchar)(unsigned char *buf);
	int (*dev_rawsend)(const unsigned char *buf, int len);

	int (*dev_dial)(void);
	int (*dev_dialoff)(void);
	int (*dev_connect)(unsigned long ip, unsigned short port, unsigned char proto);
	void (*dev_disconnect)(void);

	void (*dev_ring)(void);
	int (*dev_linestat)(void);
	void (*dev_detectline)(unsigned char uc);

	int (*dev_recvsms)(unsigned char *msgbuf, char *dstno);
	int (*dev_sendsms)(const unsigned char *msg, int msglen, const char *smscentre, const char *destphone);

	const char *name;
};

static const struct gprsdev_t const_gprsdev[] = {
	{0, 0, M590Init, M590Restart, M590CheckDev, 
	M590GetChar, M590RawSend, 
	M590Dail, M590DialOff, M590Connect, M590Disconnect,
	M590Ring, M590LineState, M590DetectLine,
	NULL, NULL, "GPRS-M590"}, 

	{0xff, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "empty"}, 
};

static const struct gprsdev_t *gprs_devp = &const_gprsdev[0];

#if 0
static void GprsDevInitGpio(void)
{
	GpioSetDirect(GPIO_POWER_RMODULE, 1);
	GpioSetDirect(GPIO_IGT, 1);
	GpioSetDirect(GPIO_GPRS_DCD, 0);
	GpioSetDirect(GPIO_GPRS_DTR, 1);
	GpioSetValue(GPIO_GPRS_DTR, 0);
	GpioSetDirect(GPIO_GPRS_CTS, 0);
	GpioSetDirect(GPIO_GPRS_RI, 0);
	GpioSetDirect(GPIO_GPRS_DSR, 0);
	GpioSetDirect(GPIO_GPRS_RTS, 1);
	GpioSetValue(GPIO_GPRS_RTS, 0);
	gprsline_pwroff;
}
#endif

void GprsDevInit(void)
{
	unsigned char dev;

	//GprsDevInitGpio();

	//svr_lineled(0);

	dev = 0;

	if(dev > 2) gprs_devp = &const_gprsdev[0];
	else gprs_devp = &const_gprsdev[dev];
	PrintLog(0, "Module is %s\n", gprs_devp->name);

	(*gprs_devp->dev_init)();
}

void GprsGetName(char *name)
{
	strcpy(name, gprs_devp->name);
}

int GprsRestart(void)
{
	static char apn[18], sms[32], usrname[36], pwd[36];

	int rtn;

	//svr_lineled(0);
	SvrCommLineState = LINESTAT_OFF;

	PrintLog(0, "GprsRestart.................................................\n");
	memcpy((unsigned char *)apn, (unsigned char *)ParaTerm.svrip.apn, 16); apn[16] = 0;
	usrname[0] = 0;
	pwd[0] = 0;
	sms[0] = 0;
	//memcpy((unsigned char *)usrname, (unsigned char *)para_term.cdma_usr, 32); usrname[32] = 0;
	//memcpy((unsigned char *)pwd, (unsigned char *)para_term.cdma_pwd, 32); pwd[32] = 0;
	//sms_make_phoneno(para_term.smsc.sms, 8, sms);

	rtn = (*gprs_devp->dev_restart)(apn, sms, usrname, pwd);

	return rtn;
}

int GprsCheck(void)
{
	return((*gprs_devp->dev_check)());
}

int GprsDail(void)
{
	GprsDevDailState = GSTAT_DIAL_OFF;
	return((*gprs_devp->dev_dial)());
}

int GprsDailOff(void)
{
	GprsDevDailState = GSTAT_DIAL_OFF;
	return((*gprs_devp->dev_dialoff)());
}

int GprsConnect(int sel)
{
	unsigned long ip;
	unsigned short port;

	printf("GprsConnect.............");
	if(0 == sel) {
		ip = MAKE_LONG(ParaTerm.svrip.ipmain);
		port = ParaTerm.svrip.portmain;
	}
	else {
		ip = MAKE_LONG(ParaTerm.svrip.ipbakup);
		port = ParaTerm.svrip.portbakup;
		if(0 == ip || 0 == port) {
			ip = MAKE_LONG(ParaTerm.svrip.ipmain);
			port = ParaTerm.svrip.portmain;
		}
	}

	ip = htonl(ip);
	return((*gprs_devp->dev_connect)(ip, port, ParaTerm.uplink.proto));
}

void GprsDisConnect(void)
{
	(*gprs_devp->dev_disconnect)();
	GprsDevLineState = GSTAT_LINE_OFF;
	//svr_lineled(0);
}


//return -1=no msg, 0-invalid msg, >0 valid msg
int GprsRecvSms(unsigned char *buf, char *srcphone)
{
	//@change later
	return 1;
	//return((*gprs_devp->dev_recvsms)(buf, srcphone));
}

int GprsSendSms(const unsigned char *msg, int msglen, const char *destphone)
{
	return 1;
	/*static char smscentre[32];

	sms_make_phoneno(para_term.smsc.sms, 8, smscentre);

	return((*gprs_devp->dev_sendsms)(msg, msglen, smscentre, destphone));*/
}

void GprsRing(void)
{
	(*gprs_devp->dev_ring)();
}

int GprsLineState(void)
{
	int rtn;

	//if(!para_uni.gprs.dcd) return 1;

	rtn = (*gprs_devp->dev_linestat)();
	//printf("GprsLineState...\n");
	if(0 == rtn) {
		//svr_lineled(0);
		printf("SvrCommLineState = LINESTAT_OFF\n");
		SvrCommLineState = LINESTAT_OFF;
	}
	return rtn;
}

void GprsDectectLine(unsigned char uc)
{
	(*gprs_devp->dev_detectline)(uc);
}

int GprsGetChar(unsigned char *buf)
{
	return((*gprs_devp->dev_getchar)(buf));
}

int GprsRawSend(const unsigned char *buf, int len)
{
	return((*gprs_devp->dev_rawsend)(buf, len));
}

unsigned char GprsSigQuality(void)
{
	return GprsDevSigState;
}

/*void gprsdev_boxdetect(void)
{
	if(gprsline_boxopen) {
		gprsline_pwroff;
	}
}*/

