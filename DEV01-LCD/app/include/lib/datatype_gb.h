/**
* datatype_gb.c -- 国标数据类型转换函数头文件
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#ifndef _DATATYPE_GB_H
#define _DATATYPE_GB_H

int Gbformat02ToPower(const unsigned char *psrc);
void PowerToGbformat02(int src, unsigned char *pdst);

#define MAX_GENE_SMALL		2000000000  //以int储存的最大值
/*#define MAX_GENE_LARGE		9999999000000ll
#define gene_t long long    //0.001kWh/厘

gene_t Gbformat03ToEne(const unsigned char *psrc);
void EneToGbformat03(gene_t src, unsigned char *pdst);*/

#define gene_t int   // kWh/厘

int Gbformat03ToShortEne(const unsigned char *psrc);
void ShortEneToGbformat03(int src, unsigned char *pdst);

#define Gbformat03ToEne Gbformat03ToShortEne
#define EneToGbformat03 ShortEneToGbformat03

#endif /*_DATATYPE_GB_H*/


