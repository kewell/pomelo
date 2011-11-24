/**
* mix.c -- 综合参数
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#include <stdio.h>
#include <string.h>

#define DEFINE_PARAMIX

#include "param_config.h"
#include "include/debug.h"
#include "include/param/mix.h"
#include "include/param/commport.h"
#include "include/sys/bin.h"
#include "include/param/operation.h"
#include "operation_inner.h"
#include "include/lib/bcd.h"

save_mix_t ParaMixSave;

const para_commport_t *GetParaCommPort(unsigned int port)
{
	AssertLogReturn(port>=MAX_COMMPORT, NULL, "invalid port(%d)\n", port);

	return &ParaMixSave.commport[port];
}

/**
* @brief 载入缺省综合参数
*/
static void LoadDefParaMix(void)
{
	int i;

	for(i=0; i<MAX_COMMPORT; i++) {
		ParaMixSave.commport[i].baudrate = 9600;
		ParaMixSave.commport[i].frame = COMMFRAME_BAUD_9600|COMMFRAME_STOPBIT_1|COMMFRAME_HAVECHECK|COMMFRAME_EVENCHECK|COMMFRAME_DATA_8;
	}

	ParaMixSave.mix.cascade.port = COMMPORT_RS485_1;
	ParaMixSave.mix.cascade.frame = COMMFRAME_BAUD_9600|COMMFRAME_STOPBIT_1|COMMFRAME_HAVECHECK|COMMFRAME_EVENCHECK|COMMFRAME_DATA_8;

	ParaMixSave.mix.bactsend = 0;
	ParaMixSave.mix.bactcomm = 0;
}

#define MIX_MAGIC	0xafc34872

/**
* @brief 从文件中载入综合参数
* @return 0成功, 否则失败
*/
int LoadParaMix(void)
{
	LoadDefParaMix();

	DebugPrint(0, "  load param mix...(size=%d)", sizeof(ParaMixSave)+12);

	if(ReadBinFile(PARAM_SAVE_PATH "mix.bin", MIX_MAGIC, (unsigned char *)&ParaMixSave, sizeof(ParaMixSave)) < 0) {
		DebugPrint(0, "no main, ");
		if(ReadBinFile(PARAM_BAK_PATH "mix.bin", MIX_MAGIC, (unsigned char *)&ParaMixSave, sizeof(ParaMixSave)) < 0)
			DebugPrint(0, "no bak\n");
			return 1;
	}

	DebugPrint(0, "ok\n");
	return 0;
}

/**
* @brief 保存综合参数
* @return 0成功, 否则失败
*/
int SaveParaMix(void)
{
	SaveBinFile(PARAM_SAVE_PATH "mix.bin", MIX_MAGIC, (unsigned char *)&ParaMixSave, sizeof(ParaMixSave));
	SaveBinFile(PARAM_BAK_PATH "mix.bin", MIX_MAGIC, (unsigned char *)&ParaMixSave, sizeof(ParaMixSave));

	return 0;
}

/**
* @brief 终端参数F21操作
* @param flag 操作方式, 0-读, 1-写
* @param metpid 测量点号
* @param buf 缓存区指针
* @param len 缓存区长度
* @param actlen 有效数据长度(由函数返回)
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
* @note 以下同类参数和返回值相同, 不做重复注释
*/
int ParaOperationF21(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) memcpy(buf, ParaMixSave.mix.feprd.period, 49);
	else memcpy(ParaMixSave.mix.feprd.period, buf, 49);

	return 0;
}


