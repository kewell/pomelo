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
    文件名      ：  c-tx-net-serverlisten.c
    描述        ：  本文件用于平台库net_server_listen函数的全部用例测试
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
	u8 id;
	//环境的初始化
	ret = net_server_init(3333,2);
	p_err(ret);

	//测试用例1
	ret = net_server_listen(&id,NET_LISTEN);
	assert(ret == 0,"net_server_listen Use Case 1 error");

	//测试用例2
	ret = net_server_listen(&id,NET_LISTEN_SELECT);
	assert((ret == 0)&&(id == 0xff),"net_server_listen Use Case 2 error");

	//测试用例3-边界
	ret = net_server_listen(&id,2);
	assert(ret == -ERR_INVAL,"net_server_listen Use Case 3 error");

	//测试用例4-边界
	ret = net_server_listen(&id,0xff);
	assert(ret == -ERR_INVAL,"net_server_listen Use Case 4 error");

	finaltest();
	exit(0);
}
