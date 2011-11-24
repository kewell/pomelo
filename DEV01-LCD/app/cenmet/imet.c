/**
* imet.c -- 内部交流采样读取
* 
* 
* 创建时间: 2010-5-16
* 最后修改时间: 2010-6-11
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/uart.h"
#include "include/sys/mutex.h"
#include "include/sys/event.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/task.h"
#include "include/debug/shellcmd.h"
#include "include/lib/bcd.h"
#include "include/lib/align.h"
#include "include/param/capconf.h"
#include "mdb/mdbconf.h"
#include "mdb/mdbcur.h"
#include "include/param/meter.h"
#include "include/param/metp.h"
#include "include/monitor/runstate.h"
#include "imet_save.h"
#include "cenmet_alm.h"

//#define VIRTUAL_IMET		1

#define IMET_UART_PORT		6
#define IMET_ADDR			1

#define IMET_VOL			0x30
#define IMET_AMP			0x31
#define IMET_PWR			0x32
#define IMET_ENE			0x33
#define IMET_CLRENE			0x34
#define IMET_STAT			0x35
#define IMET_AMPANGLE		0x3a
#define IMET_VOLANGLE		0x3b
#define IMET_VOLAMPANGLE	0x3c
#define IMET_RDCRC			0x3f
#define IMET_RDPLS			0x37
#define IMET_RDPARA		0x3e
#define IMET_SETPARA		0x38 
#define IMET_CTDATA		0x43
#define IMET_POSPENE		0x55
#define IMET_NEGPENE		0x56
#define IMET_NEGQI1ENE		0x57
#define IMET_NEGQI2ENE		0x58
#define IMET_NEGQI3ENE		0x59
#define IMET_NEGQI4ENE		0x5a //四象限

#define IMET_CLRPOSPENE		0x5b//清零有功
#define IMET_CLRNEGQENE		0x5c//清零无功
#define IMET_RESETTIMES		0x5f

static sys_mutex_t IMetMutex;
#define IMET_LOCK		SysLockMutex(&IMetMutex)
#define IMET_UNLOCK		SysUnlockMutex(&IMetMutex)

unsigned short IMetMetpid = 0;

#ifndef VIRTUAL_IMET

static unsigned char IMetCommCache[256];




int recieve_read_imet_pak(unsigned char *buf) 
{
	int timeout;  // 超时次数变量；
	char c;
	unsigned char *pdata;
	int len = 0;
	
	pdata = buf;
	timeout = 0;
	for(timeout=0;timeout<3;timeout++)
	{
		while(UartRecv(IMET_UART_PORT,&c, 1))
		{
			*pdata++ = c;
			len++;
			if(len>=256)
				break;
		}
		Sleep(10);
	}
	return len;
}

int check_recieve_read_imet_pak(unsigned char *buf,unsigned char len) 
{
	//unsigned char *pbuf = buf;
	
	if(buf[0] != 0x68)	return 1;
	if(buf[7] != 0x68)	return 1;

	return 0;
}


const unsigned char imet_addr[6] = {0x99,0x99,0x99,0x99,0x99,0x99};
const unsigned char itemid_vol[4] = {0x33,0x32,0x34,0x35};
const unsigned char itemid_amp[4] = {0x33,0x32,0x35,0x35};
const unsigned char itemid_pwr[4] = {0x33,0x32,0x36,0x35};

static int IMetComm(unsigned char cmd, unsigned char *sbuf, int slen, unsigned char *rbuf, int rlen)
{
	unsigned char *ps = IMetCommCache;
	unsigned char	itemid[4];
	unsigned int check_sum = 0;
	unsigned char i = 0;
	unsigned char *pbuf;
	unsigned char rcv_buf[256];
	int rcv_len = 0;
	int data_len = 0;

	//PrintLog(0, "IMetCommRead.........\n");
	memset(IMetCommCache,0x00,sizeof(IMetCommCache));
	memset(rcv_buf,0x00,sizeof(rcv_buf));
	memset(itemid,0x00,sizeof(itemid));
	switch(cmd)
	{
		case IMET_VOL:
			memcpy(itemid,itemid_vol,4);
			break;
		case IMET_AMP:
			memcpy(itemid,itemid_amp,4);
			break;
		case IMET_PWR:
			memcpy(itemid,itemid_pwr,4);
			break;
		default:
			return 1;
	}

	*ps++ = 0x68;
	memcpy(ps,imet_addr,6);
	ps += 6;
	*ps++ = 0x68;
	*ps++ = 0x11;
	*ps++ = 0x04;
	memcpy(ps,itemid,4);
	ps += 4;
	pbuf = IMetCommCache;
	for(i=0;i<14;i++)
	{
		check_sum += *pbuf++;
	}
	check_sum = (check_sum&0xFF);
	*ps++ = check_sum;
	*ps++ = 0x16;
	UartSend(IMET_UART_PORT, IMetCommCache, 16);
	//PrintLog(0, "IMetCommCache = ");
	//for(i=0;i<30;i++)
	//{
	//	PrintLog(0, "%02x ",IMetCommCache[i]);
	//}
	//PrintLog(0, "\n");
	rcv_len = recieve_read_imet_pak(rcv_buf);
	//PrintLog(0, "rcv_len = %d\n",rcv_len);
	if(rcv_len>0)
	{
		//PrintLog(0, "rcv_buf = ");
		//for(i=0;i<rcv_len;i++)
		//{
		//	PrintLog(0, "%02x ",rcv_buf[i]);
		//}
		//PrintLog(0, "\n");

		if(check_recieve_read_imet_pak(rcv_buf,rcv_len))	return 1;
		data_len = rcv_buf[9];
		data_len -= 4;
		if(data_len<=0)	return 1;
		//PrintLog(0, "data_len = %d\n",data_len);
		for(i=0;i<data_len;i++)
		{
			rcv_buf[14 + i] = (rcv_buf[14 + i] -0x33);
		}
		//PrintLog(0, "rcv_buf[14] =  ");
		//for(i=0;i<data_len;i++)
		//{
		//	PrintLog(0, "%02x ",rcv_buf[14 + i]);
		//}
		
		//BcdToHex(&rcv_buf[14],data_len);
		memcpy(rbuf,rcv_buf + 14,data_len);
		//BcdToHex(rbuf,data_len);
		return 0;
	}
	return 1;
}
















#if 0
/**
* @brief imet通信
* @param cmd 命令
* @param sbuf 发送缓存指针
* @param slen 发送缓存长度
* @param rbuf 接收缓存指针
* @param rlen 接收缓存长度
* @return 0成功, 否则失败
*/

