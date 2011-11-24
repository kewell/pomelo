/**
* hardware.c -- 硬件参数
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#include <stdio.h>
#include <string.h>

#define DEFINE_PARAHARDWARE

#include "param_config.h"
#include "include/debug.h"
#include "include/param/hardware.h"
#include "include/param/metp.h"
#include "include/sys/bin.h"
#include "include/param/operation.h"
#include "operation_inner.h"
#include "include/lib/bcd.h"
#include "include/lib/datatype_gb.h"

para_hardw_t ParaHardw;

#define HARDW_MAGIC	0x9200c1af

/**
* @brief 从文件中载入硬件参数
* @return 0成功, 否则失败
*/
int LoadParaHardw(void)
{
	memset(&ParaHardw, 0, sizeof(ParaHardw));

	DebugPrint(0, "  load param hardware...(size=%d)", sizeof(ParaHardw)+12);

	if(ReadBinFile(PARAM_SAVE_PATH "hardw.bin", HARDW_MAGIC, (unsigned char *)&ParaHardw, sizeof(ParaHardw)) < 0) {
		DebugPrint(0, "no main, ");
		if(ReadBinFile(PARAM_BAK_PATH "hardw.bin", HARDW_MAGIC, (unsigned char *)&ParaHardw, sizeof(ParaHardw)) < 0)
			DebugPrint(0, "no bak\n");
			return 1;
	}

	DebugPrint(0, "ok\n");
	return 0;
}

/**
* @brief 保存硬件参数
* @return 0成功, 否则失败
*/
int SaveParaHardw(void)
{
	SaveBinFile(PARAM_SAVE_PATH "hardw.bin", HARDW_MAGIC, (unsigned char *)&ParaHardw, sizeof(ParaHardw));
	SaveBinFile(PARAM_BAK_PATH "hardw.bin", HARDW_MAGIC, (unsigned char *)&ParaHardw, sizeof(ParaHardw));

	return 0;
}

/**
* @brief 终端参数F11操作
* @param flag 操作方式, 0-读, 1-写
* @param option 操作选项
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int ParaOperationF11(int flag, unsigned short metpid, param_specialop_t *option)
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
			if(portid == 0 || portid > MAX_PULSE) continue;
			if((wlen+5) > option->wlen) break;
			portid -= 1;

			*pwbuf++ = prbuf[0];
			*pwbuf++ = ParaHardw.pulse[portid].metp_id;
			*pwbuf++ = ParaHardw.pulse[portid].type;
			*pwbuf++ = ParaHardw.pulse[portid].consk & 0xff;
			*pwbuf++ = (ParaHardw.pulse[portid].consk>>8)&0xff;

			wlen += 5;
			actnum++;
		}

		option->wactlen = wlen;
		option->wbuf[0] = actnum;
	}
	else {
		int num, i, portid;
		const unsigned char *prbuf;

		num = (int)option->rbuf[0]&0xff;
		option->ractlen = num*5 + 1;	
		if(option->rlen < option->ractlen) return POERR_FATAL;

		prbuf = option->rbuf + 1;
		for(i=0; i<num; i++) {
			portid = (int)prbuf[0]&0xff;
			if(portid == 0 || portid > MAX_PULSE) {
				prbuf += 5;
				continue;
			}
			portid -= 1;
			prbuf += 1;

			ParaHardw.pulse[portid].metp_id = *prbuf++;
			ParaHardw.pulse[portid].type = *prbuf++;
			ParaHardw.pulse[portid].consk = ((unsigned short)prbuf[1]<<8) + ((unsigned short)prbuf[0]&0xff);
			prbuf += 2;
		}
		
		MappingCenMetp();
	}

	return 0;
}

/**
* @brief 终端参数F12操作
*/
int ParaOperationF12(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	if(0 == flag) {
		buf[0] = ParaHardw.isig.flagin;
		buf[1] = ParaHardw.isig.flagattr;
	}
	else {
		ParaHardw.isig.flagin = buf[0];
		ParaHardw.isig.flagattr = buf[1];
	}

	return 0;
}

