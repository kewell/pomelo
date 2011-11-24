/**
* term.h -- 终端参数
* 
* 
* 创建时间: 2010-5-6
* 最后修改时间: 2010-5-6
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFINE_PARATERM

#include "param_config.h"
#include "include/debug.h"
#include "include/param/term.h"
#include "include/param/unique.h"
#include "include/sys/xin.h"
#include "include/param/operation.h"
#include "operation_inner.h"
#include "uplink/ppp/ipcp.h"


static const unsigned char DefaultAlarmFlag[16] = {
	0xfb, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00,  //valid
	0xf0, 0xff, 0xf7, 0x3f, 0x00, 0x00, 0x00, 0x00,  //rank
};

para_term_t ParaTerm;

/**
* @brief 载入缺省的终端参数
*/
static void LoadDefParaTerm(void)
{
	ParaTerm.tcom.rsnd = 0x201e;
	//ParamTermBuffer.tcom.rsnd = 0x200a;
	ParaTerm.tcom.cycka = 15;
	ParaTerm.tcom.flagcon = 0x01;

	ParaTerm.svrip.ipmain[0] = 192;
	ParaTerm.svrip.ipmain[1] = 168;
	ParaTerm.svrip.ipmain[2] = 8;
	ParaTerm.svrip.ipmain[3] = 91;
	ParaTerm.svrip.portmain = 8822;

	strcpy((char *)ParaTerm.svrip.apn, "CMNET");

	ParaTerm.pwd.pwd[0] = ParaTerm.pwd.pwd[1] = 0x00;

	memcpy(ParaTerm.almflag.valid, DefaultAlarmFlag, 16);

	ParaTerm.termip.ipterm[0] = 192;
	ParaTerm.termip.ipterm[1] = 168;
	ParaTerm.termip.ipterm[2] = 8;
	ParaTerm.termip.ipterm[3] = 203;
	ParaTerm.termip.maskterm[0] = 255;
	ParaTerm.termip.maskterm[1] = 255;
	ParaTerm.termip.maskterm[2] = 255;
	ParaTerm.termip.maskterm[3] = 0;
	ParaTerm.termip.ipgatew[0] = 192;
	ParaTerm.termip.ipgatew[1] = 168;
	ParaTerm.termip.ipgatew[2] = 8;
	ParaTerm.termip.ipgatew[3] = 1;
	ParaTerm.termip.portlisten = 8001;

	ParaTerm.uplink.countdail = 4;
	ParaTerm.uplink.timedail = 60;
	ParaTerm.uplink.timedown = 10;
}

static void StringToIp(const char *str, unsigned char *ip)
{
	int idx, i;

	ip[0] = ip[1] = ip[2] = ip[3] = 0;
	i = idx = 0;
	for(;0 != *str; str++) {
		if('.' == *str) {
			ip[idx] = i;
			i = 0;
			idx++;
			if(idx > 3) return;
		}
		else if(*str >= '0' && *str <= '9') {
			i *= 10;
			i += *str - '0';
		}
		else {
			ip[idx] = i;
			return;
		}
	}

	ip[idx] = i;
}

