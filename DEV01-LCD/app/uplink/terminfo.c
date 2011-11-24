/**
* terminfo.c -- 终端配置信息
* 
* 
* 创建时间: 2010-5-9
* 最后修改时间: 2010-5-9
*/

#include <string.h>

#include "include/debug.h"
#include "include/version.h"
#include "include/sys/gpio.h"
#include "include/lib/bcd.h"
#include "include/param/datause.h"
#include "include/param/unique.h"
#include "include/cenmet/info.h"

#ifdef OPEN_DEBUGPRINT
#define CHAR_DEBUGINFO		'D'
#else
#ifdef OPEN_ASSERTLOG
#define CHAR_DEBUGINFO		'T'
#else
#define CHAR_DEBUGINFO		'R'
#endif
#endif


#define VERSIONINFO_LEN		41
static const unsigned char VersionInfo[VERSIONINFO_LEN] = {
	'G', 'J', 'D', 'W',
	0, 0, 0, 0, 0, 0, 0, 0,
	'0'+VERSION_MAJOR, '0'+VERSION_MINOR/10, '0'+VERSION_MINOR%10, '0'+VERSION_PROJECT,
	0, 0, 0,
	'1', '6', 'M', '/', '6', '4', 'M', '/', CHAR_DEBUGINFO, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0,
};

#define PORTINFO_LEN		65
static const unsigned char PortInfo[PORTINFO_LEN] = {
	0, 2, 0, 0,
	2040%256, 2040/256, 0x00, 0x08, 0x00, 0x08,
	0, 0, 0, 0, 0, 0, 
	4, 
	0x41, 0x40, 0x00, 0xc2, 0x01, 0x00, 2040%256, 2040/256, 0x00, 0x08, 0x00, 0x08,
	0x02, 0x60, 0x00, 0x4b, 0x00, 0x00, 0x08, 0x00, 0x00, 0x06, 0x00, 0x06,
	0x03, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x08, 0x00, 0x00, 0x06, 0x00, 0x06,
	0x04, 0x40, 0x00, 0x4b, 0x00, 0x00, 2040%256, 2040/256, 0x00, 0x06, 0x00, 0x06,
};