/**
* @brief 终端参数F13操作(空操作)
*/
int ParaOperationF13(int flag, unsigned short metpid, param_specialop_t *option)
{
	if(0 == flag) {
		int num;

		num = (int)option->rbuf[0]&0xff;
		option->ractlen = num + 1;
		if(option->rlen < option->ractlen) return POERR_FATAL;

		return POERR_INVALID;
	}
	else {
		int num;

		num = (int)option->rbuf[0]&0xff;
		option->ractlen = num*3 + 1;	
		if(option->rlen < option->ractlen) return POERR_FATAL;

		return POERR_INVALID;
	}

	return 0;
}

/**
* @brief 终端参数F14操作
*/
int ParaOperationF14(int flag, unsigned short metpid, param_specialop_t *option)
{
	if(0 == flag) {
		int num, i, tid, actnum, tmplen;
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
			tid = (int)prbuf[0]&0xff;
			if(tid == 0 || tid > MAX_TGRP) continue;
			tid -= 1;

			tmplen = (int)ParaHardw.tgrp[tid].num&0xff;
			if((wlen+tmplen+2) > option->wlen) break;

			*pwbuf++ = prbuf[0];
			*pwbuf++ = ParaHardw.tgrp[tid].num;
			if(tmplen) smallcpy(pwbuf, ParaHardw.tgrp[tid].flag, tmplen);
			pwbuf += tmplen;

			wlen += tmplen+2;
			actnum++;
		}

		option->wactlen = wlen;
		option->wbuf[0] = actnum;
	}
	else {
		int num, i, tid, tmplen;
		const unsigned char *prbuf;

		num = (int)option->rbuf[0]&0xff;

		prbuf = option->rbuf + 1;
		option->ractlen = 1;
		for(i=0; i<num; i++) {
			tid = (int)prbuf[0]&0xff;
			tmplen = (int)prbuf[1]&0xff;
			if(tid == 0 || tid > MAX_TGRP || tmplen > 15) {
				prbuf += tmplen+2;
				continue;
			}
			tid -= 1;
			prbuf += 2;

			ParaHardw.tgrp[tid].num = tmplen;
			if(tmplen) smallcpy(ParaHardw.tgrp[tid].flag, prbuf, tmplen);
			prbuf += tmplen;

			option->ractlen += tmplen + 2;
			if(option->ractlen > option->rlen) return POERR_FATAL;
		}
	}

	return 0;
}

/**
* @brief 终端参数F15操作
*/
int ParaOperationF15(int flag, unsigned short metpid, param_specialop_t *option)
{
	if(0 == flag) {
		int num, i, diffid, actnum;
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
			diffid = (int)prbuf[0]&0xff;
			if(diffid == 0 || diffid > MAX_DIFFA) continue;
			if((wlen+9) > option->wlen) break;
			diffid -= 1;

			*pwbuf++ = prbuf[0];
			*pwbuf++ = ParaHardw.diffa[diffid].tgrp_con;
			*pwbuf++ = ParaHardw.diffa[diffid].tgrp_ref;
			*pwbuf++ = ParaHardw.diffa[diffid].flag;
			*pwbuf++ = ParaHardw.diffa[diffid].percent;
			EneToGbformat03(ParaHardw.diffa[diffid].absdiff, pwbuf);
			pwbuf += 4;

			wlen += 9;
			actnum++;
		}

		option->wactlen = wlen;
		option->wbuf[0] = actnum;
	}
	else {
		int num, i, diffid;
		const unsigned char *prbuf;

		num = (int)option->rbuf[0]&0xff;
		option->ractlen = num*9 + 1;	
		if(option->rlen < option->ractlen) return POERR_FATAL;

		prbuf = option->rbuf + 1;
		for(i=0; i<num; i++) {
			diffid = (int)prbuf[0]&0xff;
			if(diffid == 0 || diffid > MAX_DIFFA) {
				prbuf += 9;
				continue;
			}
			diffid -= 1;
			prbuf += 1;

			ParaHardw.diffa[diffid].tgrp_con = *prbuf++;
			ParaHardw.diffa[diffid].tgrp_ref = *prbuf++;
			ParaHardw.diffa[diffid].flag = *prbuf++;
			ParaHardw.diffa[diffid].percent = *prbuf++;
			ParaHardw.diffa[diffid].absdiff = Gbformat03ToEne(prbuf);
			prbuf += 4;
		}
	}

	return 0;
}

