/**
* mdbana.c -- 表计分析数据
* 
* 
* 创建时间: 2010-5-11
* 最后修改时间: 2010-5-11
*/

#include <stdio.h>
#include <string.h>
#include <math.h>

#define DEFINE_MDBANA

#include "include/debug.h"
#include "include/param/capconf.h"
#include "mdbana.h"
#include "mdbcur.h"
#include "include/lib/bcd.h"
#include "include/param/metp.h"

mdbana_t MdbAnalyze[MAX_CENMETP];

static void MdbAnaReInit(unsigned short mid)
{
	memset(&MdbAnalyze[mid], 0, sizeof(mdbana_t));

	MdbAnalyze[mid].failmask = MdbAnalyze[mid].firstfail = 0xffffffff;
}

/**
* @brief 表计分析数据模块初始化
* @return 成功0, 否则失败
*/
DECLARE_INIT_FUNC(MdbAnaInit);
int MdbAnaInit(void)
{
	int i;

	for(i=0; i<MAX_CENMETP; i++) {
		MdbAnaReInit(i);
	}

	SET_INIT_FLAG(MdbAnaInit);
	return 0;
}

/**
* @brief 获取抄表失败标志位
* @param mid 测量点号, 0~
* @return 标志位
*/
unsigned int GetMdbAnaFailMask(unsigned short itemid)
{
	unsigned int mask;

	switch(itemid) {
	case 0x9010:
	case 0x901f:
		mask = MBMSK_ENEPA;
		break;
	case 0x9020:
	case 0x902f:
		mask = MBMSK_ENENA;
		break;

	case 0x9110:
	case 0x911f:
		mask = MBMSK_ENEPI;
		break;
	case 0x9120:
	case 0x912f:
		mask = MBMSK_ENENI;
		break;

	case 0xb63f: mask = MBMSK_PWRABLK; break;
	case 0xb630: mask = MBMSK_PWRAS; break;
	case 0xb631: mask = MBMSK_PWRAA; break;
	case 0xb632: mask = MBMSK_PWRAB; break;
	case 0xb633: mask = MBMSK_PWRAC; break;
	case 0xb640:
	case 0xb64f:
		mask = MBMSK_PWRI;
		break;
	case 0xb650:
	case 0xb65f:
		mask = MBMSK_PWRF;
		break;
	case 0xb61f: mask = MBMSK_VOLBLK; break;
	case 0xb611: mask = MBMSK_VOLA; break;
	case 0xb612: mask = MBMSK_VOLB; break;
	case 0xb613: mask = MBMSK_VOLC; break;
	case 0xb62f: mask = MBMSK_AMPBLK; break;
	case 0xb621: mask = MBMSK_AMPA; break;
	case 0xb622: mask = MBMSK_AMPB; break;
	case 0xb623: mask = MBMSK_AMPC; break;
	case 0xb624: mask = MBMSK_AMPN; break;
	default: mask = 0; break;
	}

	return(mask);
}

/**
* @brief 清除抄表失败标志
* @param mid 测量点号, 0~
* @param mask 标志位
*/
void ClearMdbAnaFailMask(unsigned short mid, unsigned int mask)
{
	mdbana(mid).failmask &= ~(mask);
	mdbana(mid).firstfail &= ~(mask);
}

/**
* @brief 置抄表失败标志
* @param mid 测量点号, 0~
* @param mask 标志位
*/
void SetMdbAnaFailMask(unsigned short mid, unsigned int mask)
{
	mdbana(mid).failmask |= (mask);
}

/**
* @brief 计算视在功率
* @param pwra 有功功率
* @param pwri 无功功率
* @return 视在功率
*/
static int CalculatePowerV(int pwra, int pwri)
{
	double db1, db2;
	int i;

	//if(pwra < 0) pwra *= -1;

	db1 = (double)pwra;
	db1 *= pwra;
	db2 = (double)pwri;
	db2 *= pwri;
	db1 += db2;
	db1 = sqrt(db1);
	i = (int)db1;
	if(i < 0) i *= -1;

	return(i);
}

