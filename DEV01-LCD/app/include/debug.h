/**
* debug.h -- 调试信息管理头文件
* 实现中...
* 
* 创建时间: 2009-4-3
* 最后修改时间: 2009-4-27
*/

#ifndef _DEBUG_H
#define _DEBUG_H

//配置
//#define OPEN_DEBUGPRINT
#define OPEN_ASSERTLOG

/************/

#define LOGTYPE_CLOSE			0  //关闭
#define LOGTYPE_ALARM			1  //告警
#define LOGTYPE_SHORT			2  //简短信息
//#define LOGTYPE_SHORT			0  //简短信息
#define LOGTYPE_DOWNLINK		3  //下行通信信息
#define LOGTYPE_UPLINK			4  //上行通信信息
#define LOGTYPE_DATA			5  //数据打印信息
#define LOGTYPE_DEBUG			6  //调试打印信息
#define LOGTYPE_TEST			10 //测试打印

#define LOGITF_STDIO			0   //标准输入输出
#define LOGITF_ETHER			1   //以太网
#define LOGITF_PIPE				2   //管道文件
#define LOGITF_SVRCOMM			3   //上行通信

//打印运行调试信息
void PrintLog(int type, const char *format, ...);
void PrintHexLog(int type, const unsigned char *buf, int len);

void SetLogType(int type);
int GetLogType(void);
int GetLogInterface(void);
void SetLogInterface(int itf);

//调试打印,正式运行版不出现
//#ifdef OPEN_DEBUGPRINT
//#ifdef 0
#define DebugPrint		PrintLog
//#else
//#define DebugPrint(a, ...)
//#endif

/****************/
//正式运行版本不出现
#ifdef OPEN_ASSERTLOG

//#define PATH_ASSERTLOG		"/tmp/"

//发生非正常错误时使用,将信息写入日志中
//void PrintErrorLog(const char *format, ...);
#define PrintErrorLog(format...)	PrintLog(0, format)

#define ErrorLog(format...) { \
	PrintErrorLog("\n%s line %d:", __FILE__, __LINE__); \
	PrintErrorLog(format); }

#define AssertLog(cond, format...) { \
	if(cond) { \
		PrintErrorLog("\n%s line %d:", __FILE__, __LINE__); \
		PrintErrorLog(format); \
	} \
}

#define AssertLogReturn(cond, rtn, format...) { \
	if(cond) { \
		PrintErrorLog("\n%s line %d:", __FILE__, __LINE__); \
		PrintErrorLog(format); \
		return rtn; } \
}

#define AssertLogReturnVoid(cond, format...) { \
	if(cond) { \
		PrintErrorLog("\n%s line %d:", __FILE__, __LINE__); \
		PrintErrorLog(format); \
		return; } \
}

#else /*OPEN_ASSERTLOG*/

#define ErrorLog(format...)
#define AssertLog(cond, format...)
#define AssertLogReturn(cond, rtn, format...) { if(cond) return rtn; }
#define AssertLogReturnVoid(cond, format...) { if(cond) return; }

#endif

//检测初始化函数有未执行
struct _init_flag_t {
	const char *name;
	int flag;
};

#define DECLARE_INIT_FUNC(func) \
	static struct _init_flag_t _initflag_##func \
	__attribute__((__used__)) \
	__attribute__((section("_init_flag"), unused)) \
	= {#func, 0}

#define SET_INIT_FLAG(func) _initflag_##func.flag = 1

void CheckInitFlag(void);

/**调试函数**/
#define SMALLCPY_INLINE		1
#ifdef SMALLCPY_INLINE
static inline void smallcpy(void *dest, const void *src, unsigned int len)
{
	unsigned int i;
	unsigned char *pdest = (unsigned char *)dest;
	const unsigned char *psrc = (const unsigned char *)src;

	for(i=0; i<len; i++) *pdest++ = *psrc++;
}
/*#define smallcpy(dest, src, len) ({ \
	unsigned int _tmp_ii; \
	unsigned char *_tmp_pdest = (unsigned char *)(dest); \
	const unsigned char *_tmp_psrc = (const unsigned char *)(src); \
	for(_tmp_ii=0; _tmp_ii<(len); _tmp_ii++) *_tmp_pdest++ = *_tmp_psrc++; })
*/
#else
void smallcpy(void *dest, const void *src, unsigned int len);
#endif

#endif /*_DEBUG_H*/

