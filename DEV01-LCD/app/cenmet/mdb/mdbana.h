/**
* mdbana.h -- 表计分析数据
* 
* 
* 创建时间: 2010-5-11
* 最后修改时间: 2010-5-11
*/

#ifndef _MDB_ANA_H
#define _MDB_ANA_H

//mid 一般表示测量点0~3
//metpid 一般表示测量点1~4

//需要计算及分析的表数据
typedef struct {
	unsigned int enepa;  //正向有功总电能, 0.001kWh
	unsigned int enena;  //反向有功总电能, 0.001kWh

	unsigned int enepi;  //正向无功总电能, 0.01kvarh
	unsigned int eneni;  //反向无功总电能, 0.01kvarh

	int pwra[4];  //有功功率, 0.0001kW
	int pwra_1;  //总有功功率(一次侧), 0.01kW
	int pwri;  //无功功率, 0.0001kvar
	int pwri_1;  //总无功功率(一次侧), 0.01kvar
	int pwrv;  //总视在功率, 0.0001kVA
	short pwrf;  //总功率因数, 0.001
	unsigned short vol[3];  //电压, 0.1V, ABC
	short amp[4];  //电流, 0.01A, ABCN

	int pwrav;  //15分钟平均有功功率

	unsigned short vol_unb;  //电压不平衡率 0.1%
	unsigned short amp_unb;  //电流不平衡率 0.1%

	unsigned int failmask;
	unsigned int firstfail;
} mdbana_t;

#ifndef DEFINE_MDBANA
extern const mdbana_t MdbAnalyze[MAX_CENMETP];
#endif

#define mdbana(mid)    (MdbAnalyze[mid])

#define MBMSK_ENEPA    0x00000001
#define MBMSK_ENENA    0x00000002
#define MBMSK_ENEPI    0x00000004
#define MBMSK_ENENI    0x00000008

#define MBMSK_PWRABLK    0x000000f0
#define MBMSK_PWRAS    0x00000010
#define MBMSK_PWRAA    0x00000020
#define MBMSK_PWRAB    0x00000040
#define MBMSK_PWRAC    0x00000080

#define MBMSK_PWRI    0x00000100
#define MBMSK_PWRV    0x00001000
#define MBMSK_PWRF    0x00010000

#define MBMSK_VOLBLK    0x00700000
#define MBMSK_VOLA    0x00100000
#define MBMSK_VOLB    0x00200000
#define MBMSK_VOLC    0x00400000

#define MBMSK_AMPBLK    0x07000000
#define MBMSK_AMPA    0x01000000
#define MBMSK_AMPB    0x02000000
#define MBMSK_AMPC    0x04000000
#define MBMSK_AMPN    0x08000000

#define mrd_fail(mid, mask)    (mdbana(mid).failmask & (mask))
#define mrd_firstfail(mid, mask)    (mdbana(mid).firstfail & (mask))

unsigned int GetMdbAnaFailMask(unsigned short itemid);
void ClearMdbAnaFailMask(unsigned short mid, unsigned int mask);
void SetMdbAnaFailMask(unsigned short mid, unsigned int mask);

void UpdateMdbAna(unsigned char metpid, unsigned int itemid, const unsigned char *buf, int len, unsigned char flag);


#endif /*_MDB_ANA_H*/