/**
* @brief 计算电压电流不平衡度
* @param mid 测量点号 0~
* @param flag 0-电压, 1-电流
*/
static void CalculateUnb(unsigned short mid, int flag)
{
	unsigned int avg, diff[3];
	int i;
	mdbana_t *pdb;
	unsigned short value[3];

	pdb = &MdbAnalyze[mid];

	if(flag) { // amp
		short ss;

		if(pdb->failmask & MBMSK_AMPBLK) return;

		for(i=0; i<3; i++) {
			ss = pdb->amp[i];
			if(ss < 0) ss *= -1;
			value[i] = ss;
		}
	}
	else { // vol
		if(pdb->failmask & MBMSK_VOLBLK) return;

		value[0] = pdb->vol[0];
		value[1] = pdb->vol[1];
		value[2] = pdb->vol[2];
	}

	if(PWRTYPE_3X3W == metp_pwrtype(mid)) {
		avg = (value[0] + value[2]) / 2;

		if(value[0] > avg) diff[0] = value[0] - avg;
		else diff[0] = avg - value[0];

		if(value[2] > avg) diff[2] = value[2] - avg;
		else diff[2] = avg - value[2];

		if(diff[2] > diff[0]) diff[0] = diff[2];
	}
	else if(PWRTYPE_3X4W == metp_pwrtype(mid)) {
		avg = (value[0] + value[1] + value[2]) / 3;

		for(i=0; i<3; i++) {
			if(value[i] > avg) diff[i] = value[i] - avg;
			else diff[i] = avg - value[i];

			if(i && diff[i] > diff[0]) diff[0] = diff[i];
		}
	}
	else avg = 0;

	if(avg) avg = diff[0] * 1000 / avg;

	if(flag) pdb->amp_unb = avg;
	else pdb->vol_unb = avg;

}

