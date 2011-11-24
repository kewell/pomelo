/**
* mdbcur.c -- 当前表计数据
* 
* 
* 创建时间: 2010-5-10
* 最后修改时间: 2010-5-10
*/

#include <stdio.h>
#include <string.h>

#define DEFINE_MDBCURRENT

#include "include/debug.h"
#include "include/param/capconf.h"
#include "mdbcur.h"
#include "include/sys/timeal.h"
#include "include/lib/bcd.h"
#include "include/monitor/alarm.h"
#include "mdbana.h"
#include "../cenmet_comm.h"
#include "include/param/meter.h"

mdbcur_t MdbCurrent[MAX_CENMETP];

static void MdbCurrentReInit(unsigned short mid)
{
	memset(&MdbCurrent[mid], FLAG_MDBEMPTY, sizeof(mdbcur_t));
	MdbCurrent[mid].syntony_value[0] = 0;
	MdbCurrent[mid].syntony_rating[0] = 0;
	MdbCurrent[mid].cnt_comerr = 0;
	MdbCurrent[mid].flag_comerr = 0;
}

/**
* @brief 当前表计数据初始化
* @return 成功0, 否则失败
*/
DECLARE_INIT_FUNC(MdbCurrentInit);
int MdbCurrentInit(void)
{
	int i;

	for(i=0; i<MAX_CENMETP; i++) {
		MdbCurrentReInit(i);
	}

	SET_INIT_FLAG(MdbCurrentInit);

	return 0;
}

#define MCPYFLAG_GROUP		0x80
#define MCPYFLAG_LSHIFT		0x40  //multi
#define MCPYFLAG_RSHIFT		0x20  //div
#define MCPYFLAG_FUNC		0x10
typedef struct {
	unsigned int itemid;
	unsigned char flag;  //低4位为偏移值
	unsigned char len;  //低4位为GBlen, 高4位为DL645len
	unsigned char *pbuf;
} mdbcur_cpy_t;

static const mdbcur_cpy_t MdbCopyB6XX[] = {
	{0xb63f, MCPYFLAG_GROUP, 4,  NULL}, 
	{0xb630, 0, 0x33, &mdbcur(0).pwra[0]}, //有功功率
	{0xb631, 0, 0x33, &mdbcur(0).pwra[3]}, 
	{0xb632, 0, 0x33, &mdbcur(0).pwra[6]}, 
	{0xb633, 0, 0x33, &mdbcur(0).pwra[9]}, 

	{0xb64f, MCPYFLAG_GROUP, 4, NULL}, 
	{0xb640, MCPYFLAG_LSHIFT|0x02, 0x23, &mdbcur(0).pwri[0]}, //无功功率
	{0xb641, MCPYFLAG_LSHIFT|0x02, 0x23, &mdbcur(0).pwri[3]}, 
	{0xb642, MCPYFLAG_LSHIFT|0x02, 0x23, &mdbcur(0).pwri[6]}, 
	{0xb643, MCPYFLAG_LSHIFT|0x02, 0x23, &mdbcur(0).pwri[9]}, 

	{0xb65f, MCPYFLAG_GROUP, 4, NULL}, 
	{0xb650, 0, 0x22, &mdbcur(0).pwrf[0]}, //功率因素
	{0xb651, 0, 0x22, &mdbcur(0).pwrf[2]}, 
	{0xb652, 0, 0x22, &mdbcur(0).pwrf[4]}, 
	{0xb653, 0, 0x22, &mdbcur(0).pwrf[6]}, 

	{0xb61f, MCPYFLAG_GROUP, 3, NULL},
	{0xb611, MCPYFLAG_LSHIFT|0x01, 0x22, &mdbcur(0).vol[0]}, 
	{0xb612, MCPYFLAG_LSHIFT|0x01, 0x22, &mdbcur(0).vol[2]}, 
	{0xb613, MCPYFLAG_LSHIFT|0x01, 0x22, &mdbcur(0).vol[4]}, 

	{0xb62f, MCPYFLAG_GROUP, 3, NULL},
	{0xb621, 0, 0x22, &mdbcur(0).amp[0]}, 
	{0xb622, 0, 0x22, &mdbcur(0).amp[2]}, 
	{0xb623, 0, 0x22, &mdbcur(0).amp[4]}, 

	{0xb624, 0, 0x22, &mdbcur(0).amp[6]}, 

	{0xb66f, MCPYFLAG_GROUP, 6, NULL}, 
	{0xb660, 0, 0x22, &mdbcur(0).phase_arc[0]}, 
	{0xb661, 0, 0x22, &mdbcur(0).phase_arc[2]}, 
	{0xb662, 0, 0x22, &mdbcur(0).phase_arc[4]}, 
	{0xb663, 0, 0x22, &mdbcur(0).phase_arc[6]}, 
	{0xb664, 0, 0x22, &mdbcur(0).phase_arc[8]}, 
	{0xb665, 0, 0x22, &mdbcur(0).phase_arc[10]},

	{0xb6ef, MCPYFLAG_GROUP, 4,  NULL}, 
	{0xb6e0, 0, 0x33, &mdbcur(0).pwrv[0]}, //视在功率
	{0xb6e1, 0, 0x33, &mdbcur(0).pwrv[3]}, 
	{0xb6e2, 0, 0x33, &mdbcur(0).pwrv[6]}, 
	{0xb6e3, 0, 0x33, &mdbcur(0).pwrv[9]}, 
};

static const mdbcur_cpy_t MdbCopyB3XX[] = {
	{0xb31f, MCPYFLAG_GROUP, 4, NULL},
	{0xb310, 0, 0x22, &mdbcur(0).volbr_cnt[0]}, 
	{0xb311, 0, 0x22, &mdbcur(0).volbr_cnt[2]}, 
	{0xb312, 0, 0x22, &mdbcur(0).volbr_cnt[4]}, 
	{0xb313, 0, 0x22, &mdbcur(0).volbr_cnt[6]}, 

	{0xb32f, MCPYFLAG_GROUP, 4, NULL},
	{0xb320, 0, 0x33, &mdbcur(0).volbr_times[0]}, 
	{0xb321, 0, 0x33, &mdbcur(0).volbr_times[3]}, 
	{0xb322, 0, 0x33, &mdbcur(0).volbr_times[6]}, 
	{0xb323, 0, 0x33, &mdbcur(0).volbr_times[9]}, 

	{0xb33f, MCPYFLAG_GROUP, 4, NULL},
	{0xb330, 0, 0x44, &mdbcur(0).volbr_timestart[0]}, 
	{0xb331, 0, 0x44, &mdbcur(0).volbr_timestart[4]}, 
	{0xb332, 0, 0x44, &mdbcur(0).volbr_timestart[8]}, 
	{0xb333, 0, 0x44, &mdbcur(0).volbr_timestart[12]}, 

	{0xb34f, MCPYFLAG_GROUP, 4, NULL},
	{0xb340, 0, 0x44, &mdbcur(0).volbr_timeend[0]}, 
	{0xb341, 0, 0x44, &mdbcur(0).volbr_timeend[4]}, 
	{0xb342, 0, 0x44, &mdbcur(0).volbr_timeend[8]}, 
	{0xb343, 0, 0x44, &mdbcur(0).volbr_timeend[12]},
};