static inline void IpToString(const unsigned char *ip, char *str)
{
	sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

/**
* @brief 从文件中载入终端参数
* @return 0成功, 否则失败
*/
int LoadParaTerm(void)
{
	XINREF pf;
	int i;
	char str[24];

	DebugPrint(0, "  load param term...");

	LoadDefParaTerm();

	pf = XinOpen(PARAM_SAVE_PATH "term.xin", 'r');
	if(NULL == pf) {
		pf = XinOpen(PARAM_BAK_PATH "term.xin", 'r');
		if(NULL == pf) {
			DebugPrint(0, "no file\n");
			return 1;
		}
	}

	ParaTerm.tcom.rts = XinReadInt(pf, "rts", 0);
	ParaTerm.tcom.delay = XinReadInt(pf, "delay", 0);
	i = XinReadInt(pf, "timeout", 30);
	ParaTerm.tcom.rsnd = i & 0xfff;
	i = XinReadInt(pf, "retry", 2);
	ParaTerm.tcom.rsnd |= (i<<12)&0xf000;
	ParaTerm.tcom.flagcon = XinReadInt(pf, "flagcon", 1);
	ParaTerm.tcom.cycka = XinReadInt(pf, "cycka", 15);

	if(XinReadString(pf, "ipmain", str, 24) > 0) {
		StringToIp(str, ParaTerm.svrip.ipmain);
	}
	ParaTerm.svrip.portmain = XinReadInt(pf, "portmain", 7001);

	if(XinReadString(pf, "ipbakup", str, 24) > 0) {
		StringToIp(str, ParaTerm.svrip.ipbakup);
	}
	ParaTerm.svrip.portbakup = XinReadInt(pf, "portbakup", 0);
	XinReadString(pf, "apn", ParaTerm.svrip.apn, 20);

	XinReadHex(pf, "svrphone", ParaTerm.smsc.phone, 8);
	XinReadHex(pf, "smscentre", ParaTerm.smsc.sms, 8);

	ParaTerm.pwd.art = XinReadInt(pf, "checkart", 0);
	XinReadHex(pf, "checkpwd", ParaTerm.pwd.pwd, 2);

	XinReadHex(pf, "grpaddr", ParaTerm.grpaddr.addr, 16);

	if(XinReadString(pf, "ipterm", str, 24) > 0) {
		StringToIp(str, ParaTerm.termip.ipterm);
	}
	if(XinReadString(pf, "maskterm", str, 24) > 0) {
		StringToIp(str, ParaTerm.termip.maskterm);
	}
	if(XinReadString(pf, "ipgateway", str, 24) > 0) {
		StringToIp(str, ParaTerm.termip.ipgatew);
	}
	if(XinReadString(pf, "ipproxy", str, 24) > 0) {
		StringToIp(str, ParaTerm.termip.ipproxy);
	}
	ParaTerm.termip.portproxy = XinReadInt(pf, "portproxy", 0);
	ParaTerm.termip.proxy_type = XinReadInt(pf, "proxy_type", 0);
	ParaTerm.termip.proxy_connect = XinReadInt(pf, "proxy_connect", 0);
	XinReadString(pf, "proxyuser", ParaTerm.termip.username, 32);
	XinReadString(pf, "proxypwd", ParaTerm.termip.pwd, 32);
	ParaTerm.termip.portlisten = XinReadInt(pf, "portlisten", 8001);

	ParaTerm.uplink.proto = XinReadInt(pf, "protocol", 0);
	ParaTerm.uplink.mode = XinReadInt(pf, "mode", 0);
	ParaTerm.uplink.clientmode = XinReadInt(pf, "clientmode", 0);
	ParaTerm.uplink.countdail = XinReadInt(pf, "countdail", 4);
	ParaTerm.uplink.timedail = XinReadInt(pf, "timedail", 30);
	ParaTerm.uplink.timedown = XinReadInt(pf, "timedown", 10);
	XinReadHex(pf, "onlineflag", (unsigned char *)&ParaTerm.uplink.onlineflag, 3);

	XinReadHex(pf, "alarmvalid", ParaTerm.almflag.valid, 8);
	XinReadHex(pf, "alarmrank", ParaTerm.almflag.rank, 8);

	XinReadString(pf, "vpnuser", ParaTerm.vpn.user, 32);
	XinReadString(pf, "vpnpwd", ParaTerm.vpn.pwd, 32);

	DebugPrint(0, "ok\n");
	XinClose(pf);

	return 0;
}

/**
* @brief 根据参数设置网络地址
*/
void SetParaNetAddr(void)
{
	char str[96];

	system("ifconfig eth0 down");

	sprintf(str, "ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d",
			ParaTerm.termip.ipterm[0], ParaTerm.termip.ipterm[1],
			ParaTerm.termip.ipterm[2], ParaTerm.termip.ipterm[3],
			ParaTerm.termip.maskterm[0], ParaTerm.termip.maskterm[1], 
			ParaTerm.termip.maskterm[2], ParaTerm.termip.maskterm[3]);
	DebugPrint(0, "  %s\n", str);
	system(str);

	sprintf(str, "ifconfig eth0 hw ether %02X:%02X:%02X:%02X:%02X:%02X",
			ParaUni.addr_mac[0], ParaUni.addr_mac[1], ParaUni.addr_mac[2],
			ParaUni.addr_mac[3], ParaUni.addr_mac[4], ParaUni.addr_mac[5]);
	DebugPrint(0, "  %s\n", str);
	system(str);

	system("ifconfig eth0 up");
		
}

/**
* @brief 保存终端参数
* @return 0成功, 否则失败
*/
int SaveParaTerm(void)
{
	XINREF pf;
	int i, bakup = 0;
	char str[24];

	pf = XinOpen(PARAM_SAVE_PATH "term.xin", 'w');
	if(NULL == pf) goto mark_bakup;

mark_save:
	XinWriteInt(pf, "rts", ParaTerm.tcom.rts, 0);
	XinWriteInt(pf, "delay", ParaTerm.tcom.delay, 0);
	i = ParaTerm.tcom.rsnd & 0xfff;
	XinWriteInt(pf, "timeout", i, 0);
	i = ParaTerm.tcom.rsnd >> 12;
	XinWriteInt(pf, "retry", i, 0);
	XinWriteInt(pf, "flagcon", ParaTerm.tcom.flagcon, 1);
	XinWriteInt(pf, "cycka", ParaTerm.tcom.cycka, 0);

	IpToString(ParaTerm.svrip.ipmain, str);
	XinWriteString(pf, "ipmain", str);
	XinWriteInt(pf, "portmain", ParaTerm.svrip.portmain, 0);
	IpToString(ParaTerm.svrip.ipbakup, str);
	XinWriteString(pf, "ipbakup", str);
	XinWriteInt(pf, "portbakup", ParaTerm.svrip.portbakup, 0);
	XinWriteString(pf, "apn", ParaTerm.svrip.apn);

	XinWriteHex(pf, "svrphone", ParaTerm.smsc.phone, 8);
	XinWriteHex(pf, "smscentre", ParaTerm.smsc.sms, 8);

	XinWriteInt(pf, "checkart", ParaTerm.pwd.art, 0);
	XinWriteHex(pf, "checkpwd", ParaTerm.pwd.pwd, 2);

	XinWriteHex(pf, "grpaddr", ParaTerm.grpaddr.addr, 16);

	IpToString(ParaTerm.termip.ipterm, str);
	XinWriteString(pf, "ipterm", str);
	IpToString(ParaTerm.termip.maskterm, str);
	XinWriteString(pf, "maskterm", str);
	IpToString(ParaTerm.termip.ipgatew, str);
	XinWriteString(pf, "ipgateway", str);
	IpToString(ParaTerm.termip.ipproxy, str);
	XinWriteString(pf, "ipproxy", str);
	XinWriteInt(pf, "portproxy", ParaTerm.termip.portproxy, 0);
	XinWriteInt(pf, "proxy_type", ParaTerm.termip.proxy_type, 0);
	XinWriteInt(pf, "proxy_connect", ParaTerm.termip.proxy_connect, 0);
	XinWriteString(pf, "proxyuser", ParaTerm.termip.username);
	XinWriteString(pf, "proxypwd", ParaTerm.termip.pwd);
	XinWriteInt(pf, "portlisten", ParaTerm.termip.portlisten, 0);

	XinWriteInt(pf, "protocol", ParaTerm.uplink.proto, 0);
	XinWriteInt(pf, "mode", ParaTerm.uplink.mode, 0);
	XinWriteInt(pf, "clientmode", ParaTerm.uplink.clientmode, 0);
	XinWriteInt(pf, "countdail", ParaTerm.uplink.countdail, 0);
	XinWriteInt(pf, "timedail", ParaTerm.uplink.timedail, 0);
	XinWriteInt(pf, "timedown", ParaTerm.uplink.timedown, 0);
	XinWriteHex(pf, "onlineflag", (unsigned char *)&ParaTerm.uplink.onlineflag, 3);

	XinWriteHex(pf, "alarmvalid", ParaTerm.almflag.valid, 8);
	XinWriteHex(pf, "alarmrank", ParaTerm.almflag.rank, 8);

	XinWriteString(pf, "vpnuser", ParaTerm.vpn.user);
	XinWriteString(pf, "vpnpwd", ParaTerm.vpn.pwd);

	XinClose(pf);

mark_bakup:
	if(bakup) return 0;
	bakup = 1;
	pf = XinOpen(PARAM_BAK_PATH "term.xin", 'w');
	if(NULL == pf) return 1;
	goto mark_save;
}

/**
* @brief 终端参数F1操作
* @param flag 操作方式, 0-读, 1-写
* @param metpid 测量点号
* @param buf 缓存区指针
* @param len 缓存区长度
* @param actlen 有效数据长度(由函数返回)
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
* @note 以下同类参数和返回值相同, 不做重复注释
*/
int ParaOperationF1(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) smallcpy(buf, &ParaTerm.tcom.rts, 6);
	else smallcpy(&ParaTerm.tcom.rts, buf, 6);

	return 0;
}

