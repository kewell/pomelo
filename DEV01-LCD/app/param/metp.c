/**
* metp.c -- 测量点参数
* 
* 
* 创建时间: 2010-5-7
* 最后修改时间: 2010-5-7
*/

#include <stdio.h>
#include <string.h>

#define DEFINE_PARAMETP

#include "param_config.h"
#include "include/debug.h"
#include "include/param/metp.h"
#include "include/param/hardware.h"
#include "include/sys/grpbin.h"
#include "include/param/operation.h"
#include "operation_inner.h"
#include "include/param/meter.h"
#include "include/lib/bcd.h"

para_cenmetp_t ParaCenMetp[MAX_CENMETP];
para_plcmetp_t ParaPlcMetp[MAX_METP];
para_childend_t ParaChildEnd[MAX_METP];
metp_map_t CenMetpMap[MAX_CENMETP];

#define CMETP_PARAM_MAGIC	0x782234f6
#define PMETP_PARAM_MAGIC	0x78e057f8
#define CLIEND_PARAM_MAGIC	0xa802c416

#define METP_MEMCACHE_SIZE	(sizeof(para_cenmetp_t)+128)
static unsigned char MetpCache[METP_MEMCACHE_SIZE];

/**
* @brief 映射测量点对应表号和类型
*/
void MappingCenMetp(void)
{
	unsigned short id, metpid;
	
	memset(CenMetpMap, 0, sizeof(CenMetpMap));

	for(id=0; id<MAX_PULSE; id++) {
		metpid = ParaHardw.pulse[id].metp_id;
		if(metpid != 0 && metpid <= MAX_CENMETP) {
			metpid -= 1;
			CenMetpMap[metpid].type = METP_PULSE;
			CenMetpMap[metpid].metid = id;
		}
	}

	for(id=0; id<MAX_CENMETP; id++) {
		metpid = ParaMeter[id].metp_id;
		if(metpid != 0 && metpid <= MAX_CENMETP) {
			metpid -= 1;
			CenMetpMap[metpid].type = METP_METER;
			CenMetpMap[metpid].metid = id;
		}
	}
}


