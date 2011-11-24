/**
* xin.c -- 类ini文本配置文件储存
* 
* 
* 创建时间: 2010-4-21
* 最后修改时间: 2010-5-5
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "include/sys/mutex.h"
#include "include/sys/xin.h"
#include "include/lib/crc.h"
#include "include/debug.h"

//关闭宏使xin操作可重入
//打开宏不可重入, 但更少的动态分配大内存
#define USE_XINMUTEX

#define XIN_MAGIC			0xaf7c32d1
#define XIN_MAXSIZE			0x7000
#define XIN_MAXFILENAME		128

//文件索引
typedef struct {
	int len;
	char openflag;
	char *buffer;
	char *pscan;
	char filename[XIN_MAXFILENAME];
} xinref_t;

typedef struct {
	unsigned int magic;
	unsigned short len;
	unsigned short crc;
} xinhead_t;

#ifdef USE_XINMUTEX
static xinref_t XinRef;
static char XinBuffer[XIN_MAXSIZE+128];
static sys_mutex_t XinMutex;

#define XIN_LOCK	SysLockMutex(&XinMutex)
#define XIN_UNLOCK	SysUnlockMutex(&XinMutex)
#else
#define XIN_LOCK
#define XIN_UNLOCK
#endif

/**
* @brief 初始化函数
* @return 成功0, 否则失败
*/
DECLARE_INIT_FUNC(XinInit);
int XinInit(void)
{
#ifdef USE_XINMUTEX
	SysInitMutex(&XinMutex);
#endif

	SET_INIT_FLAG(XinInit);

	return 0;
}

/**
* @brief 打开配置文件
* @param filename 打开文件名
* @param flag 打开方式. 'r':读取方式; 'w':写方式
* @return 成功时返回文件索引指针，否则为NULL(空指针)
*/
XINREF XinOpen(const char *filename, char flag)
{
	FILE *pf;
	xinref_t *pref;
	int len;

	AssertLogReturn(strlen(filename)>=XIN_MAXFILENAME, NULL, "%s filename too long\n", filename);

	XIN_LOCK;

	pf = NULL;
	if('r' == flag) {
		pf = fopen(filename, "rb");
		if(NULL == pf) {
			XIN_UNLOCK;
			return NULL;
		}
	}
	else if('w' != flag) {
		ErrorLog("invalid flag(%c)\n", flag);
		XIN_UNLOCK;
		return NULL;
	}

#ifdef USE_XINMUTEX
	pref = &XinRef;
	pref->buffer = pref->pscan = XinBuffer;
#else
	pref = malloc(sizeof(xinref_t));
	if(NULL == pref) {
		if(NULL != pf) fclose(pf);
		ErrorLog("malloc %d bytes fail\n", sizeof(xinref_t));
		XIN_UNLOCK;
		return NULL;
	}
	pref->buffer = pref->pscan = NULL;
#endif

	pref->openflag = flag;
	strcpy(pref->filename, filename);

	if('r' == flag) {
		xinhead_t head;
		unsigned short crc;

		if(fread(&head, sizeof(head), 1, pf) <= 0) {
			ErrorLog("%s file head too short\n", filename);
			goto mark_fail;
		}

		if(head.magic != XIN_MAGIC) {
			ErrorLog("%s magic invalid(0x%08X)\n", filename, head.magic);
			goto mark_fail;
		}

		len = (int)head.len & 0xffff;
		if(len <= 0 || len > XIN_MAXSIZE) {
			ErrorLog("invalid len(%d)\n", len);
			goto mark_fail;
		}

#ifndef USE_XINMUTEX
		pref->buffer = malloc(len+128);
		if(NULL == pref->buffer) {
			ErrorLog("malloc %d bytes fail\n", len+128);
			goto mark_fail;
		}
#endif
		
		if(fread(pref->buffer, len, 1, pf) <= 0) {
			ErrorLog("%s len too long(%d)\n", filename, len);
			goto mark_fail;
		}

		crc = CalculateCRC((unsigned char *)pref->buffer,len);
		if(head.crc != crc) {
			ErrorLog("%s crc erorr(0x%04X, should be 0x%04X)\n", filename, head.crc, crc);
			goto mark_fail;
		}

		pref->buffer[len] = pref->buffer[len+1] = 0;
		pref->pscan = pref->buffer;
		pref->len = len;
	}
	else {
		//printf("open %s\n", filename);
#ifndef USE_XINMUTEX
		pref->buffer = malloc(XIN_MAXSIZE+128);
		if(NULL == pref->buffer) {
			ErrorLog("malloc %d bytes fail\n", XIN_MAXSIZE+128);
			goto mark_fail;
		}
#endif

		pref->pscan = pref->buffer;
		pref->len = 0;
	}

	if(NULL != pf) fclose(pf);
	return (XINREF)pref;

mark_fail:
	if(NULL != pf) fclose(pf);

#ifndef USE_XINMUTEX
	if(NULL != pref->buffer) free(pref->buffer);
	free(pref);
#endif

	XIN_UNLOCK;
	return NULL;
}