/**
* @brief 终端参数F2操作(空操作)
*/
int ParaOperationF2(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) {
		buf[0] = 0;
		*actlen = 1;
	}
	else {
		int i;

		i = (int)buf[0] & 0xff;
		*actlen = i*2;
		if(*actlen > len) return POERR_FATAL;
	}

	return 0;
}

/**
* @brief 终端参数F3操作
*/
int ParaOperationF3(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	printf("ParaOperationF3..............................\n");
	if(0 == flag) smallcpy(buf, ParaTerm.svrip.ipmain, 28);
	else {
		smallcpy(ParaTerm.svrip.ipmain, buf, 28);
		ParaTerm.svrip.apn[16] = 0;
	}

	return 0;
}

/**
* @brief 终端参数F4操作
*/
int ParaOperationF4(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) smallcpy(buf, ParaTerm.smsc.phone, 16);
	else smallcpy(ParaTerm.smsc.phone, buf, 16);

	return 0;
}

/**
* @brief 终端参数F5操作
*/
int ParaOperationF5(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) smallcpy(buf, &ParaTerm.pwd.art, 3);
	else smallcpy(&ParaTerm.pwd.art, buf, 3);

	return 0;
}

/**
* @brief 终端参数F6操作
*/
int ParaOperationF6(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) smallcpy(buf, ParaTerm.grpaddr.addr, 16);
	else smallcpy(ParaTerm.grpaddr.addr, buf, 16);

	return 0;
}

