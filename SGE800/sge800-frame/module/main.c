/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：main.c
	描述		：本文件为工程入口，实现模块装配等过程
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.12
******************************************************************************/

//C库头文件
#include <unistd.h>

//业务平台头文件
#include "framework/base.h"
#include "framework/debug.h"
#include "framework/framework.h"

//业务模块头文件
#include "aaa.h"
#include "bbb.h"
#include "disp.h"
#include "device_test.h"

/*************************************************
  静态全局变量定义
*************************************************/
//定义对象
static struct aaaModule aaa;
static struct bbbModule bbb;
static struct dispModule disp;
static struct device_testModule device_test;
static struct BASE *objects[] = {(struct BASE *)&aaa, (struct BASE *)&bbb, (struct BASE *)&disp, (struct BASE *)&device_test};

/*************************************************
  主函数
*************************************************/
int main()
{
	int ret = 0;
	int i;

	PRINTF("BUILD:%s, %s\n\n", __DATE__, __TIME__);
	//对象类初始化,手工写也行，自动生成也行
	aaa.base.baseft = &aaa_ft;
	aaa.base.thread = 1;
	aaa.base.prio = 1;
	
	bbb.base.baseft = &bbb_ft;
	bbb.base.thread = 5;
	bbb.base.prio = 3;

	disp.base.baseft = &disp_ft;
	disp.base.thread = 6;
	disp.base.prio = 6;

	device_test.base.baseft = &device_test_ft;
	device_test.base.thread = 8;
	device_test.base.prio = 10;
	//初始化平台
	ret = framework_init();
	if (ret) {
		//错误处理
		goto error;
	}
	PRINTF("%s:%d\n\n", __FILE__, __LINE__);
	
	//初始化业务模块模型
	for (i = 0; i < sizeof(objects)/ sizeof(objects[0]); i++)
	{
		ret = objects[i]->baseft->initmodel(objects[i]);
		if (ret) {
			goto error;
		}
	}
	//初始化业务模块数据
	for (i = 0; i < sizeof(objects)/ sizeof(objects[0]); i++)
	{
		ret = objects[i]->baseft->initdata(objects[i]);
		if (ret) {
			goto error;
		}
	}
	
	//启动平台框架
	ret = framework_start();
	if (ret) {
		//错误处理
		goto error;
	}
	PRINTF("%s:%d\n\n", __FILE__, __LINE__);

	while(1) {
		sleep(1000);
	}

error:
	PRINTF("main err\n");
	return 0;
}