static const mdbcur_cpy_t MdbCopyC0XX[] = {
	{0xc01f, MCPYFLAG_GROUP, 2, NULL},
	{0xc010, MCPYFLAG_RSHIFT|0x02, 0x43, &mdbcur(0).met_clock[3]}, 
	//{0xc011, MCPYFLAG_RSHIFT|0x02, 0x32, &mdbcur(0).met_clock[0]}, 
	{0xc011, 0, 0x33, &mdbcur(0).met_clock[0]}, 

	{0xc020, 0, 0x11, &mdbcur(0).met_runstate[0]}, 
};

static const mdbcur_cpy_t MdbCopyB2XX[] = {
	{0xb21f, MCPYFLAG_GROUP, 5, NULL},
	{0xb210, MCPYFLAG_LSHIFT|0x02, 0x46, &mdbcur(0).prog_time[0]}, 
	{0xb211, MCPYFLAG_LSHIFT|0x02, 0x46, &mdbcur(0).dmnclr_time[0]}, 
	{0xb212, 0, 0x23, &mdbcur(0).prog_cnt[0]}, 
	{0xb213, 0, 0x23, &mdbcur(0).dmnclr_cnt[0]}, 
	{0xb214, 0, 0x34, &mdbcur(0).bat_runtime[0]},
};

static const mdbcur_cpy_t MdbCopy90XX[] = {
	{0x901f, MCPYFLAG_GROUP, 5, NULL},
	{0x9010, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa[0]}, 
	{0x9011, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa[5]}, 
	{0x9012, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa[10]}, 
	{0x9013, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa[15]}, 
	{0x9014, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa[20]}, 

	{0x902f, MCPYFLAG_GROUP, 5, NULL},
	{0x9020, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena[0]}, 
	{0x9021, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena[5]}, 
	{0x9022, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena[10]}, 
	{0x9023, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena[15]}, 
	{0x9024, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena[20]}, 

	{0x903f, MCPYFLAG_GROUP, 3, NULL},
	{0x9031, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_a[0]}, 
	{0x9032, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_b[0]}, 
	{0x9033, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_c[0]}, 

	{0x904f, MCPYFLAG_GROUP, 3, NULL},
	{0x9041, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_a[0]}, 
	{0x9042, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_b[0]}, 
	{0x9043, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_c[0]}, 

	{0x905f, MCPYFLAG_GROUP, 3, NULL},
	{0x9051, 0, 0x44, &mdbcur(0).enepi_a[0]},
	{0x9052, 0, 0x44, &mdbcur(0).enepi_b[0]},
	{0x9053, 0, 0x44, &mdbcur(0).enepi_c[0]},

	{0x906f, MCPYFLAG_GROUP, 3, NULL},
	{0x9061, 0, 0x44, &mdbcur(0).eneni_a[0]},
	{0x9062, 0, 0x44, &mdbcur(0).eneni_b[0]},
	{0x9063, 0, 0x44, &mdbcur(0).eneni_c[0]},
};














static const mdbcur_cpy_t MdbCopy91XX[] = {
	{0x911f, MCPYFLAG_GROUP, 5, NULL},
	{0x9110, 0, 0x44, &mdbcur(0).enepi[0]}, 
	{0x9111, 0, 0x44, &mdbcur(0).enepi[4]}, 
	{0x9112, 0, 0x44, &mdbcur(0).enepi[8]}, 
	{0x9113, 0, 0x44, &mdbcur(0).enepi[12]}, 
	{0x9114, 0, 0x44, &mdbcur(0).enepi[16]}, 

	{0x913f, MCPYFLAG_GROUP, 5, NULL},
	{0x9130, 0, 0x44, &mdbcur(0).enepi1[0]}, 
	{0x9131, 0, 0x44, &mdbcur(0).enepi1[4]}, 
	{0x9132, 0, 0x44, &mdbcur(0).enepi1[8]}, 
	{0x9133, 0, 0x44, &mdbcur(0).enepi1[12]}, 
	{0x9134, 0, 0x44, &mdbcur(0).enepi1[16]}, 

	{0x914f, MCPYFLAG_GROUP, 5, NULL},
	{0x9140, 0, 0x44, &mdbcur(0).enepi4[0]}, 
	{0x9141, 0, 0x44, &mdbcur(0).enepi4[4]}, 
	{0x9142, 0, 0x44, &mdbcur(0).enepi4[8]}, 
	{0x9143, 0, 0x44, &mdbcur(0).enepi4[12]}, 
	{0x9144, 0, 0x44, &mdbcur(0).enepi4[16]}, 

	{0x912f, MCPYFLAG_GROUP, 5, NULL},
	{0x9120, 0, 0x44, &mdbcur(0).eneni[0]}, 
	{0x9121, 0, 0x44, &mdbcur(0).eneni[4]}, 
	{0x9122, 0, 0x44, &mdbcur(0).eneni[8]}, 
	{0x9123, 0, 0x44, &mdbcur(0).eneni[12]}, 
	{0x9124, 0, 0x44, &mdbcur(0).eneni[16]}, 

	{0x915f, MCPYFLAG_GROUP, 5, NULL},
	{0x9150, 0, 0x44, &mdbcur(0).eneni2[0]}, 
	{0x9151, 0, 0x44, &mdbcur(0).eneni2[4]}, 
	{0x9152, 0, 0x44, &mdbcur(0).eneni2[8]}, 
	{0x9153, 0, 0x44, &mdbcur(0).eneni2[12]}, 
	{0x9154, 0, 0x44, &mdbcur(0).eneni2[16]}, 

	{0x916f, MCPYFLAG_GROUP, 5, NULL},
	{0x9160, 0, 0x44, &mdbcur(0).eneni3[0]}, 
	{0x9161, 0, 0x44, &mdbcur(0).eneni3[4]}, 
	{0x9162, 0, 0x44, &mdbcur(0).eneni3[8]}, 
	{0x9163, 0, 0x44, &mdbcur(0).eneni3[12]}, 
	{0x9164, 0, 0x44, &mdbcur(0).eneni3[16]},
};

static const mdbcur_cpy_t MdbCopyA0XX[] = {
	{0xa01f, MCPYFLAG_GROUP, 5, NULL},
	{0xa010, 0, 0x33, &mdbcur(0).dmnpa[0]}, 
	{0xa011, 0, 0x33, &mdbcur(0).dmnpa[3]}, 
	{0xa012, 0, 0x33, &mdbcur(0).dmnpa[6]}, 
	{0xa013, 0, 0x33, &mdbcur(0).dmnpa[9]}, 
	{0xa014, 0, 0x33, &mdbcur(0).dmnpa[12]}, 

	{0xa02f, MCPYFLAG_GROUP, 5, NULL},
	{0xa020, 0, 0x33, &mdbcur(0).dmnna[0]}, 
	{0xa021, 0, 0x33, &mdbcur(0).dmnna[3]}, 
	{0xa022, 0, 0x33, &mdbcur(0).dmnna[6]}, 
	{0xa023, 0, 0x33, &mdbcur(0).dmnna[9]}, 
	{0xa024, 0, 0x33, &mdbcur(0).dmnna[12]}, 
};

