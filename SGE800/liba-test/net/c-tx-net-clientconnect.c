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
    文件名      ：  c-tx-net-clientconnect.c
    描述        ：  本文件用于平台库net_client_connect函数的全部用例测试
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

	//环境的初始化,测试前建立端口号为3333，4444，0，0xffff的服务器端
	inittest();
	ret = net_client_init(2);
	p_err(ret);

	//测试用例2-错误超时，5555端口无服务器端
	ret = net_client_connect("192.168.2.100",5555);
	assert(ret == -ERR_TIMEOUT,"net_client_connect Use Case 2 error");

	//测试用例1
	ret = net_client_connect("192.168.2.100",3333);
	assert(ret == 0,"net_client_connect Use Case 1 error");
	ret = net_client_disconnect();
	p_err(ret);

	//测试用例4-边界
	ret = net_client_init(2);
	p_err(ret);
	ret = net_client_connect("192.168.2.100",0);
	assert(ret == -ERR_TIMEOUT,"net_client_connect Use Case 4 error");

	//测试用例5-边界
	ret = net_client_connect("192.168.2.100",65535);
	assert(ret == 0,"net_client_connect Use Case 5 error");

	ret = net_client_disconnect();
	p_err(ret);

	//测试用例3-错误网络断开
	ret = net_client_init(2);
	p_err(ret);
	system("ifconfig eth0 down");
	ret = net_client_connect("192.168.2.100",4444);
	assert(ret == -ERR_DISCONNECT,"net_client_connect Use Case 3 error");
	system("ifconfig eth0 up");

	finaltest();
	exit(0);
}
