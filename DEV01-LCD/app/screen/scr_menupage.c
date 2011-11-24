/**
* scr_menupage.c -- 显示翻页菜单
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#include <string.h>
#include <stdio.h>   
#include <stdlib.h>

#define DEFINE_PARATERM
#define DEFINE_PARAUNI

#include "scr_menu.h"
#include "scr_input.h"
#include "scr_input_api.h"
#include "scr_lcd.h"
#include "scr_menupage.h"
#include "include/param/term.h"
#include "include/param/mix.h"
#include "include/param/hardware.h"
#include "include/param/commport.h"
#include "include/param/meter.h"
#include "include/param/metp.h"
#include "include/param/datatask.h"
#include "include/param/operation.h"
#include "include/param/capconf.h"
#include "include/param/route.h"
#include "include/param/unique.h"
#include "include/lib/dbtime.h"
#include "cenmet/mdb/mdbana.h"
#include "cenmet/mdb/mdbcur.h"
#include "cenmet/mdb/mdbconf.h"
#include "include/sys/timeal.h"  
#include "plcmet/plmdb.h"
#include "plcmet/plc_stat.h"
#include "include/version.h"
#include "include/debug.h"
#include "uplink/terminfo.h"
#include "include/monitor/runstate.h"
#include "uplink/ppp/ipcp.h"
#include "include/plcmet/pltask.h"
#include "include/sys/timeal.h"
#include "include/sys/reset.h"
#include "include/sys/schedule.h"
#include "screen/scr_lcd.h"
#include "param/operation_inner.h"
#include "include/lib/align.h"
#include "include/lib/bcd.h"

#define INTNUM_MASK			0x0c
#define DOTNUM_MASK		    0x03
#define PWRPHASE				0x0c
#define TASK_RUN				0x01
#define TASK_STOP				0x00
#define COMMFRAME_DATA		    0x03
#define METPRDNUM_MASK			0x1f

#define DISPLAY_MET_MUN		5

extern para_term_t ParaTerm;
extern para_uni_t ParaUni;
extern scrpage_t scrpbuf_main;
#define CHAR_FLAG           1
//设置切换选择菜单
extern const char typ_charlist_a[];
static const char typ_charlist_a1[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',};
static const char typ_stringlist_a[11][MAX_LINECHAR] = {"  无效   ","  短信   ", "GPRS/CDMA", "  DTMF   ", " 以太网  ", "  红外   ", "  RS232  ", "   CSD   ", "  Radio  ", " 短信唤醒","  级连   "};
static const char typ_stringlist_tcp[2][MAX_LINECHAR] = {"TCP","UDP"};
static const char typ_stringlist_c[2][MAX_LINECHAR] = {"无效","有效"};
static const char typ_stringlist_d[6][MAX_LINECHAR] = {"未知    ", "485表   ", "模拟量  ", "脉冲量  ", "计算值  ", "交流采样"};
static const char typ_stringlist_c1[2][MAX_LINECHAR] = {"选项1","有效"};
static const char typ_stringlist_d1[6][MAX_LINECHAR] = {"选项2    ", "485表   ", "模拟量  ", "脉冲量  ", "计算值  ", "交流采样"};
static const char typ_stringlist_e[5][MAX_LINECHAR] = {"无效","1   ","2   ","3   ","4   "};
static const char typ_stringlist_f[27][MAX_LINECHAR] ={"无效     ","部颁     ","IEC1107  ","威胜     ","洪相     ","浩宁达   ","华隆     ",
											"龙电     ","兰吉尔D表","许继     ","科陆     ","三星     ","爱拓利   ",
											"ABBa表   ","ABB圆表  ","大崎表   ","红相MK3表","华立     ","兰吉尔B表",
											"林洋     ","东方电子 ","伊梅尔   ","伊斯卡   ","埃尔斯特 ","威胜     ",
											"部分块抄 ","单条抄   "};
static const char typ_stringlist_g[3][MAX_LINECHAR] ={"无校验", "偶校验", "奇校验"};

//设置选择切换菜单
static const char typ_charlist_1[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
static const char typ_charlist_2[] = {'5', '6', '7','8'};
static const char typ_charlist_bcoms[2][MAX_LINECHAR]={"否","是"};
static const char typ_charlist_stopbit[2][MAX_LINECHAR]={"1","2"};
static const char typ_charlist_databit[4][MAX_LINECHAR]={"5","6","7","8"};
//static const char typ_charlist_3[] = {' ','A', 'B', 'C','D','E', 'F', 'G','H','I', 'J', 'K','L','M', 'N',
//							'O','P','Q', 'R', 'S','T','U', 'V', 'W','X','Y','Z','.'};
static const char typ_charlist_3[] = {'A', 'B', 'C','D','E', 'F', 'G','H','I', 'J', 'K','L','M', 'N',
							'O','P','Q', 'R', 'S','T','U', 'V', 'W','X','Y','Z','.'};
static const char typ_charlist_4[] = {'1', '2'};
static const char typ_charlist_5[] = {' ','0', '1', '2', '3', '4', '5', '6', '7', '8', '9','a','b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
							'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u','v', 'w', 'x', 'y', 'z','A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K','L','M', 'N',
							'O','P','Q', 'R', 'S','T','U', 'V', 'W','X','Y','Z'};
static const char typ_charlist_3_1[] = {'.'};
static const char typ_stringlist_fnum[27]={0,10,20,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,55,51,18,19};
static const char typ_baudrate[10][MAX_LINECHAR]={"    0","  300","  600"," 1200"," 2400"," 4800"," 7200"," 9600","19200","57600"};
static const char typ_charlist_mid[16][MAX_LINECHAR]={"0 ","1 ","2 ","3 ","4 ","5 ","6 ","7 ","8 ","9 ","10","11","12","13","14","15"};

unsigned int GetPortNum();
unsigned char GetTaskNum();
unsigned short GetSnNum();
unsigned int GetMajorMetNum();
void DefOkClick(void);
void DefEscClick(void);
void InitControlPage(void);
int AddButton(unsigned char x, unsigned char y, char *text, pf_button_click_t pf_click);
int ShowControlPage(void);
int AddEditBox(unsigned char x, unsigned char y, int numchars, char *charlist, int defsel, unsigned char *prtn,char flag);

#define SCRINFO_NULLMET  { \
	strcpy(scrpbuf_main.buf[0],"");\
	strcpy(scrpbuf_main.buf[1], "        提示"); \
	strcpy(scrpbuf_main.buf[2],"    不能获取相应测");\
	strcpy(scrpbuf_main.buf[3], "量点的数据,请检查相");\
	strcpy(scrpbuf_main.buf[4],"应测量点配置信息");\
	scrpbuf_main.line = 5; }

extern const int SvrCommLineState;

    
/**
* @brief			轮显首界面
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpHead(unsigned short mid, ULONG arg, UCHAR key)
{
	ClearTopBuffer();
	scrpbuf_main.line = 2;  
	strcpy(scrpbuf_main.buf[1], "    武汉星程电子");
	strcpy(scrpbuf_main.buf[2], "    科技有限公司");
}

/**
* @brief			显示总表电能量界面
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpMetDataEne(unsigned short mid, ULONG arg, UCHAR key)
{
	ScreenSetHeadFlag(DISPLAY_MET_MUN,mid);
    scrpbuf_main.line = 18;
	sprintf(scrpbuf_main.buf[0],"正向有功总电能:");
	sprintf(scrpbuf_main.buf[1],"  总=%X%02X%02X.%02X%02XkWh",MdbCurrent[mid].enepa[4],MdbCurrent[mid].enepa[3],MdbCurrent[mid].enepa[2],MdbCurrent[mid].enepa[1],MdbCurrent[mid].enepa[0]);
	sprintf(scrpbuf_main.buf[2],"  P1=%X%02X%02X.%02X%02XkWh",MdbCurrent[mid].enepa[9],MdbCurrent[mid].enepa[8],MdbCurrent[mid].enepa[7],MdbCurrent[mid].enepa[6],MdbCurrent[mid].enepa[5]);
	sprintf(scrpbuf_main.buf[3],"  P2=%X%02X%02X.%02X%02XkWh",MdbCurrent[mid].enepa[14],MdbCurrent[mid].enepa[13],MdbCurrent[mid].enepa[12],MdbCurrent[mid].enepa[11],MdbCurrent[mid].enepa[10]);
	sprintf(scrpbuf_main.buf[4],"  P3=%X%02X%02X.%02X%02XkWh",MdbCurrent[mid].enepa[19],MdbCurrent[mid].enepa[18],MdbCurrent[mid].enepa[17],MdbCurrent[mid].enepa[16],MdbCurrent[mid].enepa[15]);
	sprintf(scrpbuf_main.buf[5],"  P4=%X%02X%02X.%02X%02XkWh",MdbCurrent[mid].enepa[24],MdbCurrent[mid].enepa[23],MdbCurrent[mid].enepa[22],MdbCurrent[mid].enepa[21],MdbCurrent[mid].enepa[20]);

	sprintf(scrpbuf_main.buf[7],"反向有功总电能:");
    sprintf(scrpbuf_main.buf[8],"  总=%X%02X%02X.%02X%02XkWh",MdbCurrent[mid].enena[4],MdbCurrent[mid].enena[3],MdbCurrent[mid].enena[2],MdbCurrent[mid].enena[1],MdbCurrent[mid].enena[0]);
	sprintf(scrpbuf_main.buf[9],"正向无功总电能:");
	sprintf(scrpbuf_main.buf[10],"  总=%X%02X%02X.%02Xkvarh",MdbCurrent[mid].enepi[3],MdbCurrent[mid].enepi[2],MdbCurrent[mid].enepi[1],MdbCurrent[mid].enepi[0]);
	sprintf(scrpbuf_main.buf[11],"  一=%X%02X%02X.%02Xkvarh",MdbCurrent[mid].enepi1[3],MdbCurrent[mid].enepi1[2],MdbCurrent[mid].enepi1[1],MdbCurrent[mid].enepi1[0]);
	sprintf(scrpbuf_main.buf[12],"  四=%X%02X%02X.%02Xkvarh",MdbCurrent[mid].enepi4[3],MdbCurrent[mid].enepi4[2],MdbCurrent[mid].enepi4[1],MdbCurrent[mid].enepi4[0]);

	sprintf(scrpbuf_main.buf[14],"反向无功总电能:");
	sprintf(scrpbuf_main.buf[15],"  总=%X%02X%02X.%02Xkvarh",MdbCurrent[mid].eneni[3],MdbCurrent[mid].eneni[2],MdbCurrent[mid].eneni[1],MdbCurrent[mid].eneni[0]);
	sprintf(scrpbuf_main.buf[16],"  二=%X%02X%02X.%02Xkvarh",MdbCurrent[mid].eneni2[3],MdbCurrent[mid].eneni2[2],MdbCurrent[mid].eneni2[1],MdbCurrent[mid].eneni2[0]);
	sprintf(scrpbuf_main.buf[17],"  三=%X%02X%02X.%02Xkvarh",MdbCurrent[mid].eneni3[3],MdbCurrent[mid].eneni3[2],MdbCurrent[mid].eneni3[1],MdbCurrent[mid].eneni3[0]);


}
/**
* @brief			显示总表电需量界面
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpMetDataDmn(unsigned short mid, ULONG arg, UCHAR key)
{
	ScreenSetHeadFlag(DISPLAY_MET_MUN,mid);
    scrpbuf_main.line = 14;
	sprintf(scrpbuf_main.buf[0],"正向有功最大需量:");
	sprintf(scrpbuf_main.buf[1],"  %X.%02X%02XkW",MdbCurrent[mid].dmnpa[2],MdbCurrent[mid].dmnpa[1],MdbCurrent[mid].dmnpa[0]);
	sprintf(scrpbuf_main.buf[2],"  %02X-%02X  %02X:%02X",MdbCurrent[mid].dmntpa[3],MdbCurrent[mid].dmntpa[2],MdbCurrent[mid].dmntpa[1],MdbCurrent[mid].dmntpa[0]);
	sprintf(scrpbuf_main.buf[3],"正向无功最大需量:");
	sprintf(scrpbuf_main.buf[4],"  %X.%02X%02Xkvar",MdbCurrent[mid].dmnpi[2],MdbCurrent[mid].dmnpi[1],MdbCurrent[mid].dmnpi[0]);
	sprintf(scrpbuf_main.buf[5],"  %02X-%02X  %02X:%02X",MdbCurrent[mid].dmntpi[3],MdbCurrent[mid].dmntpi[2],MdbCurrent[mid].dmntpi[1],MdbCurrent[mid].dmntpi[0]);

	sprintf(scrpbuf_main.buf[7],"反向有功最大需量:");
	sprintf(scrpbuf_main.buf[8],"  %X.%02X%02XkW",MdbCurrent[mid].dmnna[2],MdbCurrent[mid].dmnna[1],MdbCurrent[mid].dmnna[0]);
	sprintf(scrpbuf_main.buf[9],"  %02X-%02X  %02X:%02X",MdbCurrent[mid].dmntna[3],MdbCurrent[mid].dmntna[2],MdbCurrent[mid].dmntna[1],MdbCurrent[mid].dmntna[0]);
	sprintf(scrpbuf_main.buf[10],"反向无功最大需量:");
	sprintf(scrpbuf_main.buf[11],"  %X.%02X%02Xkvar",MdbCurrent[mid].dmnni[2],MdbCurrent[mid].dmnni[1],MdbCurrent[mid].dmnni[0]);
	sprintf(scrpbuf_main.buf[12],"  %02X-%02X  %02X:%02X",MdbCurrent[mid].dmntni[3],MdbCurrent[mid].dmntni[2],MdbCurrent[mid].dmntni[1],MdbCurrent[mid].dmntni[0]);
}
/**
* @brief			显示总表电瞬时量界面
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpMetDataImm(unsigned short mid, ULONG arg, UCHAR key)
{
	ScreenSetHeadFlag(DISPLAY_MET_MUN,mid);
	sprintf(scrpbuf_main.buf[0]," Ua=%d.%XV",(MdbCurrent[mid].vol[1]>>4)*100 + (MdbCurrent[mid].vol[1]&0x0F)*10 + (MdbCurrent[mid].vol[0]>>4),MdbCurrent[mid].vol[0]&0x0f);
	sprintf(scrpbuf_main.buf[1]," Ub=%d.%XV",(MdbCurrent[mid].vol[3]>>4)*100 + (MdbCurrent[mid].vol[3]&0x0F)*10 + (MdbCurrent[mid].vol[2]>>4),MdbCurrent[mid].vol[2]&0x0f);
	sprintf(scrpbuf_main.buf[2]," Uc=%d.%XV",(MdbCurrent[mid].vol[5]>>4)*100 + (MdbCurrent[mid].vol[5]&0x0F)*10 + (MdbCurrent[mid].vol[4]>>4),MdbCurrent[mid].vol[4]&0x0f);
	sprintf(scrpbuf_main.buf[3]," Ia=%X.%02XA",MdbCurrent[mid].amp[1],MdbCurrent[mid].amp[0]);
	sprintf(scrpbuf_main.buf[4]," Ib=%X.%02XA",MdbCurrent[mid].amp[3],MdbCurrent[mid].amp[2]);
	sprintf(scrpbuf_main.buf[5]," Ic=%X.%02XA",MdbCurrent[mid].amp[5],MdbCurrent[mid].amp[4]);

	sprintf(scrpbuf_main.buf[7]," In=%X.%02XA",MdbCurrent[mid].amp[7],MdbCurrent[mid].amp[6]);
	sprintf(scrpbuf_main.buf[8]," Ps=%X.%02X%02XkW",MdbCurrent[mid].pwra[2],MdbCurrent[mid].pwra[1],MdbCurrent[mid].pwra[0]);
	sprintf(scrpbuf_main.buf[9]," Pa=%X.%02X%02XkW",MdbCurrent[mid].pwra[5],MdbCurrent[mid].pwra[4],MdbCurrent[mid].pwra[3]);
	sprintf(scrpbuf_main.buf[10]," Pb=%X.%02X%02XkW",MdbCurrent[mid].pwra[8],MdbCurrent[mid].pwra[7],MdbCurrent[mid].pwra[6]);
	sprintf(scrpbuf_main.buf[11]," Pc=%X.%02X%02XkW",MdbCurrent[mid].pwra[11],MdbCurrent[mid].pwra[10],MdbCurrent[mid].pwra[9]);
	if(MdbCurrent[mid].pwri[2]&0x80)
	sprintf(scrpbuf_main.buf[12]," Qs=-%X.%02X%02Xkvar",MdbCurrent[mid].pwri[2]&0x7f,MdbCurrent[mid].pwri[1],MdbCurrent[mid].pwri[0]);
	else
	sprintf(scrpbuf_main.buf[12]," Qs=%X.%02X%02Xkvar",MdbCurrent[mid].pwri[2],MdbCurrent[mid].pwri[1],MdbCurrent[mid].pwri[0]);

	if(MdbCurrent[mid].pwri[5]&0x80)
	sprintf(scrpbuf_main.buf[14]," Qa=-%X.%02X%02Xkvar",MdbCurrent[mid].pwri[5]&0x7f,MdbCurrent[mid].pwri[4],MdbCurrent[mid].pwri[3]);
	else
	sprintf(scrpbuf_main.buf[14]," Qa=%X.%02X%02Xkvar",MdbCurrent[mid].pwri[5],MdbCurrent[mid].pwri[4],MdbCurrent[mid].pwri[3]);

	if(MdbCurrent[mid].pwri[8]&0x80)
	sprintf(scrpbuf_main.buf[15]," Qb=-%X.%02X%02Xkvar",MdbCurrent[mid].pwri[8]&0x7f,MdbCurrent[mid].pwri[7],MdbCurrent[mid].pwri[6]);
	else
	sprintf(scrpbuf_main.buf[15]," Qb=%X.%02X%02Xkvar",MdbCurrent[mid].pwri[8],MdbCurrent[mid].pwri[7],MdbCurrent[mid].pwri[6]);

	if(MdbCurrent[mid].pwri[11]&0x80)
	sprintf(scrpbuf_main.buf[16]," Qc=-%X.%02X%02Xkvar",MdbCurrent[mid].pwri[11]&0x7f,MdbCurrent[mid].pwri[10],MdbCurrent[mid].pwri[9]);
	else
	sprintf(scrpbuf_main.buf[16]," Qc=%X.%02X%02Xkvar",MdbCurrent[mid].pwri[11],MdbCurrent[mid].pwri[10],MdbCurrent[mid].pwri[9]);

	sprintf(scrpbuf_main.buf[17]," COSΦ=%d.%X%s",((MdbCurrent[mid].pwrf[1]&0x7F)>>4)*100 + ((MdbCurrent[mid].pwrf[1]&0x7F)&0x0F)*10 + (MdbCurrent[mid].pwrf[0]>>4),MdbCurrent[mid].pwrf[0]&0x0f,"%");
	sprintf(scrpbuf_main.buf[18], " S=%d.%dkVA", MdbAnalyze[mid].pwrv/10000,MdbAnalyze[mid].pwrv%10000);
	///sprintf(scrpbuf_main.buf[19], " Pavr=%d.%dkW", MdbAnalyze[mid].pwrav/10000,MdbAnalyze[mid].pwrav%10000);

	if(ParaMeter[mid].proto==METTYPE_ACSAMP)
    {
		sprintf(scrpbuf_main.buf[21]," φua=%d.%X度",((MdbCurrent[mid].phase_arc[1]&0x7F)>>4)*100 + ((MdbCurrent[mid].phase_arc[1]&0x7F)&0x0f)*10 + (MdbCurrent[mid].phase_arc[0]>>4),MdbCurrent[mid].phase_arc[0]&0x0f);
		sprintf(scrpbuf_main.buf[22]," φub=%d.%X度",((MdbCurrent[mid].phase_arc[3]&0x7F)>>4)*100 + ((MdbCurrent[mid].phase_arc[3]&0x7F)&0x0f)*10 + (MdbCurrent[mid].phase_arc[2]>>4),MdbCurrent[mid].phase_arc[2]&0x0f);
		sprintf(scrpbuf_main.buf[23]," φub=%d.%X度",((MdbCurrent[mid].phase_arc[5]&0x7F)>>4)*100 + ((MdbCurrent[mid].phase_arc[5]&0x7F)&0x0f)*10 + (MdbCurrent[mid].phase_arc[4]>>4),MdbCurrent[mid].phase_arc[4]&0x0f);	
		sprintf(scrpbuf_main.buf[24]," φia=%d.%X度",((MdbCurrent[mid].phase_arc[7]&0x7F)>>4)*100 + ((MdbCurrent[mid].phase_arc[7]&0x7F)&0x0f)*10 + (MdbCurrent[mid].phase_arc[6]>>4),MdbCurrent[mid].phase_arc[6]&0x0f);
		sprintf(scrpbuf_main.buf[25]," φib=%d.%X度",((MdbCurrent[mid].phase_arc[9]&0x7F)>>4)*100 + ((MdbCurrent[mid].phase_arc[9]&0x7F)&0x0f)*10 + (MdbCurrent[mid].phase_arc[8]>>4),MdbCurrent[mid].phase_arc[8]&0x0f);
		sprintf(scrpbuf_main.buf[26]," φic=%d.%X度",((MdbCurrent[mid].phase_arc[11]&0x7F)>>4)*100 + ((MdbCurrent[mid].phase_arc[11]&0x7F)&0x0f)*10 + (MdbCurrent[mid].phase_arc[10]>>4),MdbCurrent[mid].phase_arc[10]&0x0f);
		scrpbuf_main.line = 27;
	}
    else
		scrpbuf_main.line = 20;
}

typedef struct {
	unsigned char bak_head;
	unsigned char bak_end;
} bak_time_seg_t;
	
typedef struct {
    bak_time_seg_t  seg[24];
	unsigned char num;
} bak_seg_t;

static bak_seg_t bak_seg1;
/**
* @brief			将相邻的时间段合并显示
* @param time_seg	存储时间段的变量
*/
void GetTimeSeg(unsigned int time_seg)
{
	unsigned char i=0;
	unsigned char j=0;
	unsigned char k=0;
	unsigned char m=0;
	static unsigned char seg[24];

	for(i=0;i<24;i++)
	{
		bak_seg1.seg[i].bak_head=0;
		bak_seg1.seg[i].bak_end=0;
		seg[i]=0;
	}

	for(i=0;i<24;i++)
	{
		if(time_seg&(0x01<<i))
		seg[i]=0x24;
	}

	for(i=0,j=1,k=0;i<24;i++,j++)
	{
		if(seg[i]!=0)
		{
			bak_seg1.seg[k].bak_head=i;
			while(seg[i]==seg[j]&&i<24)
			{
				i++;
				j++;
			}
			bak_seg1.seg[k].bak_end=j;
			m=(bak_seg1.seg[k].bak_end-bak_seg1.seg[k].bak_head);
	
    		if(m>=1)
			k++;
		}
	
	}
	bak_seg1.num=k;
}