/**
* @brief 终端参数F33操作
* @param flag 操作方式, 0-读, 1-写
* @param option 操作选项
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int ParaOperationF33(int flag, unsigned short metpid, param_specialop_t *option)
{
	if(0 == flag) {
		int num, i, j, portid, actnum, periodnum;
		const unsigned char *prbuf;
		unsigned char *pwbuf;
		unsigned short wlen;

		num = (int)option->rbuf[0]&0xff;
		option->ractlen = num + 1;
		if(option->rlen < option->ractlen) return POERR_FATAL;

		wlen = 1;
		prbuf = option->rbuf + 1;
		pwbuf = option->wbuf + 1;
		actnum = 0;
		for(i=0; i<num; i++,prbuf++) {
			portid = (int)prbuf[0]&0xff;//得到端口号(1-31)
			if(portid == 0 || portid > MAX_COMMPORT) continue;
			if((wlen+14) > option->wlen) break;
			portid -= 1;//端口号减去1

			*pwbuf++ = prbuf[0];
			*pwbuf++ = ParaMixSave.commport[portid].flag & 0xff;
			*pwbuf++ = ParaMixSave.commport[portid].flag >> 8;
			*pwbuf++ = ParaMixSave.commport[portid].dateflag & 0xff;
			*pwbuf++ = (ParaMixSave.commport[portid].dateflag>>8) & 0xff;
			*pwbuf++ = (ParaMixSave.commport[portid].dateflag>>16) & 0xff;
			*pwbuf++ = (ParaMixSave.commport[portid].dateflag>>24) & 0xff;
			pwbuf[0] = ParaMixSave.commport[portid].time_minute;
			pwbuf[1] = ParaMixSave.commport[portid].time_hour;
			HexToBcd(pwbuf, 2); pwbuf += 2;
			*pwbuf++ = ParaMixSave.commport[portid].cycle;
			pwbuf[0] = ParaMixSave.commport[portid].chktime_minute;
			pwbuf[1] = ParaMixSave.commport[portid].chktime_hour;
			pwbuf[2] = ParaMixSave.commport[portid].chktime_day;
			HexToBcd(pwbuf, 3); pwbuf += 3;

			*pwbuf++ = ParaMixSave.commport[portid].periodnum;	
			periodnum = (int)ParaMixSave.commport[portid].periodnum&0xff;
			for(j=0; j<periodnum; j++) {
				pwbuf[0] = ParaMixSave.commport[portid].period[j].min_start;
				pwbuf[1] = ParaMixSave.commport[portid].period[j].hour_start;
				pwbuf[2] = ParaMixSave.commport[portid].period[j].min_end;
				pwbuf[3] = ParaMixSave.commport[portid].period[j].hour_end;
				HexToBcd(pwbuf, 4); pwbuf += 4;
			}
			wlen += 14;
			wlen += periodnum * 4;
			actnum++;
		}

		option->wactlen = wlen;
		option->wbuf[0] = actnum;
	}
	else {
		int num, i, j, portid;
		const unsigned char *prbuf;
		unsigned char periodnum;

		num = (int)option->rbuf[0]&0xff;
		option->ractlen = 1;	
		//if(option->rlen < option->ractlen) return POERR_FATAL;

		prbuf = option->rbuf + 1;
		for(i=0; i<num; i++) {
			portid = (int)prbuf[0]&0xff;
			if(portid == 0 || portid > MAX_COMMPORT) {
				return POERR_INVALID;;
			}
			portid -= 1;
			prbuf += 1;

			ParaMixSave.commport[portid].flag = ((unsigned short)prbuf[1]<<8) + ((unsigned short)prbuf[0]&0xff); prbuf += 2;
			ParaMixSave.commport[portid].dateflag = ((unsigned int)prbuf[3]<<24) + ((unsigned int)prbuf[2]<<16)
							+ ((unsigned int)prbuf[1]<<8) + ((unsigned int)prbuf[0]&0xff);
			prbuf += 4;
			ParaMixSave.commport[portid].time_minute = *prbuf++;
			ParaMixSave.commport[portid].time_hour = *prbuf++;
			BcdToHex(&ParaMixSave.commport[portid].time_hour, 2);
			ParaMixSave.commport[portid].cycle = *prbuf++;
			ParaMixSave.commport[portid].chktime_minute = *prbuf++;
			ParaMixSave.commport[portid].chktime_hour = *prbuf++;
			ParaMixSave.commport[portid].chktime_day = *prbuf++;
			BcdToHex(&ParaMixSave.commport[portid].chktime_day, 3);
			periodnum = *prbuf++;
			if(periodnum > MAXNUM_PERIOD) return POERR_INVALID;
			ParaMixSave.commport[portid].periodnum = periodnum;
			for(j=0; j<periodnum; j++) {
				ParaMixSave.commport[portid].period[j].min_start= *prbuf++;
				ParaMixSave.commport[portid].period[j].hour_start= *prbuf++;
				ParaMixSave.commport[portid].period[j].min_end= *prbuf++;
				ParaMixSave.commport[portid].period[j].hour_end= *prbuf++;
				BcdToHex(&ParaMixSave.commport[portid].period[j].hour_start, 4);
			}
			option->ractlen += 14;
			option->ractlen += periodnum*4;
		}
	}

	return 0;
}

/**
* @brief 终端参数F34操作
* @param flag 操作方式, 0-读, 1-写
* @param option 操作选项
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int ParaOperationF34(int flag, unsigned short metpid, param_specialop_t *option)
{
	if(0 == flag) {
		int num, i, portid, actnum;
		const unsigned char *prbuf;
		unsigned char *pwbuf;
		unsigned short wlen;

		num = (int)option->rbuf[0]&0xff;
		option->ractlen = num + 1;
		if(option->rlen < option->ractlen) return POERR_FATAL;

		wlen = 1;
		prbuf = option->rbuf + 1;
		pwbuf = option->wbuf + 1;
		actnum = 0;
		for(i=0; i<num; i++,prbuf++) {
			portid = (int)prbuf[0]&0xff;
			if(portid == 0 || portid > MAX_COMMPORT) continue;
			if((wlen+6) > option->wlen) break;
			portid -= 1;

			*pwbuf++ = prbuf[0];
			*pwbuf++ = ParaMixSave.commport[portid].frame;
			*pwbuf++ = ParaMixSave.commport[portid].baudrate & 0xff;
			*pwbuf++ = (ParaMixSave.commport[portid].baudrate>>8) & 0xff;
			*pwbuf++ = (ParaMixSave.commport[portid].baudrate>>16) & 0xff;
			*pwbuf++ = (ParaMixSave.commport[portid].baudrate>>24) & 0xff;
			wlen += 6;
			actnum++;
		}

		option->wactlen = wlen;
		option->wbuf[0] = actnum;
	}
	else {
		int num, i, portid;
		const unsigned char *prbuf;

		num = (int)option->rbuf[0]&0xff;
		option->ractlen = num*6 + 1;	
		if(option->rlen < option->ractlen) return POERR_FATAL;

		prbuf = option->rbuf + 1;
		for(i=0; i<num; i++) {
			portid = (int)prbuf[0]&0xff;
			if(portid == 0 || portid > MAX_COMMPORT) {
				prbuf += 6;
				continue;
			}
			portid -= 1;
			prbuf += 1;

			ParaMixSave.commport[portid].frame = *prbuf++;
			ParaMixSave.commport[portid].baudrate = ((unsigned int)prbuf[3]<<24) + ((unsigned int)prbuf[2]<<16)
							+ ((unsigned int)prbuf[1]<<8) + ((unsigned int)prbuf[0]&0xff);
			prbuf += 4;
		}
	}

	return 0;
}

/**
* @brief 终端参数F35操作
*/
int ParaOperationF35(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned char num;
	int alen;

	if(0 == flag) {
		num = ParaMixSave.mix.impuser.num;
		if(num > MAX_IMPORTANT_USER) num = MAX_IMPORTANT_USER;

		alen = (int)num&0xff;
		alen = alen*2+1;
		*actlen = alen;
		if(alen > len) return POERR_FATAL;

		*buf++ = num;
		memcpy(buf, ParaMixSave.mix.impuser.metid, alen-1);
	}
	else {
		num = *buf++;

		alen = (int)num&0xff;
		alen = alen*2+1;
		*actlen = alen;
		if(alen > len) return POERR_FATAL;
		if(num > MAX_IMPORTANT_USER) return POERR_INVALID;

		ParaMixSave.mix.impuser.num = num;
		memcpy(ParaMixSave.mix.impuser.metid, buf, alen-1);
	}

	return 0;
}

