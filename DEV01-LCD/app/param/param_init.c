/**
* operation.c -- 参数模块初始化
* 
* 
* 创建时间: 2010-5-8
* 最后修改时间: 2010-5-8
*/

#include <stdio.h>

#include "include/debug.h"
#include "operation_inner.h"

extern int ParamSaveInit(void);
extern void MappingCenMetp(void);

/**
* @brief 参数模块初始化
* @return 0成功, 否则失败
*/
DECLARE_INIT_FUNC(ParamInit);
int ParamInit(void)
{
	printf("load param ...\n");

	LoadParaUni();
	LoadParaTerm();
	SetParaNetAddr();
	LoadParaMeter();
	LoadParaCenMetp();
	LoadParaPlcMetp();
	LoadParaChildEnd();
	LoadParaMix();
	LoadParaDataUse();
	LoadParaTask();
	LoadParaHardw();
	LoadParaRoute();
	MappingCenMetp();
	
	if(ParamSaveInit()) return 1;

	SET_INIT_FLAG(ParamInit);

	return 0;
}

