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
    文件名         ：  c-tx-msg-send-recv.c
    描述	       ：  本文件用于平台库msg_send函数和msg_recv函数的测试
    版本              ：  0.1
    作者              ：  孙锐
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>			//exit
#include <unistd.h>			//sleep
#include <db.h>
#include <string.h>

//平台库头文件
#include "../include/msg.h"
#include "../include/thread.h"
#include "../include/error.h"
#include "../include/xj_assert.h"

msg_t message_send1 = {1,2,3,4};
msg_t message_send2 = {2,6,7,8};
msg_t message_send3 = {3,2,3,4};
u8 message = 0;

void thread_test1()
{
	int ref,i;
	u8 id;
	u8 flag;
	msg_t message_rev;
	msg_t message_send;
	id = 2;
	//printf("block rev start!\n");
	ref = msg_recv(id, &message_rev, 0);                          //永久等待
	/*************测试用例1********************/
	assert((message_rev.type==message_send1.type) && (message_rev.bPara==message_send1.bPara)
		 &&(message_rev.lPara==message_send1.lPara) && (message_rev.wPara==message_send1.wPara)
		 && (ref==0), "test1:send-blockrev msg fail!");

	/*************测试用例2********************/
	memset(&message_rev, 0, sizeof(msg_t));
	flag = 1;
	while(flag){   //循环接收
		ref = msg_recv(id, &message_rev, MSG_RECV_NONBLOCK);      //不等待
		if (ref == -ERR_TIMEOUT){
			flag = 1;
			sleep(1);
		}else if(ref<0){
			flag = 0;
			assert(ref==0,"test2:send-nonblockrev msg fail!");
		}else{
			flag = 0;   //停止循环接收
			assert((message_rev.type==message_send2.type) && (message_rev.bPara==message_send2.bPara)
					&&(message_rev.lPara==message_send2.lPara) && (message_rev.wPara==message_send2.wPara)
					&& (ref==0), "test2：send-nonblockrev msg fail!");
		}
	}
	/*************测试用例3********************/
	u8 timeout = 10;
	flag = 1;
	while(flag){
		system("date");
		ref = msg_recv(id, &message_rev, timeout);       //超时等待
		if (ref == -ERR_TIMEOUT){
			//printf("test3:timeout\n");
			system("date");
			flag = 1;
			sleep(1);
		}else if(ref<0){
			flag = 0;
			assert(ref==0,"test3：send-timeoutblockrev msg fail!");
		}else{
			flag = 0;
			assert((message_rev.type==message_send3.type) && (message_rev.bPara==message_send3.bPara)
					&&(message_rev.lPara==message_send3.lPara) && (message_rev.wPara==message_send3.wPara)
					&& (ref==0), "test3：send-timeoutblockrev msg fail!");
			system("date");
		}
	}
	/*************测试用例7********************/
	id = 31;
	ref = msg_recv(id, &message_rev, 0);
	assert((message_rev.type==message_send3.type) && (message_rev.bPara==message_send3.bPara)
			&&(message_rev.lPara==message_send3.lPara) && (message_rev.wPara==message_send3.wPara)
			&& (ref==0), "test7：send-rev msg fail!");

	/*************测试用例8********************/
	id = 0;
	ref = msg_recv(id, &message_rev, 0);
	assert((message_rev.type==message_send2.type) && (message_rev.bPara==message_send2.bPara)
			&&(message_rev.lPara==message_send2.lPara) && (message_rev.wPara==message_send2.wPara)
			&& (ref==0), "test8：send-nonblockrev msg fail!");

	/*************测试用例4（性能测试）***************/
	id = 5;
	for (i = 1; i<10; i++){
		message++;
		message_send.type = message;
		ref = msg_send(id, &message_send, 5);      //发送消息
		if (ref){
			printf("test4:send msg fail!\n");
		}
	}

	sleep(5);

}

void thread_test2()
{
	int ret,i;
	u8 id;
	msg_t message_send;
	/*************测试用例4（性能测试）***************/
	id = 5;
	for (i = 1; i<10; i++){
		message++;
		message_send.type = message;
		ret = msg_send(id, &message_send, 5);      //发送消息
		if (ret){
			printf("test2:send msg2 fail!\n");
		}
		sleep(1);
	}
	while(1){

	}

}