static int IMetComm(unsigned char cmd, unsigned char *sbuf, int slen, unsigned char *rbuf, int rlen)
{
	unsigned char *ps = IMetCommCache;
	unsigned char *pr;
	int rcvlen, timecnt;
	unsigned char chk;

	*ps++ = 0x68;
	*ps++ = IMET_ADDR;
	*ps++ = cmd;
	if((NULL != sbuf) && (slen > 0)) {
		*ps++ = (unsigned char)slen;
		for(rcvlen=0; rcvlen<slen; rcvlen++) *ps++ = *sbuf++;
	}
	else {
		slen = 0;
		*ps++ = 0;
	}

	slen += 4;
	chk = 0;
	for(rcvlen=0; rcvlen<slen; rcvlen++)
		chk += IMetCommCache[rcvlen];
	*ps++ = chk;
	*ps++ = 0x0d;

	while(UartRecv(IMET_UART_PORT, &chk, 1) > 0);
	UartSend(IMET_UART_PORT, IMetCommCache, slen+2);
	Sleep(10);

	rcvlen = 0;
	timecnt = 0;
	pr = IMetCommCache;
	rlen += 6;

	while(rcvlen < rlen) {
		while(UartRecv(IMET_UART_PORT, pr, 1) <= 0) {
			timecnt += 10;
			Sleep(10);
			if(timecnt > 100) return 1;
		}

		switch(rcvlen) {
		case 0:
			if(0x68 == *pr) {
				pr++;
				rcvlen++;
			}
			break;
		case 1:
			if(IMET_ADDR == *pr) {
				pr++;
				rcvlen++;
			}
			else {
				rcvlen = 0;
				pr = IMetCommCache;
			}
			break;
		case 2:
			if(*pr == (cmd|0x80)) {
				pr++;
				rcvlen++;
			}
			else {
				rcvlen = 0;
				pr = IMetCommCache;
			}
			break;

		default:
			pr++;
			rcvlen++;
			break;
		}
	}

	if(0x0d != IMetCommCache[rlen-1]) return 1;

	chk = 0;
	for(rcvlen=0; rcvlen<(rlen-2); rcvlen++)
		chk += IMetCommCache[rcvlen];
	if(chk != IMetCommCache[rlen-2]) return 1;

	rlen -= 6;
	pr = &(IMetCommCache[4]);

	for(rcvlen=0; rcvlen<rlen; rcvlen++) *rbuf++ = *pr++;
	
	return 0;
}

#endif
/**
* @brief 读取交流采样数据
* @param command 命令(数据地址)
* @param rbuf 接收缓存指针
* @param rlen 接收缓存长度
* @return 0成功, 否则失败
*/
static int IMetRead(unsigned char command, unsigned char *rbuf, int rlen)
{
	//@change later: 添加失败复位
	int retry;

	IMET_LOCK;
	for(retry=0; retry<3; retry++) {
		if(!IMetComm(command, NULL, 0, rbuf, rlen)) break;

		Sleep(20);
	}
	IMET_UNLOCK;

	if(retry >= 3) return 1;
	else return 0;
}