static const mdbcur_cpy_t MdbCopyB0XX[] = {
	{0xb01f, MCPYFLAG_GROUP, 5, NULL},
	{0xb010, 0, 0x44, &mdbcur(0).dmntpa[0]}, 
	{0xb011, 0, 0x44, &mdbcur(0).dmntpa[4]}, 
	{0xb012, 0, 0x44, &mdbcur(0).dmntpa[8]}, 
	{0xb013, 0, 0x44, &mdbcur(0).dmntpa[12]}, 
	{0xb014, 0, 0x44, &mdbcur(0).dmntpa[16]}, 

	{0xb02f, MCPYFLAG_GROUP, 5, NULL},
	{0xb020, 0, 0x44, &mdbcur(0).dmntna[0]}, 
	{0xb021, 0, 0x44, &mdbcur(0).dmntna[4]}, 
	{0xb022, 0, 0x44, &mdbcur(0).dmntna[8]}, 
	{0xb023, 0, 0x44, &mdbcur(0).dmntna[12]}, 
	{0xb024, 0, 0x44, &mdbcur(0).dmntna[16]}, 
};

static const mdbcur_cpy_t MdbCopyA1XX[] = {
	{0xa11f, MCPYFLAG_GROUP, 5, NULL},
	{0xa110, 0, 0x33, &mdbcur(0).dmnpi[0]}, 
	{0xa111, 0, 0x33, &mdbcur(0).dmnpi[3]}, 
	{0xa112, 0, 0x33, &mdbcur(0).dmnpi[6]}, 
	{0xa113, 0, 0x33, &mdbcur(0).dmnpi[9]}, 
	{0xa114, 0, 0x33, &mdbcur(0).dmnpi[12]},

	{0xa12f, MCPYFLAG_GROUP, 5, NULL},
	{0xa120, 0, 0x33, &mdbcur(0).dmnni[0]}, 
	{0xa121, 0, 0x33, &mdbcur(0).dmnni[3]}, 
	{0xa122, 0, 0x33, &mdbcur(0).dmnni[6]}, 
	{0xa123, 0, 0x33, &mdbcur(0).dmnni[9]}, 
	{0xa124, 0, 0x33, &mdbcur(0).dmnni[12]}, 
};

static const mdbcur_cpy_t MdbCopyB1XX[] = {
	{0xb11f, MCPYFLAG_GROUP, 5, NULL},
	{0xb110, 0, 0x44, &mdbcur(0).dmntpi[0]}, 
	{0xb111, 0, 0x44, &mdbcur(0).dmntpi[4]}, 
	{0xb112, 0, 0x44, &mdbcur(0).dmntpi[8]}, 
	{0xb113, 0, 0x44, &mdbcur(0).dmntpi[12]}, 
	{0xb114, 0, 0x44, &mdbcur(0).dmntpi[16]}, 

	{0xb12f, MCPYFLAG_GROUP, 5, NULL},
	{0xb120, 0, 0x44, &mdbcur(0).dmntni[0]}, 
	{0xb121, 0, 0x44, &mdbcur(0).dmntni[4]}, 
	{0xb122, 0, 0x44, &mdbcur(0).dmntni[8]}, 
	{0xb123, 0, 0x44, &mdbcur(0).dmntni[12]}, 
	{0xb124, 0, 0x44, &mdbcur(0).dmntni[16]}, 
};

static const mdbcur_cpy_t MdbCopy94XX[] = {
	{0x941f, MCPYFLAG_GROUP, 5, NULL},
	{0x9410, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_lm[0]}, 
	{0x9411, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_lm[5]}, 
	{0x9412, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_lm[10]}, 
	{0x9413, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_lm[15]}, 
	{0x9414, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_lm[20]},

	{0x942f, MCPYFLAG_GROUP, 5, NULL},
	{0x9420, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_lm[0]}, 
	{0x9421, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_lm[5]}, 
	{0x9422, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_lm[10]}, 
	{0x9423, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_lm[15]}, 
	{0x9424, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_lm[20]}, 

	{0x943f, MCPYFLAG_GROUP, 3, NULL},
	{0x9431, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_a_lm[0]}, 
	{0x9432, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_b_lm[0]}, 
	{0x9433, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enepa_c_lm[0]}, 

	{0x944f, MCPYFLAG_GROUP, 3, NULL},
	{0x9441, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_a_lm[0]}, 
	{0x9442, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_b_lm[0]}, 
	{0x9443, MCPYFLAG_LSHIFT|0x02, 0x45, &mdbcur(0).enena_c_lm[0]}, 

	{0x945f, MCPYFLAG_GROUP, 3, NULL},
	{0x9451, 0, 0x44, &mdbcur(0).enepi_a_lm[0]},
	{0x9452, 0, 0x44, &mdbcur(0).enepi_b_lm[0]},
	{0x9453, 0, 0x44, &mdbcur(0).enepi_c_lm[0]},

	{0x946f, MCPYFLAG_GROUP, 3, NULL},
	{0x9461, 0, 0x44, &mdbcur(0).eneni_a_lm[0]},
	{0x9462, 0, 0x44, &mdbcur(0).eneni_b_lm[0]},
	{0x9463, 0, 0x44, &mdbcur(0).eneni_c_lm[0]},
};

static const mdbcur_cpy_t MdbCopy95XX[] = {
	{0x951f, MCPYFLAG_GROUP, 5, NULL},
	{0x9510, 0, 0x44, &mdbcur(0).enepi_lm[0]}, 
	{0x9511, 0, 0x44, &mdbcur(0).enepi_lm[4]}, 
	{0x9512, 0, 0x44, &mdbcur(0).enepi_lm[8]}, 
	{0x9513, 0, 0x44, &mdbcur(0).enepi_lm[12]}, 
	{0x9514, 0, 0x44, &mdbcur(0).enepi_lm[16]},

	{0x953f, MCPYFLAG_GROUP, 5, NULL},
	{0x9530, 0, 0x44, &mdbcur(0).enepi1_lm[0]}, 
	{0x9531, 0, 0x44, &mdbcur(0).enepi1_lm[4]}, 
	{0x9532, 0, 0x44, &mdbcur(0).enepi1_lm[8]}, 
	{0x9533, 0, 0x44, &mdbcur(0).enepi1_lm[12]}, 
	{0x9534, 0, 0x44, &mdbcur(0).enepi1_lm[16]}, 

	{0x954f, MCPYFLAG_GROUP, 5, NULL},
	{0x9540, 0, 0x44, &mdbcur(0).enepi4_lm[0]}, 
	{0x9541, 0, 0x44, &mdbcur(0).enepi4_lm[4]}, 
	{0x9542, 0, 0x44, &mdbcur(0).enepi4_lm[8]}, 
	{0x9543, 0, 0x44, &mdbcur(0).enepi4_lm[12]}, 
	{0x9544, 0, 0x44, &mdbcur(0).enepi4_lm[16]},

	{0x952f, MCPYFLAG_GROUP, 5, NULL},
	{0x9520, 0, 0x44, &mdbcur(0).eneni_lm[0]}, 
	{0x9521, 0, 0x44, &mdbcur(0).eneni_lm[4]}, 
	{0x9522, 0, 0x44, &mdbcur(0).eneni_lm[8]}, 
	{0x9523, 0, 0x44, &mdbcur(0).eneni_lm[12]}, 
	{0x9524, 0, 0x44, &mdbcur(0).eneni_lm[16]},

	{0x955f, MCPYFLAG_GROUP, 5, NULL},
	{0x9550, 0, 0x44, &mdbcur(0).eneni2_lm[0]}, 
	{0x9551, 0, 0x44, &mdbcur(0).eneni2_lm[4]}, 
	{0x9552, 0, 0x44, &mdbcur(0).eneni2_lm[8]}, 
	{0x9553, 0, 0x44, &mdbcur(0).eneni2_lm[12]}, 
	{0x9554, 0, 0x44, &mdbcur(0).eneni2_lm[16]},

	{0x956f, MCPYFLAG_GROUP, 5, NULL},
	{0x9560, 0, 0x44, &mdbcur(0).eneni3_lm[0]}, 
	{0x9561, 0, 0x44, &mdbcur(0).eneni3_lm[4]}, 
	{0x9562, 0, 0x44, &mdbcur(0).eneni3_lm[8]}, 
	{0x9563, 0, 0x44, &mdbcur(0).eneni3_lm[12]}, 
	{0x9564, 0, 0x44, &mdbcur(0).eneni3_lm[16]},
};