/**
* @brief 终端参数F36操作
*/
int ParaOperationF36(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) {
		buf[0] = ParaMixSave.mix.upflow_max & 0xff;
		buf[1] = (ParaMixSave.mix.upflow_max>>8) & 0xff;
		buf[2] = (ParaMixSave.mix.upflow_max>>16) & 0xff;
		buf[3] = (ParaMixSave.mix.upflow_max>>24) & 0xff;
	}
	else {
		ParaMixSave.mix.upflow_max = ((unsigned int)buf[3]<<24) + ((unsigned int)buf[2]<<16) 
						+ ((unsigned int)buf[1]<<8) + ((unsigned int)buf[0]&0xff);
	}

	return 0;
}

/**
* @brief 终端参数F37操作
*/
int ParaOperationF37(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	int alen;

	if(0 == flag) {
		alen = (int)ParaMixSave.mix.cascade.num & 0xff;
		if(alen > MAX_CASCADE) alen = MAX_CASCADE;

		alen = alen*4;
		*actlen = alen + 7;
		if(*actlen > len) return POERR_FATAL;

		*buf++ = ParaMixSave.mix.cascade.port;
		*buf++ = ParaMixSave.mix.cascade.frame;
		*buf++ = ParaMixSave.mix.cascade.timeout;
		*buf++ = ParaMixSave.mix.cascade.timeout_byte;
		*buf++ = ParaMixSave.mix.cascade.retry;
		*buf++ = ParaMixSave.mix.cascade.cycle;
		*buf++ = ((ParaMixSave.mix.cascade.flag&0x01)<<7) + (ParaMixSave.mix.cascade.num&0x0f);
		memcpy(buf, ParaMixSave.mix.cascade.addr, alen);
	}
	else {
		alen = buf[6] & 0x0f;
		*actlen = alen*4 + 7;
		if(*actlen > len) return POERR_FATAL;

		if(buf[6]&0x80) {
			if(alen > 1) return POERR_INVALID;
		}
		else {
			if(alen > MAX_CASCADE) return POERR_INVALID;
		}
		alen *= 4;

		ParaMixSave.mix.cascade.port = *buf++;
		ParaMixSave.mix.cascade.frame = *buf++;
		ParaMixSave.mix.cascade.timeout = *buf++;
		ParaMixSave.mix.cascade.timeout_byte = *buf++;
		ParaMixSave.mix.cascade.retry = *buf++;
		ParaMixSave.mix.cascade.cycle = *buf++;
		ParaMixSave.mix.cascade.flag = (*buf&0x80) ? 1 : 0;
		ParaMixSave.mix.cascade.num = *buf & 0x0f;
		memcpy(ParaMixSave.mix.cascade.addr, buf+1, alen);
	}

	return 0;
}