static int IMetWrite(unsigned char command, unsigned char *sbuf, int slen)
{
	int retry;
	unsigned char rbuf[4];

	IMET_LOCK;
	for(retry=0; retry<3; retry++) {
		if(!IMetComm(command, sbuf, slen, rbuf, 1)) break;

		Sleep(20);
	}
	IMET_UNLOCK;

	if(retry >= 3) return 1;

	if(rbuf[0] != 0x01) return 1;
	else return 0;
}

#else
static int IMetRead(unsigned char command, unsigned char *rbuf, int rlen);
#endif /*VIRTUAL_IMET*/

/**
* @brief 更新交流采样测量点号
*/
static void UpdateIMetMetpid(void)
{
	int metid;

	IMetMetpid = 0;

	for(metid=0; metid<MAX_CENMETP; metid++) 
	{
		if(METTYPE_ACSAMP == ParaMeter[metid].proto) 
		{
			IMetMetpid = ParaMeter[metid].metp_id;//得到交流采样测量点号
			if(IMetMetpid > MAX_CENMETP) IMetMetpid = 0;
			return;
		}
	}
}

/**
* @brief 计算功率因数
* @param pwra 有功功率, 0.1w
* @param pwri 无功功率, 0.1var
* @return 功率因数 0.1%
*/
static int CalculatePwrf(int pwra, int pwri, int *pwrv)
{
	double db1, db2;
	int i;

	db1 = (double)pwra;
	db1 *= pwra;
	db2 = (double)pwri;
	db2 *= pwri;
	db1 += db2;
	db1 = sqrt(db1);
	i = (int)db1;
	*pwrv = i;
	if(0 == i) i = 1000;
	else
	{
		pwra *= 1000;
		i = pwra / i;
	}

	return i;
}

//static void IMetProcEne(void);

unsigned char IMetStatus[2];

