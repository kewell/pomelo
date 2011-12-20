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
    文件名      ：  c-tx-net-client_send.c
    描述        ：  本文件用于平台库net_client_send函数的全部用例测试
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include <string.h>
//平台库头文件
#include "../include/net.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

int main()
{
	int ret;
	u8 buf[10]="hello!";

	//环境的初始化
	ret = net_client_init(5);
	p_err(ret);
	ret = net_client_connect("192.168.2.100",3333);
	p_err(ret);

	//测试用例1,服务器端 接收字符串hello
	ret = net_client_send(buf,6);
	assert(ret == 6,"net_client_send Use Case 1 error");

//	//测试用例5-边界，服务器端 接收字符串hello
//	ret = net_client_send(buf,0xffff);
//	assert(ret == 0,"net_client_send Use Case 5 error");

	//测试用例2-错误
	ret = net_client_send(buf,0);
	assert(ret == -ERR_INVAL,"net_client_send Use Case 2 error");

	//测试用例4-错误
	ret = net_client_disconnect();
	p_err(ret);
	ret = net_client_send(buf,6);
	assert(ret == -ERR_TIMEOUT,"net_client_send Use Case 4 error");

	//测试用例3-错误，
	ret = net_client_init(5);
	p_err(ret);
	ret = net_client_connect("192.168.2.100",3333);
	p_err(ret);
	system("ifconfig eth0 down");		 //断开网络
	sleep(2);
	ret = net_client_send(buf,6);
	assert(ret == 6,"net_client_send Use Case 3 error");
	p_err(ret);

	p_err(ret);

	system("ifconfig eth0 up");			 //启用网络

	finaltest();
	exit(0);
}
