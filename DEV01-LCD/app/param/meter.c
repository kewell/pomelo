/**
* meter.c -- 表计参数
* 
* 
* 创建时间: 2010-5-7
* 最后修改时间: 2010-5-7
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFINE_PARAMETER

#include "param_config.h"
#include "include/debug.h"
#include "include/param/meter.h"
#include "include/param/commport.h"
#include "include/sys/grpbin.h"
#include "include/param/operation.h"
#include "operation_inner.h"
#include "include/param/metp.h"
#include "include/debug/shellcmd.h"

para_meter_t ParaMeter[MAX_METER];
static unsigned char meter_param_change = 0;


#define METER_PARAM_MAGIC	0x872563aa


int get_meter_param_change_flag(void)
{
	return (meter_param_change);
}

void clear_meter_param_change_flag(void)
{
	meter_param_change = 0;
}

void set_meter_param_change_flag(void)
{
	meter_param_change = 1;
}


/**
* @brief 从文件中载入表计参数
* @return 0成功, 否则失败
*/
int LoadParaMeter(void)
{
	#define MEMCACHE_SIZE	(sizeof(para_meter_t)+40)
	unsigned char memcache[MEMCACHE_SIZE];
	grpbin_ref_t ref;
	int i, readlen, retry=0;
	para_meter_t *pdoc = (para_meter_t *)memcache;

	DebugPrint(0, "  load meter param(maxsize=%d,itemlen=%d)...", 
			(sizeof(para_meter_t)+2)*MAX_METER+12, sizeof(para_meter_t));

	if(OpenGrpBinFile(PARAM_SAVE_PATH "meter.gin", METER_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no main, ");
		goto mark_fail;
	}

mark_retry:

	if(ref.itemlen > MEMCACHE_SIZE) {
		ErrorLog("invalid itemlen(%d)\n", ref.itemlen);
		CloseGrpBinFile(&ref);
		goto mark_fail;
		
	}

	if(ref.itemnum > MAX_METER) {
		ErrorLog("too long itemnum(%d)\n", ref.itemnum);
		ref.itemnum = MAX_METER;
	}

	for(i=0; i<ref.itemnum; i++) {
		readlen = ReadGrpBinFileItem(&ref, memcache, MEMCACHE_SIZE);
		if(readlen < 0) break;

		if(pdoc->index >= MAX_METER) continue;

		if(readlen > sizeof(para_meter_t)) readlen = sizeof(para_meter_t);
		memcpy(&ParaMeter[pdoc->index], memcache, readlen);
	}

	DebugPrint(0, "ok\n");

	CloseGrpBinFile(&ref);
	return 0;

mark_fail:
	if(retry) {
		DebugPrint(0, "fail\n");
		return 1;
	}
	retry = 1;

	if(OpenGrpBinFile(PARAM_BAK_PATH "meter.gin", METER_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no bak\n");
		return 1;
	}

	goto mark_retry;
}

/**
* @brief 保存表计参数
* @return 0成功, 否则失败
*/
int SaveParaMeter(void)
{
	grpbin_ref_t ref;
	int i, retry = 0;


	ref.itemlen = sizeof(para_meter_t);
	ref.itemnum = MAX_METER;
	if(OpenGrpBinFile(PARAM_SAVE_PATH "meter.gin", METER_PARAM_MAGIC, 'w', &ref)) return 1;

mark_retry:
	for(i=0; i<MAX_METER; i++) {
		if(ParaMeter[i].metp_id == 0) continue;

		ParaMeter[i].index = i;
		WriteGrpBinFileItem(&ref, (unsigned char *)&ParaMeter[i]);
	}

	CloseGrpBinFile(&ref);

	if(retry) return 0;
	retry = 1;
	ref.itemlen = sizeof(para_meter_t);
	ref.itemnum = MAX_METER;
	if(OpenGrpBinFile(PARAM_BAK_PATH "meter.gin", METER_PARAM_MAGIC, 'w', &ref)) return 1;
	goto mark_retry;
	
	return 0;
}

/**
* @brief 终端参数F10操作
* @param flag 操作方式, 0-读, 1-写
* @param option 操作选项
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
*/
int ParaOperationF10(int flag, unsigned short metpid, param_specialop_t *option)
{

	if(0 == flag) 
	{
		unsigned short num, i, metid, actnum;
		const unsigned char *prbuf;
		unsigned char *pwbuf;
		unsigned short wlen;

		printf("ReadParaOperationF10\n");
		num = ((unsigned short)option->rbuf[1]<<8) + ((unsigned short)option->rbuf[0]&0xff);
		option->ractlen = ((int)num<<1) + 2;
		if(option->rlen < option->ractlen) return POERR_FATAL;

		wlen = 2;
		prbuf = option->rbuf + 2;
		pwbuf = option->wbuf + 2;
		actnum = 0;
		for(i=0; i<num; i++,prbuf+=2) 
		{
			metid = ((unsigned short)prbuf[1]<<8) + ((unsigned short)prbuf[0]&0xff);
			if(metid == 0 || metid > MAX_METER) continue;
			if((wlen+27) > option->wlen) break;
			metid -= 1;

			*pwbuf++ = prbuf[0];
			*pwbuf++ = prbuf[1];
			memcpy(pwbuf, &ParaMeter[metid].metp_id, 24); pwbuf += 24;
			*pwbuf++ = (ParaMeter[metid].userclass<<4) + (ParaMeter[metid].metclass&0x0f);
			wlen += 27;
			actnum++;
		}

		option->wactlen = wlen;
		option->wbuf[0] = actnum;
		option->wbuf[1] = actnum>>8;
	}
	else 
	{
		unsigned short num, i, metid, metpid;
		const unsigned char *prbuf;
		int cenmet_remap = 0;
		
		printf("WriteParaOperationF10\n");
		num = ((unsigned short)option->rbuf[1]<<8) + ((unsigned short)option->rbuf[0]&0xff);
		option->ractlen = (int)num*27 + 2;		
		if(option->rlen < option->ractlen) return POERR_FATAL;

		prbuf = option->rbuf + 2;
		for(i=0; i<num; i++) 
		{
			metid = ((unsigned short)prbuf[1]<<8) + ((unsigned short)prbuf[0]&0xff);
			metpid = ((unsigned short)prbuf[3]<<8) + ((unsigned short)prbuf[2]&0xff);
			if(metid == 0 || metid > MAX_METER) goto mark_skip;
			else if(metpid != 0) 
			{
				if(metid > MAX_CENMETP) 
				{
					if(metpid != metid) goto mark_skip;
				}
				else 
				{
					if(metpid > MAX_CENMETP) goto mark_skip;
					cenmet_remap = 1;
				}
			}
			metid -= 1;
			prbuf += 2;

			memcpy(&ParaMeter[metid].metp_id, prbuf, 24); prbuf += 24;
			ParaMeter[metid].userclass = *prbuf >> 4;
			ParaMeter[metid].metclass = *prbuf & 0x0f;
			prbuf++;
			if((ParaMeter[metid].portcfg & 0x1F) == PLC_PORT && (ParaMeter[metid].metp_id >= 3))
			{
				set_meter_param_change_flag();
			}
			continue;

		mark_skip:
			prbuf += 27;
		}

		if(cenmet_remap) MappingCenMetp();
	}

	return 0;
}




static int shell_createplmetparam(int argc, char *argv[])
{
	int num, metid;

	if(2 != argc) {
		PrintLog(0, "usage: cplmet num\n");
		return 1;
	}

	num = atoi(argv[1]);
	if(num < 0 || num > MAX_PLCMET) {
		PrintLog(0, "invalid num\n");
		return 1;
	}
	
	memset(ParaMeter, 0, sizeof(ParaMeter));

	num += PLC_BASEMETP;
	for(metid=PLC_BASEMETP; metid<num; metid++) {
		ParaMeter[metid].metp_id = metid+1;

		ParaMeter[metid].proto = METTYPE_PLC;
		ParaMeter[metid].addr[0] = metid&0xff;
		ParaMeter[metid].addr[1] = (metid>>8) & 0xff;
	}

	SaveParaMeter();

	PrintLog(0, "create plmet param ok\n");
	return 0;
}
DECLARE_SHELL_CMD("cplmet", shell_createplmetparam, "create plc meter param");

/*void ParaMeterTest(void)
{
	int i;
	printf("start test parameter...\n");
	if(0) {
		for(i=0; i<MAX_METER; i++) {
			ParaMeter[i].metp_id = i+1;
		}
		for(i=0; i<6; i++) ParaMeter[10].addr[i] = i;

		SaveParaMeter();
	}
	else {
		int bprinted = 0;
		int errnum = 0;
		for(i=0; i<MAX_METER; i++) {
			if(ParaMeter[i].metp_id != (i+1)) {
				if(!bprinted) {
					printf("met %d error(%d)\n", i, ParaMeter[i].metp_id);
					bprinted = 1;
				}
				errnum++;
			}
		}

		printf("%d errors\n", errnum);
		for(i=0; i<6; i++) printf("%d ", ParaMeter[10].addr[i]);
		printf("\n");
	}
	printf("end\n");
}*/