/**
* @brief 关闭配置文件
* @param pf 文件索引指针
*/
void XinClose(XINREF pf)
{
	xinref_t *pref = (xinref_t *)pf;

	if(NULL==pf) {
		ErrorLog("null file\n");
		return;
	}

	if('r' == pref->openflag) {
		AssertLog(NULL==pref->buffer, "null pointer\n");
		goto mark_end;
	}
	else {
		xinhead_t head;
		FILE *pfile;

		AssertLog(NULL==pref->buffer, "null pointer\n");
		if(pref->len <= 0 || pref->len > XIN_MAXSIZE) {
			ErrorLog("invalid len(%d)\n", pref->len);
#ifndef USE_XINMUTEX
			free(pref->buffer);
			free(pref);
#endif
			XIN_UNLOCK;
			return;
		}

		head.magic = XIN_MAGIC;
		head.len = pref->len;
		head.crc = CalculateCRC((unsigned char *)pref->buffer, pref->len);

		remove(pref->filename);
		pfile = fopen(pref->filename, "wb");
		if(NULL == pfile) {
			ErrorLog("can not open %s for write\n", pref->filename);
			goto mark_end;
		}

		fwrite(&head, sizeof(head), 1, pfile);
		fwrite(pref->buffer, pref->len, 1, pfile);
		fclose(pfile);
	}

mark_end:
#ifndef USE_XINMUTEX
	free(pref->buffer);
	free(pref);
#endif
	XIN_UNLOCK;
}

/**
* @brief 查找一个变量
* @param pref 文件索引指针
* @param varname 变量名
* @return 成功变量值指针, 失败返回NULL
*/
static const char *FindVariable(xinref_t *pref, const char *varname)
{
	char *pstr, *pbak;
	int brescan;
	const char *pname;

	pbak = pref->pscan;
	brescan = 0;

	while(1) {
		if(!brescan) {
			if('\0' == pref->pscan[0]) {
				brescan = 1;
				pref->pscan = pref->buffer;
			}
		}
		else {
			if(pref->pscan >= pbak) return NULL;
		}

		pstr = pref->pscan;
		pname = varname;
		while('\0' != *pname) {
			if(*pstr++ != *pname++) break;
		}
		if(('\0' == *pname) && ('=' == *pstr)) { //found
			pstr++;
			pref->pscan += strlen(pref->pscan) + 1;
			return pstr;
		}

		if('\0' == *pstr) pref->pscan = pstr+1;
		else pref->pscan += strlen(pref->pscan) + 1;
	}
}

/**
* @brief 将一个字符串转换为整型值
* @param str 字符串指针
* @return 转换后的整型值
*/
static int XinStrToInt(const char *str)
{
	if('0' == str[0] && ('x' == str[1] || 'X' == str[1])) {
		int sum = 0;
		int num;

		str += 2;
		while(0 != *str) {
			if((*str >= '0') && (*str <= '9')) num =  *str - '0';
			else if((*str >= 'A') && (*str <= 'F')) num = *str - 'A' + 10;
			else if((*str >= 'a') && (*str <= 'f')) num = *str - 'a' + 10;
			else break;

			sum <<= 4;
			sum += num;

			str++;
		}

		return sum;
	}
	else return(atoi(str));
}


/**
* @brief 读取一个整数
* @param pf 文件索引指针
* @param varname 变量名
* @param defvalue 缺省值
* @return 成功返回变量值, 失败返回缺省值
*/
int XinReadInt(XINREF pf, const char *varname, int defvalue)
{
	xinref_t *pref = (xinref_t *)pf;
	const char *pvalue;

	AssertLogReturn(NULL==pf, defvalue, "null file\n");
	AssertLogReturn('r'!=pref->openflag, defvalue, "invalid operation\n");

	pvalue = FindVariable(pref, varname);
	if(NULL == pvalue) return defvalue;

	return XinStrToInt(pvalue);
}

/**
* @brief 读取一个字符串
* @param pf 文件索引指针
* @param varname 变量名
* @param buffer 缓存指针
* @param len 缓存长度
* @return 成功字符串长度, 失败返回-1
*/
int XinReadString(XINREF pf, const char *varname, char *buffer, int len)
{
	xinref_t *pref = (xinref_t *)pf;
	const char *pvalue;
	int copylen;

	AssertLog(len<=0, "invalid len(%d)\n", len);
	AssertLogReturn(NULL==pf, -1, "null file\n");
	AssertLogReturn('r'!=pref->openflag, -1, "invalid operation\n");

	pvalue = FindVariable(pref, varname);
	if(NULL == pvalue) return -1;

	*buffer = 0;
	copylen = 0;
	while(copylen < (len-1)) {
		if('\0' == *pvalue) break;

		*buffer++ = *pvalue++;
		copylen++;
	}

	*buffer = 0;
	return copylen;
}

