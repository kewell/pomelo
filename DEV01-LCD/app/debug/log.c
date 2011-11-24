/**
* log.c 运行信息打印
* 
* 
* 创建时间: 2010-4-3
* 最后修改时间: 2010-4-27
*/

#include <stdio.h>
#include <stdarg.h>

#include "include/debug.h"
#include "include/debug/shellcmd.h"

#define DEF_LOGTYPE		0

static int LogType = DEF_LOGTYPE;
static int LogInterface = 0;

extern void EthShellPrint(const char *str);
extern char *GetEthShellBuffer(void);
extern void EthShellQuit(void);

extern void PipeShellPrint(const char *str);
extern char *GetPipeShellBuffer(void);
extern void PipeShellQuit(void);

extern void SvrShellPrint(const char *str);
extern char *GetSvrShellBuffer(void);
extern void SvrShellQuit(void);

/**
* @brief 设置当前日志打印类型
* @param type 打印类型
*/
void SetLogType(int type)
{
	LogType = type;
}

/**
* @brief 读取当前日志打印类型
* @return 打印类型
*/
int GetLogType(void)
{
	return LogType;
}

/**
* @brief 读取当前日志打印接口
* @return 打印接口
*/
int GetLogInterface(void)
{
	return LogInterface;
}

/**
* @brief 设置当前日志打印接口
* @param itf 打印接口
*/
void SetLogInterface(int itf)
{
	LogInterface = itf;
}

/**
* @brief 打印运行调试信息
* @param format 打印格式
*/
void PrintLog(int type, const char *format, ...)
{
	va_list va;
	char *pbuf;

	if(type > LogType) return;
	if(type > LOGTYPE_SHORT && type != LogType) return;

	switch(LogInterface) {
	case LOGITF_STDIO:
		va_start(va, format);
		vprintf(format, va);
		va_end(va);
		break;

	case LOGITF_ETHER:
		pbuf = GetEthShellBuffer();
		va_start(va, format);
		vsprintf(pbuf, format, va);
		va_end(va);
		EthShellPrint(pbuf);
		break;

	case LOGITF_PIPE:
		pbuf = GetPipeShellBuffer();
		va_start(va, format);
		vsprintf(pbuf, format, va);
		va_end(va);
		PipeShellPrint(pbuf);
		break;

	case LOGITF_SVRCOMM:
		pbuf = GetSvrShellBuffer();
		va_start(va, format);
		vsprintf(pbuf, format, va);
		va_end(va);
		SvrShellPrint(pbuf);
		break;

	default: break;
	}
}

static const char FormatHexChar[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7', 
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/**
* @brief 打印一串十六进制数组
* @param buf 缓存区指针
* @param len 缓存区长度
*/
void PrintHexLog(int type, const unsigned char *buf, int len)
{
	char str[128], *pstr;
	int i;
	unsigned char uc;

	if(LogType != type && 0 != type) return;

	pstr = str;
	for(i=1; i<=len; i++) 
	{
		uc = *buf++;
		*pstr++ = FormatHexChar[uc>>4];
		*pstr++ = FormatHexChar[uc&0x0f];
		*pstr++ = ' ';
		*pstr = 0;
		//sprintf(pstr, "%02X ", *buf++);
		if(0 == (i&0x1f)) 
		{
			PrintLog(type, "%s\n", str);
			pstr = str;
			*pstr = 0;
		}
		else if(0 == (i&0x07)) 
		{
			*pstr++ = ':';
			*pstr++ = ' ';
			*pstr = 0;
		}
	}

	if(0 != *str) PrintLog(type, "%s\n", str);
}

static int shell_exit(int argc, char *argv[])
{
	if(LOGITF_ETHER == LogInterface) EthShellQuit();
	else if(LOGITF_PIPE == LogInterface) PipeShellQuit();
	else if(LOGITF_SVRCOMM == LogInterface) SvrShellQuit();

	LogInterface = 0;
	LogType = DEF_LOGTYPE;

	return 0;
}
DECLARE_SHELL_CMD("exit", shell_exit, "退出命令行");

#ifndef SMALLCPY_INLINE
void smallcpy(void *dest, const void *src, unsigned int len)
{
	unsigned int i;
	unsigned char *pdest = (unsigned char *)dest;
	const unsigned char *psrc = (const unsigned char *)src;

	for(i=0; i<len; i++) *pdest++ = *psrc++;
}
#endif