typedef struct {
	unsigned int time_seg_head;
	unsigned int time_seg_end;
} time_seg_1_t;

typedef struct {
    time_seg_1_t  seg[96];
	unsigned char num;
} time_seg_2_t;

static time_seg_2_t all_seg;
/**
* @brief			将相邻的时间段合并显示
* @param period		存储时间段的数组
*/
void GetTime2(unsigned char *period)
{
	unsigned char i=0;
	unsigned char j=0;
	unsigned char k=0;
	unsigned int m=0;
	static unsigned int seg[96];
	unsigned char *str=period;
	
	for(i=0;i<96;i++)
		seg[i]=0;

	for(i=0;i<12;i++)
		for(j=0;j<8;j++)
		{
			if(str[i]&(0x01<<j))
				seg[i*8+j]=(i*8+j-1)*0x0f+0x0f;
    		else
				seg[i*8+j]=0x02;
		}

	for(i=0,j=1,k=0;i<96;i++,j++)
	{
		if(seg[i]!=0x02)
		{
			all_seg.seg[k].time_seg_head=seg[i];
			while((seg[j]-seg[i])==0x0f&&i<96)
			{
				i++;
				j++;
			}
			all_seg.seg[k].time_seg_end=seg[i]+0x0f;
			m=(all_seg.seg[k].time_seg_end-all_seg.seg[k].time_seg_head);
			if((m%15)==0)
				k++;
		}
	}
	all_seg.num=k;

	
}


