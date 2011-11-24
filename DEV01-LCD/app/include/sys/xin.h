/**
* xin.c -- 类ini文本配置文件储存头文件
* 
* 
* 创建时间: 2010-4-22
* 最后修改时间: 2010-4-22
*/

#ifndef _XIN_H
#define _XIN_H

//文件索引指针
#define XINREF		char *

/**
* @brief 打开配置文件
* @param filename 打开文件名
* @param flag 打开方式. 'r':读取方式; 'w':写方式
* @return 成功时返回文件索引指针，否则为NULL(空指针)
*/
XINREF XinOpen(const char *filename, char flag);
/**
* @brief 关闭配置文件
* @param pf 文件索引指针
*/
void XinClose(XINREF pf);

/**
* @brief 读取一个整数
* @param pf 文件索引指针
* @param varname 变量名
* @param defvalue 缺省值
* @return 成功返回变量值, 失败返回缺省值
*/
int XinReadInt(XINREF pf, const char *varname, int defvalue);
/**
* @brief 读取一个字符串
* @param pf 文件索引指针
* @param varname 变量名
* @param buffer 缓存指针
* @param len 缓存长度
* @return 成功字符串长度, 失败返回-1
*/
int XinReadString(XINREF pf, const char *varname, char *buffer, int len);
/**
* @brief 读取一串十六进制数组
* @param pf 文件索引指针
* @param varname 变量名
* @param buffer 缓存指针
* @param len 缓存长度
* @return 成功拷贝长度, 失败返回-1
*/
int XinReadHex(XINREF pf, const char *varname, unsigned char *buffer, int len);

/**
* @brief 写入一个整数
* @param pf 文件索引指针
* @param varname 变量名
* @param value 变量值
* @param hex 写入格式, 如果非零, 则打印成'0x...'形式
* @return 成功时返回0，否则返回非零值
*/
int XinWriteInt(XINREF pf, const char *varname, int value, int hex);
/**
* @brief 写入一个字符串
* @param pf 文件索引指针
* @param varname 变量名
* @param str 变量值
* @return 成功时返回0，否则返回非零值
*/
int XinWriteString(XINREF pf, const char *varname, const char *str);
/**
* @brief 写入一串十六进制数组
* @param pf 文件索引指针
* @param varname 变量名
* @param str 变量值
* @return 成功时返回0，否则返回非零值
*/
int XinWriteHex(XINREF pf, const char *varname, const unsigned char *buffer, int len);

#endif /*_XIN_H*/