/**
* @brief 更新表计分析数据
* @param metpid 测量点号, 1~
* @param itemid 数据标识
* @param buf 源数据缓存区指针
* @param len 缓存区长度
* @param flag 更新标志
*/
void UpdateMdbAna(unsigned char metpid, unsigned int itemid, const unsigned char *buf, int len, unsigned char flag)
{
	unsigned char dealflag = flag&UPCURFLAG_MASK;
	mdbana_t *pdb;
	unsigned int ul;
	int i;

	if((metpid==0) ||(metpid > MAX_CENMETP)) return;
	metpid -= 1;

	pdb = &MdbAnalyze[metpid];

	switch(itemid) {
	case 0x901f:
	case 0x902f:
	case 0x9010:
	case 0x9020:

	case 0x0001FF00:
	case 0x0002FF00:
		{
			unsigned int ul2;

			if(UPCURFLAG_645 == dealflag) {
				ul = BcdToUnsigned(buf, 4);
				ul *= 10;
			}
			else {
				ul = BcdToUnsigned(&buf[4], 1);
				ul *= 10000000;
				ul2 = BcdToUnsigned(buf, 4);
				ul2 /= 10;
				ul += ul2;
			}

			if((0x9010 == itemid) || (0x901f == itemid)) {
				ClearMdbAnaFailMask(metpid, MBMSK_ENEPA);
				pdb->enepa = ul;
			}
			else {
				ClearMdbAnaFailMask(metpid, MBMSK_ENENA);
				pdb->enena = ul;
			}
		}
		break;

	case 0x911f:
	case 0x9110:

	case 0x00030000:	
		ul = BcdToUnsigned(buf, 4);
		pdb->enepi = ul;
		ClearMdbAnaFailMask(metpid, MBMSK_ENEPI);
		break;

	case 0x912f:
	case 0x9120:

	case 0x00040000:		
		ul = BcdToUnsigned(buf, 4);
		pdb->eneni = ul;
		ClearMdbAnaFailMask(metpid, MBMSK_ENENI);
		break;

	case 0xb63f:
	case 0x0203FF00:
		
	case 0xb630:
		{
			long long li;

			i = BcdToInt(buf, 3);
			pdb->pwra[0] = i;
			li = i;

			ul = ParaCenMetp[metpid].base.pt;
			ul *= ParaCenMetp[metpid].base.ct;
			li *= ul;
			li /= 100;
			pdb->pwra_1 = (int)li;
		}
		ClearMdbAnaFailMask(metpid, MBMSK_PWRAS);

		if(0xb63f == itemid) {
			for(i=1; i<=3; i++) {
				pdb->pwra[i] = BcdToInt(&buf[i*3], 3);
			}
			ClearMdbAnaFailMask(metpid, MBMSK_PWRABLK);
		}
		break;

	case 0xb631:
		pdb->pwra[1] = BcdToInt(buf, 3);
		ClearMdbAnaFailMask(metpid, MBMSK_PWRAA);
		break;
	case 0xb632:
		pdb->pwra[2] = BcdToInt(buf, 3);
		ClearMdbAnaFailMask(metpid, MBMSK_PWRAB);
		break;
	case 0xb633:
		pdb->pwra[3] = BcdToInt(buf, 3);
		ClearMdbAnaFailMask(metpid, MBMSK_PWRAC);
		break;

	case 0xb64f:
	case 0xb640:
	case 0x0204FF00:
		
		{
			long long li;

			if(UPCURFLAG_645 == dealflag) {
				i = BcdToUnsigned(buf, 2);
				i *= 100;
			}
			else i = BcdToInt(buf, 3);

			pdb->pwri = i;
			li = i;

			ul = ParaCenMetp[metpid].base.pt;
			ul *= ParaCenMetp[metpid].base.ct;
			li *= ul;
			li /= 100;
			pdb->pwri_1 = (int)li;

			pdb->pwrv = CalculatePowerV(pdb->pwra[0], pdb->pwri);
		}
		ClearMdbAnaFailMask(metpid, MBMSK_PWRAS);
		break;

	case 0xb65f:
	case 0xb650:
	case 0x0206FF00:
		if(UPCURFLAG_645 == dealflag) i = BcdToUnsigned(buf, 2);
		else i = BcdToInt(buf, 2);
		pdb->pwrf = i;
		ClearMdbAnaFailMask(metpid, MBMSK_PWRF);
		break;

	case 0xb611:
	case 0xb612:
	case 0xb613:
		{
			int abc = itemid-0xb611;

			if(UPCURFLAG_645 == dealflag) {
				ul = BcdToUnsigned(buf, 2);
				ul *= 10;
			}
			else ul = BcdToUnsigned(buf, 2);

			pdb->vol[abc] =ul;
			ClearMdbAnaFailMask(metpid, ((unsigned int)MBMSK_VOLA<<abc));

			if(0xb613 == itemid) CalculateUnb(metpid, 0);
		}
		break;

	case 0xb61f:
	case 0x0201FF00:
		
		{
			int abc;

			for(abc=0; abc<3; abc++) {
				if(UPCURFLAG_645 == dealflag) {
					ul = BcdToUnsigned(&buf[abc*2], 2);
					ul *= 10;
				}
				else ul = BcdToUnsigned(&buf[abc*2], 2);

				pdb->vol[abc] =ul;
			}
			ClearMdbAnaFailMask(metpid, MBMSK_VOLBLK);
			CalculateUnb(metpid, 0);
		}
		break;

	case 0xb621:
	case 0xb622:
	case 0xb623:
		{
			int abc = itemid-0xb621;

			if(UPCURFLAG_645 == dealflag) i = BcdToUnsigned(buf, 2);
			else i = BcdToInt(buf, 2);

			pdb->amp[abc] = i;
			ClearMdbAnaFailMask(metpid, ((unsigned int)MBMSK_AMPA<<abc));
			if(0xb623 == itemid) CalculateUnb(metpid, 1);
		}
		break;

	case 0xb62f:
	case 0x0202FF00:		
		{
			int abc;

			for(abc=0; abc<3; abc++) {
				if(UPCURFLAG_645 == dealflag) i = BcdToUnsigned(&buf[abc*2], 2);
				else i = BcdToInt(&buf[abc*2], 2);

				pdb->amp[abc] = i;
			}
			ClearMdbAnaFailMask(metpid, MBMSK_AMPBLK);
			CalculateUnb(metpid, 1);
		}
		break;

	case 0xb624:
		if(UPCURFLAG_645 == dealflag) i = BcdToUnsigned(buf, 2);
		else i = BcdToInt(buf, 2);

		pdb->amp[3] = i;
		ClearMdbAnaFailMask(metpid, MBMSK_AMPN);
		break;

	default: break;
	}
}