/**
* @brief 更新交流采样数据
*/
static void IMetProc(void)
{
	//@change later: 谐波
	unsigned char buf[32];
	unsigned char dbuf[8];
	int i, ui;
	unsigned int ul;
	unsigned char ampflag[3];

	UpdateIMetMetpid();//更新交流采样测量点号
	if(0 == IMetMetpid) return;//如果没有配置交流采样测量点则返回

	UpdateMdbCurRdTime(IMetMetpid-1);//更新抄表时间

/*
	if(!IMetRead(IMET_STAT, buf, 2)) 
	{
		IMetStatus[0] = buf[0];
		IMetStatus[1] = buf[1];
	}
*/
	Sleep(5);

	if(!IMetRead(IMET_VOL, buf, 10)) 
	{
		//BcdToHex(buf,10);
		for(i=0; i<3; i++) {
			//ul = MAKE_SHORT(&buf[i<<1]);
			ul = BcdToUnsigned(&buf[i<<1],2);
			//PrintLog(0, "ul = %d\n",ul);
			//ul += 5;//四舍五入
			if(PWRTYPE_3X3W == ParaCenMetp[IMetMetpid-1].base.pwrtype && i == 1) ul = 0;
			
			//UnsignedToBcd(ul/10, dbuf, 2);
			UnsignedToBcd(ul, dbuf, 2);
			//PrintLog(0, "dbuf[0] = %02x dbuf[1] = %02x \n",dbuf[0],dbuf[1]);
			UpdateMdbCurrent(IMetMetpid, 0xb611+i, dbuf, 2, UPCURFLAG_GB);//更新三相电压数据
		}
	}
	else 
	{
		PrintLog(LOGTYPE_ALARM, "read imet vol fail\n");
		UpdateMdbCurrent(IMetMetpid, 0xb61f, dbuf, 6, UPCURFLAG_ERROR);
	}

	Sleep(5);



	Sleep(5);

	if(!IMetRead(IMET_AMP, buf, 12)) {
		for(i=0; i<4; i++) {
			ul = (unsigned int)buf[3*i] + ((unsigned int)buf[3*i+1]<<8) + ((unsigned int)buf[3*i+2]<<16);
			ul += 5;
			if(PWRTYPE_3X3W == ParaCenMetp[IMetMetpid-1].base.pwrtype && i == 1) ul = 0;

			ui = ul;
			if(ampflag[i]) ui *= -1;
			IntToBcd(ui/10, dbuf, 2);
			UpdateMdbCurrent(IMetMetpid, 0xb621+i, dbuf, 2, UPCURFLAG_GB);
		}
	}
	else {
		PrintLog(LOGTYPE_ALARM, "read imet amp fail\n");
		UpdateMdbCurrent(IMetMetpid, 0xb62f, dbuf, 6, UPCURFLAG_ERROR);
	}

	Sleep(5);


	if(!IMetRead(IMET_PWR, buf, 24)) {
		int pwra, pwri;
		int spwra, spwri;
		int pwrv, spwrv;

		pwra = pwri = 0;
		pwrv = 0;
		for(i=0; i<3; i++) {
			spwra = (int)MAKE_LONG(&buf[i<<2]);
			pwra += spwra;
			spwra = (spwra+5)/10;
			if(spwra < 0) ampflag[i] = 1;
			else ampflag[i] = 0;
			IntToBcd(spwra, dbuf, 3);
			UpdateMdbCurrent(IMetMetpid, 0xb631+i, dbuf, 3, UPCURFLAG_GB);

			spwri = (int)MAKE_LONG(&buf[(i<<2)+12]);
			pwri += spwri;
			spwri = (spwri+5)/10;
			IntToBcd(spwri, dbuf, 3);
			UpdateMdbCurrent(IMetMetpid, 0xb641+i, dbuf, 3, UPCURFLAG_GB);

			ui = CalculatePwrf(spwra, spwri, &spwrv);
			IntToBcd(ui, dbuf, 2);
			UpdateMdbCurrent(IMetMetpid, 0xb651+i, dbuf, 2, UPCURFLAG_GB);

			pwrv += spwrv;
			IntToBcd(spwrv, dbuf, 3);
			UpdateMdbCurrent(IMetMetpid, 0xb6e1+i, dbuf, 3, UPCURFLAG_GB);
		}

		pwra = (pwra+5)/10;
		IntToBcd(pwra, dbuf, 3);
		UpdateMdbCurrent(IMetMetpid, 0xb630, dbuf, 3, UPCURFLAG_GB);
		pwri = (pwri+5)/10;
		IntToBcd(pwri, dbuf, 3);
		UpdateMdbCurrent(IMetMetpid, 0xb640, dbuf, 3, UPCURFLAG_GB);

		ui = CalculatePwrf(pwra, pwri, &spwrv);
		IntToBcd(ui, dbuf, 2);
		UpdateMdbCurrent(IMetMetpid, 0xb650, dbuf, 2, UPCURFLAG_GB);

		IntToBcd(pwrv, dbuf, 3);
		UpdateMdbCurrent(IMetMetpid, 0xb6e0, dbuf, 3, UPCURFLAG_GB);
	}
	else {
		PrintLog(LOGTYPE_ALARM, "read imet pwr fail\n");
		UpdateMdbCurrent(IMetMetpid, 0xb63f, dbuf, 6, UPCURFLAG_ERROR);
		UpdateMdbCurrent(IMetMetpid, 0xb64f, dbuf, 6, UPCURFLAG_ERROR);
		UpdateMdbCurrent(IMetMetpid, 0xb65f, dbuf, 6, UPCURFLAG_ERROR);
	}

	Sleep(5);





	
/*
	if(!IMetRead(IMET_PWR, buf, 24)) {
		int pwra, pwri;
		int spwra, spwri;
		int pwrv, spwrv;

		pwra = pwri = 0;
		pwrv = 0;
		for(i=0; i<3; i++) {
			spwra = (int)MAKE_LONG(&buf[i<<2]);
			pwra += spwra;
			spwra = (spwra+5)/10;
			if(spwra < 0) ampflag[i] = 1;
			else ampflag[i] = 0;
			IntToBcd(spwra, dbuf, 3);
			UpdateMdbCurrent(IMetMetpid, 0xb631+i, dbuf, 3, UPCURFLAG_GB);

			spwri = (int)MAKE_LONG(&buf[(i<<2)+12]);
			pwri += spwri;
			spwri = (spwri+5)/10;
			IntToBcd(spwri, dbuf, 3);
			UpdateMdbCurrent(IMetMetpid, 0xb641+i, dbuf, 3, UPCURFLAG_GB);

			ui = CalculatePwrf(spwra, spwri, &spwrv);
			IntToBcd(ui, dbuf, 2);
			UpdateMdbCurrent(IMetMetpid, 0xb651+i, dbuf, 2, UPCURFLAG_GB);

			pwrv += spwrv;
			IntToBcd(spwrv, dbuf, 3);
			UpdateMdbCurrent(IMetMetpid, 0xb6e1+i, dbuf, 3, UPCURFLAG_GB);
		}

		pwra = (pwra+5)/10;
		IntToBcd(pwra, dbuf, 3);
		UpdateMdbCurrent(IMetMetpid, 0xb630, dbuf, 3, UPCURFLAG_GB);
		pwri = (pwri+5)/10;
		IntToBcd(pwri, dbuf, 3);
		UpdateMdbCurrent(IMetMetpid, 0xb640, dbuf, 3, UPCURFLAG_GB);

		ui = CalculatePwrf(pwra, pwri, &spwrv);
		IntToBcd(ui, dbuf, 2);
		UpdateMdbCurrent(IMetMetpid, 0xb650, dbuf, 2, UPCURFLAG_GB);

		IntToBcd(pwrv, dbuf, 3);
		UpdateMdbCurrent(IMetMetpid, 0xb6e0, dbuf, 3, UPCURFLAG_GB);
	}
	else {
		PrintLog(LOGTYPE_ALARM, "read imet pwr fail\n");
		UpdateMdbCurrent(IMetMetpid, 0xb63f, dbuf, 6, UPCURFLAG_ERROR);
		UpdateMdbCurrent(IMetMetpid, 0xb64f, dbuf, 6, UPCURFLAG_ERROR);
		UpdateMdbCurrent(IMetMetpid, 0xb65f, dbuf, 6, UPCURFLAG_ERROR);
	}

	Sleep(5);

	if(!IMetRead(IMET_AMP, buf, 12)) {
		for(i=0; i<4; i++) {
			ul = (unsigned int)buf[3*i] + ((unsigned int)buf[3*i+1]<<8) + ((unsigned int)buf[3*i+2]<<16);
			ul += 5;
			if(PWRTYPE_3X3W == ParaCenMetp[IMetMetpid-1].base.pwrtype && i == 1) ul = 0;

			ui = ul;
			if(ampflag[i]) ui *= -1;
			IntToBcd(ui/10, dbuf, 2);
			UpdateMdbCurrent(IMetMetpid, 0xb621+i, dbuf, 2, UPCURFLAG_GB);
		}
	}
	else {
		PrintLog(LOGTYPE_ALARM, "read imet amp fail\n");
		UpdateMdbCurrent(IMetMetpid, 0xb62f, dbuf, 6, UPCURFLAG_ERROR);
	}

	Sleep(5);

	if(!IMetRead(IMET_VOLANGLE, buf, 6)) {
		for(i=0; i<3; i++) {
			ul = MAKE_SHORT(&buf[i<<1]);
			ul += 5;
			if(PWRTYPE_3X3W == ParaCenMetp[IMetMetpid-1].base.pwrtype && i == 1) ul = 0;
			UnsignedToBcd(ul/10, dbuf, 2);
			UpdateMdbCurrent(IMetMetpid, 0xb660+i, dbuf, 2, UPCURFLAG_GB);
		}
	}
	else {
		PrintLog(LOGTYPE_ALARM, "read imet vol arc fail\n");
	}

	Sleep(5);

	if(!IMetRead(IMET_AMPANGLE, buf, 6)) {
		for(i=0; i<3; i++) {
			ul = MAKE_SHORT(&buf[i<<1]);
			ul += 5;
			if(PWRTYPE_3X3W == ParaCenMetp[IMetMetpid-1].base.pwrtype && i == 1) ul = 0;
			UnsignedToBcd(ul/10, dbuf, 2);
			UpdateMdbCurrent(IMetMetpid, 0xb663+i, dbuf, 2, UPCURFLAG_GB);
		}
	}
	else {
		PrintLog(LOGTYPE_ALARM, "read imet vol arc fail\n");
	}
	
	IMetProcEne();
	*/
}

