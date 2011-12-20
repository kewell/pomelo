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
    文件名      ：  c-tx-net-serversend.c
    描述        ：  本文件用于平台库net_server_send函数的全部用例测试
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
	u8 id;
	unsigned char buf[10]="hello";
	//环境的初始化
	ret = net_server_init(3333,5);
	p_err(ret);
	ret = net_server_listen(&id,NET_LISTEN);
	p_err(ret);
	ret = net_server_connect(&id);
	p_err(ret);

	//测试用例1
	ret = net_server_send(id,buf,5);
	assert(ret == 5,"net_server_send Use Case 1 error");

	//测试用例2-错误
	ret = net_server_send(8,buf,5);
	assert(ret == -ERR_INVAL,"net_server_send Use Case 2 error");

	//测试用例3-错误
	ret = net_server_send(id,buf,0);
	assert(ret == -ERR_INVAL,"net_server_send Use Case 3 error");

	//测试用例4-错误
	system("ifconfig eth0 down");
	ret = net_server_send(id,buf,6);
	assert(ret == -ERR_DISCONNECT,"net_server_send Use Case 4 error");
	system("ifconfig eth0 up");

	finaltest();
	exit(0);
}