/**
* @brief 终端参数F59操作
*/
int ParaOperationF59(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) {
		buf[0] = ParaMixSave.mix.metabnor.diff;
		buf[1] = ParaMixSave.mix.metabnor.fly;
		HexToBcd(buf, 2);
		buf[2] = ParaMixSave.mix.metabnor.stop;
		buf[3] = ParaMixSave.mix.metabnor.time;
	}
	else {
		ParaMixSave.mix.metabnor.diff = buf[0];
		ParaMixSave.mix.metabnor.fly = buf[1];
		BcdToHex(&ParaMixSave.mix.metabnor.diff, 2);
		ParaMixSave.mix.metabnor.stop = buf[2];
		ParaMixSave.mix.metabnor.time = buf[3];
	}

	return 0;
}

/**
* @brief 终端参数F60操作
*/
int ParaOperationF60(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	int i;
	unsigned short *ps;

	if(0 == flag) {
		ps = &ParaMixSave.mix.syntony.vol_sum;
		for(i=0; i<40; i++,buf+=2) UnsignedToBcd(*ps++, buf, 2);
	}
	else {
		ps = &ParaMixSave.mix.syntony.vol_sum;
		for(i=0; i<40; i++,buf+=2) {
			buf[1] &= 0x7f;
			*ps++ = BcdToUnsigned(buf, 2);
		}
	}

	return 0;
}