static const mdbcur_cpy_t MdbCopyA4XX[] = {
	{0xa41f, MCPYFLAG_GROUP, 5, NULL},
	{0xa410, 0, 0x33, &mdbcur(0).dmnpa_lm[0]}, 
	{0xa411, 0, 0x33, &mdbcur(0).dmnpa_lm[3]}, 
	{0xa412, 0, 0x33, &mdbcur(0).dmnpa_lm[6]}, 
	{0xa413, 0, 0x33, &mdbcur(0).dmnpa_lm[9]}, 
	{0xa414, 0, 0x33, &mdbcur(0).dmnpa_lm[12]}, 

	{0xa42f, MCPYFLAG_GROUP, 5, NULL},
	{0xa420, 0, 0x33, &mdbcur(0).dmnna_lm[0]}, 
	{0xa421, 0, 0x33, &mdbcur(0).dmnna_lm[3]}, 
	{0xa422, 0, 0x33, &mdbcur(0).dmnna_lm[6]}, 
	{0xa423, 0, 0x33, &mdbcur(0).dmnna_lm[9]}, 
	{0xa424, 0, 0x33, &mdbcur(0).dmnna_lm[12]},
};

static const mdbcur_cpy_t MdbCopyB4XX[] = {
	{0xb41f, MCPYFLAG_GROUP, 5, NULL},
	{0xb410, 0, 0x44, &mdbcur(0).dmntpa_lm[0]}, 
	{0xb411, 0, 0x44, &mdbcur(0).dmntpa_lm[4]}, 
	{0xb412, 0, 0x44, &mdbcur(0).dmntpa_lm[8]}, 
	{0xb413, 0, 0x44, &mdbcur(0).dmntpa_lm[12]}, 
	{0xb414, 0, 0x44, &mdbcur(0).dmntpa_lm[16]}, 

	{0xb42f, MCPYFLAG_GROUP, 5, NULL},
	{0xb420, 0, 0x44, &mdbcur(0).dmntna_lm[0]}, 
	{0xb421, 0, 0x44, &mdbcur(0).dmntna_lm[4]}, 
	{0xb422, 0, 0x44, &mdbcur(0).dmntna_lm[8]}, 
	{0xb423, 0, 0x44, &mdbcur(0).dmntna_lm[12]}, 
	{0xb424, 0, 0x44, &mdbcur(0).dmntna_lm[16]}, 
};

static const mdbcur_cpy_t MdbCopyA5XX[] = {
	{0xa51f, MCPYFLAG_GROUP, 5, NULL},
	{0xa510, 0, 0x33, &mdbcur(0).dmnpi_lm[0]}, 
	{0xa511, 0, 0x33, &mdbcur(0).dmnpi_lm[3]}, 
	{0xa512, 0, 0x33, &mdbcur(0).dmnpi_lm[6]}, 
	{0xa513, 0, 0x33, &mdbcur(0).dmnpi_lm[9]}, 
	{0xa514, 0, 0x33, &mdbcur(0).dmnpi_lm[12]},

	{0xa52f, MCPYFLAG_GROUP, 5, NULL},
	{0xa520, 0, 0x33, &mdbcur(0).dmnni_lm[0]}, 
	{0xa521, 0, 0x33, &mdbcur(0).dmnni_lm[3]}, 
	{0xa522, 0, 0x33, &mdbcur(0).dmnni_lm[6]}, 
	{0xa523, 0, 0x33, &mdbcur(0).dmnni_lm[9]}, 
	{0xa524, 0, 0x33, &mdbcur(0).dmnni_lm[12]}, 
};

static const mdbcur_cpy_t MdbCopyB5XX[] = {
	{0xb51f, MCPYFLAG_GROUP, 5, NULL},
	{0xb510, 0, 0x44, &mdbcur(0).dmntpi_lm[0]}, 
	{0xb511, 0, 0x44, &mdbcur(0).dmntpi_lm[4]}, 
	{0xb512, 0, 0x44, &mdbcur(0).dmntpi_lm[8]}, 
	{0xb513, 0, 0x44, &mdbcur(0).dmntpi_lm[12]}, 
	{0xb514, 0, 0x44, &mdbcur(0).dmntpi_lm[16]}, 

	{0xb52f, MCPYFLAG_GROUP, 5, NULL},
	{0xb520, 0, 0x44, &mdbcur(0).dmntni_lm[0]}, 
	{0xb521, 0, 0x44, &mdbcur(0).dmntni_lm[4]}, 
	{0xb522, 0, 0x44, &mdbcur(0).dmntni_lm[8]}, 
	{0xb523, 0, 0x44, &mdbcur(0).dmntni_lm[12]}, 
	{0xb524, 0, 0x44, &mdbcur(0).dmntni_lm[16]}, 
};

