/**
* bcd.h -- BCD码转换函数头文件
* 
* 
* 创建时间: 2010-4-1
* 最后修改时间: 2010-5-5
*/

#ifndef _LIB_BCD_H
#define _LIB_BCD_H

unsigned int BcdToUnsigned(const unsigned char *bcd, int len);
int UnsignedToBcd(unsigned int ul, unsigned char *bcd, int maxlen);
int BcdToInt(const unsigned char *bcd, int len);
int IntToBcd(int l, unsigned char *bcd, int maxlen);
void HexToBcd(unsigned char *str, int len);
void BcdToHex(unsigned char *str, int len);

static inline int HexIsEmpty(const unsigned char *buf, int len, unsigned char flag)
{
	int i;

	for(i=0; i<len; i++) {
		if(buf[i] != flag) return 0;
	}

	return 1;
}

static inline int HexComp(const unsigned char *buf1, const unsigned char *buf2, int len)
{
	int i;

	for(i=0; i<len; i++) {
		if(buf1[i] != buf2[i]) return 1;
	}

	return 0;
}

#endif /*_LIB_BCD_H*/
