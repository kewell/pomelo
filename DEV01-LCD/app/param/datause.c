/**
* datause.c -- 支持数据配置参数
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#include <stdio.h>
#include <string.h>

#define DEFINE_PARADATAUSE

#include "param_config.h"
#include "include/debug.h"
#include "include/param/datause.h"
#include "include/sys/bin.h"
#include "include/param/operation.h"
#include "operation_inner.h"

para_datause_t ParaDataUse;

const cfg_datafns_t ValidDataCls1_1 = {
	19,
	{
	0xfe, 0x03, 0x00, 0xff,
	0xff, 0xff, 0x01, 0x03,
	0x00, 0x00, 0x00, 0xff,
	0x7f, 0xff, 0x0f, 0x00,
	0xff, 0xff, 0xff, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 
	}
};

const cfg_datafns_t ValidDataCls2_1 = {
	14,
	{
	0xff, 0x0f, 0xff, 0xff,
	0xff, 0x0c, 0x3f, 0x00,
	0x00, 0x00, 0xff, 0xff,
	0xff, 0x3f, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	}
};

const cfg_datafns_t ValidDataCls1_2 = {
	17,
	{
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	}
};

const cfg_datafns_t ValidDataCls2_2 = {
	23,
	{
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	}
};

/**
* @brief 载入缺省支持数据参数
*/
static void LoadDefParaDataUse(void)
{
	memset(&ParaDataUse, 0, sizeof(ParaDataUse));

	memcpy(&ParaDataUse.datacls1[1].met[0].num, &ValidDataCls1_1.num, sizeof(cfg_datafns_t));
	memcpy(&ParaDataUse.datacls1[2].met[0].num, &ValidDataCls1_1.num, sizeof(cfg_datafns_t));
	memcpy(&ParaDataUse.datacls1[3].met[0].num, &ValidDataCls1_2.num, sizeof(cfg_datafns_t));
	memcpy(&ParaDataUse.datacls1[4].met[0].num, &ValidDataCls1_2.num, sizeof(cfg_datafns_t));
	memcpy(&ParaDataUse.datacls1[5].met[0].num, &ValidDataCls1_1.num, sizeof(cfg_datafns_t));

	memcpy(&ParaDataUse.datacls2[1].met[0].num, &ValidDataCls2_1.num, sizeof(cfg_datafns_t));
	memcpy(&ParaDataUse.datacls2[2].met[0].num, &ValidDataCls2_1.num, sizeof(cfg_datafns_t));
	memcpy(&ParaDataUse.datacls2[3].met[0].num, &ValidDataCls2_2.num, sizeof(cfg_datafns_t));
	memcpy(&ParaDataUse.datacls2[4].met[0].num, &ValidDataCls2_2.num, sizeof(cfg_datafns_t));
	memcpy(&ParaDataUse.datacls2[5].met[0].num, &ValidDataCls2_1.num, sizeof(cfg_datafns_t));
}

#define DATAUSE_MAGIC	0x9572acd0

/**
* @brief 从文件中载入支持数据参数
* @return 0成功, 否则失败
*/
int LoadParaDataUse(void)
{
	LoadDefParaDataUse();

	DebugPrint(0, "  load param datause...(size=%d)", sizeof(ParaDataUse)+12);

	if(ReadBinFile(PARAM_SAVE_PATH "datause.bin", DATAUSE_MAGIC, (unsigned char *)&ParaDataUse, sizeof(ParaDataUse)) < 0) {
		DebugPrint(0, "no main, ");
		if(ReadBinFile(PARAM_BAK_PATH "datause.bin", DATAUSE_MAGIC, (unsigned char *)&ParaDataUse, sizeof(ParaDataUse)) < 0)
			DebugPrint(0, "no bak\n");
			return 1;
	}

	DebugPrint(0, "ok\n");
	return 0;
}

/**
* @brief 保存支持数据参数
* @return 0成功, 否则失败
*/
int SaveParaDataUse(void)
{
	SaveBinFile(PARAM_SAVE_PATH "datause.bin", DATAUSE_MAGIC, (unsigned char *)&ParaDataUse, sizeof(ParaDataUse));
	SaveBinFile(PARAM_BAK_PATH "datause.bin", DATAUSE_MAGIC, (unsigned char *)&ParaDataUse, sizeof(ParaDataUse));

	return 0;
}