#define GROUP_MASK		0xff00
typedef struct {
	unsigned short itemid;
	unsigned short num;
	const mdbcur_cpy_t *list;
} mdbcur_grpcpy_t;
static const mdbcur_grpcpy_t MdbCopyGroup[] = {
	{0xb600, sizeof(MdbCopyB6XX)/sizeof(mdbcur_cpy_t), MdbCopyB6XX},
	{0xb300, sizeof(MdbCopyB3XX)/sizeof(mdbcur_cpy_t), MdbCopyB3XX},
	{0xc000, sizeof(MdbCopyC0XX)/sizeof(mdbcur_cpy_t), MdbCopyC0XX},
	{0xb200, sizeof(MdbCopyB2XX)/sizeof(mdbcur_cpy_t), MdbCopyB2XX},
	{0x9000, sizeof(MdbCopy90XX)/sizeof(mdbcur_cpy_t), MdbCopy90XX},
	{0x9100, sizeof(MdbCopy91XX)/sizeof(mdbcur_cpy_t), MdbCopy91XX},
	{0xa000, sizeof(MdbCopyA0XX)/sizeof(mdbcur_cpy_t), MdbCopyA0XX},
	{0xb000, sizeof(MdbCopyB0XX)/sizeof(mdbcur_cpy_t), MdbCopyB0XX},
	{0xa100, sizeof(MdbCopyA1XX)/sizeof(mdbcur_cpy_t), MdbCopyA1XX},
	{0xb100, sizeof(MdbCopyB1XX)/sizeof(mdbcur_cpy_t), MdbCopyB1XX},
	{0x9400, sizeof(MdbCopy94XX)/sizeof(mdbcur_cpy_t), MdbCopy94XX},
	{0x9500, sizeof(MdbCopy95XX)/sizeof(mdbcur_cpy_t), MdbCopy95XX},
	{0xa400, sizeof(MdbCopyA4XX)/sizeof(mdbcur_cpy_t), MdbCopyA4XX},
	{0xb400, sizeof(MdbCopyB4XX)/sizeof(mdbcur_cpy_t), MdbCopyB4XX},
	{0xa500, sizeof(MdbCopyA5XX)/sizeof(mdbcur_cpy_t), MdbCopyA5XX},
	{0xb500, sizeof(MdbCopyB5XX)/sizeof(mdbcur_cpy_t), MdbCopyB5XX},

	{0, 0, NULL},
};















/**
* @brief BCD移位操作
*/
static void BcdShiftRight(unsigned char *bcd, int len)
{
	int i;
	unsigned char uc;

	if(len <= 0) return;

	len -= 1;

	for(i=0; i<len; i++) {
		uc = *bcd;
		uc = (uc>>4)&0x0f;
		uc |= (bcd[1]<<4)&0xf0;
		*bcd++ = uc;
	}

	uc = *bcd;
	*bcd = (uc>>4)&0x0f;
}

static void BcdShiftLeft(unsigned char *bcd, int len)
{
	int i;
	unsigned char uc;
	unsigned char *p;

	if(len <= 0) return;

	len -= 1;
	bcd += len;

	for(i=len; i>0; i--) {
		p = bcd;
		uc = *bcd--;

		uc = (uc<<4)&0xf0;
		uc |= ((*bcd)>>4)&0x0f;
		*p = uc;
	}

	uc = *bcd;
	*bcd = (uc<<4)&0xf0;
}

/**
* @brief 将读取到的数据拷贝到mdbcurrent
* @param pcfg 数据项拷贝配置
* @param offset 偏移值
* @param flag 拷贝选项
*/
static void MdbCurCopyData(const mdbcur_cpy_t *pcfg, const unsigned char *psrc, unsigned int offset, unsigned char flag)
{
	unsigned char *pdst = pcfg->pbuf;
	unsigned char gblen, dl645len, i, shift, uc;

	pdst += offset;

	//PrintLog(0, "MdbCurCopyData.......................\n");
	if((UPCURFLAG_GB == flag) || (0 == pcfg->flag)) {
		memcpy(pdst, psrc, pcfg->len&0x0f);
		return;
	}

	shift = 0;
	gblen = pcfg->len&0x0f;
	dl645len = (pcfg->len>>4)&0x0f;
	if(pcfg->flag&MCPYFLAG_RSHIFT) { //div
		shift = pcfg->flag&0x0f;
		uc = shift&0x01;
		shift >>= 1;
		psrc += shift;
		dl645len -= shift;
		shift = uc;
	}
	else if(pcfg->flag&MCPYFLAG_LSHIFT) { //multi
		shift = pcfg->flag&0x0f;
		uc = shift&0x01;
		shift >>= 1;
		for(i=0; i<shift; i++) *pdst++ = 0;
		gblen -= shift;
		shift = uc;
	}

	for(i=0; i<gblen; i++) {
		if(i>= dl645len) pdst[i] = 0;
		else pdst[i] = psrc[i];
	}

	if(!shift) return;

	if(pcfg->flag&MCPYFLAG_RSHIFT) { //div
		if(2 == gblen) {
			unsigned short us = ((unsigned short)pdst[1]<<8)+(unsigned short)pdst[0];

			us >>= 4;
			pdst[1] = (us>>8) & 0xff;
			pdst[0] = us & 0xff;
		}
		else if(4 == gblen) {
			unsigned long ul = ((unsigned long)pdst[3]<<24) + ((unsigned long)pdst[2]<<16) \
							+ ((unsigned long)pdst[1]<<8) + (unsigned long)pdst[0];

			ul >>= 4;
			pdst[0] = ul & 0xff;
			pdst[1] = (ul>>8) & 0xff;
			pdst[2] = (ul>>16) & 0xff;
			pdst[3] = (ul>>24) & 0xff;
		}
		else
			BcdShiftRight(pdst, gblen);
	}
	else {
		if(2 == gblen) {
			unsigned short us = ((unsigned short)pdst[1]<<8)+(unsigned short)pdst[0];

			us <<= 4;
			pdst[1] = (us>>8) & 0xff;
			pdst[0] = us & 0xff;
		}
		else if(4 == gblen) {
			unsigned long ul = ((unsigned long)pdst[3]<<24) + ((unsigned long)pdst[2]<<16) \
							+ ((unsigned long)pdst[1]<<8) + (unsigned long)pdst[0];

			ul <<= 4;
			pdst[0] = ul & 0xff;
			pdst[1] = (ul>>8) & 0xff;
			pdst[2] = (ul>>16) & 0xff;
			pdst[3] = (ul>>24) & 0xff;
		}
		else
			BcdShiftLeft(pdst, gblen);
	}
}



unsigned short consv_itemid_07_to_97(unsigned int itemid)
{
	unsigned short itemid_97;
	switch(itemid)
	{
	case 0x0001FF00:
		itemid_97 = 0x901f;
		break;
	case 0x0002FF00:
		itemid_97 = 0x902f;
		break;	
	case 0x00030000:
		itemid_97 = 0x9110;
		break;	
	case 0x00040000:
		itemid_97 = 0x9120;
		break;
	case 0x0201FF00:
		itemid_97 = 0xb61f;
		break;
	case 0x0202FF00:
		itemid_97 = 0xb62f;
		break;
	case 0x0203FF00:
		itemid_97 = 0xb63f;
		break;
	case 0x0204FF00:
		itemid_97 = 0xb64f;
		break;
	case 0x0206FF00:
		itemid_97 = 0xb65f;
		break;
	default:
		itemid_97 = 0;
		break;		
	}
	return	itemid_97;
}




