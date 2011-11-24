/**
* sigin.c -- 输入信号驱动接口
* 
* 
* 创建时间: 2010-5-16
* 最后修改时间: 2010-5-16
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "include/debug.h"
#include "include/sys/sigin.h"

static int FidSigin = -1;

/**
* @brief 输入信号驱动接口初始化
* @return 0成功, 否则失败
*/
DECLARE_INIT_FUNC(SiginInit);
int SiginInit(void)
{
	//FidSigin = open("/dev/sigin", O_RDWR);
	FidSigin = open("/dev/keyboard", O_RDWR);
	if(-1 == FidSigin) 
	{
		printf("can not open sigin driver\n");
		return 1;
	}
	else
	{
		printf("open sigin driver succ\n");
	}

	SET_INIT_FLAG(SiginInit);

	return 0;
}

/**
* @brief 读取输入量状态
* @return 输入量状态,每BIT代表一个
*/
unsigned int SiginReadState(void)
{
	unsigned int state = 0;

	AssertLog(-1==FidSigin, "invalid fid(%d)\n", FidSigin);

	ioctl(FidSigin, 1, &state);

	return state;
}

/**
* @brief 读取输入按键
* @return 输入按键信息,返回0表示没有输入
*/
char SiginGetChar(void)
{
	char c = 0;
	int read_len = 0;

	AssertLog(-1==FidSigin, "invalid fid(%d)\n", FidSigin);

	read_len = read(FidSigin, &c, 1);
	//if(read(FidSigin, &c, 1) != 1) 
	if(read_len != 2)	
	{
		ErrorLog("read sigin fail\n");
		printf("c1 = %d\n",c);
	}

	//if(c != 0)
	//{
	//	printf("c2 = %d\n",c);
	//	printf("read_len = %d\n",read_len);
	//}
	
	//printf("key_num  = %d \n",c);
	return c;
}

