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
    文件名         ：  c-tx-msg-clear.c
    描述	       ：  本文件用于平台库msg_clear函数的测试
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
#include "../include/thread.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

msg_t message_send[3]= {{1,2,3,4}, {2,6,7,8}, {3,2,3,4}};
void thread_test1()
{
	int ref;
	u8 id;
	msg_t message_rev;
	id = 2;

	/*************测试用例1********************/
	u8 timeout = 5;
	system("date");
	ref = msg_recv(id, &message_rev, timeout);       //超时等待
	assert(ref==-ERR_TIMEOUT,"teset1:clear msg error!");
	system("date");

	/*************测试用例2********************/
	system("date");
	id = 0;
	ref = msg_recv(id, &message_rev, timeout);       //超时等待
	assert(ref==-ERR_TIMEOUT,"test2:clear msg error!");
	system("date");


	/*************测试用例3********************/
	system("date");
	id = 31;
	ref = msg_recv(id, &message_rev, timeout);       //超时等待
	assert(ref==-ERR_TIMEOUT,"test3:clear msg error!");
	system("date");
	while(1){

	}
}


int main()
{
	int ret,i;
	//环境的初始化
	inittest();
	u8 thread_id = 7;
	ret = thread_init();                //线程模块初始化
	if (ret){
		printf("init thread fail!\n");
		goto error;
	}
	//测试用例5
	u8 id1 = 2;
	ret = msg_clear(id1);
	assert(ret == -ERR_NOINIT,"test5:clear msg error!");
	ret = msg_init();                  //消息模块初始化
	if (ret){
		printf("msg init fail!\n");
		goto error;
	}
	id1 = 2;
	u8 prio = 5;
	for(i=0;i<3;i++){
		ret = msg_send(id1, &message_send[i], prio);   //发送消息
		if (ret){
			printf("test1:send fail!\n");
		}
		prio++;
	}

	id1 = 0;
	prio = 6;
	for(i=0;i<3;i++){
		ret = msg_send(id1, &message_send[i], prio);   //发送消息
		if (ret){
			printf("test1:send fail!\n");
		}
		prio++;
	}

	id1 = 31;
	prio = 6;
	for(i=0;i<3;i++){
		ret = msg_send(id1, &message_send[i], prio);   //发送消息
		if (ret){
			printf("test1:send fail!\n");
		}
		prio++;
	}
	//测试用例1
	id1 = 2;
	ret = msg_clear(id1);
	if (ret){
		assert(ret == 0,"test1:clear msg error!");
		goto error;
	}
	printf("msg clear!\n");

	//测试用例2
	id1 = 0;
	ret = msg_clear(id1);
	if (ret){
		assert(ret == 0,"test2:clear msg error!");
		goto error;
	}
	printf("msg clear!\n");

	//测试用例3
	id1 = 31;
	ret = msg_clear(id1);
	if (ret){
		assert(ret == 0,"test3:clear msg error!");
		goto error;
	}
	printf("msg clear!\n");

	ret = thread_create(thread_id, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);   //建立接收线程
	if (ret){
		printf("thread create fail!\n");
		goto error;
	}
	//测试用例4
	id1 = 32;
	ret = msg_clear(id1);
	assert(ret==-ERR_NODEV,"test4:clear msg error!\n");
	sleep(15);
	finaltest();
error:
	exit(0);
	}