//void UpdateMdbCurrent(unsigned char metpid, unsigned short itemid, const unsigned char *buf, int len, unsigned char flag)
void UpdateMdbCurrent(unsigned short metpid, unsigned int itemid, const unsigned char *buf, int len, unsigned char flag)
{
	unsigned char  dealflag = flag&UPCURFLAG_MASK;
	const mdbcur_grpcpy_t *pgrp = MdbCopyGroup;
	const mdbcur_cpy_t *pitem;
	int i, num, index;
	unsigned int offset;
	unsigned char bakrunstate = FLAG_MDBEMPTY;
	unsigned short itemid_tmp = 0;
	PrintLog(0, "UpdateMdbCurrent.........\n");
	PrintLog(0, "metpid = %d itemid = %x\n",metpid,itemid);
	//if((metpid==0) ||(metpid > MAX_CENMETP)) return;
	//metpid -= 1;
	//metpid += 1;
	
	offset = metpid;
	offset *= sizeof(mdbcur_t);

	if(flag&UPCURFLAG_ERROR) 
	{
		unsigned int errmask;

		errmask = GetMdbAnaFailMask(itemid);
		SetMdbAnaFailMask(metpid, errmask);
		if(0 == MdbCurrent[metpid].flag_comerr) 
		{
			MdbCurrent[metpid].cnt_comerr++;
			if(MdbCurrent[metpid].cnt_comerr > 20) 
			{
				alarm_t alarmbuf;

				MdbCurrent[metpid].cnt_comerr = 0;

				memset(&alarmbuf, 0, sizeof(alarmbuf));
				alarmbuf.erc = 31;
				alarmbuf.len = 16;
				alarmbuf.data[0] = (unsigned char)(metpid+1);
				alarmbuf.data[1] = (unsigned char)((metpid+1)>>8)|0x80;
				smallcpy(alarmbuf.data+2, MdbCurrent[metpid].rdtime, 5);
				smallcpy(alarmbuf.data+7, MdbCurrent[metpid].enepa, 5);
				smallcpy(alarmbuf.data+12, MdbCurrent[metpid].enepi, 4);

				MakeAlarm(ALMFLAG_ABNOR, &alarmbuf);

				MdbCurrentReInit(metpid);  // empty data(通讯失败时，清除数据)
				MdbCurrent[metpid].flag_comerr = 1;
			}
		}

		return;
	}
	else {
		mdbcur(metpid).cnt_comerr = 0;
		mdbcur(metpid).flag_comerr = 0;
	}

	UpdateMdbAna(metpid+1, itemid, buf, len, flag);
	
	UpdateMdbCurRdTime(metpid);//更新抄表时间	@add
	if(ParaMeter[metpid].proto == METTYPE_DL645_2007)
	{
		itemid_tmp = consv_itemid_07_to_97(itemid);
	}
	else if((ParaMeter[metpid].proto == METTYPE_DL645) ||(ParaMeter[metpid].proto == METTYPE_ACSAMP))
	{
		itemid_tmp = itemid;
	}
	 
	for(;0 != pgrp->itemid; pgrp++) 
	{
		if((itemid_tmp&0xff00) != pgrp->itemid) continue;
		//PrintLog(0, "itemid&0xff00 = %02x pgrp->itemid = %02x\n",itemid_tmp&0xff00,pgrp->itemid);
		pitem = pgrp->list;
		for(index=0; index<pgrp->num; index++,pitem++) 
		{
			if(itemid_tmp != pitem->itemid) continue;
			//PrintLog(0, "itemid = %02x pitem->itemid = %02x\n",itemid_tmp,pitem->itemid);
			if(0xc020 == itemid_tmp) bakrunstate = MdbCurrent[metpid].met_runstate[0];

			if(MCPYFLAG_GROUP == pitem->flag) {
				num = pitem->len & 0xff;
				pitem++;

				for(i=0; i<num; i++,pitem++) {
					MdbCurCopyData(pitem, buf, offset, dealflag);
					if(UPCURFLAG_GB == dealflag) buf += pitem->len&0x0f;
					else buf += (pitem->len >> 4)&0x0f;
				}
			}
			else {
				MdbCurCopyData(pitem, buf, offset, dealflag);
			}

			if(0xc020 == itemid_tmp) {
				if(FLAG_MDBEMPTY != bakrunstate)
					MdbCurrent[metpid].flagchg_state[0] = bakrunstate ^ MdbCurrent[metpid].met_runstate[0];
			}

			return;
		}
	}
}

static const unsigned short mdbids_F25[] = {
	0xb63f, 0xb64f, 0xb65f, 0xb61f,
	0xb62f,
};

#if 0
static const unsigned short mdbids_F26[] = {
	0xb310, 0xb311, 0xb312, 0xb313, 
	0xb320, 0xb321, 0xb322, 0xb323, 
	0xb330, 0xb331, 0xb332, 0xb333, 
	0xb340, 0xb341, 0xb342, 0xb343, 
};
#else
static const unsigned short mdbids_F26[] = {
	0xb31f, 0xb32f, 0xb33f, 0xb34f, 
};
#endif

static const unsigned short mdbids_F27[] = {
	0xc010, 0xc011, 0xc020, 
	0xb210, 0xb211, 0xb212, 0xb213, 
	0xb214, 
};

static const unsigned short mdbids_F33[] = {
	0x901f, 0x911f, 0x913f, 0x914f,
};

static const unsigned short mdbids_F34[] = {
	0x902f, 0x912f, 0x915f, 0x916f,
};

static const unsigned short mdbids_F35[] = {
	0xa01f, 0xb01f, 0xa11f, 0xb11f,
};

static const unsigned short mdbids_F36[] = {
	0xa02f, 0xb02f, 0xa12f, 0xb12f,
};

static const unsigned short mdbids_F37[] = {
	0x941f, 0x951f, 0x953f, 0x954f,
};

static const unsigned short mdbids_F38[] = {
	0x942f, 0x952f, 0x955f, 0x956f,
};

static const unsigned short mdbids_F39[] = {
	0xa41f, 0xb41f, 0xa51f, 0xb51f,
};

static const unsigned short mdbids_F40[] = {
	0xa42f, 0xb42f, 0xa52f, 0xb52f,
};

#if 1
static const unsigned short mdbids_F129[] = {
	0x901f,
};
#endif

#if 0
static const unsigned short mdbids_F129[] = {
	0x9010,
};
#endif

static const unsigned short mdbids_F130[] = {
	0x911f,
};

static const unsigned short mdbids_F131[] = {
	0x902f,
};

static const unsigned short mdbids_F132[] = {
	0x912f,
};

static const unsigned short mdbids_F133[] = {
	0x913f,
};

static const unsigned short mdbids_F134[] = {
	0x915f,
};

static const unsigned short mdbids_F135[] = {
	0x916f,
};

static const unsigned short mdbids_F136[] = {
	0x914f,
};

static const unsigned short mdbids_F137[] = {
	0x941f,
};

static const unsigned short mdbids_F138[] = {
	0x951f,
};

static const unsigned short mdbids_F139[] = {
	0x942f,
};

static const unsigned short mdbids_F140[] = {
	0x952f,
};

static const unsigned short mdbids_F141[] = {
	0x953f,
};

static const unsigned short mdbids_F142[] = {
	0x955f,
};

static const unsigned short mdbids_F143[] = {
	0x956f,
};

static const unsigned short mdbids_F144[] = {
	0x954f,
};

static const unsigned short mdbids_F145[] = {
	0xa01f, 0xb01f,
};

static const unsigned short mdbids_F146[] = {
	0xa11f, 0xb11f,
};