#define ELSEINFO_LEN		35
static const unsigned char ElseInfo[ELSEINFO_LEN] = {
	2040%256, 2040/256, 8, 24, 8, 14, 1, 1,
	1, 1, 1, 31, 12, 0, 19, 0,
	20, 0x3e, 0x00, 0, 16, 16, 16, 16, 16, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const cfg_datafns_t ValidParamFns = {
	9,
	{
	0xfd, 0xeb, 0x10, 0x7f,
	0x7f, 0x00, 0x00, 0x0c,
	0x0f, 
	}
};

static const cfg_datafns_t ValidCtrlFns = {
	7,
	{
	0x00, 0x00, 0x00, 0x40,
	0x70, 0x00, 0x1f, 
	}
};

static const unsigned char ValidAlarmFlag[8] = {
	0x8f, 0x3f, 0xb9, 0xff,
	0x03, 0x00, 0x00, 0x00,
};

/**
* @brief 读取F1 终端版本信息
* @param buf 缓存区指针
* @param len 缓存区长度, 成功返回实际读取长度, 否则返回-1
*/
static int ReadVersionInfo(unsigned char *buf, int len)
{
	unsigned char tmpbuf[32];
	int i;

	if(len < VERSIONINFO_LEN) return -1;

	//@change later 规约版本号读入

	memcpy(buf, VersionInfo, VERSIONINFO_LEN);
	for(i=0; i<8; i++) {
		if(0 == ParaUni.manuno[i]) break;
		buf[i+4] = ParaUni.manuno[i];
	}
	buf[16] = _mk_day;
	buf[17] = _mk_month;
	buf[18] = _mk_year;
	HexToBcd(buf+16, 3);

	if(PrintCMetProtoInfo(tmpbuf) > 0) {
		buf[30] = '0'+tmpbuf[5]/10;
		buf[31] = '0'+tmpbuf[5]%10;
		buf[32] = '0'+tmpbuf[6]/10;
		buf[33] = '0'+tmpbuf[6]%10;
	}

	if(ReadDriverVersion(tmpbuf, 8) > 0) {
		buf[34] = '0'+tmpbuf[0]/10;
		buf[35] = '0'+tmpbuf[0]%10;
		buf[36] = '0'+tmpbuf[1]/10;
		buf[37] = '0'+tmpbuf[1]%10;
		buf[38] = tmpbuf[4];
		buf[39] = tmpbuf[3];
		buf[40] = tmpbuf[2];
	}

	return VERSIONINFO_LEN;
}

/**
* @brief 读取F2 终端支持的输入、输出及通信端口配置
*/
static int ReadPortInfo(unsigned char *buf, int len)
{
	if(len < PORTINFO_LEN) return -1;

	memcpy(buf, PortInfo, PORTINFO_LEN);
	memcpy(buf+10, ParaUni.addr_mac, 6);

	return PORTINFO_LEN;
}

/**
* @brief 读取F3 终端支持的其他配置
*/
static int ReadElseInfo(unsigned char *buf, int len)
{
	if(len < ELSEINFO_LEN) return -1;

	memcpy(buf, ElseInfo, ELSEINFO_LEN);

	return ELSEINFO_LEN;
}

/**
* @brief 读取F4 终端支持的参数配置
*/
static int ReadValidParamFns(unsigned char *buf, int len)
{
	int alen;

	alen = ((int)ValidParamFns.num&0xff) + 1;
	if(len < alen) return -1;

	memcpy(buf, &ValidParamFns.num, alen);

	return alen;
}

/**
* @brief 读取F5 终端支持的控制配置
*/
static int ReadValidCtrlFns(unsigned char *buf, int len)
{
	int alen;

	alen = ((int)ValidCtrlFns.num&0xff) + 1;
	if(len < alen) return -1;

	memcpy(buf, &ValidCtrlFns.num, alen);

	return alen;
}

/**
* @brief 读取F6 终端支持的1类数据配置
*/
static int ReadValidDataCls1Fns(unsigned char *buf, int len)
{
	int alen, tmplen, i;

	alen = 2;
	if(len < alen) return -1;
	*buf++ = 0x3e;
	*buf++= 0;

	for(i=0; i<2; i++) {
		tmplen = ((int)ValidDataCls1_1.num&0xff)+1;
		alen += tmplen;
		if(len < alen) return -1;
		memcpy(buf, &ValidDataCls1_1.num, tmplen);
		buf += tmplen;
	}

	for(i=0; i<2; i++) {
		tmplen = ((int)ValidDataCls1_2.num&0xff)+1;
		alen += tmplen;
		if(len < alen) return -1;
		memcpy(buf, &ValidDataCls1_2.num, tmplen);
		buf += tmplen;
	}

	tmplen = ((int)ValidDataCls1_1.num&0xff)+1;
	alen += tmplen;
	if(len < alen) return -1;
	memcpy(buf, &ValidDataCls1_1.num, tmplen);
	buf += tmplen;

	return alen;
}

/**
* @brief 读取F7 终端支持的2类数据配置
*/
static int ReadValidDataCls2Fns(unsigned char *buf, int len)
{
	int alen, tmplen, i;

	alen = 2;
	if(len < alen) return -1;
	*buf++ = 0x3e;
	*buf++ = 0;
	
	for(i=0; i<2; i++) {
		tmplen = ((int)ValidDataCls2_1.num&0xff)+1;
		alen += tmplen;
		if(len < alen) return -1;
		memcpy(buf, &ValidDataCls2_1.num, tmplen);
		buf += tmplen;
	}

	for(i=0; i<2; i++) {
		tmplen = ((int)ValidDataCls2_2.num&0xff)+1;
		alen += tmplen;
		if(len < alen) return -1;
		memcpy(buf, &ValidDataCls2_2.num, tmplen);
		buf += tmplen;
	}

	tmplen = ((int)ValidDataCls2_1.num&0xff)+1;
	alen += tmplen;
	if(len < alen) return -1;
	memcpy(buf, &ValidDataCls2_1.num, tmplen);
	buf += tmplen;

	return alen;
}

/**
* @brief 读取F7 终端支持的2类数据配置
*/
static int ReadValidAlarmFlag(unsigned char *buf, int len)
{
	if(len < 8) return -1;

	memcpy(buf, ValidAlarmFlag, 8);
	return 8;
}

typedef struct {
	unsigned char bend;
	unsigned char grpid;
	unsigned char fnmask;
	int (*pfunc)(unsigned char *, int);
} readinfo_list_t;
static const readinfo_list_t ReadInfoList[] = {
	/*F1  */{0, 0, 0x01, ReadVersionInfo},
	/*F2  */{0, 0, 0x02, ReadPortInfo},
	/*F3  */{0, 0, 0x04, ReadElseInfo},
	/*F4  */{0, 0, 0x08, ReadValidParamFns},
	/*F5  */{0, 0, 0x10, ReadValidCtrlFns},
	/*F6  */{0, 0, 0x20, ReadValidDataCls1Fns},
	/*F7  */{0, 0, 0x40, ReadValidDataCls2Fns},
	/*F8  */{0, 0, 0x80, ReadValidAlarmFlag},

	{1, 0, 0, NULL},
};

/**
* @brief 读取终端配置信息
* @param pnfn pn,fn信息指针
* @param buf 缓存区指针
* @param len 缓存区长度, 成功返回实际读取长度, 否则返回-1
*/
int ReadTermInfo(const unsigned char *pnfn, unsigned char *buf, int len)
{
	const readinfo_list_t *plist = ReadInfoList;
	unsigned char grpid, fns, echofn;
	unsigned char *pbak = buf;
	int actlen, rtn;

	if(0 != pnfn[0] || 0 != pnfn[1] || len <= 4) return -1;

	grpid = pnfn[3];
	fns = pnfn[2];

	buf[0] = buf[1] = buf[2] = 0;
	buf[3] = pnfn[3];
	buf += 4;
	actlen = 4;
	len -= 4;
	echofn = 0;

	for(; 0==plist->bend; plist++) {
		if(grpid != plist->grpid || 0 == (fns&plist->fnmask)) continue;

		rtn = (*plist->pfunc)(buf, len);
		if(rtn < 0) break;

		echofn |= plist->fnmask;
		buf += rtn;
		actlen += rtn;
		len -= rtn;
		if(len < 0) return -1;
	}

	if(0 == echofn) return -1;
	pbak[2] = echofn;
	return actlen;
}