/**
* @brief 终端参数F7操作
*/
int ParaOperationF7(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	int alen;
	int lenusr, lenpwd;
	unsigned char *pbuf;
	unsigned long term_gprs_ipaddr = 0;
	unsigned char term_ipaddr[4];

	if(0 == flag) {
		lenusr = strlen(ParaTerm.termip.username);
		if(lenusr > 0) lenusr += 1;
		lenpwd = strlen(ParaTerm.termip.pwd);
		if(lenpwd > 0) lenpwd += 1;
		alen = 24 + lenusr + lenpwd;
		if(alen > len) return POERR_FATAL;
		*actlen = alen;

		pbuf = buf;

		term_gprs_ipaddr = ipcp_get_addr();
		
		term_ipaddr[0] = (unsigned char)(term_gprs_ipaddr>>24)&0xff;
		term_ipaddr[1] = (unsigned char)(term_gprs_ipaddr>>16)&0xff;
		term_ipaddr[2] = (unsigned char)(term_gprs_ipaddr>>8)&0xff;
		term_ipaddr[3] = (unsigned char)term_gprs_ipaddr&0xff;
		
		smallcpy(pbuf, term_ipaddr, 4); pbuf += 4;
		smallcpy(pbuf, ParaTerm.termip.maskterm, 8); pbuf += 8;
		//smallcpy(pbuf, ParaTerm.termip.ipterm, 12); pbuf += 12;
		*pbuf++ = ParaTerm.termip.proxy_type;
		smallcpy(pbuf, ParaTerm.termip.ipproxy, 4); pbuf += 4;
		*pbuf++ = (unsigned char)ParaTerm.termip.portproxy;
		*pbuf++ = (unsigned char)(ParaTerm.termip.portproxy >> 8);
		*pbuf++ = ParaTerm.termip.proxy_connect;
		*pbuf++ = lenusr;
		if(lenusr) strcpy((char *)pbuf, ParaTerm.termip.username);
		pbuf += lenusr;
		*pbuf++ = lenpwd;
		if(lenpwd) strcpy((char *)pbuf, ParaTerm.termip.pwd);
		pbuf += lenpwd;
		pbuf[0] = ParaTerm.termip.portlisten;
		pbuf[1] = ParaTerm.termip.portlisten >> 8;
	}
	else {
		lenusr = (int)buf[20]&0xff;
		pbuf = buf;
		pbuf += 21 + lenusr;
		lenpwd = (int)(*pbuf)&0xff;
		alen = 24 + lenusr + lenpwd;
		*actlen = alen;
		if(alen > len) return POERR_FATAL;

		if(lenusr > 32 || lenpwd > 32) return POERR_INVALID;

		pbuf = buf;
		smallcpy(ParaTerm.termip.ipterm, pbuf, 12); pbuf += 12;
		ParaTerm.termip.proxy_type = *pbuf++;
		smallcpy(ParaTerm.termip.ipproxy, pbuf, 4); pbuf += 4;
		ParaTerm.termip.portproxy = ((unsigned short)pbuf[1]<<8) + (unsigned short)pbuf[0];
		pbuf += 2;
		ParaTerm.termip.proxy_connect = *pbuf++;
		if(lenusr) smallcpy(ParaTerm.termip.username, pbuf, lenusr);
		if(lenusr < 32) ParaTerm.termip.username[lenusr] = 0;
		else ParaTerm.termip.username[31] = 0;
		pbuf += lenusr+1;
		if(lenpwd) smallcpy(ParaTerm.termip.pwd, pbuf, lenpwd);
		if(lenpwd < 32) ParaTerm.termip.pwd[lenpwd] = 0;
		else ParaTerm.termip.pwd[31] = 0;
		pbuf += lenpwd+1;
		ParaTerm.termip.portlisten = ((unsigned short)pbuf[1]<<8) + (unsigned short)pbuf[0];
	}

	return 0;
}

