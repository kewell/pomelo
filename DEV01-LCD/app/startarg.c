/**
* startarg.c -- 启动参数
* 
* 
* 创建时间: 2010-6-8
* 最后修改时间: 2010-6-8
*/

#include <string.h>

struct start_arg {
	char str[16];
};
static struct start_arg StartArgs[52];

/**
* @brief 设置启动参数
* @param arg 参数内容
* @return 成功0, 失败1
*/
int SetStartArg(const char *arg)
{
	int i;

	i = strlen(arg) + 1;
	if(i > 16) return 1;

	if(*arg >= 'a' && *arg <= 'z') {
		i = *arg - 'a';
		strcpy(StartArgs[i].str, arg);
		return 0;
	}
	else if(*arg >= 'A' && *arg <= 'Z') {
		i = *arg - 'A' + 26;
		strcpy(StartArgs[i].str, arg);
		return 0;
	}

	return 1;
}

/**
* @brief 读取启动参数
* @param name 参数名称(a-z,A-Z)
* @param buf 读取缓存指针
* @param maxlen 读取缓存长度
* @return 成功0, 失败1
*/
int GetStartArg(char name, char *buf, int maxlen)
{
	int i, len;

	if(name >= 'a' && name <= 'z') i = name - 'a';
	else if(name >= 'A' && name <= 'Z') i = name - 'A' + 26;
	else return 1;

	if(StartArgs[i].str[0] != name) return 1;

	if(NULL == buf || 0 == maxlen) return 0;

	buf[0] = 0;
	if(0 == StartArgs[i].str[1]) return 0;

	len = strlen(StartArgs[i].str + 1) + 1;
	if(len > maxlen) return 1;

	strcpy(buf, StartArgs[i].str + 1);
	return 0;
}