/**
* @brief 将字符串转换为十六进制数组
* @param buffer 缓存指针
* @param len 缓存长度
* @return 拷贝长度
*/
static int XinStrToHex(const char *str, unsigned char *buffer, int len)
{
	int copylen = 0, skip = 0;
	unsigned char uc;

	while(0 != *str) {
		if((*str >= '0') && (*str <= '9')) uc = *str - '0';
		else if((*str >= 'A') && (*str <= 'F')) uc = *str - 'A' + 10;
		else if((*str >= 'a') && (*str <= 'f')) uc = *str - 'a' + 10;
		else break;

		if(skip) {
			*buffer |= uc;
			skip = 0;
			buffer++;
			copylen++;
			if(copylen >= len) break;
		}
		else {
			*buffer = uc << 4;
			skip = 1;
		}

		str++;
	}

	return copylen;
}


/**
* @brief 读取一串十六进制数组
* @param pf 文件索引指针
* @param varname 变量名
* @param buffer 缓存指针
* @param len 缓存长度
* @return 成功拷贝长度, 失败返回-1
*/
int XinReadHex(XINREF pf, const char *varname, unsigned char *buffer, int len)
{
	xinref_t *pref = (xinref_t *)pf;
	const char *pvalue;

	AssertLog(len<=0, "invalid len(%d)\n", len);
	AssertLogReturn(NULL==pf, -1, "null file\n");
	AssertLogReturn('r'!=pref->openflag, -1, "invalid operation\n");

	pvalue = FindVariable(pref, varname);
	if(NULL == pvalue) return -1;

	return XinStrToHex(pvalue, buffer, len);
}

#define ADD_POINTER(pref) { \
	int __add_len; \
	__add_len = strlen(pref->pscan); \
	if((pref->len+__add_len) >= XIN_MAXSIZE) return 1; \
	pref->len += __add_len + 1; \
	pref->pscan += __add_len + 1; \
}

/**
* @brief 写入一个整数
* @param pf 文件索引指针
* @param varname 变量名
* @param value 变量值
* @param hex 写入格式, 如果非零, 则打印成'0x...'形式
* @return 成功时返回0，否则返回非零值
*/
int XinWriteInt(XINREF pf, const char *varname, int value, int hex)
{
	xinref_t *pref = (xinref_t *)pf;

	AssertLogReturn(NULL==pf, 1, "null file\n");
	AssertLogReturn('w'!=pref->openflag, 1, "invalid operation\n");

	if(hex) sprintf(pref->pscan, "%s=0x%X", varname, (unsigned int)value);
	else sprintf(pref->pscan, "%s=%d", varname, value);

	ADD_POINTER(pref);
	return 0;
}

/**
* @brief 写入一个字符串
* @param pf 文件索引指针
* @param varname 变量名
* @param str 变量值
* @return 成功时返回0，否则返回非零值
*/
int XinWriteString(XINREF pf, const char *varname, const char *str)
{
	xinref_t *pref = (xinref_t *)pf;

	AssertLogReturn(NULL==pf, 1, "null file\n");
	AssertLogReturn('w'!=pref->openflag, 1, "invalid operation\n");

	sprintf(pref->pscan, "%s=%s", varname, str);

	ADD_POINTER(pref);
	return 0;
}

/**
* @brief 写入一串十六进制数组
* @param pf 文件索引指针
* @param varname 变量名
* @param str 变量值
* @return 成功时返回0，否则返回非零值
*/
int XinWriteHex(XINREF pf, const char *varname, const unsigned char *buffer, int len)
{
	xinref_t *pref = (xinref_t *)pf;
	int i, tmplen, scanlen;
	char *pscan;

	AssertLog(len<=0, "invalid len(%d)\n", len);
	AssertLogReturn(NULL==pf, 1, "null file\n");
	AssertLogReturn('w'!=pref->openflag, 1, "invalid operation\n");

	pscan = pref->pscan;
	scanlen = pref->len;
	sprintf(pscan, "%s=", varname);
	tmplen = strlen(pscan);
	if((scanlen+tmplen) >= XIN_MAXSIZE) return 1;
	pscan += tmplen;
	scanlen += tmplen;

	for(i=0; i<len; i++) {
		sprintf(pscan, "%02X", *buffer++);
		if((scanlen+2) >= XIN_MAXSIZE) return 1;
		pscan += 2;
		scanlen += 2;
	}

	pref->pscan = pscan + 1;
	pref->len = scanlen + 1;

	return 0;
}