/**
* @brief 更新当前电能量到当前数据库
*/
void UpdateIMetEneMdb(void)
{
	unsigned char buf[12];
	int i;

	UnsignedToBcd(IMetEneSave.enepa, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9010, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSave.enepi, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9110, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSave.enepi1, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9130, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSave.enepi4, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9140, buf, 4, UPCURFLAG_645);

	UnsignedToBcd(IMetEneSave.enena, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9020, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSave.eneni, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9120, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSave.eneni2, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9150, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSave.eneni3, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9160, buf, 4, UPCURFLAG_645);

	for(i=0; i<3; i++) UnsignedToBcd(IMetEneSave.enepaa[i], buf+(i<<2), 4);
	UpdateMdbCurrent(IMetMetpid, 0x903f, buf, 4, UPCURFLAG_645);
	for(i=0; i<3; i++) UnsignedToBcd(IMetEneSave.enenaa[i], buf+(i<<2), 4);
	UpdateMdbCurrent(IMetMetpid, 0x904f, buf, 4, UPCURFLAG_645);
	for(i=0; i<3; i++) UnsignedToBcd(IMetEneSave.enepia[i], buf+(i<<2), 4);
	UpdateMdbCurrent(IMetMetpid, 0x905f, buf, 4, UPCURFLAG_645);
	for(i=0; i<3; i++) UnsignedToBcd(IMetEneSave.enenia[i], buf+(i<<2), 4);
	UpdateMdbCurrent(IMetMetpid, 0x906f, buf, 4, UPCURFLAG_645);

}

