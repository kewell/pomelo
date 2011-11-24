/**
* ad_calib.c -- AD校准
* 
* 
* 创建时间: 2010-6-12
* 最后修改时间: 2010-6-12
*/

#include <stdio.h>
#include <string.h>

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/adc.h"
#include "include/sys/xin.h"
#include "include/debug/shellcmd.h"

#define TEMPCALIB_FILE		WORK_PATH "tempcalib.xin"
#define BATCALIB_FILE		WORK_PATH "batcalib.xin"

#define MAX_TMPCALIB_NUM	6
#define MAX_BATCALIB_NUM	6

struct ad_calib {
	int adv;
	int realv;
};
static struct ad_calib TempCalib[MAX_TMPCALIB_NUM];
static struct ad_calib BatCalib[MAX_BATCALIB_NUM];

static int TempCalibNum = 0;
static int BatCalibNum = 0;

/**
* @brief 根据校准参数计算校准后数据
* @param array 校准参数数组
* @param num 数组长度
* @param adv 原始数据
* @return 校准后数据
*/
static int CalByCalib(const struct ad_calib *array, int num, int adv)
{
	int i, m;

	for(i=0; i<num; i++) {
		if(adv < array[i].adv) break;
	}

	if(i >= num) i = num - 2;
	else if(i) i = i - 1;

	if(array[i+1].adv == array[i].adv) return adv;

	m = (array[i+1].realv - array[i].realv) * adv;
	m += array[i].realv * array[i+1].adv - array[i+1].realv * array[i].adv;
	m = m / (array[i+1].adv - array[i].adv);

	return m;
}

//-40~99, 0.01kO
static const unsigned short DefTempTab[] = {
19250,18210,17230,16310,15450,14630,13870,13150,12470,11830,  // -40~-31
11230,10660,10120,9618,9140,8689,8264,7861,7482,7121,  // -30~-21
6781,6459,6155,5867,5593,5335,5090,4857,4637,4427,  // -20~-11
4228,4041,3861,3691,3529,3377,3230,3091,2958,2833,  // -10~-1
2720,2607,2497,2394,2295,2200,2110,2031,1949,1870,  // 0~9
1795,1724,1655,1590,1528,1471,1414,1359,1307,1257,  // 10~19
1214,1167,1122,1080,1039,1000,963,928,894,862,  // 20~29
831,801,773,745,719,694,670,647,624,603,  // 30~39
582,563,544,526,508,491,475,459,444,430,  // 40~49
416,403,390,378,366,354,343,332,322,312,  // 50~59
302,293,284,275,267,259,251,244,237,230,  // 60~69
223,217,210,204,198,193,187,182,177,172,  // 70~79
167,162,158,153,149,145,141,137,134,130,  // 80~89
127,123,120,117,114,111,108,105,102,0,  // 90~99
};
#define NUM_DEFTEMPTAB		(sizeof(DefTempTab)/sizeof(DefTempTab[0]))

//return 0.1C
static int CalDefTemp(int adv)
{
#define VOL_MAX		3300  // 3.3v
#define R_Z			3000  // 30kO
#define BASE_TEMP	-40  // -40C
#define DIV_TEMP	10    // 1C

	int rx, i, j;
	unsigned short rxs;

	//printf("adv = %dmV\n", adv);

	if(adv >= VOL_MAX) return -410;  // -41
	else if(adv <= 0) return 1000;  //100

	rx = (R_Z * adv) / (VOL_MAX - adv);
	rxs = (unsigned short)rx;
	if(rxs > DefTempTab[0]) return -410;
	else if(rxs == 0) return 1000;

	//printf("rx=%d.%02dkO\n", rx/100, rx%100);

	for(i=0; i<(NUM_DEFTEMPTAB-1); i++) {
		if(rxs <= DefTempTab[i] && rxs > DefTempTab[i+1]) break;
	}
	//printf("find %d array, should be %d to %d\n", i, BASE_TEMP+i, BASE_TEMP+i+1);

	j = (BASE_TEMP+i+1)*(int)DefTempTab[i];
	j -= (BASE_TEMP+i)*(int)DefTempTab[i+1];
	j -= rx;
	j *= DIV_TEMP;
	j /= (int)(DefTempTab[i] - DefTempTab[i+1]);

	//printf("tmp = %d\n", j);

	return j;
}