/**
* @brief 从文件中载入多功能测量点参数
* @return 0成功, 否则失败
*/
int LoadParaCenMetp(void)
{
	grpbin_ref_t ref;
	int i, readlen, retry = 0;
	para_cenmetp_t *pdoc = (para_cenmetp_t *)MetpCache;

	DebugPrint(0, "  load param cenmetp(maxsize=%d)...",
			(sizeof(para_cenmetp_t)+2)*MAX_CENMETP+12);


	if(OpenGrpBinFile(PARAM_SAVE_PATH "cenmetp.gin", CMETP_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no main, ");
		goto mark_fail;
	}

mark_retry:

	if(ref.itemlen > METP_MEMCACHE_SIZE) {
		ErrorLog("invalid itemlen(%d)\n", ref.itemlen);
		CloseGrpBinFile(&ref);
		goto mark_fail;
	}

	if(ref.itemnum > MAX_CENMETP) {
		ErrorLog("too long itemnum(%d)\n", ref.itemnum);
		ref.itemnum = MAX_CENMETP;
	}

	for(i=0; i<ref.itemnum; i++) {
		readlen = ReadGrpBinFileItem(&ref, MetpCache, METP_MEMCACHE_SIZE);
		if(readlen < 0) break;

		if(pdoc->mid >= MAX_CENMETP) continue;

		if(readlen > sizeof(para_cenmetp_t)) readlen = sizeof(para_cenmetp_t);
		memcpy(&ParaCenMetp[pdoc->mid], MetpCache, readlen);
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

	if(OpenGrpBinFile(PARAM_BAK_PATH "cenmetp.gin", CMETP_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no bak\n");
		return 1;
	}
	goto mark_retry;
}

/**
* @brief 保存多功能测量点参数
* @return 0成功, 否则失败
*/
int SaveParaCenMetp(void)
{
	grpbin_ref_t ref;
	int i, retry = 0;

	ref.itemlen = sizeof(para_cenmetp_t);
	ref.itemnum = MAX_CENMETP;
	if(OpenGrpBinFile(PARAM_SAVE_PATH "cenmetp.gin", CMETP_PARAM_MAGIC, 'w', &ref)) return 1;

mark_retry:
	for(i=0; i<MAX_CENMETP; i++) {
		ParaCenMetp[i].mid = i;
		WriteGrpBinFileItem(&ref, (unsigned char *)&ParaCenMetp[i]);
	}

	CloseGrpBinFile(&ref);

	if(retry) return 0;
	retry = 1;

	ref.itemlen = sizeof(para_cenmetp_t);
	ref.itemnum = MAX_CENMETP;
	if(OpenGrpBinFile(PARAM_BAK_PATH "cenmetp.gin", CMETP_PARAM_MAGIC, 'w', &ref)) return 1;
	goto mark_retry;
	return 0;
}

/**
* @brief 从文件中载入载波测量点参数
* @return 0成功, 否则失败
*/
int LoadParaPlcMetp(void)
{
	grpbin_ref_t ref;
	int i, readlen, retry = 0;
	para_plcmetp_t *pdoc = (para_plcmetp_t *)MetpCache;

	DebugPrint(0, "  load param plcmetp...(maxsize=%d)",
			(sizeof(para_plcmetp_t)+2)*MAX_METP+12);

	if(OpenGrpBinFile(PARAM_SAVE_PATH "plcmetp.gin", PMETP_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no main, ");
		goto mark_fail;
	}

mark_retry:
	if(ref.itemlen > METP_MEMCACHE_SIZE) {
		ErrorLog("invalid itemlen(%d)\n", ref.itemlen)
		CloseGrpBinFile(&ref);
		goto mark_fail;
	}

	if(ref.itemnum > MAX_METP) {
		ErrorLog("too long itemnum(%d)\n", ref.itemnum);
		ref.itemnum = MAX_METP;
	}

	for(i=0; i<ref.itemnum; i++) {
		readlen = ReadGrpBinFileItem(&ref, MetpCache, METP_MEMCACHE_SIZE);
		if(readlen < 0) break;

		if(pdoc->mid >= MAX_METP || pdoc->mid < MAX_CENMETP) continue;

		if(readlen > sizeof(para_plcmetp_t)) readlen = sizeof(para_plcmetp_t);
		memcpy(&ParaPlcMetp[pdoc->mid], MetpCache, readlen);
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

	if(OpenGrpBinFile(PARAM_BAK_PATH "plcmetp.gin", PMETP_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no bak\n");
		return 1;
	}
	goto mark_retry;
}

/**
* @brief 保存载波测量点参数
* @return 0成功, 否则失败
*/
int SaveParaPlcMetp(void)
{
	grpbin_ref_t ref;
	int i, retry = 0;
	const para_meter_t *pmeter = ParaMeter;

	ref.itemlen = sizeof(para_plcmetp_t);
	ref.itemnum = MAX_METP;
	if(OpenGrpBinFile(PARAM_SAVE_PATH "plcmetp.gin", PMETP_PARAM_MAGIC, 'w', &ref)) return 1;

mark_retry:
	for(i=MAX_CENMETP; i<MAX_METP; i++) {
		if(pmeter[i].metp_id == 0 || pmeter[i].proto != 31) continue;
		//if(MetpMap[i].type != METP_METER || MetpMap[i].metid >= MAX_METER) continue;
		//if(pmeter[MetpMap[i].metid].proto != 31) continue;

		ParaPlcMetp[i].mid = i;
		WriteGrpBinFileItem(&ref, (unsigned char *)&ParaPlcMetp[i]);
	}

	CloseGrpBinFile(&ref);

	if(retry) return 0;
	retry = 1;

	ref.itemlen = sizeof(para_plcmetp_t);
	ref.itemnum = MAX_METP;
	if(OpenGrpBinFile(PARAM_BAK_PATH "plcmetp.gin", PMETP_PARAM_MAGIC, 'w', &ref)) return 1;
	goto mark_retry;
	return 0;
}

/**
* @brief 从文件中载入载波从节点参数
* @return 0成功, 否则失败
*/
int LoadParaChildEnd(void)
{
	grpbin_ref_t ref;
	int i, readlen;
	para_childend_t *pdoc = (para_childend_t *)MetpCache;

	DebugPrint(0, "  load param childend...(maxize=%d)",
			(sizeof(para_childend_t)+2)*MAX_METP+12);

	if(OpenGrpBinFile(PARAM_SAVE_PATH "childend.gin", CLIEND_PARAM_MAGIC, 'r', &ref)) {
		DebugPrint(0, "no main\n");
		return 1;
	}

	if(ref.itemlen > METP_MEMCACHE_SIZE) {
		ErrorLog("invalid itemlen(%d)\n", ref.itemlen)
		CloseGrpBinFile(&ref);
		return 1;
	}

	if(ref.itemnum > MAX_METP) {
		ErrorLog("too long itemnum(%d)\n", ref.itemnum);
		ref.itemnum = MAX_METP;
	}

	for(i=0; i<ref.itemnum; i++) {
		readlen = ReadGrpBinFileItem(&ref, MetpCache, METP_MEMCACHE_SIZE);
		if(readlen < 0) break;

		if(pdoc->mid >= MAX_METP || pdoc->mid < MAX_CENMETP) continue;

		if(readlen > sizeof(para_childend_t)) readlen = sizeof(para_childend_t);
		memcpy(&ParaChildEnd[pdoc->mid], MetpCache, readlen);
	}

	DebugPrint(0, "ok\n");

	CloseGrpBinFile(&ref);
	return 0;
}

/**
* @brief 保存载波从节点参数
* @return 0成功, 否则失败
*/
int SaveParaChildEnd(void)
{
	grpbin_ref_t ref;
	int i;

	ref.itemlen = sizeof(para_plcmetp_t);
	ref.itemnum = MAX_METP;
	if(OpenGrpBinFile(PARAM_SAVE_PATH "childend.gin", CLIEND_PARAM_MAGIC, 'w', &ref)) return 1;

	for(i=MAX_CENMETP; i<MAX_METP; i++) {
		if(ParaChildEnd[i].num == 0 || ParaChildEnd[i].num > 20) continue;

		ParaChildEnd[i].mid = i;
		WriteGrpBinFileItem(&ref, (unsigned char *)&ParaChildEnd[i]);
	}

	CloseGrpBinFile(&ref);
	return 0;
}

/**
* @brief 终端参数F25操作
* @param flag 操作方式, 0-读, 1-写
* @param metpid 测量点号
* @param buf 缓存区指针
* @param len 缓存区长度
* @param actlen 有效数据长度(由函数返回)
* @return 0成功, 否则失败(参看POERR_XXX宏定义)
* @note 以下同类参数和返回值相同, 不做重复注释
*/
int ParaOperationF25(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned short mid = metpid - 1;

	if(0 == flag) {
		if(mid >= MAX_CENMETP) {
			memset(buf, 0, 11);
			//buf[10] = (ParaPlcMetp[mid].phase<<2)&0x0c;
		}
		else {
			buf[0] = ParaCenMetp[mid].base.pt & 0xff;
			buf[1] = ParaCenMetp[mid].base.pt >> 8;
			buf[2] = ParaCenMetp[mid].base.ct & 0xff;
			buf[3] = ParaCenMetp[mid].base.ct >> 8;
			UnsignedToBcd(ParaCenMetp[mid].base.vol_rating, &buf[4], 2);
			UnsignedToBcd(ParaCenMetp[mid].base.amp_rating/10, &buf[6], 1);
			UnsignedToBcd(ParaCenMetp[mid].base.pwr_rating, &buf[7], 3);
			buf[10] = ((ParaCenMetp[mid].base.pwrphase&0x03)<<2) + (ParaCenMetp[mid].base.pwrtype&0x03);
		}
	}
	else {
		if(mid >= MAX_CENMETP) {
			//ParaPlcMetp[mid].phase = (buf[10]>>2)&0x03;
			//SetSaveParamFlag(SAVEFLAG_METP);
		}
		else {
			ParaCenMetp[mid].base.pt = ((unsigned short)buf[1]<<8) + ((unsigned short)buf[0]);
			ParaCenMetp[mid].base.ct = ((unsigned short)buf[3]<<8) + ((unsigned short)buf[2]);
			ParaCenMetp[mid].base.vol_rating = BcdToUnsigned(&buf[4], 2);
			ParaCenMetp[mid].base.amp_rating = BcdToUnsigned(&buf[6], 1) * 10;
			ParaCenMetp[mid].base.pwr_rating = BcdToUnsigned(&buf[7], 3);
			ParaCenMetp[mid].base.pwrtype = buf[10] & 0x03;
			ParaCenMetp[mid].base.pwrphase = (buf[10]>>2)&0x03;
		}
	}

	return 0;
}

/**
* @brief 根据标准值和恢复值计算恢复系数
* @param value 标准值
* @param restorev 恢复值
* @param buf 返回计算后的恢复系数(BCD码)
*/
static void CalRestoreRate(unsigned int value, unsigned int restorev, unsigned char *buf)
{
	unsigned int i;

	if(0 == value) {
		buf[0] = buf[1] = 0;
		return;
	}

	if(restorev > value) i = restorev - value;
	else i = value - restorev;

	i = (i * 1000) / value;
	if(i > 1000) i = 1000;

	UnsignedToBcd(i, buf, 2);
}

/**
* @brief 根据标准值和恢复系数计算恢复值
* @param value 标准值
* @param flag 1-减, 0-加
* @param buf 恢复系数(BCD码)
* @return 恢复值
*/
static unsigned int CalRestoreValue(unsigned int value, int flag, const unsigned char *buf)
{
	unsigned int i;
	unsigned char tmp[2];

	if(0 == value) return 0;

	tmp[0] = buf[0];
	tmp[1] = buf[1] & 0x0f;

	i = BcdToUnsigned(tmp, 2);
	if(i > 1000) i = 1000;

	if(flag) i = 1000 - i;
	else i = 1000 + i;

	i = (i * (unsigned int)value) / 1000;

	return i;
}

/**
* @brief 终端参数F26操作
*/
int ParaOperationF26(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned short mid = metpid - 1;
	unsigned char *puc = buf;

	if(0 == flag) {
		if(mid >= MAX_CENMETP) {
			memset(buf, 0, 54);
		}
		else {
			UnsignedToBcd(ParaCenMetp[mid].limit.volok_up, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].limit.volok_low, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].limit.vol_lack, puc, 2); puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.vol_over, puc, 2); puc += 2;
			*puc++ = ParaCenMetp[mid].limit.time_volover;
			CalRestoreRate(ParaCenMetp[mid].limit.vol_over, ParaCenMetp[mid].limit.restore_volover, puc);
			puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.vol_less, puc, 2); puc += 2;
			*puc++ = ParaCenMetp[mid].limit.time_volless;
			CalRestoreRate(ParaCenMetp[mid].limit.vol_less, ParaCenMetp[mid].limit.restore_volless, puc);
			puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.amp_over, puc, 2); puc += 2;
			*puc++ = ParaCenMetp[mid].limit.time_ampover;
			CalRestoreRate(ParaCenMetp[mid].limit.amp_over, ParaCenMetp[mid].limit.restore_ampover, puc);
			puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.amp_limit, puc, 2); puc += 2;
			*puc++ = ParaCenMetp[mid].limit.time_amplimit;
			CalRestoreRate(ParaCenMetp[mid].limit.amp_limit, ParaCenMetp[mid].limit.restore_amplimit, puc);
			puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.zamp_limit, puc, 2); puc += 2;
			*puc++ = ParaCenMetp[mid].limit.time_zamp;
			CalRestoreRate(ParaCenMetp[mid].limit.zamp_limit, ParaCenMetp[mid].limit.restore_zamp, puc);
			puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.pwr_over, puc, 3); puc += 3;
			*puc++ = ParaCenMetp[mid].limit.time_pwrover;
			CalRestoreRate(ParaCenMetp[mid].limit.pwr_over, ParaCenMetp[mid].limit.restore_pwrover, puc);
			puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.pwr_limit, puc, 3); puc += 3;
			*puc++ = ParaCenMetp[mid].limit.time_pwrlimit;
			CalRestoreRate(ParaCenMetp[mid].limit.pwr_limit, ParaCenMetp[mid].limit.restore_pwrlimit, puc);
			puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.vol_unb, puc, 2); puc += 2;
			*puc++ = ParaCenMetp[mid].limit.time_volunb;
			CalRestoreRate(ParaCenMetp[mid].limit.vol_unb, ParaCenMetp[mid].limit.restore_volunb, puc);
			puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].limit.amp_unb, puc, 2); puc += 2;
			*puc++ = ParaCenMetp[mid].limit.time_ampunb;
			CalRestoreRate(ParaCenMetp[mid].limit.amp_unb, ParaCenMetp[mid].limit.restore_ampunb, puc);
			puc += 2;

			*puc = ParaCenMetp[mid].limit.time_volless_2;			
		}
	}
	else {
		if(mid < MAX_CENMETP) {
			ParaCenMetp[mid].limit.volok_up = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.volok_low = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.vol_lack = BcdToUnsigned(puc, 2); puc += 2;

			ParaCenMetp[mid].limit.vol_over = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.time_volover = *puc++;
			ParaCenMetp[mid].limit.restore_volover = CalRestoreValue(ParaCenMetp[mid].limit.vol_over, 1, puc);
			puc += 2;

			ParaCenMetp[mid].limit.vol_less = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.time_volless = *puc++;
			ParaCenMetp[mid].limit.restore_volless = CalRestoreValue(ParaCenMetp[mid].limit.vol_less, 0, puc);
			puc += 2;

			ParaCenMetp[mid].limit.amp_over = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.time_ampover = *puc++;
			ParaCenMetp[mid].limit.restore_ampover = CalRestoreValue(ParaCenMetp[mid].limit.amp_over, 1, puc);
			puc += 2;

			ParaCenMetp[mid].limit.amp_limit = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.time_amplimit = *puc++;
			ParaCenMetp[mid].limit.restore_amplimit = CalRestoreValue(ParaCenMetp[mid].limit.amp_limit, 1, puc);
			puc += 2;

			ParaCenMetp[mid].limit.zamp_limit = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.time_zamp = *puc++;
			ParaCenMetp[mid].limit.restore_zamp = CalRestoreValue(ParaCenMetp[mid].limit.zamp_limit, 1, puc);
			puc += 2;

			ParaCenMetp[mid].limit.pwr_over = BcdToUnsigned(puc, 3); puc += 3;
			ParaCenMetp[mid].limit.time_pwrover = *puc++;
			ParaCenMetp[mid].limit.restore_pwrover = CalRestoreValue(ParaCenMetp[mid].limit.pwr_over, 1, puc);
			puc += 2;

			ParaCenMetp[mid].limit.pwr_limit = BcdToUnsigned(puc, 3); puc += 3;
			ParaCenMetp[mid].limit.time_pwrlimit = *puc++;
			ParaCenMetp[mid].limit.restore_pwrlimit = CalRestoreValue(ParaCenMetp[mid].limit.pwr_limit, 1, puc);
			puc += 2;

			ParaCenMetp[mid].limit.vol_unb = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.time_volunb = *puc++;
			ParaCenMetp[mid].limit.restore_volunb = CalRestoreValue(ParaCenMetp[mid].limit.vol_unb, 1, puc);
			puc += 2;

			ParaCenMetp[mid].limit.amp_unb = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].limit.time_ampunb = *puc++;
			ParaCenMetp[mid].limit.restore_ampunb = CalRestoreValue(ParaCenMetp[mid].limit.amp_unb, 1, puc);
			puc += 2;

			ParaCenMetp[mid].limit.time_volless_2 = *puc;
		}
	}

	return 0;
}