static const unsigned short mdbids_F147[] = {
	0xa02f, 0xb02f,
};

static const unsigned short mdbids_F148[] = {
	0xa12f, 0xb12f,
};

static const unsigned short mdbids_F149[] = {
	0xa41f, 0xb41f,
};

static const unsigned short mdbids_F150[] = {
	0xa51f, 0xb51f,
};

static const unsigned short mdbids_F151[] = {
	0xa42f, 0xb42f,
};

static const unsigned short mdbids_F152[] = {
	0xa52f, 0xb52f,
};

typedef struct {
	unsigned short itemid;   //数据单元标识
	unsigned short len;    //数据长度
	unsigned char *pbase;  //数据所在指针
	const unsigned short *rditems;    //抄表数据标识
	unsigned short rdnum;   //抄表数据项数目
	unsigned char  bfenum;   //是否带费率数
	unsigned char  brdtime;   //是否带终端抄表时间
} mdbcur_offset_t;


static const mdbcur_offset_t MdbOffsetF25[] = {
	{0x0301, 58, mdbcur(0).pwra, mdbids_F25, sizeof(mdbids_F25)/2, 0, 1}, //F25
	{0x0302, 52, mdbcur(0).volbr_cnt, mdbids_F26, sizeof(mdbids_F26)/2, 0, 1}, //F26
	{0x0304, 55, mdbcur(0).met_clock, mdbids_F27, sizeof(mdbids_F27)/2, 0, 1}, //F27
	{0x0308, 28, mdbcur(0).flagchg_state, NULL, 0, 0, 1}, //F28
	{0x0310, 50, mdbcur(0).enecopper, NULL, 0, 0, 1}, //F29
	{0x0320, 50, mdbcur(0).enecopper_lm, NULL, 0, 0, 1}, //F30
	{0x0340, 54, mdbcur(0).enepa_a, NULL, 0, 0, 1}, //F31
	{0x0380, 54, mdbcur(0).enepa_a_lm, NULL, 0, 0, 1}, //F32
};

static const mdbcur_offset_t MdbOffsetF33[] = {
	{0x0401, 85, mdbcur(0).enepa, mdbids_F33, sizeof(mdbids_F33)/2, 1, 1}, //F33
	{0x0402, 85, mdbcur(0).enena, mdbids_F34, sizeof(mdbids_F34)/2, 1, 1}, //F34
	{0x0404, 70, mdbcur(0).dmnpa, mdbids_F35, sizeof(mdbids_F35)/2, 1, 1}, //F35
	{0x0408, 70, mdbcur(0).dmnna, mdbids_F36, sizeof(mdbids_F36)/2, 1, 1}, //F36
	{0x0410, 85, mdbcur(0).enepa_lm, mdbids_F37, sizeof(mdbids_F37)/2, 1, 1}, //F37
	{0x0420, 85, mdbcur(0).enena_lm, mdbids_F38, sizeof(mdbids_F38)/2, 1, 1}, //F38
	{0x0440, 70, mdbcur(0).dmnpa_lm, mdbids_F39, sizeof(mdbids_F39)/2, 1, 1}, //F39
	{0x0480, 70, mdbcur(0).dmnna_lm, mdbids_F40, sizeof(mdbids_F40)/2, 1, 1}, //F40
};

static const mdbcur_offset_t MdbOffsetF49[] = {
	{0x0601, 12, mdbcur(0).phase_arc, NULL, 0, 0, 0}, //F49
};

static const mdbcur_offset_t MdbOffsetF129[] = {
	{0x1001, 25, mdbcur(0).enepa, mdbids_F129, sizeof(mdbids_F129)/2, 1, 1}, //F129
	{0x1002, 20, mdbcur(0).enepi, mdbids_F130, sizeof(mdbids_F130)/2, 1, 1}, //F130
	{0x1004, 25, mdbcur(0).enena, mdbids_F131, sizeof(mdbids_F131)/2, 1, 1}, //F131
	{0x1008, 20, mdbcur(0).eneni, mdbids_F132, sizeof(mdbids_F132)/2, 1, 1}, //F132
	{0x1010, 20, mdbcur(0).enepi1, mdbids_F133, sizeof(mdbids_F133)/2, 1, 1}, //F133
	{0x1020, 20, mdbcur(0).eneni2, mdbids_F134, sizeof(mdbids_F134)/2, 1, 1}, //F134
	{0x1040, 20, mdbcur(0).eneni3, mdbids_F135, sizeof(mdbids_F135)/2, 1, 1}, //F135
	{0x1080, 20, mdbcur(0).enepi4, mdbids_F136, sizeof(mdbids_F136)/2, 1, 1}, //F136
};

static const mdbcur_offset_t MdbOffsetF137[] = {
	{0x1101, 25, mdbcur(0).enepa_lm, mdbids_F137, sizeof(mdbids_F137)/2, 1, 1}, //F137
	{0x1102, 20, mdbcur(0).enepi_lm, mdbids_F138, sizeof(mdbids_F138)/2, 1, 1}, //F138
	{0x1104, 25, mdbcur(0).enena_lm, mdbids_F139, sizeof(mdbids_F139)/2, 1, 1}, //F139
	{0x1108, 20, mdbcur(0).eneni_lm, mdbids_F140, sizeof(mdbids_F140)/2, 1, 1}, //F140
	{0x1110, 20, mdbcur(0).enepi1_lm, mdbids_F141, sizeof(mdbids_F141)/2, 1, 1}, //F141
	{0x1120, 20, mdbcur(0).eneni2_lm, mdbids_F142, sizeof(mdbids_F142)/2, 1, 1}, //F142
	{0x1140, 20, mdbcur(0).eneni3_lm, mdbids_F143, sizeof(mdbids_F143)/2, 1, 1}, //F143
	{0x1180, 20, mdbcur(0).enepi4_lm, mdbids_F144, sizeof(mdbids_F144)/2, 1, 1}, //F144
};

/**
* @brief 更新抄表时间
* @param mid 测量点号, 0~
*/
void UpdateMdbCurRdTime(unsigned short mid)
{
	sysclock_t clock;

	SysClockReadCurrent(&clock);

	mdbcur(mid).rdtime[0] = clock.minute;
	mdbcur(mid).rdtime[1] = clock.hour;
	mdbcur(mid).rdtime[2] = clock.day;
	mdbcur(mid).rdtime[3] = clock.month;
	mdbcur(mid).rdtime[4] = clock.year;
	HexToBcd(mdbcur(mid).rdtime, 5);
}

static int ReadMdbF57(unsigned short mid, unsigned short itemid, unsigned char *buf, int len)
{
	int actlen;
	unsigned char *pdata;

	switch(itemid) {
	case 0x0701://F57
		actlen = (int)MdbCurrent[mid].syntony_value[0]&0xff;
		if(actlen > 19) actlen = 19;

		if(0 == actlen) actlen = 1;
		else actlen = (actlen-1)*6*2 + 1;

		pdata = MdbCurrent[mid].syntony_value;
		break;

	case 0x0702://F58
		actlen = (int)MdbCurrent[mid].syntony_rating[0]&0xff;
		if(actlen > 19) actlen = 19;

		if(0 == actlen) actlen = 1;
		else actlen = actlen*3*2 + (actlen-1)*3*2 + 1 ;

		pdata = MdbCurrent[mid].syntony_rating;
		break;

	default:
		return -2;
	}

	if(actlen > len) return -1;
	memcpy(buf, pdata, actlen);
	return actlen;
}