/**
* @brief 终端参数F38操作
* @param flag 操作方式, 0-读, 1-写
* @param option 操作选项
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int ParaOperationF38(int flag, unsigned short metpid, param_specialop_t *option)
{
	if(0 == flag) {
		int usercls, num, i, metcls, actnum;
		const unsigned char *prbuf;
		unsigned char *pwbuf;
		unsigned short wlen, tmplen;

		usercls = (int)option->rbuf[0]&0xff;
		num = (int)option->rbuf[1]&0xff;
		option->ractlen = num + 2;
		if(option->rlen < option->ractlen) return POERR_FATAL;
		if(usercls == 0 || usercls > MAX_USERCLASS) return POERR_INVALID;
		usercls -= 1;

		wlen = 2;
		prbuf = option->rbuf + 2;
		pwbuf = option->wbuf + 2;
		actnum = 0;
		for(i=0; i<num; i++,prbuf++) {
			metcls = (int)prbuf[0]&0xff;
			if(metcls == 0 || metcls > MAX_METCLASS) continue;
			metcls -= 1;

			tmplen = (int)ParaDataUse.datacls1[usercls].met[metcls].num & 0xff;
			if((wlen+tmplen+2) > option->wlen) break;

			*pwbuf++ = prbuf[0];
			*pwbuf++ = ParaDataUse.datacls1[usercls].met[metcls].num;
			if(tmplen) memcpy(pwbuf, ParaDataUse.datacls1[usercls].met[metcls].fnmask, tmplen);
			pwbuf += tmplen;

			wlen += tmplen+2;
			actnum++;
		}

		option->wactlen = wlen;
		option->wbuf[0] = usercls+1;
		option->wbuf[1] = actnum;
	}
	else {
		int usercls, num, i, metcls;
		const unsigned char *prbuf;
		int tmplen;

		usercls = (int)option->rbuf[0]&0xff;
		num = (int)option->rbuf[1]&0xff;
		if(usercls == 0 || usercls > MAX_USERCLASS) return POERR_FATAL;
		usercls -= 1;

		option->ractlen = 2;
		prbuf = option->rbuf + 2;
		for(i=0; i<num; i++) {
			metcls = (int)(*prbuf++)&0xff;
			if(metcls == 0 || metcls > MAX_METCLASS) return POERR_FATAL;
			metcls -= 1;
			prbuf++;

			tmplen = (int)(*prbuf++)&0xff;
			option->ractlen += tmplen+2;
			if(option->ractlen > option->rlen) return POERR_FATAL;
			if(tmplen > 31) tmplen = 31;

			ParaDataUse.datacls1[usercls].met[metcls].num = tmplen;
			if(tmplen) memcpy(ParaDataUse.datacls1[usercls].met[metcls].fnmask, prbuf, tmplen);
			prbuf += tmplen;
		}
	}

	return 0;
}

/**
* @brief 终端参数F39操作
* @param flag 操作方式, 0-读, 1-写
* @param option 操作选项
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int ParaOperationF39(int flag, unsigned short metpid, param_specialop_t *option)
{
	if(0 == flag) {
		int usercls, num, i, metcls, actnum;
		const unsigned char *prbuf;
		unsigned char *pwbuf;
		unsigned short wlen, tmplen;

		usercls = (int)option->rbuf[0]&0xff;
		num = (int)option->rbuf[1]&0xff;
		option->ractlen = num + 2;
		if(option->rlen < option->ractlen) return POERR_FATAL;
		if(usercls == 0 || usercls > MAX_USERCLASS) return POERR_INVALID;
		usercls -= 1;

		wlen = 2;
		prbuf = option->rbuf + 2;
		pwbuf = option->wbuf + 2;
		actnum = 0;
		for(i=0; i<num; i++,prbuf++) {
			metcls = (int)prbuf[0]&0xff;
			if(metcls == 0 || metcls > MAX_METCLASS) continue;
			metcls -= 1;

			tmplen = (int)ParaDataUse.datacls2[usercls].met[metcls].num & 0xff;
			if((wlen+tmplen+2) > option->wlen) break;

			*pwbuf++ = prbuf[0];
			*pwbuf++ = ParaDataUse.datacls2[usercls].met[metcls].num;
			if(tmplen) memcpy(pwbuf, ParaDataUse.datacls2[usercls].met[metcls].fnmask, tmplen);
			pwbuf += tmplen;

			wlen += tmplen+2;
			actnum++;
		}

		option->wactlen = wlen;
		option->wbuf[0] = usercls+1;
		option->wbuf[1] = actnum;
	}
	else {
		int usercls, num, i, metcls;
		const unsigned char *prbuf;
		int tmplen;

		usercls = (int)option->rbuf[0]&0xff;
		num = (int)option->rbuf[1]&0xff;
		if(usercls == 0 || usercls > MAX_USERCLASS) return POERR_FATAL;
		usercls -= 1;

		option->ractlen = 2;
		prbuf = option->rbuf + 2;
		for(i=0; i<num; i++) {
			metcls = (int)(*prbuf++)&0xff;
			if(metcls == 0 || metcls > MAX_METCLASS) return POERR_FATAL;
			metcls -= 1;
			prbuf++;

			tmplen = (int)(*prbuf++)&0xff;
			option->ractlen += tmplen+2;
			if(option->ractlen > option->rlen) return POERR_FATAL;
			if(tmplen > 31) tmplen = 31;

			ParaDataUse.datacls2[usercls].met[metcls].num = tmplen;
			if(tmplen) memcpy(ParaDataUse.datacls2[usercls].met[metcls].fnmask, prbuf, tmplen);
			prbuf += tmplen;
		}
	}

	return 0;
}

