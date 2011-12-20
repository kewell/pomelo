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
    文件名         ：  c-tx-msg-getsize.c
    描述	       ：  本文件用于平台库msg_getsize函数的测试
    版本              ：  0.1
    作者              ：  孙锐
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>			//exit
#include <unistd.h>			//sleep
#include <db.h>

//平台库头文件
#include "../include/msg.h"
#include "../include/xj_assert.h"
#include "../include/error.h"
#include "../include/thread.h"

msg_t message_send[3]= {{1,2,3,4}, {2,6,7,8}, {3,2,3,4}};

int main()
{
	int ret,i;
	//环境的初始化
	inittest();
	ret = thread_init();                //线程模块初始化
	if (ret){
		printf("init thread fail!\n");
		goto error;
	}

	//测试用例6
	u8 id = 2;
	ret = msg_getsize(id);
	assert(ret == -ERR_NOINIT,"test1:msg getsize error!");

	ret = msg_init();                  //消息模块初始化
	if (ret){
		printf("msg init fail!\n");
		goto error;
	}
	//正常测试环境
	id = 2;
	u8 prio = 5;
	for(i=0;i<2;i++){
		ret = msg_send(id, &message_send[i], prio);   //发送消息
		if (ret){
			printf("test3:send fail!\n");
			goto error;
		}
		prio++;
	}

	//id = 31(边界测试)
	prio = 5;
	id = 31;
	for(i=0;i<1;i++){
		ret = msg_send(id, &message_send[i], prio);   //发送消息
		if (ret){
			printf("test4:send fail!\n");
			goto error;
		}
		prio++;
	}
	//id = 0(边界测试)
	prio = 5;
	id = 0;
	for(i=0;i<1;i++){
		ret = msg_send(id, &message_send[i], prio);   //发送消息
		if (ret){
			printf("test3:send fail!\n");
			goto error;
		}
		prio++;
	}
	//测试用例1
	id = 2;
	ret = msg_getsize(id);
	assert(ret == 2,"test1:msg getsize error!");

	//测试用例2
	ret = msg_clear(id);
	if(ret){
		printf("test2:clear msg error!");
		goto error;
	}
	ret = msg_getsize(id);
	assert(ret == 0,"test2:msg getsize error!");

	//测试用例3
	id = 0;
	ret = msg_getsize(id);
	assert(ret == 1,"test3:msg getsize error!");

	//测试用例4
	id = 31;
	ret = msg_getsize(id);
	assert(ret == 1,"test4:msg getsize error!");

	//测试用例5
	id = 32;
	ret = msg_getsize(id);
	assert(ret == -ERR_NODEV,"test5:msg getsize error!");

	finaltest();
error:
	exit(0);
}
