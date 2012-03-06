/*****************************************************************************/
/*许继电气股份有限公司                                     版权：2008-2015   */
/*****************************************************************************/
/* 本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许    */
/* 可不得擅自修改或发布，否则将追究相关的法律责任。                          */
/*                                                                           */
/*                      河南许昌许继股份有限公司                             */
/*                      www.xjgc.com                                         */
/*                      (0374) 321 2924                                      */
/*                                                                           */
/*****************************************************************************/


/******************************************************************************
    项目名称    ：  SGE800计量智能终端平台
    文件名      ：  c-tx-net-serverinit.c
    描述        ：  本文件用于平台库net_server_init函数的全部用例测试
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

//平台库头文件
#include "../include/net.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

int main()
{
	int ret;
	//环境的初始化

	//测试用例1
	ret = net_server_init(3333,2);
	assert(ret == 0,"net_server_init Use Case 1 error");

	//测试用例2-错误
	ret = net_server_init(0,2);
	assert(ret == 0,"net_server_init Use Case 2 error");


	//测试用例3-错误
	ret = net_server_init(23,2);
	assert(ret == -ERR_INVAL,"net_server_init Use Case 3 error");

	//测试用例4-边界
	ret = net_server_init(65535,2);
	assert(ret == 0,"net_server_init Use Case 4 error");

	//测试用例5-边界
	ret = net_server_init(4444,65535);
	assert(ret == 0,"net_server_init Use Case 5 error");

	//测试用例5-边界
	ret = net_server_init(5555,0);
	assert(ret == 0,"net_server_init Use Case 5 error");

	finaltest();
	exit(0);
}