/**
* @brief 终端参数F8操作
*/
int ParaOperationF8(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned char *pbuf = buf;

	if(0 == flag) {
		pbuf[0] = (ParaTerm.uplink.proto) ? 0x80 : 0;
		pbuf[0] |= (ParaTerm.uplink.mode&0x03) << 4;
		pbuf[0] |= (ParaTerm.uplink.clientmode+1) & 0x03;
		pbuf[1] = ParaTerm.uplink.timedail;
		pbuf[2] = ParaTerm.uplink.timedail>>8;
		pbuf[3] = ParaTerm.uplink.countdail;
		pbuf[4] = ParaTerm.uplink.timedown;
		pbuf[5] = ParaTerm.uplink.onlineflag & 0xff;
		pbuf[6] = (ParaTerm.uplink.onlineflag >> 8) & 0xff;
		pbuf[7] = (ParaTerm.uplink.onlineflag >> 16) & 0xff;
	}
	else {
		ParaTerm.uplink.proto = (pbuf[0]&0x80) ? 1 : 0;
		ParaTerm.uplink.mode = (pbuf[0]>>4)&0x03;
		ParaTerm.uplink.clientmode = pbuf[0] & 0x03;
		if(ParaTerm.uplink.clientmode) ParaTerm.uplink.clientmode -= 1;
		ParaTerm.uplink.timedail = ((unsigned short)pbuf[2]<<8) +(unsigned short)pbuf[1];
		ParaTerm.uplink.countdail = pbuf[3];
		ParaTerm.uplink.timedown = pbuf[4];
		ParaTerm.uplink.onlineflag = ((unsigned long)pbuf[7]<<16) + ((unsigned long)pbuf[6]<<8) + pbuf[5];
	}

	return 0;
}

/**
* @brief 终端参数F9操作
*/
int ParaOperationF9(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) memcpy(buf, ParaTerm.almflag.valid, 16);
	else memcpy(ParaTerm.almflag.valid, buf, 16);

	return 0;
}

/**
* @brief 终端参数F16操作
*/
int ParaOperationF16(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) memcpy(buf, ParaTerm.vpn.user, 64);
	else memcpy(ParaTerm.vpn.user, buf, 64);

	return 0;
}

/*void ParamTermTest(void)
{
	printf("start param term test..\n");
	SaveParaTerm();
	printf("end\n");
}*/