/**
* @brief 更新上月电能量到当前数据库
*/
void UpdateIMetEneLmMdb(void)
{
	unsigned char buf[12];
	int i;

	UnsignedToBcd(IMetEneSaveLm.enepa, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9410, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSaveLm.enepi, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9510, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSaveLm.enepi1, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9530, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSaveLm.enepi4, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9540, buf, 4, UPCURFLAG_645);

	UnsignedToBcd(IMetEneSaveLm.enena, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9420, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSaveLm.eneni, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9520, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSaveLm.eneni2, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9550, buf, 4, UPCURFLAG_645);
	UnsignedToBcd(IMetEneSaveLm.eneni3, buf, 4);
	UpdateMdbCurrent(IMetMetpid, 0x9560, buf, 4, UPCURFLAG_645);

	for(i=0; i<3; i++) UnsignedToBcd(IMetEneSaveLm.enepaa[i], buf+(i<<2), 4);
	UpdateMdbCurrent(IMetMetpid, 0x943f, buf, 4, UPCURFLAG_645);
	for(i=0; i<3; i++) UnsignedToBcd(IMetEneSaveLm.enenaa[i], buf+(i<<2), 4);
	UpdateMdbCurrent(IMetMetpid, 0x944f, buf, 4, UPCURFLAG_645);
	for(i=0; i<3; i++) UnsignedToBcd(IMetEneSaveLm.enepia[i], buf+(i<<2), 4);
	UpdateMdbCurrent(IMetMetpid, 0x945f, buf, 4, UPCURFLAG_645);
	for(i=0; i<3; i++) UnsignedToBcd(IMetEneSaveLm.enenia[i], buf+(i<<2), 4);
	UpdateMdbCurrent(IMetMetpid, 0x946f, buf, 4, UPCURFLAG_645);

}

/**
* @brief 更新电能量
* @param readi 读取的电能量
* @param pbaki 备份的电能量指针
* @return 计算得出的电能量增量
*/
static inline unsigned int UpdateEne(unsigned int readi, unsigned int *pbaki)
{
	unsigned int baki = *pbaki;
	unsigned int diff;

	if(readi < baki) {
		if(readi < 100) diff = readi;  //交采板复位
		else diff = 0;
	}
	else {
		diff = readi - baki;
		if(diff > 100) diff = 0;  //invalid
	}

	*pbaki = readi;
	return diff;
}

/*#define ADD_ENE(ene, addi) { \
	(ene) += (addi); \
	if((ene) > 99999999) (ene) -= 99999999; }

struct imet_bakene {
	unsigned int enepa[3];
	unsigned int enena[3];
	unsigned int enepi1[3];
	unsigned int enepi4[3];
	unsigned int eneni2[3];
	unsigned int eneni3[3];
};
static struct imet_bakene IMetBakEne;*/


#if 0
/**
* @brief 处理交流采样电能量数据
*/
static void IMetProcEne(void)
{
	unsigned char buf[36];
	unsigned int ene;
	int readone = 0;
	int i;

	if(!IMetRead(IMET_POSPENE, buf, 16)) {
		for(i=0; i<4; i++) {
			ene = MAKE_LONG(buf + (i<<2));
			ene = (ene+5)/10;
			//DebugPrint(0, "pos ene=%d\n", ene);
			if(3 == i) IMetEneSave.enepa = ene;
			else IMetEneSave.enepaa[i] = ene;
		}
		readone = 1;
	}

	Sleep(5);

	if(!IMetRead(IMET_NEGPENE, buf, 16)) {
		for(i=0; i<4; i++) {
			ene = MAKE_LONG(buf + (i<<2));
			ene = (ene+5)/10;
			if(3 == i) IMetEneSave.enena = ene;
			else IMetEneSave.enenaa[i] = ene;
		}
		readone = 1;
	}

	Sleep(5);

	if(!IMetRead(IMET_NEGQI1ENE, buf, 16)) {
		Sleep(5);

		if(!IMetRead(IMET_NEGQI4ENE, buf+16, 16)) {
			for(i=0; i<4; i++) {
				ene = MAKE_LONG(buf + (i<<2));
				ene = (ene+5)/10;
				if(3 == i) {
					IMetEneSave.enepi1 = ene;
					IMetEneSave.enepi = ene;
				}
				else IMetEneSave.enepia[i] = ene;
			}

			for(i=0; i<4; i++) {
				ene = MAKE_LONG(buf + 16 + (i<<2));
				ene = (ene+5)/10;
				if(3 == i) {
					IMetEneSave.enepi4 = ene;
					IMetEneSave.enepi += ene;
				}
				else IMetEneSave.enepia[i] += ene;
			}

			readone = 1;
		}
	}

	Sleep(5);

	if(!IMetRead(IMET_NEGQI2ENE, buf, 16)) {
		Sleep(5);

		if(!IMetRead(IMET_NEGQI3ENE, buf+16, 16)) {
			for(i=0; i<4; i++) {
				ene = MAKE_LONG(buf + (i<<2));
				ene = (ene+5)/10;
				if(3 == i) {
					IMetEneSave.eneni2 = ene;
					IMetEneSave.eneni = ene;
				}
				else IMetEneSave.enenia[i] = ene;
			}

			for(i=0; i<4; i++) {
				ene = MAKE_LONG(buf + 16 + (i<<2));
				ene = (ene+5)/10;
				if(3 == i) {
					IMetEneSave.eneni3 = ene;
					IMetEneSave.eneni += ene;
				}
				else IMetEneSave.enenia[i] += ene;
			}

			readone = 1;
		}
	}


//存储交流采样数据
/*
	if(readone) {
		SaveIMetEne();
		UpdateIMetEneMdb();
	}
	*/
}
#endif

