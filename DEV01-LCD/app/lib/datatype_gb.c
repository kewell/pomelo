/**
* datatype_gb.c -- 国标数据类型转换函数
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#include "include/lib/bcd.h"
#include "include/lib/datatype_gb.h"

/**
* @brief 将国标数据格式02转换为功率
* @param psrc 源数据指针(国标数据格式02)
* @return 计算结果功率(0.01kW单位)
*/
int Gbformat02ToPower(const unsigned char *psrc)
{
	int rtn, i, base;
	unsigned char uc;

	rtn = (int)(psrc[1]&0x0f);
	rtn *= 100;
	uc = (psrc[0]&0xf0)>>4;
	uc *= 10;
	uc += psrc[0]&0x0f;
	rtn += (int)uc&0xff;

	uc = (psrc[1]&0xe0)>>5;
	uc = (~uc)&0x07;

	base = (int)uc;
	if(base <= 0) {
		rtn /= 10;
	}
	else {
		base--;
		for(i=0; i<base; i++) rtn *= 10;
	}

	if(psrc[1]&0x10) rtn *= -1;

	return(rtn);
}

/**
* @brief 将功率转换为国标数据格式02
* @param src 源功率(0.01kW单位)
* @param pdst 目的数据指针(国标数据格式02)
*/
void PowerToGbformat02(int src, unsigned char *pdst)
{
	int i;
	unsigned char flag, uc;

	if(src < 0) {
		src *= -1;
		flag = 1;
	}
	else flag = 0;

	//为了科陆测试台的参数一致性检查
	if((src <= 99900) && (0 == (src%100))) {
		src /= 100;

		uc = 0x80;
		if(flag) uc |= 0x10;
		uc += (unsigned char)(src/100);
		pdst[1] = uc;

		src %= 100;
		uc = (unsigned char)(src/10)<<4;
		uc += (unsigned char)(src%10);
		pdst[0] = uc;

		return;
	}

	for(i=1; i<7; i++) {
		if(src < 1000) break;
		src /= 10;
	}
	if(src >= 1000) src = 999;

	uc = i;
	uc = (~uc)&0x07;
	uc = (uc<<5)&0xe0;
	if(flag) uc |= 0x10;

	uc += (unsigned char)(src/100);
	pdst[1] = uc;

	src %= 100;

	uc = (unsigned char)(src/10)<<4;
	uc += (unsigned char)(src%10);
	pdst[0] = uc;
}

#if 0
/**
* @brief 将国标数据格式03转换为电能量
* @param psrc 源数据指针(国标数据格式03)
* @return 计算结果电能量(0.001kWh/厘单位)
*/
gene_t Gbformat03ToEne(const unsigned char *psrc)
{
	unsigned int tmp;
	gene_t rtn;

	tmp = (unsigned long)psrc[3]&0x0f;
	tmp *= 1000000;
	tmp += BcdToUnsigned(psrc, 3);

	if(psrc[3]&0x40) {
		rtn = (gene_t)tmp;
		if(psrc[3]&0x10) rtn *= -1000000;
		else rtn *= 1000000;
	}
	else {
		rtn = (gene_t)tmp;
		if(psrc[3]&0x10) rtn *= -1000;
		else rtn *= 1000;
	}

	return(rtn);
}

/**
* @brief 将电能量转换为国标数据格式03
* @param src 源电能量(0.001kWh/厘单位)
* @param pdst 目的数据指针(国标数据格式03)
*/
void EneToGbformat03(gene_t src, unsigned char *pdst)
{
	unsigned char flag;
	unsigned int tmp;

	if(src < 0) {
		flag = 0x10;
		src *= -1;
	}
	else flag = 0;

	if(src > 9999999000ll) {
		src /= 1000000;
		flag |= 0x40;
	}
	else src /= 1000;

	if(src > 9999999) tmp = 9999999;
	else tmp = (unsigned int)src;

	UnsignedToBcd(tmp, pdst, 4);
	pdst[3] &= 0x0f;
	pdst[3] |= flag;
}
#endif

/**
* @brief 将国标数据格式03转换为短格式电能量
* @param psrc 源数据指针(国标数据格式03)
* @return 计算结果电能量(1kWh/厘单位)
*/
int Gbformat03ToShortEne(const unsigned char *psrc)
{
	unsigned int tmp;
	int rtn;

	tmp = (unsigned long)psrc[3]&0x0f;
	tmp *= 1000000;
	tmp += BcdToUnsigned(psrc, 3);

	if(psrc[3]&0x40) {
		if(tmp > (MAX_GENE_SMALL/1000)) tmp = MAX_GENE_SMALL/1000;
		rtn = (int)tmp;
		if(psrc[3]&0x10) rtn *= -1000;
		else rtn *= 1000;
	}
	else {
		if(tmp > MAX_GENE_SMALL) tmp = MAX_GENE_SMALL;
		rtn = (int)tmp;
		if(psrc[3]&0x10) rtn *= -1;
	}

	return(rtn);
}

/**
* @brief 将短格式电能量转换为国标数据格式03
* @param src 源电能量(1kWh/厘单位)
* @param pdst 目的数据指针(国标数据格式03)
*/
void ShortEneToGbformat03(int src, unsigned char *pdst)
{
	unsigned char flag;

	if(src < 0) {
		flag = 0x10;
		src *= -1;
	}
	else flag = 0;

	if(src > 9999999) {
		src /= 1000;
		flag |= 0x40;
	}

	UnsignedToBcd(src, pdst, 4);
	pdst[3] &= 0x0f;
	pdst[3] |= flag;
}