/**
* @brief 终端参数F27操作
*/
int ParaOperationF27(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned short mid = metpid - 1;
	unsigned char *puc = buf;

	if(0 == flag) {
		if(mid >= MAX_CENMETP) {
			memset(buf, 0, 24);
		}
		else {
			UnsignedToBcd(ParaCenMetp[mid].copper.Ra, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Xa, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Ga, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Ba, puc, 2); puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].copper.Rb, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Xb, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Gb, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Bb, puc, 2); puc += 2;

			UnsignedToBcd(ParaCenMetp[mid].copper.Rc, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Xc, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Gc, puc, 2); puc += 2;
			UnsignedToBcd(ParaCenMetp[mid].copper.Bc, puc, 2); puc += 2;
		}
	}
	else {
		if(mid < MAX_CENMETP) {
			ParaCenMetp[mid].copper.Ra = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Xa = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Ga = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Ba = BcdToUnsigned(puc, 2); puc += 2;

			ParaCenMetp[mid].copper.Rb = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Xb = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Gb = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Bb = BcdToUnsigned(puc, 2); puc += 2;

			ParaCenMetp[mid].copper.Rc = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Xc = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Gc = BcdToUnsigned(puc, 2); puc += 2;
			ParaCenMetp[mid].copper.Bc = BcdToUnsigned(puc, 2); puc += 2;
		}
	}

	return 0;
}