static int ReadMdbF145(unsigned short mid, unsigned short itemid, unsigned char *buf, int len)
{
	#define ITEM_LEN	(7*MAXNUM_METPRD+6)

	unsigned char *pdmn, *ptime;
	unsigned short items[2];
	int i;

	if(len < ITEM_LEN) return -1;

	switch(itemid) {
	case 0x1201://F145
		pdmn = MdbCurrent[mid].dmnpa;
		ptime = MdbCurrent[mid].dmntpa;
		items[0] = 0xa01f;
		items[1] = 0xb01f;
		break;
	case 0x1202://F146
		pdmn = MdbCurrent[mid].dmnpi;
		ptime = MdbCurrent[mid].dmntpi;
		items[0] = 0xa11f;
		items[1] = 0xb11f;
		break;
	case 0x1204://F47
		pdmn = MdbCurrent[mid].dmnna;
		ptime = MdbCurrent[mid].dmntna;
		items[0] = 0xa02f;
		items[1] = 0xb02f;
		break;
	case 0x1208://F148
		pdmn = MdbCurrent[mid].dmnni;
		ptime = MdbCurrent[mid].dmntni;
		items[0] = 0xa42f;
		items[1] = 0xb42f;
		break;
	case 0x1210://F149
		pdmn = MdbCurrent[mid].dmnpa_lm;
		ptime = MdbCurrent[mid].dmntpa_lm;
		items[0] = 0xa41f;
		items[1] = 0xb41f;
		break;
	case 0x1220://F150
		pdmn = MdbCurrent[mid].dmnpi_lm;
		ptime = MdbCurrent[mid].dmntpi_lm;
		items[0] = 0xa51f;
		items[1] = 0xb51f;
		break;
	case 0x1240://F151
		pdmn = MdbCurrent[mid].dmnna_lm;
		ptime = MdbCurrent[mid].dmntna_lm;
		items[0] = 0xa42f;
		items[1] = 0xb42f;
		break;
	case 0x1280://F152
		pdmn = MdbCurrent[mid].dmnni_lm;
		ptime = MdbCurrent[mid].dmntni_lm;
		items[0] = 0xa52f;
		items[1] = 0xb52f;
		break;
	default: return -2;
	}

	ReadImmMdbCur(mid, items, 2);
	UpdateMdbCurRdTime(mid);

	smallcpy(buf, MdbCurrent[mid].rdtime, 5);
	buf += 5;
	*buf++ = MAXNUM_FEEPRD;
	
	for(i=0; i<MAXNUM_METPRD; i++) {
		smallcpy(buf, pdmn, 3); buf += 3; pdmn += 3;
		smallcpy(buf, ptime, 4); buf += 4; ptime += 4;
	}

	return ITEM_LEN;
}

typedef int (*readmdbcur_fn)(unsigned short, unsigned short, unsigned char *, int);
typedef struct {
	unsigned short itemid;
	unsigned short bfunc;
	const mdbcur_offset_t *list;
	unsigned short num;
} mdbcur_grpread_t;
static const mdbcur_grpread_t MdbGroupRead[] = {
	{0x0301, 0, MdbOffsetF25,  sizeof(MdbOffsetF25)/sizeof(mdbcur_offset_t)},
	{0x0401, 0, MdbOffsetF33,  sizeof(MdbOffsetF33)/sizeof(mdbcur_offset_t)},
	{0x0601, 0, MdbOffsetF49,  sizeof(MdbOffsetF49)/sizeof(mdbcur_offset_t)},
	{0x0701, 1, (const mdbcur_offset_t *)ReadMdbF57, 0},
	{0x1001, 0, MdbOffsetF129, sizeof(MdbOffsetF129)/sizeof(mdbcur_offset_t)}, 
	{0x1101, 0, MdbOffsetF137, sizeof(MdbOffsetF137)/sizeof(mdbcur_offset_t)}, 
	{0x1201, 1, (const mdbcur_offset_t *)ReadMdbF145, 0},

	{0, 0, NULL, 0},
};

/**
* @brief 读取当前表计数据
* @param metpid 测量点号, 1~MAX_CENMETP
* @param itemid 数据项编号
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度, 失败返回负数, 无此数据项返回-2, 缓存区溢出返回-1
*/
int ReadMdbCurrent(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len)
{
	mdbcur_t *pdb;
	unsigned char *pdata;
	int i, actlen;
	const mdbcur_grpread_t *pgrp = MdbGroupRead;
	const mdbcur_offset_t *poffset;

	actlen = 0;

	PrintLog(0, "ReadMdbCurrent1\n");
	if((metpid==0) ||(metpid > MAX_CENMETP)) return -2;
	metpid -= 1;
	itemid &= 0x7fff;

	for(;0 != pgrp->itemid; pgrp++) {
		if((pgrp->itemid&0xff00) == (itemid&0xff00)) break;
	}
	if(0 == pgrp->itemid) return -2;
	PrintLog(0, "metpid = %x itemid = %x\n",metpid,itemid);
	//为F57、F145时调用ReadMdbF57、ReadMdbF145
	if(pgrp->bfunc) return ((*((readmdbcur_fn)pgrp->list))(metpid, itemid, buf, len));

	PrintLog(0, "metpid = %x itemid = %x\n",metpid,itemid);
	poffset = pgrp->list;
	pdb = &MdbCurrent[metpid];
	PrintLog(0, "ReadMdbCurrent2\n");

	for(i=0; i<pgrp->num; i++,poffset++) {
		PrintLog(0, "itemid = %x poffset->itemid = %x\n",itemid,poffset->itemid);
		if(itemid == poffset->itemid) break;
	}
	PrintLog(0, "ReadMdbCurrent3\n");
	if(i >= pgrp->num) return -2;
	PrintLog(0, "ReadMdbCurrent4\n");
	actlen = poffset->len;
	if(poffset->brdtime) actlen += 5;
	if(poffset->bfenum) actlen += 1;
	if(actlen > len) return -1;
	PrintLog(0, "ReadMdbCurrent5\n");
	//if(metpid>=1)	metpid -= 1;
	//PrintLog(0, "metpid = %d\n",metpid);
	if(NULL != poffset->rditems)
		ReadImmMdbCur(metpid, poffset->rditems, poffset->rdnum);
	PrintLog(0, "ReadMdbCurrent6\n");
	if(poffset->brdtime) {
		smallcpy(buf, pdb->rdtime, 5);//终端抄表时间
		buf += 5;
	}
	PrintLog(0, "ReadMdbCurrent7\n");
	if(poffset->bfenum) *buf++ = MAXNUM_FEEPRD;//费率固定为4

	pdata = poffset->pbase;
	pdata += (unsigned int)metpid*sizeof(mdbcur_t);

	smallcpy(buf, pdata, poffset->len);
	PrintLog(0, "ReadMdbCurrent8\n");
	return actlen;
}