/**
* @brief			终端参数之通信参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpTermParaCom(unsigned short mid, ULONG arg, UCHAR key)
{
	static const char *const_work_mode[3] = {"混合模式", "客户机模式", "服务器模式"};
    static const char *const_client_mode[3] = {"  永久在线模式", "  被动激活模式", "  时段在线模式"};
	unsigned char i=0,j=0;
		unsigned long term_gprs_ipaddr = 0;
	unsigned char term_ipaddr[4];
	
	GetTimeSeg(ParaTerm.uplink.onlineflag);
	term_gprs_ipaddr = ipcp_get_addr();
	
	term_ipaddr[0] = (unsigned char)(term_gprs_ipaddr>>24)&0xff;
	term_ipaddr[1] = (unsigned char)(term_gprs_ipaddr>>16)&0xff;
	term_ipaddr[2] = (unsigned char)(term_gprs_ipaddr>>8)&0xff;
	term_ipaddr[3] = (unsigned char)term_gprs_ipaddr&0xff;
	strcpy(scrpbuf_main.buf[0], "主站IP地址:");
    sprintf(scrpbuf_main.buf[1], "  %d.%d.%d.%d", ParaTerm.svrip.ipmain[0], ParaTerm.svrip.ipmain[1], ParaTerm.svrip.ipmain[2], ParaTerm.svrip.ipmain[3]);
    sprintf(scrpbuf_main.buf[2], "主站端口号:%d", ParaTerm.svrip.portmain);
    strcpy(scrpbuf_main.buf[3], "主站备用IP地址：");
    sprintf(scrpbuf_main.buf[4], "  %d.%d.%d.%d", ParaTerm.svrip.ipbakup[0], ParaTerm.svrip.ipbakup[1], ParaTerm.svrip.ipbakup[2], ParaTerm.svrip.ipbakup[3]);
    sprintf(scrpbuf_main.buf[5], "主站备用端口号:%d", ParaTerm.svrip.portbakup);
	sprintf(scrpbuf_main.buf[6], "APN:%s", ParaTerm.svrip.apn);

    sprintf(scrpbuf_main.buf[7], "终端侦听端口:%d", ParaTerm.termip.portlisten);
    sprintf(scrpbuf_main.buf[8], "通信协议:%s", (ParaTerm.uplink.proto)?"UDP":"TCP");
    sprintf(scrpbuf_main.buf[9], "月通信流量门限:%d", ParaMixSave.mix.upflow_max);
	strcpy(scrpbuf_main.buf[10], "工作模式:");
	strcat(scrpbuf_main.buf[10], (char *)const_work_mode[ParaTerm.uplink.mode]);
    strcpy(scrpbuf_main.buf[11], "客户机在线模式:");
	strcpy(scrpbuf_main.buf[12], (char *)const_client_mode[ParaTerm.uplink.clientmode]);
    sprintf(scrpbuf_main.buf[13], "重拨间隔:%d秒", ParaTerm.uplink.timedail);
    sprintf(scrpbuf_main.buf[14], "重拨次数:%d次", ParaTerm.uplink.countdail);
    sprintf(scrpbuf_main.buf[15], "自动断线时间:%d分", ParaTerm.uplink.timedown);
	if(bak_seg1.num != 0)
	{
		strcpy(scrpbuf_main.buf[16], "允许在线时段:");
		for(i=0;i<bak_seg1.num;i++)
			sprintf(scrpbuf_main.buf[17+i], "  %02d:00-%02d:00",bak_seg1.seg[i].bak_head,bak_seg1.seg[i].bak_end);
	}
	else
	{
		strcpy(scrpbuf_main.buf[16], "允许在线时段:未配置");
	}
	j=17+i;
	sprintf(scrpbuf_main.buf[j++], "数传机延时时间:%dms", ParaTerm.tcom.rts*20);
	sprintf(scrpbuf_main.buf[j++], "传输延时时间:%d分", ParaTerm.tcom.delay);
	sprintf(scrpbuf_main.buf[j++], "超时时间:%d秒", ParaTerm.tcom.rsnd&0xFFF);
	sprintf(scrpbuf_main.buf[j++], "重发次数:%d次", (ParaTerm.tcom.rsnd&(~0xCFFF))>>12);
    sprintf(scrpbuf_main.buf[j++], "是否需要主站确认:");
	sprintf(scrpbuf_main.buf[j++], "  1类数据自动上报:%s", (ParaTerm.tcom.flagcon&0x01)?"是":"否");
	sprintf(scrpbuf_main.buf[j++], "  2类数据自动上报:%s", (ParaTerm.tcom.flagcon&0x02)?"是":"否");
    sprintf(scrpbuf_main.buf[j++], "  3类数据自动上报:%s", (ParaTerm.tcom.flagcon&0x04)?"是":"否");
	sprintf(scrpbuf_main.buf[j++], "心跳周期:%d分", ParaTerm.tcom.cycka);
    strcpy(scrpbuf_main.buf[j++], "主站电话号码:");
    sprintf(scrpbuf_main.buf[j++],"  %d%d%d%d%d%d%d%d%d%d%d", 
		ParaTerm.smsc.phone[0]>>4,ParaTerm.smsc.phone[0]&0x0f,
		ParaTerm.smsc.phone[1]>>4,ParaTerm.smsc.phone[1]&0x0f,
		ParaTerm.smsc.phone[2]>>4,ParaTerm.smsc.phone[2]&0x0f,
		ParaTerm.smsc.phone[3]>>4,ParaTerm.smsc.phone[3]&0x0f,
		ParaTerm.smsc.phone[4]>>4,ParaTerm.smsc.phone[4]&0x0f,
		ParaTerm.smsc.phone[5]>>4
    );

    strcpy(scrpbuf_main.buf[j++], "短信中心号码:");
    sprintf(scrpbuf_main.buf[j++],"  %d%d%d%d%d%d%d%d%d%d%d", 
		ParaTerm.smsc.sms[0]>>4,ParaTerm.smsc.sms[0]&0x0f,
		ParaTerm.smsc.sms[1]>>4,ParaTerm.smsc.sms[1]&0x0f,
		ParaTerm.smsc.sms[2]>>4,ParaTerm.smsc.sms[2]&0x0f,
		ParaTerm.smsc.sms[3]>>4,ParaTerm.smsc.sms[3]&0x0f,
		ParaTerm.smsc.sms[4]>>4,ParaTerm.smsc.sms[4]&0x0f,
		ParaTerm.smsc.sms[5]>>4
    );
	strcpy(scrpbuf_main.buf[j++], "终端IP地址:");
	sprintf(scrpbuf_main.buf[j++], "  %d.%d.%d.%d", ParaTerm.termip.ipterm[0], ParaTerm.termip.ipterm[1], ParaTerm.termip.ipterm[2], ParaTerm.termip.ipterm[3]);
	strcpy(scrpbuf_main.buf[j++], "子网掩码地址:");
	sprintf(scrpbuf_main.buf[j++], "  %d.%d.%d.%d", ParaTerm.termip.maskterm[0], ParaTerm.termip.maskterm[1], ParaTerm.termip.maskterm[2], ParaTerm.termip.maskterm[3]);
	strcpy(scrpbuf_main.buf[j++], "网关地址:");
	sprintf(scrpbuf_main.buf[j++], "  %d.%d.%d.%d", ParaTerm.termip.ipgatew[0], ParaTerm.termip.ipgatew[1], ParaTerm.termip.ipgatew[2], ParaTerm.termip.ipgatew[3]);
	sprintf(scrpbuf_main.buf[j++],"终端地址:%02X%02X-%02X%02X",ParaUni.addr_area[1],ParaUni.addr_area[0],ParaUni.addr_sn[1],ParaUni.addr_sn[0]);
	strcpy(scrpbuf_main.buf[j++], "终端GPRS获取IP地址:");
	sprintf(scrpbuf_main.buf[j++], "  %d.%d.%d.%d", term_ipaddr[0], term_ipaddr[1], term_ipaddr[2], term_ipaddr[3]);
	///sprintf(scrpbuf_main.buf[j++],"终端序号:%02X%02X",ParaUni.addr_sn[1],ParaUni.addr_sn[0]);
	scrpbuf_main.line = j;
}
/**
* @brief			得到抄表日期
* @param date		抄表日期存储变量
* @return           返回抄表日期
*/
unsigned int GetReadMetDate(unsigned int date)
{ 
	unsigned int i=0;
	while(date>>i)
		i++;
	return i;
}

