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
    文件名      ：  c-tx-net-clientreceive.c
    描述        ：  本文件用于平台库net_client_receive函数的全部用例测试
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
	u8 buf[10];
	u16 len;

	//环境的初始化，需要服务器端的配合
	ret = net_client_init(8);
	p_err(ret);
	ret = net_client_connect("192.168.2.100",3333);
	p_err(ret);

	//测试用例1,服务器端发送字符串hello
	ret = net_client_receive(buf,255,&len,NET_BLOCK);
	buf[len]='\0';
	assert((ret == 0)&&(strcmp(buf,"hello")==0),"net_client_receive Use Case 1 error");

	//测试用例2-错误
	ret = net_client_receive(buf,255,&len,NET_NONBLOCK);
	assert(ret == -ERR_TIMEOUT,"net_client_receive Use Case 2 error");

	//测试用例3-错误
	ret = net_client_receive(buf,255,&len,2);
	assert(ret == -ERR_INVAL,"net_client_receive Use Case 3 error");

	//测试用例4-覆盖，服务器端发送大于8字节数据
	printf("Use Case 4 start, please send >8 Byte data\n");
	ret = net_client_receive(buf,8,&len,NET_BLOCK);
	assert((ret == 0)&&(len == 8),"net_client_receive Use Case 4 error");

	finaltest();
	exit(0);
}