/**
* @brief 终端参数F28操作
*/
int ParaOperationF28(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned short mid = metpid - 1;

	if(0 == flag) {
		if(mid >= MAX_CENMETP) {
			memset(buf, 0, 4);
		}
		else {
			UnsignedToBcd(ParaCenMetp[mid].pwrf.limit1, &buf[0], 2);
			UnsignedToBcd(ParaCenMetp[mid].pwrf.limit2, &buf[2], 2);
		}
	}
	else {
		if(mid < MAX_CENMETP) {
			ParaCenMetp[mid].pwrf.limit1 = BcdToUnsigned(&buf[0], 2);
			ParaCenMetp[mid].pwrf.limit2 = BcdToUnsigned(&buf[2], 2);
		}
	}

	return 0;
}

/**
* @brief 终端参数F29操作
*/
int ParaOperationF29(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned short mid = metpid - 1;

	if(0 == flag) {
		if(mid >= MAX_CENMETP) {
			memcpy(buf, ParaPlcMetp[mid].displayno, 12);
		}
		else {
			memcpy(buf, ParaCenMetp[mid].displayno, 12);
		}
	}
	else 
	{
		if(mid >= MAX_CENMETP) 
		{
			memcpy(ParaPlcMetp[mid].displayno, buf, 12);
		}
		else 
		{
			memcpy(ParaCenMetp[mid].displayno, buf, 12);
			SetSaveParamFlag(SAVEFLAG_CMETP);
		}
	}

	return 0;
}