void thread_test3()
{
	int ret,i;
	u8 id;
	msg_t message_send;

	/*************测试用例4（性能测试）***************/
	id = 5;
	for (i = 1; i<10; i++){
		message++;
		message_send.type = message;
		ret = msg_send(id, &message_send, 5);      //发送消息
		if (ret){
			printf("test2:send msg2 fail!\n");
		}
		sleep(1);
	}

	while(1){

	}

}

int main()
{
	int ret,ref,i;
	msg_t message_rev;
	//环境的初始化
	inittest();
	u8 thread_id1 = 7;
	ret = thread_init();                //线程模块初始化
	if (ret){
		printf("init thread fail!\n");
		goto error;
	}
	ret = msg_init();                  //消息模块初始化
	if (ret){
		printf("msg init fail!\n");
		goto error;
	}
	u8 id1 = 2;
	ret = thread_create(thread_id1, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);   //建立接收线程
	if (ret){
		printf("thread create fail!\n");
		goto error;
	}
	ret = msg_send(id1, &message_send1, 5);      //发送第一条消息
	if (ret){
		assert(ret==0,"test1:send msg1 fail!");   //用例1
		goto error;
	}
	sleep(1);
	ret = msg_send(id1, &message_send2, 5);      //发送第二条消息
	if (ret){
		assert(ret==0,"test2:send msg2 fail!");   //用例2
		goto error;
	}
	sleep(1);
	ret = msg_send(id1, &message_send3, 5);      //发送第三条消息
	if (ret){
		assert(ret==0,"test3:send msg3 fail!");   //用例3
		goto error;
	}


	/*******测试用例7************************/
	id1 = 31;
	ret = msg_send(id1, &message_send3, 3);
	if (ret){
		assert(ret==0,"test7:send msg3 fail!");
		goto error;
	}
	/*******测试用例8************************/
	id1 = 0;
	ret = msg_send(id1, &message_send2, 8);
	if (ret){
		assert(ret==0,"test8:send msg2 fail!");
		goto error;
	}
	/*******测试用例9************************/
	id1 = 32;
	ret = msg_send(id1, &message_send3, 3);
	ref = msg_recv(id1, &message_rev, 0);
	assert((ret==-ERR_NODEV)&&(ref==-ERR_NODEV),"test9:send-rev fail!");

	/*******测试用例10************************/
	id1 = 5;
	ret = msg_send(id1, &message_send2, 100);
	assert(ret==-ERR_INVAL,"test10:send-rev fail!");


	thread_id1 = 5;
	ret = thread_create(thread_id1, (void *)thread_test2, NULL, THREAD_MODE_NORMAL, 0);
	if (ret){
		printf("thread create fail!\n");
		goto error;
	}

	thread_id1 = 6;
	ret = thread_create(thread_id1, (void *)thread_test3, NULL, THREAD_MODE_NORMAL, 0);
	if (ret){
		printf("thread create fail!\n");
		goto error;
	}

	//测试用例4性能测试
	id1 = 5;
	u8 flag;
	for(i=1;i<28;i++){
		ref = msg_recv(id1, &message_rev, 0);                          //永久等待
		if (message_rev.type == i){
			flag = 1;
		}else{
			flag = 0;
			i = 28;
		}
	}
	assert(flag == 1,"test4:send-rev fail!");

	//测试用例5性能测试
	id1 = 6;
	for(i=1;i<65;i++){
		ret = msg_send(id1, &message_send1, 5);      //发送消息--使消息队列满
		if (ret){
			printf("test5:send msg fail!\n");
		}
	}
	ret = msg_send(id1, &message_send1, 5);
	assert(ret==-ERR_NOMEM,"test5:send msg fail!");

	id1 = 28;
	u8 timeout = 3;
	ret = msg_recv(id1, &message_rev, timeout);       //超时等待
	assert(ret== -ERR_TIMEOUT,"test6:send-recv fail!");



	sleep(5);
	finaltest();
error:
	exit(0);
	}