#define IMETEV_READMET			1

static sys_event_t IMetEventCtrl;

static int CTimerIMet(unsigned long arg)
{
	SysSendEvent(&IMetEventCtrl, IMETEV_READMET);

	return 0;
}

/**
* @brief 交流采样自动读取任务
*/
static void *IMetTask(void *arg)
{
	unsigned long ev;

	Sleep(50);
	while(RunState.pwroff) Sleep(100);

	SysAddCTimer(60, CTimerIMet, 0);//一分钟交流采样
	IMetProc();//一上电就读一次采样

	while(1) {
		SysWaitEvent(&IMetEventCtrl, 1, IMETEV_READMET, &ev);

		if(ev&IMETEV_READMET) 
		{
			//if(!RunState.pwroff) 
			//{
				//DebugPrint(LOGTYPE_SHORT, "imet proc...\n");
				IMetProc();
				//CMetAlm1MinProc();
			//}
		}
	}

	return 0;
}

/**
* @brief 初始化交流采样模块
* @return 0成功, 否则失败
*/
DECLARE_INIT_FUNC(IMetInit);
int IMetInit(void)
{
	printf("  imet init ...\n");
	//if(UartOpen(IMET_UART_PORT)) return 1;
	//UartSet(IMET_UART_PORT, 38400, 8, 1, 'e');

	SysInitMutex(&IMetMutex);
	SysInitEvent(&IMetEventCtrl);

	LoadIMetSave();

	SysCreateTask(IMetTask, NULL);

	SET_INIT_FLAG(IMetInit);

	return 0;
}

/**
* @brief 读取交流采样数据命令
*/
static int shell_imetdata(int argc, char *argv[])
{
	unsigned char buf[32];
	int i, si;
	unsigned int ul;

	if(!IMetRead(IMET_STAT, buf, 2)) {
		PrintLog(0, "状态字:L=%02X H=%02X\n", buf[0], buf[1]);
	}
	else {
		PrintLog(0, "读取状态字失败\n");
	}

	Sleep(5);

	if(!IMetRead(IMET_VOL, buf, 10)) {
		PrintLog(0, "电压(V): ");
		for(i=0; i<3; i++) {
			ul = MAKE_SHORT(&buf[i<<1]);
			PrintLog(0, "%c=%d.%02d ", 'A'+i, ul/100, ul%100);
		}
		ul = MAKE_SHORT(buf+6);
		PrintLog(0, "\n频率=%d.%03dHz ", ul/1000, ul%1000);
		ul = MAKE_SHORT(buf+8);
		PrintLog(0, "温度=%d度\n", ul);
	}
	else {
		PrintLog(0, "读取电压失败\n");
	}

	Sleep(5);

	if(!IMetRead(IMET_PWR, buf, 24)) {
		for(i=0; i<3; i++) {
			char sigc;

			PrintLog(0, "%c相功率: ", 'A'+i);

			si = (int)MAKE_LONG(&buf[i<<2]);
			if(si < 0) {
				si *= -1;
				sigc = '-';
			}
			else sigc = '+';
			PrintLog(0, "有功=%c%d.%02dW ", sigc, si/100, si%100);

			si = (int)MAKE_LONG(&buf[(i<<2)+12]);
			if(si < 0) {
				si *= -1;
				sigc = '-';
			}
			else sigc = '+';
			PrintLog(0, "无功=%c%d.%02dvar\n", sigc, si/100, si%100);
		}
	}
	else {
		PrintLog(0, "读取功率失败\n");
	}

	Sleep(5);

	if(!IMetRead(IMET_AMP, buf, 12)) {
		char phasec;

		PrintLog(0, "电流(A): ");
		for(i=0; i<4; i++) {
			if(i < 3) phasec = 'A'+i;
			else phasec = 'Z';
			ul = (unsigned int)buf[3*i] + ((unsigned int)buf[3*i+1]<<8) + ((unsigned int)buf[3*i+2]<<16);
			PrintLog(0, "%c=%d.%03d ", phasec, ul/1000, ul%1000);
		}
		PrintLog(0, "\n");
	}
	else {
		PrintLog(0, "读取电流失败\n");
	}

	return 0;
}
DECLARE_SHELL_CMD("imet", shell_imetdata, "读取交流采样数据");

#ifndef VIRTUAL_IMET