/**
* @brief 读取当前温度
* @return 当前温度(0.1度)
*/
int ReadTemperature(void)
{
	int adv = AdcRead(ADC_CHN_TEMP);

	if(TempCalibNum < 2) return CalDefTemp(adv);

	return CalByCalib(TempCalib, TempCalibNum, adv);
}

/**
* @brief 读取当前电池电压
* @return 当前电池电压(mv)
*/
int ReadBatVol(void)
{
#define MULTI_RATE		335

	int adv = AdcRead(ADC_CHN_BAT);

	if(BatCalibNum < 2) {
		adv = adv * MULTI_RATE / 100;
		return adv;
	}

	return CalByCalib(BatCalib, BatCalibNum, adv);
}

/**
* @brief 将校准参数数组按AD值从小到大排列
* @param array 校准参数数组
* @param num 数组长度
*/
static void ArrangeCalib(struct ad_calib *array, int num)
{
	int vtmp, itmp, rtmp;
	int i, j;

	for(i=0; i<(num-1); i++) {
		vtmp = array[i].adv;
		itmp = i;

		for(j=(i+1); j<num; j++) {
			if(array[j].adv < vtmp) {
				vtmp = array[j].adv;
				itmp = j;
			}
		}

		if(itmp != i) {
			j = array[i].adv;
			rtmp = array[i].realv;

			array[i].adv = vtmp;
			array[i].realv = array[itmp].realv;
			array[itmp].adv = j;
			array[itmp].realv = rtmp;
		}
	}
}

/**
* @brief 载入校准参数文件
* @param file 文件名
* @param array 校准数组
* @param maxlen 校准数组最大长度
* @return 成功返回校准数组长度, 失败返回-1
*/
static int LoadCalibFile(const char *file, struct ad_calib *array, int maxnum)
{
	XINREF pf;
	int i, num, value;
	char name[32];

	DebugPrint(0, "  load %s...", file);

	pf = XinOpen(file, 'r');
	if(NULL == pf) {
		DebugPrint(0, "no file\n");
		return -1;
	}

	num = XinReadInt(pf, "num", 0);
	if(num < 2) {
		DebugPrint(0, "error\n");
		goto mark_fail;
	}
	if(num > maxnum) num = maxnum;

	for(i=0; i<num; i++) {
		sprintf(name, "adv%d", i);
		value = XinReadInt(pf, name, -1);
		if(value < 0) {
			DebugPrint(0, "error %s\n", name);
			goto mark_fail;
		}
		array[i].adv = value;

		sprintf(name, "realv%d", i);
		value = XinReadInt(pf, name, -1000);
		if(value == -1000) {
			DebugPrint(0, "error %s\n", name);
			goto mark_fail;
		}
		array[i].realv = value;
	}

	DebugPrint(0, "ok\n");
	XinClose(pf);
	return num;

mark_fail:
	XinClose(pf);
	return -1;
}

/**
* @brief 载入校准参数
*/
void LoadAdCalib(void)
{
	//@change later: 温度和电池电压需要缺省校准参数
	int num;

	num = LoadCalibFile(TEMPCALIB_FILE, TempCalib, MAX_TMPCALIB_NUM);
	if(num >= 2) {
		TempCalibNum = num;
		ArrangeCalib(TempCalib, TempCalibNum);
	}

	num = LoadCalibFile(BATCALIB_FILE, BatCalib, MAX_BATCALIB_NUM);
	if(num >= 2) {
		BatCalibNum = num;
		ArrangeCalib(BatCalib, BatCalibNum);
	}
}

static int shell_adcalib(int argc, char *argv[])
{
	int i;

	i = ReadBatVol();
	PrintLog(0, "电池电压: %dmV\n", i);
	i = ReadTemperature();
	PrintLog(0, "当前温度: %d.%dC\n", i/10, i%10);

	return 0;
}
DECLARE_SHELL_CMD("calad", shell_adcalib, "读取校准AD值");