/**
* @brief 终端参数F30操作
*/
int ParaOperationF30(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned short mid = metpid - 1;

	if(0 == flag) {
		if(mid >= MAX_CENMETP) {
			buf[0] = ParaPlcMetp[mid].stopped;
		}
		else {
			buf[0] = ParaCenMetp[mid].stopped;
		}
	}
	else {
		if(mid >= MAX_CENMETP) {
			ParaPlcMetp[mid].stopped = buf[0];
		}
		else {
			ParaCenMetp[mid].stopped = buf[0];
			SetSaveParamFlag(SAVEFLAG_CMETP);
		}
	}

	return 0;
}

/**
* @brief 终端参数F31操作
*/
int ParaOperationF31(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	unsigned short mid = metpid - 1;
	int n;

	if(0 == flag) {
		if(mid >= MAX_CENMETP) {
			n = (int)buf[0]&0xff;
			n *= 6;
			*actlen = n+1;
			if(*actlen > len) return POERR_FATAL;

			buf[0] = ParaChildEnd[mid].num;
			memcpy(&buf[1], ParaChildEnd[mid].addr, n);
		}
		else {
			buf[0] = 0;
			*actlen = 1;
		}
	}
	else {
		if(mid >= MAX_CENMETP) {
			n = (int)buf[0]&0xff;
			n *= 6;
			*actlen = n+1;
			if(*actlen > len) return POERR_FATAL;

			if(buf[0] > 20) return POERR_INVALID;

			ParaChildEnd[mid].num = buf[0];
			memcpy(ParaChildEnd[mid].addr, &buf[1], n);
		}
		else {
			n = (int)buf[0]&0xff;
			n *= 6;
			*actlen = n+1;
			if(*actlen > len) return POERR_FATAL;
			if(buf[0] > 20) return POERR_INVALID;
		}
	}

	return 0;
}