static int shell_clrene(int argc, char *argv[])
{
	char option;
	unsigned char sbuf[4];

	if(2 != argc) {
		printf("usage: clrene a/p/i\n");
		return 1;
	}

	option = argv[1][0];

	if('a' == option) {
		sbuf[0] = 0;
		if(IMetWrite(IMET_CLRPOSPENE, sbuf, 1)) PrintLog(0, "清除有功电量失败\n");
		else PrintLog(0, "清除有功电量成功\n");

		sbuf[0] = 0;
		if(IMetWrite(IMET_CLRNEGQENE, sbuf, 1)) PrintLog(0, "清除无功电量失败\n");
		else PrintLog(0, "清除无功电量成功\n");
	}
	else if('p' == option) {
		sbuf[0] = 0;
		if(IMetWrite(IMET_CLRPOSPENE, sbuf, 1)) PrintLog(0, "清除有功电量失败\n");
		else PrintLog(0, "清除有功电量成功\n");
	}
	else if('i' == option) {
		sbuf[0] = 0;
		if(IMetWrite(IMET_CLRNEGQENE, sbuf, 1)) PrintLog(0, "清除无功电量失败\n");
		else PrintLog(0, "清除无功电量成功\n");
	}
	else {
		printf("usage: clrene a/p/i\n");
		return 1;
	}

	return 0;
}
DECLARE_SHELL_CMD("clrene", shell_clrene, "清除交流采样电能量");

#endif

#ifdef VIRTUAL_IMET

static unsigned short vimet_vol[3] = {22011, 22353, 21536};
static unsigned int vimet_amp[4] = {1730, 1860, 1601, 83};
static int vimet_pwr[6] = {37460, 40232, 35123, 1256, 2761, 1987};
static unsigned int vimet_enepa[3] = {161, 165, 163};

/**
* @brief 读取交流采样数据(虚拟数据)
* @param command 命令(数据地址)
* @param rbuf 接收缓存指针
* @param rlen 接收缓存长度
* @return 0成功, 否则失败
*/
static int IMetRead(unsigned char command, unsigned char *rbuf, int rlen)
{
	memset(rbuf, 0, rlen);

	switch(command) {
	case IMET_VOL:
		smallcpy(rbuf, vimet_vol, 6);
		break;
	case IMET_AMP:
		DEPART_LONG(vimet_amp[0], rbuf);
		DEPART_LONG(vimet_amp[1], rbuf+3);
		DEPART_LONG(vimet_amp[2], rbuf+6);
		DEPART_LONG(vimet_amp[3], rbuf+9);
		break;
	case IMET_PWR:
		smallcpy(rbuf, vimet_pwr, 24);
		break;
	case IMET_POSPENE:
		smallcpy(rbuf, vimet_enepa, 12);
		vimet_enepa[0] += 1;
		vimet_enepa[1] += 2;
		vimet_enepa[2] += 1;
		break;
	default:
		break;
	}

	return 0;
}

static int shell_vimet(int argc, char *argv[])
{
	char cmd;
	int i;

	if(argc < 2) {
		PrintLog(0, "usage: vimet cmd [value...]\n");
		return 1;
	}

	cmd = *argv[1];

	switch(cmd) {
	case 's':
		PrintLog(0, "vol(0.01V) =%d,%d,%d\n", vimet_vol[0], vimet_vol[1], vimet_vol[2]);
		PrintLog(0, "amp(mA)    =%d,%d,%d,%d\n", vimet_amp[0], vimet_amp[1], vimet_amp[2], vimet_amp[3]);
		PrintLog(0, "pwr(0.01W) =%d,%d,%d\n", vimet_pwr[0], vimet_pwr[1], vimet_pwr[2]);
		PrintLog(0, "ipwr       =%d,%d,%d\n", vimet_pwr[3], vimet_pwr[4], vimet_pwr[5]);
		PrintLog(0, "enepa      =%d,%d,%d\n", vimet_enepa[0],vimet_enepa[1], vimet_enepa[2]);
		return 0;
		break;

	case 'v':
		if(argc != 5) {
			PrintLog(0, "usage: vimet vol vola volb volc\n");
			return 1;
		}
		for(i=0; i<3; i++) vimet_vol[i] = atoi(argv[i+2]);
		break;

	case 'a':
		if(argc != 6) {
			PrintLog(0, "usage: vimet amp ampa ampb ampc ampz\n");
			return 1;
		}
		for(i=0; i<4; i++) vimet_amp[i] = atoi(argv[i+2]);
		break;

	case 'p':
		if(argc != 5) {
			PrintLog(0, "usage: vimet pwr pwra pwrb pwrc\n");
			return 1;
		}
		for(i=0; i<3; i++) vimet_pwr[i] = atoi(argv[i+2]);
		break;

	case 'i':
		if(argc != 5) {
			PrintLog(0, "usage: vimet ipwr ipwra ipwrb ipwrc\n");
			return 1;
		}
		for(i=0; i<3; i++) vimet_pwr[i+3] = atoi(argv[i+2]);
		break;

	case 'e':
		if(argc != 5) {
			PrintLog(0, "usage: vimet ene enea eneb enec\n");
			return 1;
		}
		for(i=0; i<3; i++) vimet_enepa[i] = atoi(argv[i+2]);
		break;

	default:
		PrintLog(0, "unknown command\n");
		return 1;
	}

	PrintLog(0, "set %s ok\n", argv[1]);
	return 0;
}
DECLARE_SHELL_CMD("vimet", shell_vimet, "查看或设置虚拟交流采样数据");

#endif

