/**
* storage.c -- 系统储存模块初始化函数
* 
* 
* 创建时间: 2010-5-5
* 最后修改时间: 2010-5-5
*/

#include <stdio.h>

#include "include/debug.h"

extern int XinInit(void);
extern int FlatInit(void);

/**
* @brief 系统储存模块初始化函数
* @return 返回0表示成功, 否则失败
*/
DECLARE_INIT_FUNC(StorageInit);
int StorageInit(void)
{
	printf("  Storage init...\n");

	if(XinInit()) return 1;
	//if(FlatInit()) return 1;

	SET_INIT_FLAG(StorageInit);

	return 0;
}