/**
* @brief			终端参数之抄表参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
/*
void ScrpReadMetPara(unsigned short mid, ULONG arg, UCHAR key)
{
    unsigned char i=0,j=0;
	unsigned char *str;
	unsigned int  port_num=GetPortNum();
	const para_commport_t *port_para;
	port_para=GetParaCommPort(port_num);
	unsigned int date =GetReadMetDate(port_para->dateflag);
	
    str = (unsigned char*)port_para->period;
	GetTime2(str);
    sprintf(scrpbuf_main.buf[i++], "重点户个数:%d", ParaMixSave.mix.impuser.num);
    for(i=1;i<=ParaMixSave.mix.impuser.num;i++)
		sprintf(scrpbuf_main.buf[i], "重点户%d表计序号:%d",i,ParaMixSave.mix.impuser.metid[i-1]);
	sprintf(scrpbuf_main.buf[i++], "抄读电表状态字:%s", port_para->flag&RDMETFLAG_RDSTATUS?"是":"否");
	sprintf(scrpbuf_main.buf[i++], "允许自动抄表:%s", port_para->flag&RDMETFLAG_ENABLE?"否":"是");
    sprintf(scrpbuf_main.buf[i++], "只抄重点表:%s", port_para->flag&RDMETFLAG_ALL?"是":"否");
	sprintf(scrpbuf_main.buf[i++], "广播冻结抄表:%s", port_para->flag&RDMETFLAG_FREZ?"是":"否");
	sprintf(scrpbuf_main.buf[i++], "定时广播校时:%s", port_para->flag&RDMETFLAG_CHECKTIME?"是":"否");
	sprintf(scrpbuf_main.buf[i++], "搜寻电表:%s", port_para->flag&RDMETFLAG_FINDMET?"是":"否");
	if(all_seg.num != 0)
	{
		strcpy(scrpbuf_main.buf[i++], "允许抄表时段:"); 
    	for(j=0;j<all_seg.num;j++)
			sprintf(scrpbuf_main.buf[i++], "  %02d:%02d-%02d:%02d",all_seg.seg[j].time_seg_head/60,all_seg.seg[j].time_seg_head%60,all_seg.seg[j].time_seg_end/60,all_seg.seg[j].time_seg_end%60); 
	}
	else
	{
		strcpy(scrpbuf_main.buf[i++], "允许抄表时段:未配置"); 
	}
	sprintf(scrpbuf_main.buf[i++], "抄表日:%02d日%02d:%02d", date,port_para->time_hour,port_para->time_minute);
	sprintf(scrpbuf_main.buf[i++], "抄表时间间隔:%d分", port_para->cycle);
	strcpy(scrpbuf_main.buf[i++], "广播校时定时时间:");
	sprintf(scrpbuf_main.buf[i++], "  %02d日%02d:%02d", port_para->chktime_day,port_para->chktime_hour,port_para->chktime_minute);
	
	scrpbuf_main.line = i;
}
*/
/**
* @brief			1类数据任务参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpClass1DataTask(unsigned short mid, ULONG arg, UCHAR key)
{
    unsigned char i;
	unsigned char task_number;
	task_number=GetTaskNum()-1;
	static const char *dev_snd[4] = {"分","时", "日", "月"};

    strcpy(scrpbuf_main.buf[0], "上报1类任务:");
	if(ParaTaskCls1[task_number].valid==TASK_RUN)
		strcat(scrpbuf_main.buf[0], "启动");
	else if(ParaTaskCls1[task_number].valid==TASK_STOP)
		strcat(scrpbuf_main.buf[0], "停止");
	else 
		strcat(scrpbuf_main.buf[0], "未配置");
	sprintf(scrpbuf_main.buf[1], "定时上报周期:%d%s",ParaTaskCls1[task_number].dev_snd&0x3F,dev_snd[ParaTaskCls1[task_number].dev_snd>>6]);
    strcpy(scrpbuf_main.buf[2], "上报基准时间:");
	sprintf(scrpbuf_main.buf[3], "  %02d年%02d月%02d日",ParaTaskCls1[task_number].base_year,ParaTaskCls1[task_number].base_month,ParaTaskCls1[task_number].base_day);	
	sprintf(scrpbuf_main.buf[4], "  %02d:%02d:%02d",ParaTaskCls1[task_number].base_hour,ParaTaskCls1[task_number].base_minute,ParaTaskCls1[task_number].base_second);
	sprintf(scrpbuf_main.buf[5], "曲线数据抽取倍率:%d",ParaTaskCls1[task_number].freq);
	sprintf(scrpbuf_main.buf[6], "数据单元标识个数:%d",ParaTaskCls1[task_number].num);
	
	for(i=0;i<ParaTaskCls1[task_number].num;i++)
    {
		sprintf(scrpbuf_main.buf[7+i], "数据标识%d:%02X%02X %02X%02X",i+1,ParaTaskCls1[task_number].duid[i].da[0],ParaTaskCls1[task_number].duid[i].da[1],
																		ParaTaskCls1[task_number].duid[i].dt[0],ParaTaskCls1[task_number].duid[i].dt[1]);
	}

	scrpbuf_main.line = 7 + i;
}
/**
* @brief			终端参数之2类数据任务参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpClass2DataTask(unsigned short mid, ULONG arg, UCHAR key)
{
    unsigned char i;
	unsigned char task_number;
	task_number=GetTaskNum()-1;
	static const char *dev_snd[4] = {"分","时", "日", "月"};
   
	strcpy(scrpbuf_main.buf[0], "上报2类任务:");
	if(ParaTaskCls2[task_number].valid==TASK_RUN) 
		strcat(scrpbuf_main.buf[0], "启动");
    else if(ParaTaskCls2[task_number].valid==TASK_STOP)
		strcat(scrpbuf_main.buf[0], "停止");
	else 
		strcat(scrpbuf_main.buf[0], "未配置");
	sprintf(scrpbuf_main.buf[1], "定时上报周期:%d%s",ParaTaskCls2[task_number].dev_snd&0x3F,dev_snd[ParaTaskCls2[task_number].dev_snd>>6]);
    strcpy(scrpbuf_main.buf[2], "上报基准时间:");
	sprintf(scrpbuf_main.buf[3], "  %02d年%02d月%02d日",ParaTaskCls2[task_number].base_year,ParaTaskCls2[task_number].base_month,ParaTaskCls2[task_number].base_day);	
	sprintf(scrpbuf_main.buf[4], "  %02d:%02d:%02d",ParaTaskCls2[task_number].base_hour,ParaTaskCls2[task_number].base_minute,ParaTaskCls2[task_number].base_second);
	sprintf(scrpbuf_main.buf[5], "曲线数据抽取倍率:%d",ParaTaskCls2[task_number].freq);
	sprintf(scrpbuf_main.buf[6], "数据单元标识个数:%d",ParaTaskCls2[task_number].num);
	for(i=0;i<ParaTaskCls2[task_number].num;i++)
    {
		sprintf(scrpbuf_main.buf[7+i], "数据标识%d:%02X%02X %02X%02X",i+1,ParaTaskCls2[task_number].duid[i].da[0],ParaTaskCls2[task_number].duid[i].da[1],
																		ParaTaskCls2[task_number].duid[i].dt[0],ParaTaskCls2[task_number].duid[i].dt[1]);
	}
	scrpbuf_main.line = 7+i;
}

/**
* @brief			终端参数之事件记录配置
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpEventReadCfg(unsigned short mid, ULONG arg, UCHAR key)
{
    scrpbuf_main.line = 35;
    sprintf(scrpbuf_main.buf[0], "1.版本变更%s %s", ((ParaTerm.almflag.valid[0]&0x01)?"Y":"N"),(ParaTerm.almflag.rank[0]&0x01)?"I":"");
    sprintf(scrpbuf_main.buf[1], "2.参数丢失%s %s", ((ParaTerm.almflag.valid[0]&(0x01<<1))?"Y":"N"),(ParaTerm.almflag.rank[0]&(0x01<<1))?"I":"");
    sprintf(scrpbuf_main.buf[2], "3.参数变更%s %s", ((ParaTerm.almflag.valid[0]&(0x01<<2))?"Y":"N"),(ParaTerm.almflag.rank[0]&(0x01<<2))?"I":"");
    sprintf(scrpbuf_main.buf[3], "4.状态量变位%s %s", ((ParaTerm.almflag.valid[0]&(0x01<<3))?"Y":"N"),(ParaTerm.almflag.rank[0]&(0x01<<3))?"I":"");
    sprintf(scrpbuf_main.buf[4], "5.遥控跳闸%s %s", ((ParaTerm.almflag.valid[0]&(0x01<<4))?"Y":"N"),(ParaTerm.almflag.rank[0]&(0x01<<4))?"I":"");
    sprintf(scrpbuf_main.buf[5], "6.功控跳闸%s %s", ((ParaTerm.almflag.valid[0]&(0x01<<5))?"Y":"N"),(ParaTerm.almflag.rank[0]&(0x01<<5))?"I":"");
    sprintf(scrpbuf_main.buf[6], "7.电控跳闸%s %s", ((ParaTerm.almflag.valid[0]&(0x01<<6))?"Y":"N"),(ParaTerm.almflag.rank[0]&(0x01<<6))?"I":"");
    sprintf(scrpbuf_main.buf[7], "8.电表参数变更%s %s", ((ParaTerm.almflag.valid[0]&(0x01<<7))?"Y":"N"),(ParaTerm.almflag.rank[0]&(0x01<<7))?"I":"");
    sprintf(scrpbuf_main.buf[8], "9.电流回路异常%s %s", ((ParaTerm.almflag.valid[1]&0x01)?"Y":"N"),(ParaTerm.almflag.rank[1]&0x01)?"I":"");
    sprintf(scrpbuf_main.buf[9], "10.电压回路异常%s %s", ((ParaTerm.almflag.valid[1]&(0x01<<1))?"Y":"N"),(ParaTerm.almflag.rank[1]&(0x01<<1))?"I":"");
    sprintf(scrpbuf_main.buf[10], "11.相序异常%s %s", ((ParaTerm.almflag.valid[1]&(0x01<<2))?"Y":"N"),(ParaTerm.almflag.rank[1]&(0x01<<2))?"I":"");
    sprintf(scrpbuf_main.buf[11], "12.电表时间超差%s %s", ((ParaTerm.almflag.valid[1]&(0x01<<3))?"Y":"N"),(ParaTerm.almflag.rank[1]&(0x01<<3))?"I":"");
    sprintf(scrpbuf_main.buf[12], "13.电表故障信息%s %s", ((ParaTerm.almflag.valid[1]&(0x01<<4))?"Y":"N"),(ParaTerm.almflag.rank[1]&(0x01<<4))?"I":"");
    sprintf(scrpbuf_main.buf[13], "14.终端停/上电%s %s", ((ParaTerm.almflag.valid[1]&(0x01<<5))?"Y":"N"),(ParaTerm.almflag.rank[1]&(0x01<<5))?"I":"");
    sprintf(scrpbuf_main.buf[14], "15.谐波越限告警%s %s", ((ParaTerm.almflag.valid[1]&(0x01<<6))?"Y":"N"),(ParaTerm.almflag.rank[1]&(0x01<<6))?"I":"");
    sprintf(scrpbuf_main.buf[15], "16.直流模拟量越限%s %s", ((ParaTerm.almflag.valid[1]&(0x01<<7))?"Y":"N"),(ParaTerm.almflag.rank[1]&(0x01<<7))?"I":"");
    sprintf(scrpbuf_main.buf[16], "17.压流不平衡越限%s %s", ((ParaTerm.almflag.valid[2]&0x01)?"Y":"N"),(ParaTerm.almflag.rank[2]&0x01)?"I":"");
    sprintf(scrpbuf_main.buf[17], "18.电容器投切自锁%s %s", ((ParaTerm.almflag.valid[2]&(0x01<<1))?"Y":"N"),(ParaTerm.almflag.rank[2]&(0x01<<1))?"I":"");
    sprintf(scrpbuf_main.buf[18], "19.购电参数设置%s %s", ((ParaTerm.almflag.valid[2]&(0x01<<2))?"Y":"N"),(ParaTerm.almflag.rank[2]&(0x01<<2))?"I":"");
    sprintf(scrpbuf_main.buf[19], "20.消息认证错误%s %s", ((ParaTerm.almflag.valid[2]&(0x01<<3))?"Y":"N"),(ParaTerm.almflag.rank[2]&(0x01<<3))?"I":"");
    sprintf(scrpbuf_main.buf[20], "21.终端故障%s %s", ((ParaTerm.almflag.valid[2]&(0x01<<4))?"Y":"N"),(ParaTerm.almflag.rank[2]&(0x01<<4))?"I":"");
    sprintf(scrpbuf_main.buf[21], "22.有功总差动越限%s %s", ((ParaTerm.almflag.valid[2]&(0x01<<5))?"Y":"N"),(ParaTerm.almflag.rank[2]&(0x01<<5))?"I":"");
    sprintf(scrpbuf_main.buf[22], "23.电控告警事件%s %s", ((ParaTerm.almflag.valid[2]&(0x01<<6))?"Y":"N"),(ParaTerm.almflag.rank[2]&(0x01<<6))?"I":"");
    sprintf(scrpbuf_main.buf[23], "24.电压越限%s %s", ((ParaTerm.almflag.valid[2]&(0x01<<7))?"Y":"N"),(ParaTerm.almflag.rank[2]&(0x01<<7))?"I":"");
    sprintf(scrpbuf_main.buf[24], "25.电流越限%s %s", ((ParaTerm.almflag.valid[3]&0x01)?"Y":"N"),(ParaTerm.almflag.rank[3]&0x01)?"I":"");
    sprintf(scrpbuf_main.buf[25], "26.视在功率越限%s %s", ((ParaTerm.almflag.valid[3]&(0x01<<1))?"Y":"N"),(ParaTerm.almflag.rank[3]&(0x01<<1))?"I":"");
    sprintf(scrpbuf_main.buf[26], "27.电能表示度下降%s %s", ((ParaTerm.almflag.valid[3]&(0x01<<2))?"Y":"N"),(ParaTerm.almflag.rank[3]&(0x01<<2))?"I":"");
    sprintf(scrpbuf_main.buf[27], "28.电能量超差%s %s", ((ParaTerm.almflag.valid[3]&(0x01<<3))?"Y":"N"),(ParaTerm.almflag.rank[3]&(0x01<<3))?"I":"");
    sprintf(scrpbuf_main.buf[28], "29.电能表飞走%s %s", ((ParaTerm.almflag.valid[3]&(0x01<<4))?"Y":"N"),(ParaTerm.almflag.rank[3]&(0x01<<4))?"I":"");
    sprintf(scrpbuf_main.buf[29], "30.电能表停走%s %s", ((ParaTerm.almflag.valid[3]&(0x01<<5))?"Y":"N"),(ParaTerm.almflag.rank[3]&(0x01<<5))?"I":"");
    sprintf(scrpbuf_main.buf[30], "31.485抄表失败%s %s", ((ParaTerm.almflag.valid[3]&(0x01<<6))?"Y":"N"),(ParaTerm.almflag.rank[3]&(0x01<<6))?"I":"");
    sprintf(scrpbuf_main.buf[31], "32.通信流量超门限%s %s", ((ParaTerm.almflag.valid[3]&(0x01<<7))?"Y":"N"),(ParaTerm.almflag.rank[3]&(0x01<<7))?"I":"");
    sprintf(scrpbuf_main.buf[32], "33.运行状态字变位%s %s", ((ParaTerm.almflag.valid[4]&0x01)?"Y":"N"),(ParaTerm.almflag.rank[4]&0x01)?"I":"");
    sprintf(scrpbuf_main.buf[33], "34.CT异常%s %s", ((ParaTerm.almflag.valid[4]&(0x01<<1))?"Y":"N"),(ParaTerm.almflag.rank[4]&(0x01<<1))?"I":"");
    sprintf(scrpbuf_main.buf[34], "35.发现未知电表%s %s", ((ParaTerm.almflag.valid[4]&(0x01<<2))?"Y":"N"),(ParaTerm.almflag.rank[4]&(0x01<<2))?"I":"");
}


/**
* @brief			终端参数之端口参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpReadMetPara(unsigned short mid, ULONG arg, UCHAR key)
{
    unsigned char i=0,j=0;
	unsigned char *str;
	const para_commport_t *port_para;
	port_para=GetParaCommPort(arg);
	unsigned int date =GetReadMetDate(port_para->dateflag);
	
    str = (unsigned char*)port_para->period;
	GetTime2(str);

	PrintLog(0, "port_para->frame= %d.\r\n", port_para->frame);
    sprintf(scrpbuf_main.buf[i++], "波特率:%d",port_para->baudrate);
	sprintf(scrpbuf_main.buf[i++], "停止位:%s",port_para->frame&COMMFRAME_STOPBIT_2?"2":"1");
	sprintf(scrpbuf_main.buf[i++], "有无校验:%s",port_para->frame&COMMFRAME_HAVECHECK?"有":"无");
	sprintf(scrpbuf_main.buf[i++], "奇偶校验:%s",port_para->frame&COMMFRAME_EVENCHECK?"奇":"偶");
	sprintf(scrpbuf_main.buf[i++], "数据位:%d",(port_para->frame&COMMFRAME_DATA)+5);
	sprintf(scrpbuf_main.buf[i++], "抄读电表状态字:%s", port_para->flag&RDMETFLAG_RDSTATUS?"是":"否");
	sprintf(scrpbuf_main.buf[i++], "允许自动抄表:%s", port_para->flag&RDMETFLAG_ENABLE?"否":"是");
    sprintf(scrpbuf_main.buf[i++], "只抄重点表:%s", port_para->flag&RDMETFLAG_ALL?"是":"否");
	sprintf(scrpbuf_main.buf[i++], "广播冻结抄表:%s", port_para->flag&RDMETFLAG_FREZ?"是":"否");
	sprintf(scrpbuf_main.buf[i++], "定时广播校时:%s", port_para->flag&RDMETFLAG_CHECKTIME?"是":"否");
	sprintf(scrpbuf_main.buf[i++], "搜寻电表:%s", port_para->flag&RDMETFLAG_FINDMET?"是":"否");
	if(all_seg.num != 0)
	{
		strcpy(scrpbuf_main.buf[i++], "允许抄表时段:"); 
    	for(j=0;j<all_seg.num;j++)
			sprintf(scrpbuf_main.buf[i++], "  %02d:%02d-%02d:%02d",all_seg.seg[j].time_seg_head/60,all_seg.seg[j].time_seg_head%60,all_seg.seg[j].time_seg_end/60,all_seg.seg[j].time_seg_end%60); 
	}
	else
	{
		strcpy(scrpbuf_main.buf[i++], "允许抄表时段:未配置"); 
	}
	sprintf(scrpbuf_main.buf[i++], "抄表日:%02d日%02d:%02d", date,port_para->time_hour,port_para->time_minute);
	sprintf(scrpbuf_main.buf[i++], "抄表时间间隔:%d分", port_para->cycle);
	strcpy(scrpbuf_main.buf[i++], "广播校时定时时间:");
	sprintf(scrpbuf_main.buf[i++], "  %02d日%02d:%02d", port_para->chktime_day,port_para->chktime_hour,port_para->chktime_minute);
    sprintf(scrpbuf_main.buf[i++], "重点户个数:%d", ParaMixSave.mix.impuser.num);
	for(j=1;j<=ParaMixSave.mix.impuser.num;j++)
		sprintf(scrpbuf_main.buf[i++], "重点户%d表计序号:%d",j,ParaMixSave.mix.impuser.metid[j-1]);

	scrpbuf_main.line = i;
}

/**
* @brief			终端参数之级联参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpCascadePara(unsigned short mid, ULONG arg, UCHAR key)
{
	static const char *const_baud[8] = {"300","600", "1200", "2400","4800", "7200", "9600","19200"};
    unsigned char i = 0,j = 0,k = 0;

	PrintLog(0, "frame= %d.\r\n", ParaMixSave.mix.cascade.frame);
	sprintf(scrpbuf_main.buf[0], "端口号:%d",ParaMixSave.mix.cascade.port);
    sprintf(scrpbuf_main.buf[1], "波特率:%s",const_baud[ParaMixSave.mix.cascade.frame>>5]);
	sprintf(scrpbuf_main.buf[2], "停止位:%s",ParaMixSave.mix.cascade.frame&COMMFRAME_STOPBIT_2?"2":"1");
	sprintf(scrpbuf_main.buf[3], "有无校验:%s",ParaMixSave.mix.cascade.frame&COMMFRAME_HAVECHECK?"有":"无");
	sprintf(scrpbuf_main.buf[4], "奇偶校验:%s",ParaMixSave.mix.cascade.frame&COMMFRAME_EVENCHECK?"奇":"偶");
	sprintf(scrpbuf_main.buf[5], "数据位:%d",(ParaMixSave.mix.cascade.frame&COMMFRAME_DATA)+5);
	sprintf(scrpbuf_main.buf[6], "报文超时时间:%dms",(ParaMixSave.mix.cascade.timeout*100));
	sprintf(scrpbuf_main.buf[7], "字节超时时间:%dms",(ParaMixSave.mix.cascade.timeout_byte*10));
	sprintf(scrpbuf_main.buf[8], "重发次数:%d次",ParaMixSave.mix.cascade.retry);
	sprintf(scrpbuf_main.buf[9], "级联巡测周期:%d分",ParaMixSave.mix.cascade.cycle);
	sprintf(scrpbuf_main.buf[10], "级联标志:%s",ParaMixSave.mix.cascade.flag?"被级联方":"主动站");
	sprintf(scrpbuf_main.buf[11], "级联终端个数:%d个",ParaMixSave.mix.cascade.num);
	for(i = 0,j = 0,k = 0;j < ParaMixSave.mix.cascade.num;j++,i++)
	{
		sprintf(scrpbuf_main.buf[11 + (++k)], "级联终端%d地址:",i + 1);
		sprintf(scrpbuf_main.buf[11 + (++k)], "  %02X%02X%02X%02X",ParaMixSave.mix.cascade.addr[j + 1],ParaMixSave.mix.cascade.addr[j],ParaMixSave.mix.cascade.addr[j + 3],ParaMixSave.mix.cascade.addr[j + 2]);
	}
	scrpbuf_main.line = 13 + j * 2;
}

/**
* @brief			总表参数之基本参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpMajorMetBasePara(unsigned short mid, ULONG arg, UCHAR key)
{
#define PWRTYPE  0x03

    //unsigned int  major_met_number=GetMajorMetNum()-1;
	static const char *const_baud[8] = {"无需设置","600", "1200", "2400","4800", "7200", "9600","19200"};
	static const char *user_class[6] = {"  大型专变用户","  中小型专变用户", "  低压三相工商业用户", "  低压单相工商业用户","  居民用户", "  公变考核计量点"};
    unsigned char i,j;
	unsigned char displayno[14];
	int scr_cen_meter_cnt;


	scr_cen_meter_cnt = get_cen_meter_total_cnt();
	if(!scr_cen_meter_cnt)
	{
		scrpbuf_main.line = 2;
		sprintf(scrpbuf_main.buf[1],"    未配置总表!");	
		sprintf(scrpbuf_main.buf[2],"    按ESC键返回!");	
		return;
	}

    //if(major_met_number!=0)
    //{
	//	if(ParaMeter[major_met_number].metp_id == 0)
	//	{
	//		scrpbuf_main.line = 2;
	//		sprintf(scrpbuf_main.buf[1],"未配置该序号的表计!");	
	//		sprintf(scrpbuf_main.buf[2],"    按ESC键返回!");	
	//		return;
	//	}
   // }



	scrpbuf_main.line = 26;
    ///sprintf(scrpbuf_main.buf[0], "装置序号:%d", CEN_METER_NUMBER + 1);
	sprintf(scrpbuf_main.buf[0], "所属测量点号:%d",ParaMeter[CEN_METER_NUMBER].metp_id);
    sprintf(scrpbuf_main.buf[1], "通信速率:%s", const_baud[ParaMeter[CEN_METER_NUMBER].portcfg>>5]);
	sprintf(scrpbuf_main.buf[2], "通信端口号:%d", ParaMeter[CEN_METER_NUMBER].portcfg&METPORT_MASK);
    strcpy(scrpbuf_main.buf[3], "通信规约类型:");
	switch(ParaMeter[CEN_METER_NUMBER].proto)
    {
	case 0:
		strcpy(scrpbuf_main.buf[4], "  无需进行抄表");
		break;
	case METTYPE_DL645:
		strcpy(scrpbuf_main.buf[4], "  DL/T645-1997规约");
		break;
	case METTYPE_ACSAMP:
		strcpy(scrpbuf_main.buf[4], "  交流采样通信协议");
		break;
	case METTYPE_DL645_2007:
		strcpy(scrpbuf_main.buf[4], "  DL/T645-2007规约");
		break;
	case METTYPE_PLC:
		strcpy(scrpbuf_main.buf[4], "  串行接口窄带协议");
		break;
	default: break;
    }
	strcpy(scrpbuf_main.buf[5], "通信地址:");
    sprintf(scrpbuf_main.buf[6], "  %02X%02X%02X%02X%02X%02X",
    ParaMeter[CEN_METER_NUMBER].addr[5],
    ParaMeter[CEN_METER_NUMBER].addr[4],
    ParaMeter[CEN_METER_NUMBER].addr[3],
    ParaMeter[CEN_METER_NUMBER].addr[2],
    ParaMeter[CEN_METER_NUMBER].addr[1],
    ParaMeter[CEN_METER_NUMBER].addr[0]
	);
	sprintf(scrpbuf_main.buf[7], "通信密码:%d%d%d%d%d%d", ParaMeter[CEN_METER_NUMBER].pwd[0],ParaMeter[CEN_METER_NUMBER].pwd[1],
		ParaMeter[CEN_METER_NUMBER].pwd[2],ParaMeter[CEN_METER_NUMBER].pwd[3],ParaMeter[CEN_METER_NUMBER].pwd[4],ParaMeter[CEN_METER_NUMBER].pwd[5]);
	sprintf(scrpbuf_main.buf[8], "电能费率个数:%d", ParaMeter[CEN_METER_NUMBER].prdnum&METPRDNUM_MASK);

	sprintf(scrpbuf_main.buf[9], "有功电能整数位个数:%d", (ParaMeter[CEN_METER_NUMBER].intdotnum&INTNUM_MASK)+4);
	sprintf(scrpbuf_main.buf[10], "有功电能小数位个数:%d", (ParaMeter[CEN_METER_NUMBER].intdotnum&DOTNUM_MASK)+1);
	strcpy(scrpbuf_main.buf[11], "用户分类号:");
	sprintf(scrpbuf_main.buf[12], "%s",user_class[(ParaMeter[CEN_METER_NUMBER].userclass>>4)]);
    sprintf(scrpbuf_main.buf[13], "电表分类号:%d",ParaMeter[CEN_METER_NUMBER].userclass&0x0f);
    sprintf(scrpbuf_main.buf[14], "电压互感器倍率:%d",ParaCenMetp[CEN_METER_NUMBER].base.pt);
    sprintf(scrpbuf_main.buf[15], "电流互感器倍率:%d",ParaCenMetp[CEN_METER_NUMBER].base.ct);
	sprintf(scrpbuf_main.buf[16], "额定电压:%d.%dV",ParaCenMetp[CEN_METER_NUMBER].base.vol_rating/10,(ParaCenMetp[CEN_METER_NUMBER].base.vol_rating%100)%10);
	sprintf(scrpbuf_main.buf[17], "额定电流:%d.%dA",ParaCenMetp[CEN_METER_NUMBER].base.amp_rating/100,ParaCenMetp[CEN_METER_NUMBER].base.amp_rating%100);
	sprintf(scrpbuf_main.buf[18], "额定负荷:%d.%dkVA",ParaCenMetp[CEN_METER_NUMBER].base.pwr_rating/10000,ParaCenMetp[CEN_METER_NUMBER].base.pwr_rating%10000);
    strcpy(scrpbuf_main.buf[19], "电源接线方式:");
	switch(ParaCenMetp[CEN_METER_NUMBER].base.pwrtype&PWRTYPE)
	{
	case 1:
		strcpy(scrpbuf_main.buf[20], "  三相三线");
		break;
	case 2:
		strcpy(scrpbuf_main.buf[20], "  三相四线");
		break;
	case 3:
    	if(((ParaCenMetp[CEN_METER_NUMBER].base.pwrtype&PWRPHASE)>>2)==0)
			strcpy(scrpbuf_main.buf[20], "  单相不确定");
		else if(((ParaCenMetp[CEN_METER_NUMBER].base.pwrtype&PWRPHASE)>>2)==1)
			strcpy(scrpbuf_main.buf[20], "  单相A");
		else if(((ParaCenMetp[CEN_METER_NUMBER].base.pwrtype&PWRPHASE)>>2)==2)
			strcpy(scrpbuf_main.buf[20], "  单相B");
		else if(((ParaCenMetp[CEN_METER_NUMBER].base.pwrtype&PWRPHASE)>>2)==3)
			strcpy(scrpbuf_main.buf[20], "  单相C");
		break;
	default: break;
   	}
	
	sprintf(scrpbuf_main.buf[21], "抄表停投设置:%s",ParaCenMetp[CEN_METER_NUMBER].stopped&0x01?"停抄":"投抄");
	sprintf(scrpbuf_main.buf[22], "当地电能表显示号:");
	for(i = 0,j = 2;i < 12;i++,j++)
		displayno[j] = ParaCenMetp[CEN_METER_NUMBER].displayno[i];
	displayno[0] = displayno[1] = 32;         ///形成空格
	memcpy(scrpbuf_main.buf[23],displayno,14);
	sprintf(scrpbuf_main.buf[24], "功率因数限值1:%d.%d",(ParaCenMetp[CEN_METER_NUMBER].pwrf.limit1)/10,((ParaCenMetp[CEN_METER_NUMBER].pwrf.limit1)%100)%10);
	strcat(scrpbuf_main.buf[24], "%");
	sprintf(scrpbuf_main.buf[25], "功率因数限值2:%d.%d",(ParaCenMetp[CEN_METER_NUMBER].pwrf.limit2)/10,((ParaCenMetp[CEN_METER_NUMBER].pwrf.limit2)%100)%10);
	strcat(scrpbuf_main.buf[25], "%");
}

/**
* @brief			总表参数之限值参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpMajorMetLmtPara(unsigned short mid, ULONG arg, UCHAR key)
{
	scrpbuf_main.line = 36;
    //unsigned int  major_met_number=GetMajorMetNum()-1;

	int scr_cen_meter_cnt;


	scr_cen_meter_cnt = get_cen_meter_total_cnt();
	if(!scr_cen_meter_cnt)
	{
		scrpbuf_main.line = 2;
		sprintf(scrpbuf_main.buf[1],"    未配置总表!");	
		sprintf(scrpbuf_main.buf[2],"    按ESC键返回!");	
		return;
	}

	

	//if(ParaMeter[major_met_number].metp_id == 0)
	//{
	//	scrpbuf_main.line = 2;
	//	sprintf(scrpbuf_main.buf[1],"未配置该序号的表计!");	
	//	sprintf(scrpbuf_main.buf[2],"    按ESC键返回!");	
	//	return;
	//}
		
	sprintf(scrpbuf_main.buf[0], "电压合格上限:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.volok_up/10,(ParaCenMetp[CEN_METER_NUMBER].limit.volok_up%100)%10);
	sprintf(scrpbuf_main.buf[1], "电压合格下限:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.volok_low/10,(ParaCenMetp[CEN_METER_NUMBER].limit.volok_low%100)%10);
	sprintf(scrpbuf_main.buf[2], "电压断相门限:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.vol_lack/10,(ParaCenMetp[CEN_METER_NUMBER].limit.vol_lack%100)%10);

	sprintf(scrpbuf_main.buf[3], "电压上上限:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.vol_over/10,(ParaCenMetp[CEN_METER_NUMBER].limit.vol_over%100)%10);
	sprintf(scrpbuf_main.buf[4], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_volover);
	sprintf(scrpbuf_main.buf[5], "越限恢复电压:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.restore_volover/10,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_volover%100)%10);

	sprintf(scrpbuf_main.buf[6], "电压下下限:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.vol_less/10,(ParaCenMetp[CEN_METER_NUMBER].limit.vol_less%100)%10);
	sprintf(scrpbuf_main.buf[7], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_volless);
	sprintf(scrpbuf_main.buf[8], "越限恢复电压:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.restore_volless/10,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_volless%100)%10);

	sprintf(scrpbuf_main.buf[9], "过流门限:%d.%dA", ParaCenMetp[CEN_METER_NUMBER].limit.amp_over/100,(ParaCenMetp[CEN_METER_NUMBER].limit.amp_over%100));
	sprintf(scrpbuf_main.buf[10], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_ampover);
	sprintf(scrpbuf_main.buf[11], "越限恢复电流:%d.%dA", ParaCenMetp[CEN_METER_NUMBER].limit.restore_ampover/100,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_ampover%100));

	sprintf(scrpbuf_main.buf[12], "额定电流门限:%d.%dA", ParaCenMetp[CEN_METER_NUMBER].limit.amp_limit/100,(ParaCenMetp[CEN_METER_NUMBER].limit.amp_limit%100));
	sprintf(scrpbuf_main.buf[13], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_amplimit);
	sprintf(scrpbuf_main.buf[14], "越限恢复电流:%d.%dA", ParaCenMetp[CEN_METER_NUMBER].limit.restore_amplimit/100,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_amplimit%100));

	sprintf(scrpbuf_main.buf[15], "零序电流上限:%d.%dA", ParaCenMetp[CEN_METER_NUMBER].limit.zamp_limit/100,(ParaCenMetp[CEN_METER_NUMBER].limit.zamp_limit%100));
	sprintf(scrpbuf_main.buf[16], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_zamp);
	sprintf(scrpbuf_main.buf[17], "越限恢复电流:%d.%dA", ParaCenMetp[CEN_METER_NUMBER].limit.restore_zamp/100,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_zamp%100));

	sprintf(scrpbuf_main.buf[18], "视在功率门限:");
	sprintf(scrpbuf_main.buf[19], "  %d.%dkVA", ParaCenMetp[CEN_METER_NUMBER].limit.pwr_over/10000,(ParaCenMetp[CEN_METER_NUMBER].limit.pwr_over%10000));
	sprintf(scrpbuf_main.buf[20], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_pwrover);
	sprintf(scrpbuf_main.buf[21], "越限恢复功率:");
	sprintf(scrpbuf_main.buf[22], "  %d.%dkVA", ParaCenMetp[CEN_METER_NUMBER].limit.restore_pwrover/10000,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_pwrover%10000));

	sprintf(scrpbuf_main.buf[23], "视在功率上限:");
	sprintf(scrpbuf_main.buf[24], "  %d.%dkVA", ParaCenMetp[CEN_METER_NUMBER].limit.pwr_limit/10000,(ParaCenMetp[CEN_METER_NUMBER].limit.pwr_limit%10000));
	sprintf(scrpbuf_main.buf[25], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_pwrlimit);
	sprintf(scrpbuf_main.buf[26], "越限恢复功率:");
	sprintf(scrpbuf_main.buf[27], "  %d.%dkVA", ParaCenMetp[CEN_METER_NUMBER].limit.restore_pwrlimit/10000,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_pwrlimit%10000));

	strcpy(scrpbuf_main.buf[28], "三相电压不平衡限值:");
	sprintf(scrpbuf_main.buf[29], "  %d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.vol_unb/10,(ParaCenMetp[CEN_METER_NUMBER].limit.vol_unb%100)%10);
	sprintf(scrpbuf_main.buf[30], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_volunb);
	sprintf(scrpbuf_main.buf[31], "越限恢复电压:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.restore_volunb/10,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_volunb%100)%10);

	strcpy(scrpbuf_main.buf[32], "三相电流不平衡限值:");
	sprintf(scrpbuf_main.buf[33], "  %d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.amp_unb/10,(ParaCenMetp[CEN_METER_NUMBER].limit.amp_unb%100)%10);
	sprintf(scrpbuf_main.buf[34], "越限持续时间:%d分", ParaCenMetp[CEN_METER_NUMBER].limit.time_ampunb);
	sprintf(scrpbuf_main.buf[35], "越限恢复电流:%d.%dV", ParaCenMetp[CEN_METER_NUMBER].limit.restore_ampunb/10,(ParaCenMetp[CEN_METER_NUMBER].limit.restore_ampunb%100)%10);
}

/**
* @brief			载波表参数
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpUserMetPara (unsigned short mid, ULONG arg, UCHAR key)
{
	static const char *const_baud[8] = {"无需设置","600", "1200", "2400","4800", "7200", "9600","19200"};
	static const char *user_class[6] = {"大型专变用户","中小型专变用户", "低压三相工商业用户", "低压单相工商业用户","居民用户", "公变考核计量点"};
    	unsigned int  sn_number = GetSnNum() - 1;

	if(sn_number<2||sn_number>2039)
	{
		scrpbuf_main.line = 1;
		sprintf(scrpbuf_main.buf[0],"输入电表号超出范围!");	
		return;
	}
	if(ParaMeter[sn_number].metp_id == 0)
	{
		scrpbuf_main.line = 2;
		sprintf(scrpbuf_main.buf[1],"未配置该序号的表计!");	
		sprintf(scrpbuf_main.buf[2],"    按ESC键返回!");	
		return;
	}

    sprintf(scrpbuf_main.buf[0], "所属测量点号:%d", ParaMeter[sn_number].metp_id);
    sprintf(scrpbuf_main.buf[1], "通信速率:%s", const_baud[ParaMeter[sn_number].portcfg>>5]);
	sprintf(scrpbuf_main.buf[2], "通信端口号:%d", ParaMeter[sn_number].portcfg&METPORT_MASK);

	strcpy(scrpbuf_main.buf[3], "通信规约类型:");
	switch(ParaMeter[sn_number].proto)
    {
	case 0:
		strcpy(scrpbuf_main.buf[4], "  无需进行抄表");
		break;
	case METTYPE_DL645:
		strcpy(scrpbuf_main.buf[4], "  DL/T645-1997规约");
		break;
	case METTYPE_ACSAMP:
		strcpy(scrpbuf_main.buf[4], "  交流采样装置通信协议");
		break;
	case METTYPE_DL645_2007:
		strcpy(scrpbuf_main.buf[4], "  DL/T645-2007规约");
		break;
	case METTYPE_PLC:
		strcpy(scrpbuf_main.buf[4], "  串行载波通信协议");
		break;
	default: break;
    }
	strcpy(scrpbuf_main.buf[5], "通信地址:");
	sprintf(scrpbuf_main.buf[6], "  %02X%02X%02X%02X%02X%02X", 
		ParaMeter[sn_number].addr[5],
		ParaMeter[sn_number].addr[4],
		ParaMeter[sn_number].addr[3],
		ParaMeter[sn_number].addr[2],
		ParaMeter[sn_number].addr[1],
		ParaMeter[sn_number].addr[0]
	);

	strcpy(scrpbuf_main.buf[7], "所属采集器地址:");
	sprintf(scrpbuf_main.buf[8], "  %02X%02X%02X%02X%02X%02X", 
		ParaMeter[sn_number].owneraddr[5],
		ParaMeter[sn_number].owneraddr[4],
		ParaMeter[sn_number].owneraddr[3],
		ParaMeter[sn_number].owneraddr[2],
		ParaMeter[sn_number].owneraddr[1],
		ParaMeter[sn_number].owneraddr[0]
	);
	sprintf(scrpbuf_main.buf[9], "通信密码:%d%d%d%d%d%d", ParaMeter[sn_number].pwd[0],ParaMeter[sn_number].pwd[1],ParaMeter[sn_number].pwd[2],ParaMeter[sn_number].pwd[3],ParaMeter[sn_number].pwd[4],ParaMeter[sn_number].pwd[5]);
    sprintf(scrpbuf_main.buf[10], "电能费率个数:%d", ParaMeter[sn_number].prdnum&METPRDNUM_MASK);
	sprintf(scrpbuf_main.buf[11], "有功电能整数位个数:%d", ((ParaMeter[sn_number].intdotnum&INTNUM_MASK)>>2)+4);
	sprintf(scrpbuf_main.buf[12], "有功电能小数位个数:%d", (ParaMeter[sn_number].intdotnum&DOTNUM_MASK)+1);
	strcpy(scrpbuf_main.buf[14], "用户分类号:");
	if(ParaMeter[sn_number].userclass >= 1&&ParaMeter[sn_number].userclass <= 8)
	sprintf(scrpbuf_main.buf[15], "  %s",user_class[(ParaMeter[sn_number].userclass)-1]);
	else 
	strcpy(scrpbuf_main.buf[15], "  未配置");
	sprintf(scrpbuf_main.buf[16], "电表分类号:%d",(ParaMeter[sn_number].metclass)&0x0f);
    sprintf(scrpbuf_main.buf[17], "抄表停投设置:%s",ParaPlcMetp[sn_number].stopped&0x01?"停抄":"投抄");
	//strcpy(scrpbuf_main.buf[18], "当地电能表显示号:");
	//for(i = 0,j = 2;i < 12;i++,j++)
		//displayno[j] = ParaPlcMetp[sn_number].displayno[i];
	//displayno[0] = displayno[1] = 32;         ///形成空格
	//memcpy(scrpbuf_main.buf[19],displayno,14);
		scrpbuf_main.line = 18;
}

/**
* @brief			终端配置信息之版本信息
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpVesionInfo(unsigned short mid, ULONG arg, UCHAR key)
{
	unsigned char VesionInfo[80];
	unsigned char Vesion_Info[4];
	char comply_code[4];
	char equipment_code[10];
	char soft_version[6];
	char soft_date[3];
	char capacity_info[13];
	char comm_agreement[6];
	char hardware_version[6];
	char hardware_date[3];
	
	Vesion_Info[0] = 0x00;
	Vesion_Info[1] = 0x00;
	Vesion_Info[2] = 0x01;
	Vesion_Info[3] = 0x00;

	ReadTermInfo(Vesion_Info, VesionInfo, 45);

	memcpy(comply_code, (char *)&VesionInfo[4], 4);
	memcpy(&equipment_code[2], (char *)&VesionInfo[8], 8);
	memcpy(&soft_version[2], (char *)&VesionInfo[16], 4);
	memcpy(soft_date, (char *)&VesionInfo[20], 3);
	memcpy(&capacity_info[2], (char *)&VesionInfo[23], 11);
	memcpy(&comm_agreement[2], (char *)&VesionInfo[34], 4);
	memcpy(&hardware_version[2], (char *)&VesionInfo[38], 4);
	memcpy(hardware_date, (char *)&VesionInfo[42], 3);

	scrpbuf_main.line = 10;
	//sprintf(scrpbuf_main.buf[0],"厂商代号:%s",&comply_code[0]);
	sprintf(scrpbuf_main.buf[0],"设备编号:");
	equipment_code[0] = equipment_code[1] = 32;
	memcpy(scrpbuf_main.buf[1],equipment_code,10);
	sprintf(scrpbuf_main.buf[2],"软件版本号:");
	soft_version[0] = soft_version[1] = 32;
	memcpy(scrpbuf_main.buf[3],soft_version,6);
	sprintf(scrpbuf_main.buf[4],"软件发布日期:");
	sprintf(scrpbuf_main.buf[5],"  20%02X年%02X月%02X日",soft_date[2],soft_date[1],soft_date[0]);
	sprintf(scrpbuf_main.buf[6],"终端配置容量信息码:");
	capacity_info[0] = capacity_info[1] = 32;
	memcpy(scrpbuf_main.buf[7],capacity_info,13);
	sprintf(scrpbuf_main.buf[8],"终端通信规约版本号:");
	comm_agreement[0] = comm_agreement[1] = 32;
	//memcpy(scrpbuf_main.buf[10],comm_agreement,6);
	sprintf(scrpbuf_main.buf[9],"  Q／GDW_376.1-2009");
	
	//sprintf(scrpbuf_main.buf[11],"终端硬件版本号:");
	//hardware_version[0] = hardware_version[1] = 32;
	//memcpy(scrpbuf_main.buf[12],hardware_version,6);
	//sprintf(scrpbuf_main.buf[13],"硬件发布日期:");
	//sprintf(scrpbuf_main.buf[14],"  20%02X年%02X月%02X日",hardware_date[2],hardware_date[1],hardware_date[0]);
}

/**
* @brief			终端配置信息之端口配置
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpPortCfg(unsigned short mid, ULONG arg, UCHAR key)
{
	unsigned char PortCfg[80];
	unsigned char Port_Cfg[4];
	static const char *channel_type[3] = {"直接RS485接口","直接RS232接口", "串行载波通信模块"};
	static const char *met_type[4] = {"专变","公变抄表", "变电站抄表","台区低压集抄"};
	Port_Cfg[0] = 0x00;
	Port_Cfg[1] = 0x00;
	Port_Cfg[2] = 0x02;
	Port_Cfg[3] = 0x00;
	
	ReadTermInfo(Port_Cfg, PortCfg, 80);
	
	scrpbuf_main.line = 27;
	sprintf(scrpbuf_main.buf[0],"脉冲量输入路数:%d",PortCfg[4]);
	sprintf(scrpbuf_main.buf[1],"开关量输入路数:%d",PortCfg[5]);
	sprintf(scrpbuf_main.buf[2],"直流模拟量输入路数:%d",PortCfg[6]);
	sprintf(scrpbuf_main.buf[3],"开关量输出路数:%d",PortCfg[7]);
	sprintf(scrpbuf_main.buf[4],"装置最多个数:%d",(PortCfg[9]>>4)*4096 + (PortCfg[9]&0x0f)*256 + (PortCfg[8]>>4)*16 + (PortCfg[8]&0x0f));
	sprintf(scrpbuf_main.buf[5],"最大接收缓存区:%d",(PortCfg[11]>>4)*4096 + (PortCfg[11]&0x0f)*256 + (PortCfg[10]>>4)*16 + (PortCfg[10]&0x0f));
	sprintf(scrpbuf_main.buf[6],"最大发送缓存区:%d",(PortCfg[13]>>4)*4096 + (PortCfg[13]&0x0f)*256 + (PortCfg[12]>>4)*16 + (PortCfg[12]&0x0f));
	sprintf(scrpbuf_main.buf[7],"终端MAC地址:");
	sprintf(scrpbuf_main.buf[8],"  %02X%02X%02X%02X%02X%02X",PortCfg[19],PortCfg[18],PortCfg[17],PortCfg[16],PortCfg[15],PortCfg[14]);
	sprintf(scrpbuf_main.buf[9],"通信端口数量:%d",PortCfg[20]);
	sprintf(scrpbuf_main.buf[10],"端口%d通道类型:",PortCfg[21]&0x1F);
	sprintf(scrpbuf_main.buf[11],"  %s",channel_type[PortCfg[21]>>5]);
	sprintf(scrpbuf_main.buf[12],"端口%d抄表类型:",PortCfg[21]&0x1F);
	sprintf(scrpbuf_main.buf[13],"  %s",met_type[PortCfg[22]>>5]);

	sprintf(scrpbuf_main.buf[14],"端口%d通道类型:",PortCfg[33]&0x1F);
	sprintf(scrpbuf_main.buf[15],"  %s",channel_type[PortCfg[33]>>5]);
	sprintf(scrpbuf_main.buf[16],"端口%d抄表类型:",PortCfg[33]&0x1F);
	sprintf(scrpbuf_main.buf[17],"  %s",met_type[PortCfg[34]>>5]);

	sprintf(scrpbuf_main.buf[18],"端口%d通道类型:",PortCfg[45]&0x1F);
	sprintf(scrpbuf_main.buf[19],"  %s",channel_type[PortCfg[45]>>5]);
	sprintf(scrpbuf_main.buf[20],"端口%d抄表类型:",PortCfg[45]&0x1F);
	sprintf(scrpbuf_main.buf[21],"  %s",met_type[PortCfg[46]>>5]);

	sprintf(scrpbuf_main.buf[22],"端口%d通道类型:",PortCfg[57]&0x1F);
	sprintf(scrpbuf_main.buf[23],"  %s",channel_type[PortCfg[57]>>5]);
	sprintf(scrpbuf_main.buf[24],"端口%d抄表类型:",PortCfg[57]&0x1F);
	sprintf(scrpbuf_main.buf[25],"  %s",met_type[PortCfg[58]>>5]);
	
}

/**
* @brief			终端配置信息之其它配置
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpOtherCfg(unsigned short mid, ULONG arg, UCHAR key)
{
	unsigned int max_meter = MAX_METER;
	unsigned int max_cenmrtp = MAX_CENMETP;
	unsigned int max_plc_basemetp = PLC_BASEMETP + 1;
	unsigned int max_important_user = MAX_IMPORTANT_USER;

	scrpbuf_main.line = 8;
	sprintf(scrpbuf_main.buf[0],"测量点最多点数:%d",max_meter);
	sprintf(scrpbuf_main.buf[1],"最大总表测量点数:%d",max_cenmrtp);
	sprintf(scrpbuf_main.buf[2],"载波表起始表号:%d",max_plc_basemetp);
	sprintf(scrpbuf_main.buf[3],"最大重点用户表数:%d",max_important_user);
	sprintf(scrpbuf_main.buf[4],"终端外部生产编号:");
	sprintf(scrpbuf_main.buf[5],"  %s",ParaUni.manuno);
	sprintf(scrpbuf_main.buf[6],"终端内部生产编号:");
	sprintf(scrpbuf_main.buf[7],"  %s",ParaUni.manuno_inner);
}

int get_one_day_meter_data(unsigned char *ScrDay,plmdb_day_t *ScrPlMdbDay);
/**
* @brief			用户数据之日冻结数据
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpPlmMetDayData(unsigned short mid, ULONG arg, UCHAR key)
{
	#if 0
	plmdb_day_t PlMdbDayTmp[MAX_PLCMET];
	unsigned int  sn_number=GetSnNum()-1;
	
	//if(sn_number<7||sn_number>2039)
	if(sn_number<3||sn_number>2039)	
	{
		scrpbuf_main.line = 1;
		sprintf(scrpbuf_main.buf[0],"输入电表号超出范围!");	
		return;
	}
	if(ParaMeter[sn_number].metp_id == 0)
	{
		scrpbuf_main.line = 2;
		sprintf(scrpbuf_main.buf[1],"未配置该序号的表计!");	
		sprintf(scrpbuf_main.buf[2],"    按ESC键返回!");	
		return;
	}
	
    	scrpbuf_main.line = 5;
	//sprintf(scrpbuf_main.buf[0],"电表地址:");	
	//sprintf(scrpbuf_main.buf[1],"当前总电量:");
	//sprintf(scrpbuf_main.buf[2],"  %02X%02X%02X.%02XKWh",PlMdbDay[sn_number-MAX_CENMETP].meter_ene[3],PlMdbDay[sn_number-MAX_CENMETP-1].meter_ene[2],PlMdbDay[sn_number-MAX_CENMETP-1].meter_ene[1],PlMdbDay[sn_number-MAX_CENMETP-1].meter_ene[0]);
	//sprintf(scrpbuf_main.buf[3],"日冻结时间:");
	//sprintf(scrpbuf_main.buf[4],"  %d时%d分",PlMdbDay[sn_number-MAX_CENMETP].readtime/60,PlMdbDay[sn_number-MAX_CENMETP-1].readtime%60);
	#endif
}


/**
* @brief			用户数据之日冻结数据
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpPlmMetDay(unsigned short mid, ULONG arg, UCHAR key)
{
	int meter_index = 2;
	
	scrpbuf_main.line = meter_total_cnt - cen_meter_cnt;
	for(meter_index=2;meter_index<(meter_total_cnt - cen_meter_cnt + 2);meter_index++)
	{
		sprintf(scrpbuf_main.buf[meter_index -2],"%04d:%02x%02x%02x%02x.%02xKWh",meter_index + 1,meter_ene_buffer[meter_index].meter_ene[4],
		meter_ene_buffer[meter_index].meter_ene[3],meter_ene_buffer[meter_index].meter_ene[2],meter_ene_buffer[meter_index].meter_ene[1],
		meter_ene_buffer[meter_index].meter_ene[0]);
		//strcat(scrpbuf_main.buf[meter_index -2],".");
		//strcat(scrpbuf_main.buf[meter_index -2],"%02x",meter_ene_buffer[meter_index].meter_ene[0]);
	}
}


/**
* @brief			用户数据之重点用户数据
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpImportantUserData(unsigned short mid, ULONG arg, UCHAR key)
{
    unsigned int i,j,k;
	int  sn_number = GetSnNum() - 1;
	unsigned char sn_number_1 = sn_number - MAX_CENMETP - 1;

	if(sn_number<=2||sn_number>2039)
	{
		scrpbuf_main.line = 1;
		sprintf(scrpbuf_main.buf[0],"输入电表号超出范围!");	
		return;
	}
	if(ParaMeter[sn_number].metp_id == 0)
	{
		scrpbuf_main.line = 2;
		sprintf(scrpbuf_main.buf[1],"未配置该序号的表计!");	
		sprintf(scrpbuf_main.buf[2],"    按ESC键返回!");	
		return;
	}
	
	for(i=0;i < ParaMix.impuser.num;i++)
	{
		if(sn_number == ParaMix.impuser.metid[i])
			break;
		else
		{
			scrpbuf_main.line = 2;
			sprintf(scrpbuf_main.buf[0],"此输入电表号不是重点");	
			sprintf(scrpbuf_main.buf[1],"用户!");	
			return;
		}
	}
    	scrpbuf_main.line = 48;
	for(i=0,j=0,k=0;i<24;j+=4)
	{
		sprintf(scrpbuf_main.buf[k++],"当日%d点电量:",i++);
		sprintf(scrpbuf_main.buf[k++],"  %02X%02X%02X.%02XKWh",PlMdbImp[sn_number_1].ene[j+3],PlMdbImp[sn_number_1].ene[j+2],PlMdbImp[sn_number_1].ene[j+1],PlMdbImp[sn_number_1].ene[j]);
	}
}



/**
* @brief			用户数据之载波抄表信息
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpPlmInfo(unsigned short mid, ULONG arg, UCHAR key)
{
#define   PHASE_A   (0x01<<4)
#define   PHASE_B   (0x01<<5)
#define   PHASE_C   (0x01<<6)

#define   PHASE_A_1   0x01
#define   PHASE_B_1   (0x01<<1)
#define   PHASE_C_1   (0x01<<2)

	int  sn_number = GetSnNum() - 1;

	if(sn_number < 7||sn_number > 2039)
	{
		scrpbuf_main.line = 1;
		sprintf(scrpbuf_main.buf[0],"输入电表号超出范围!");	
		return;
	}
	if(ParaMeter[sn_number].metp_id == 0)
	{
		scrpbuf_main.line = 2;
		sprintf(scrpbuf_main.buf[1],"未配置该序号的表计!");	
		sprintf(scrpbuf_main.buf[2],"    按ESC键返回!");	
		return;
	}
	
    scrpbuf_main.line = 13;
	sprintf(scrpbuf_main.buf[0],"中继路由级数:%d",PlcState[sn_number].routes);
	switch(PlcState[sn_number].phase&0X70)
	{
	case PHASE_A:
		sprintf(scrpbuf_main.buf[1],"载波抄读相位:A相");
		break;
	case PHASE_B:
		sprintf(scrpbuf_main.buf[1],"载波抄读相位:B相");
   		break;
	case PHASE_C:
		sprintf(scrpbuf_main.buf[1],"载波抄读相位:C相");
    	break;
	default: 
		sprintf(scrpbuf_main.buf[1],"载波抄读相位:未配置");
		break;
	}
	
	switch(PlcState[sn_number].phase&0X07)
	{
	case  PHASE_A_1:
		sprintf(scrpbuf_main.buf[2],"电表实际相位:A相");
    	break;
	case  PHASE_B_1:
		sprintf(scrpbuf_main.buf[2],"电表实际相位:B相");
    	break;
	case  PHASE_C_1:
		sprintf(scrpbuf_main.buf[2],"电表实际相位:C相");
    	break;
	default: 
		sprintf(scrpbuf_main.buf[2],"电表实际相位:未知");
		break;
	}
	
	sprintf(scrpbuf_main.buf[3],"最近一次抄表");
	sprintf(scrpbuf_main.buf[4],"是否成功:%s",(PlcState[sn_number].okflag&0x01)?"是":"否");
	sprintf(scrpbuf_main.buf[5],"最近抄表连续失败");
	sprintf(scrpbuf_main.buf[6],"累计次数:%d",PlcState[sn_number].failcount);
	sprintf(scrpbuf_main.buf[7],"最近一次抄表成功时间");
	sprintf(scrpbuf_main.buf[8],"  20%02d年%d月%d日",PlcState[sn_number].oktime.year,PlcState[sn_number].oktime.month,PlcState[sn_number].oktime.day);
	sprintf(scrpbuf_main.buf[9],"  %d:%d:%d",PlcState[sn_number].oktime.hour,PlcState[sn_number].oktime.minute,PlcState[sn_number].oktime.second);
	sprintf(scrpbuf_main.buf[10],"最近一次抄表失败时间");
	sprintf(scrpbuf_main.buf[11],"  20%02d年%d月%d日",PlcState[sn_number].failtime.year,PlcState[sn_number].failtime.month,PlcState[sn_number].failtime.day);
	sprintf(scrpbuf_main.buf[12],"  %d:%d:%d",PlcState[sn_number].failtime.hour,PlcState[sn_number].failtime.minute,PlcState[sn_number].failtime.second);
}


/**
* @brief			状态菜单界面
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void ScrpRunState(unsigned short mid, ULONG arg, UCHAR key)
{
	runstate_t   *runstate;
	runstate = RunStateModify();
	sysclock_t sys_clock;
	GetClockSysStart(&sys_clock);

    	scrpbuf_main.line = 5;
	//sprintf(scrpbuf_main.buf[0],"供电状态:%s",runstate->pwroff?"非正常":"正常");
	//sprintf(scrpbuf_main.buf[1],"电池充电状态:%s",runstate->batcharge?"充电":"非充电");
	//sprintf(scrpbuf_main.buf[2],"遥信量1状态:%s",runstate->isig_stat&0x01?"合":"开");
	//sprintf(scrpbuf_main.buf[3],"遥信量2状态:%s",runstate->isig_stat&0x02?"合":"开");
	sprintf(scrpbuf_main.buf[0],"系统启动时间:");
	sprintf(scrpbuf_main.buf[1],"  20%02d年%02d月%02d日",sys_clock.year,sys_clock.month,sys_clock.day);
	sprintf(scrpbuf_main.buf[2],"  %02d:%02d:%02d",sys_clock.hour,sys_clock.minute,sys_clock.second);
	sprintf(scrpbuf_main.buf[3],"终端与主站连接状态:");
	sprintf(scrpbuf_main.buf[4],"  %s",SvrCommLineState?"连接建立":"连接断开");
}


void ScrpMeterCfg(unsigned short mid, ULONG arg, UCHAR key)
{
	int scr_meter_total_cnt;
	int scr_cen_meter_cnt;
	int scr_imp_meter_cnt;
	scr_meter_total_cnt = get_meter_total_cnt();
	scr_cen_meter_cnt = get_cen_meter_total_cnt();
	scr_imp_meter_cnt = get_imp_meter_total_cnt();

	
	sprintf(scrpbuf_main.buf[0],"电表总数:  %d个",scr_meter_total_cnt);
	sprintf(scrpbuf_main.buf[1],"集抄电表总数:  %d个",scr_meter_total_cnt - scr_cen_meter_cnt);
	sprintf(scrpbuf_main.buf[2],"总表个数:  %d个",scr_cen_meter_cnt);
	sprintf(scrpbuf_main.buf[3],"重点表个数:  %d个",scr_imp_meter_cnt);
	scrpbuf_main.line = 4;
}


extern sysclock_t read_meter_start_time;
extern sysclock_t read_meter_finish_time;

void ScrpReadMeterStat(unsigned short mid, ULONG arg, UCHAR key)
{
	unsigned char start_time[4];
	unsigned char finish_time[4];
	int i = 0,j = 0;

	memcpy(start_time,&read_meter_start_time.month,4);
	memcpy(finish_time,&read_meter_finish_time.month,4);
	HexToBcd(start_time,4);
	HexToBcd(finish_time,4);

	for(i=0;i<MAX_METER_CNT;i++)
	{
		if(!check_buf(meter_ene_buffer[i].meter_ene) 
			&& (meter_ene_buffer[i].met_id>=2) 
			&&(meter_ene_buffer[i].portcfg&0x1F) == PLC_PORT)
		{
			j++;
		}
	}
	sprintf(scrpbuf_main.buf[0],"用户表成功数:%d个",j);
	sprintf(scrpbuf_main.buf[1],"重点表成功数:%d个",imp_meter_read_succ_cnt);
	sprintf(scrpbuf_main.buf[2],"抄表起始时间: ");
	sprintf(scrpbuf_main.buf[3],"  %02x月%02x日%02x:%02x", start_time[0], start_time[1], start_time[2],start_time[3]);
	sprintf(scrpbuf_main.buf[4],"抄表完成时间:");
	if(!read_meter_finish)
	{
		sprintf(scrpbuf_main.buf[5],"  抄表进行中.....");
	}
	else
	{
		sprintf(scrpbuf_main.buf[5],"  %02x月%02x日%02x:%02x", finish_time[0], finish_time[1], finish_time[2],finish_time[3]);
	}
	sprintf(scrpbuf_main.buf[6],"抄表状态: ");
	if(!read_meter_finish)
	{
		strcat(scrpbuf_main.buf[6],"正在抄表");
	}
	else
	{
		strcat(scrpbuf_main.buf[6],"抄表完成");
	}
		
	scrpbuf_main.line = 7;
}


void ScrpRestartTerm(unsigned short mid, ULONG arg, UCHAR key)
{
	ClearLcdScreen();
	sprintf(scrpbuf_main.buf[0],"  系统正在重启.....");
	scrpbuf_main.line = 1;
	DisplayNormal((unsigned char*)scrpbuf_main.buf,strlen((char *)&scrpbuf_main.buf[0]),16 * 3,0);
	//DisplayNormal(const unsigned char *str,unsigned char len,unsigned char x,unsigned char y);
	DisplayLcdBuffer();
	Sleep(500);
	SysRestart();
}

static void PortTranStr(unsigned short port, unsigned char *pstr)
{
	pstr[0] = port/10000;
	pstr[1] = (port-pstr[0]*10000)/1000;
	pstr[2] = (port - pstr[0]*10000 - pstr[1]*1000)/100;
	pstr[3] = (port - pstr[0]*10000 - pstr[1]*1000 - pstr[2]*100)/10;
	pstr[4] = port%10;
}

void ScrShowTermPara(unsigned short mid, ULONG arg, UCHAR key)
{
  	int x, y;
	unsigned char svrip_rtn[12],svrbakip_rtn[12],  port_rtn[5],city_area_rtn[4],scr_term_addr_rtn[5];
	unsigned char apn_rtn[16];
	unsigned char port_ini[5],term_addr_ini[5];
	unsigned short term_addr;
	char apn[16];
	//unsigned char apn_len;
	int i;

	ClrShowBuf(&scrpbuf_main);
	InitControlPage();
	PortTranStr(ParaTerm.svrip.portmain, port_ini);
	memset(term_addr_ini,0x00,5);
	term_addr = MAKE_SHORT(ParaUni.addr_sn);
	PortTranStr(term_addr,term_addr_ini);
	
	DisplayNormal("    通信参数设置",16,2*16-MENU_LINE_POS,0);	
	
 	x = 4; 
	y = 5;
	DisplayNormal("终端地址:",13,3*16-MENU_LINE_POS,0);

	AddEditBox(3, 10, 10, (char *)&typ_charlist_a[0], ParaUni.addr_area[1]>>4, &city_area_rtn[0], CHAR_FLAG);
	AddEditBox(3, 11, 10, (char *)&typ_charlist_a[0], ParaUni.addr_area[1]&0x0f, &city_area_rtn[1], CHAR_FLAG);	
	AddEditBox(3, 12, 10, (char *)&typ_charlist_a[0], ParaUni.addr_area[0]>>4, &city_area_rtn[2], CHAR_FLAG);
	AddEditBox(3, 13, 10, (char *)&typ_charlist_a[0], ParaUni.addr_area[0]&0x0f, &city_area_rtn[3], CHAR_FLAG);
	DisplayNormal("-",1,3*16-MENU_LINE_POS,14*8);
	AddEditBox(3, 15, 10, (char *)&typ_charlist_a[0], term_addr_ini[0], &scr_term_addr_rtn[0], CHAR_FLAG);
	AddEditBox(3, 16, 10, (char *)&typ_charlist_a[0], term_addr_ini[1], &scr_term_addr_rtn[1], CHAR_FLAG);	
	AddEditBox(3, 17, 10, (char *)&typ_charlist_a[0], term_addr_ini[2], &scr_term_addr_rtn[2], CHAR_FLAG);
	AddEditBox(3, 18, 10, (char *)&typ_charlist_a[0], term_addr_ini[3], &scr_term_addr_rtn[3], CHAR_FLAG);
	AddEditBox(3, 19, 10, (char *)&typ_charlist_a[0], term_addr_ini[4], &scr_term_addr_rtn[4], CHAR_FLAG);

	DisplayNormal("APN:",strlen("APN:"),x*16-MENU_LINE_POS,0);

/*
	for(i=0;i<16;i++)
	{
		apn_rtn[i] = 0;
	}

	for(i=0;i<16;i++)
	{
		if(ParaTerm.svrip.apn[i] != '.')
		{
			apn[i] = (ParaTerm.svrip.apn[i]-typ_charlist_3[0]);
		}
		else
		{
			apn[i] = (ParaTerm.svrip.apn[i]-'.'+26);
			PrintLog(0, "ParaTerm.svrip.apn i=%d\n",i);
		}
	}
	*/

	for(i=0;i<16;i++)
	{
		apn_rtn[i] = '\0';
	}
	

	for(i=0;i<strlen(ParaTerm.svrip.apn);i++)
	{
		if(ParaTerm.svrip.apn[i] != '.')
		{
			apn[i] = (ParaTerm.svrip.apn[i]-typ_charlist_3[0]);
		}
		else
		{
			apn[i] = (ParaTerm.svrip.apn[i]-'.'+26);
			PrintLog(0, "ParaTerm.svrip.apn i=%d\n",i);
		}
	}
	
	
    	AddEditBox(x, 4, 28, (char*)&typ_charlist_3[0],apn[0],&apn_rtn[0], CHAR_FLAG);
    	AddEditBox(x, 5, 28, (char*)&typ_charlist_3[0],apn[1],&apn_rtn[1], CHAR_FLAG);
    	AddEditBox(x, 6, 28, (char*)&typ_charlist_3[0],apn[2],&apn_rtn[2], CHAR_FLAG);
    	AddEditBox(x, 7, 28, (char*)&typ_charlist_3[0],apn[3],&apn_rtn[3], CHAR_FLAG);
    	AddEditBox(x, 8, 28, (char*)&typ_charlist_3[0],apn[4],&apn_rtn[4], CHAR_FLAG);
    	AddEditBox(x, 9, 28, (char*)&typ_charlist_3[0],apn[5],&apn_rtn[5], CHAR_FLAG);
    	AddEditBox(x, 10, 28, (char*)&typ_charlist_3[0],apn[6],&apn_rtn[6], CHAR_FLAG);
    	AddEditBox(x, 11, 28, (char*)&typ_charlist_3[0],apn[7],&apn_rtn[7], CHAR_FLAG);
    	AddEditBox(x, 12, 28, (char*)&typ_charlist_3[0],apn[8],&apn_rtn[8], CHAR_FLAG);
	x++;
	y = 0; 
	
    	DisplayNormal("主IP:",strlen("主IP:"),x*16-MENU_LINE_POS,0);
    	y = 5;
	
	int pls = y*8;
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], ParaTerm.svrip.ipmain[0]/100, &svrip_rtn[0], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipmain[0]%100)/10, &svrip_rtn[1], CHAR_FLAG);	
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipmain[0]%100)%10, &svrip_rtn[2], CHAR_FLAG);
	DisplayNormal(".",1,x*16-MENU_LINE_POS,pls+24);
	y++;
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], ParaTerm.svrip.ipmain[1]/100, &svrip_rtn[3], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipmain[1]%100)/10, &svrip_rtn[4], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipmain[1]%100)%10, &svrip_rtn[5], CHAR_FLAG);	
	DisplayNormal(".",1,x*16-MENU_LINE_POS,pls+56);
	y++;
    	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], ParaTerm.svrip.ipmain[2]/100, &svrip_rtn[6], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipmain[2]%100)/10, &svrip_rtn[7], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0],(ParaTerm.svrip.ipmain[2]%100)%10, &svrip_rtn[8], CHAR_FLAG);	
	DisplayNormal(".",1,x*16-MENU_LINE_POS,pls+88);
	y++;
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], ParaTerm.svrip.ipmain[3]/100,  &svrip_rtn[9], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipmain[3]%100)/10, &svrip_rtn[10], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipmain[3]%100)%10, &svrip_rtn[11], CHAR_FLAG);		
	x++;
    	DisplayNormal("备IP:",strlen("备IP:"),x*16-MENU_LINE_POS,0);
    	y = 5;
	
	int plsbak = y*8;
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], ParaTerm.svrip.ipbakup[0]/100, &svrbakip_rtn[0], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipbakup[0]%100)/10, &svrbakip_rtn[1], CHAR_FLAG);	
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipbakup[0]%100)%10, &svrbakip_rtn[2], CHAR_FLAG);
	DisplayNormal(".",1,x*16-MENU_LINE_POS,plsbak+24);
	y++;
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], ParaTerm.svrip.ipbakup[1]/100, &svrbakip_rtn[3], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipbakup[1]%100)/10, &svrbakip_rtn[4], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipbakup[1]%100)%10, &svrbakip_rtn[5], CHAR_FLAG);	
	DisplayNormal(".",1,x*16-MENU_LINE_POS,plsbak+56);
	y++;
    	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], ParaTerm.svrip.ipbakup[2]/100, &svrbakip_rtn[6], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipbakup[2]%100)/10, &svrbakip_rtn[7], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0],(ParaTerm.svrip.ipbakup[2]%100)%10, &svrbakip_rtn[8], CHAR_FLAG);	
	DisplayNormal(".",1,x*16-MENU_LINE_POS,plsbak+88);
	y++;
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], ParaTerm.svrip.ipbakup[3]/100,  &svrbakip_rtn[9], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipbakup[3]%100)/10, &svrbakip_rtn[10], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], (ParaTerm.svrip.ipbakup[3]%100)%10, &svrbakip_rtn[11], CHAR_FLAG);	
	x++;
    	DisplayNormal("主站端口:",strlen("主站端口:"),x*16-MENU_LINE_POS,0);
	
	y = 11;
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], port_ini[0], &port_rtn[0], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], port_ini[1], &port_rtn[1], CHAR_FLAG);	
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], port_ini[2], &port_rtn[2], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], port_ini[3], &port_rtn[3], CHAR_FLAG);
	AddEditBox(x, y++, 10, (char *)&typ_charlist_a[0], port_ini[4], &port_rtn[4], CHAR_FLAG);
	
    	x++;
	x++;

	//PrintLog(0, "x=%d",x);
	AddButton(8, 5, "确定", DefOkClick);
	AddButton(8, 11, "取消", DefEscClick);
   
	if(CONTROL_OK_EXIT == ShowControlPage()) {	

		PrintHexLog(0, svrip_rtn, 12);
		for(i=0; i<4; i++) ParaTerm.svrip.ipmain[i] = svrip_rtn[0+i*3]*100+svrip_rtn[1+i*3]*10+svrip_rtn[2+i*3];
		ParaTerm.svrip.portmain = port_rtn[0]*10000+port_rtn[1]*1000+port_rtn[2]*100+port_rtn[3]*10+port_rtn[4];
		term_addr = scr_term_addr_rtn[0]*10000+scr_term_addr_rtn[1]*1000+scr_term_addr_rtn[2]*100+scr_term_addr_rtn[3]*10+scr_term_addr_rtn[4];
		ParaUni.addr_area[1] = city_area_rtn[1]+city_area_rtn[0]*16;
		ParaUni.addr_area[0] = city_area_rtn[3]+city_area_rtn[2]*16;
		DEPART_SHORT(term_addr,ParaUni.addr_sn);


	for(i=0;i<strlen(ParaTerm.svrip.apn);i++)
	{
		if(ParaTerm.svrip.apn[i] != '.')
		{
			apn[i] = (ParaTerm.svrip.apn[i]-typ_charlist_3[0]);
		}
		else
		{
			apn[i] = (ParaTerm.svrip.apn[i]-'.'+26);
			PrintLog(0, "ParaTerm.svrip.apn i=%d\n",i);
		}
	}



		for(i=0;i<strlen(apn_rtn);i++)
		{
			if(apn_rtn[i] != 26)
			{
				apn_rtn[i] += (typ_charlist_3[0]);
			}
			else 
			{
				apn_rtn[i] += '.' - 26;
			}
			ParaTerm.svrip.apn[i] = apn_rtn[i];
		}
		//apn_len = i;
		//PrintLog(0, "apn_len=%d\n",apn_len);
		//strcpy((char *)ParaTerm.svrip.apn, "CMNET");
		
		//for(i=0;i<apn_len;i++)
			//ParaTerm.svrip.apn[i] = apn_rtn[i];
		//ParaUni.uplink = chn_rtn;
		SetSaveParamFlag(SAVEFLAG_TERM|SAVEFLAG_UNI);
		SaveParam();
		ClearLcdScreen();
		scrpbuf_main.line = 1;
		strcpy(scrpbuf_main.buf[0], " 设置成功!");
		//strcpy(scrpbuf_main.buf[1], " 系统5秒后复位...");
		DisplayNormal((unsigned char*)&scrpbuf_main.buf[0],strlen((char *)&scrpbuf_main.buf[0]),16 * 3,0);
		//DisplayNormal((unsigned char*)&scrpbuf_main.buf[1],strlen((char *)&scrpbuf_main.buf[1]),16 * 4,0);
		DisplayLcdBuffer();
		//Sleep(500);
		//SysRestart();
		//SysAddCTimer(5, CTimerSysRestart, 0);
		return; 
	}
	else {
		scrpbuf_main.line = 1;
		strcpy(scrpbuf_main.buf[0], " 退出,不保存!");
		return;
	}
	return;
	
}




