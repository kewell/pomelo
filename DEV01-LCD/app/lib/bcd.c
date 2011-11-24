/**
* bcd.c -- BCD码转换函数
* 
* 
* 创建时间: 2010-4-1
* 最后修改时间: 2010-5-5
*/

unsigned int BcdToUnsigned(const unsigned char *bcd, int len)
{
	unsigned int rtn;
	int i;
	unsigned char uc;

	rtn=0;

	for(i=(len-1); i>=0; i--) {
		rtn *= 100;

		uc = ((bcd[i]&0xf0)>>4)*10;
		uc += bcd[i]&0x0f;

		rtn += (unsigned int)uc&0xff;
	}

	return(rtn);
}

int UnsignedToBcd(unsigned int ul, unsigned char *bcd, int maxlen)
{
	int i;
	unsigned char uc;

	for(i=0; i<maxlen; i++) {
		if(0 == ul) {
			*bcd++ = 0;
		}
		else {
			uc = ul%10;
			ul /= 10;
			uc += (unsigned char)((ul%10)<<4);
			ul /= 10;

			*bcd++ = uc;
		}
	}

	if(0 != ul) return 1;
	else return 0;
}

int BcdToInt(const unsigned char *bcd, int len)
{
	int rtn;
	int i;
	unsigned char uc, flag;

	if(len <= 0) return 1;
	len -= 1;

	if(bcd[len]&0x80) {
		flag = 1;
		uc = ((bcd[len]&0x70)>>4)*10;
	}
	else {
		flag = 0;
		uc = ((bcd[len]&0xf0)>>4)*10;
	}

	uc += bcd[len]&0x0f;
	rtn = (int)uc&0xff;

	for(i=len-1; i>=0; i--) {
		rtn *= 100;

		uc = ((bcd[i]&0xf0)>>4)*10;
		uc += bcd[i]&0x0f;

		rtn += (int)uc&0xff;
	}

	if(flag) rtn *= -1;

	return(rtn);
}

int IntToBcd(int l, unsigned char *bcd, int maxlen)
{
	int i;
	unsigned char uc, flag;

	if(maxlen <= 0) return 0;

	if(l < 0) {
		l *= -1;
		flag = 1;
	}
	else flag = 0;

	for(i=0; i<maxlen; i++) {
		if(0 == l) {
			bcd[i] = 0;
		}
		else {
			uc = l%10;
			l /= 10;
			uc += (unsigned char)((l%10)<<4);
			l /= 10;

			bcd[i] = uc;
		}
	}
	
	if(flag) bcd[maxlen-1] |= 0x80;
	else bcd[maxlen-1] &= 0x7f;

	if(0 != l) return 1;
	else return 0;
}

void HexToBcd(unsigned char *str, int len)
{
	int i;
	unsigned char uc1, uc2;

	for(i=0; i<len; i++) {
		uc1 = str[i]%10;
		uc2 = str[i]/10;
		if(uc2 > 9) uc2 = 9;
		uc1 |= (uc2<<4)&0xf0;

		str[i] = uc1;
	}
}

void BcdToHex(unsigned char *str, int len)
{
	int i;
	unsigned char uc1, uc2;

	for(i=0; i<len; i++) {
		uc1 = str[i]&0x0f;
		uc2 = ((str[i]&0xf0)>>4)*10;
		uc1 += uc2;

		str[i] = uc1;
	}
}

