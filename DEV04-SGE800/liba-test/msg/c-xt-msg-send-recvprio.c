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
    文件名         ：  c-tx-msg-send-recvprio.c
    描述	       ：  本文件用于平台库msg_send函数和msg_recvprio函数的测试
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
#include "../include/thread.h"
#include "../include/error.h"
#include "../include/msg.h"
#include "../include/xj_assert.h"

msg_t message_send[3]= {{1,2,3,4}, {2,6,7,8}, {3,2,3,4}};
u8 message = 0;
void thread_test1()
{
	int ref;
	u8 id,i;
	msg_t message_rev;

	u8 result;
	printf("rev test thread1 start!\n");
	/*************正常功能测试用例1********************/
	id = 3;
	for(i=3;i>0;i--){
		memset(&message_rev, 0 ,sizeof(msg_t));
		ref = msg_recv_prio(id, &message_rev, 0);              //永久等待
		if ((message_rev.type==message_send[i-1].type) && (message_rev.bPara==message_send[i-1].bPara)
			 &&(message_rev.lPara==message_send[i-1].lPara) && (message_rev.wPara==message_send[i-1].wPara)
			 && (ref==0)){
			result = 1;
		}else{
			result = 0;
			i = 0;
		}
	}
	assert(result == 1,"test1:send-revpiro msg fail!");

	/*************测试用例7********************/
	id = 31;
	memset(&message_rev, 0 ,sizeof(msg_t));
	ref = msg_recv_prio(id, &message_rev, 0);
	assert((message_rev.type==message_send[1].type) && (message_rev.bPara==message_send[1].bPara)
			&&(message_rev.lPara==message_send[1].lPara) && (message_rev.wPara==message_send[1].wPara)
			&& (ref==0), "test7：send-rev msg fail!");

	/*************测试用例8********************/
	id = 0;
	memset(&message_rev, 0 ,sizeof(msg_t));
	ref = msg_recv_prio(id, &message_rev, 0);
	assert((message_rev.type==message_send[1].type) && (message_rev.bPara==message_send[1].bPara)
			&&(message_rev.lPara==message_send[1].lPara) && (message_rev.wPara==message_send[1].wPara)
			&& (ref==0), "test8:send-rev msg fail!");

	/*************测试用例9********************/
	id = 7;
	memset(&message_rev, 0 ,sizeof(msg_t));
	ref = msg_recv_prio(id, &message_rev, 0);
	assert((message_rev.type==message_send[1].type) && (message_rev.bPara==message_send[1].bPara)
			&&(message_rev.lPara==message_send[1].lPara) && (message_rev.wPara==message_send[1].wPara)
			&& (ref==0), "test8：send-rev msg fail!");

	/*************测试用例10********************/
	id = 32;  //越限
	memset(&message_rev, 0 ,sizeof(msg_t));
	ref = msg_recv_prio(id, &message_rev, 0);
	assert(ref==-ERR_NODEV,"test10:send-rev msg fail!");

	while(1){
	}

}
void thread_test2()
{
	int ref;
	u8 id,i,flag;
	u8 result = 0;
	msg_t message_rev;

	printf("rev test thread2 start!\n");
	/*************正常功能测试用例2********************/
	id = 4;
	result = 0;
	for(i=3;i>0;i--){
		flag = 1;
		while(flag){   //循环接收
			memset(&message_rev, 0 ,sizeof(msg_t));
			ref = msg_recv_prio(id, &message_rev, MSG_RECV_NONBLOCK);      //不等待
			if (ref == -ERR_TIMEOUT){
				flag = 1;
				sleep(1);
			}else if(ref<0){
				flag = 0;
				assert(ref==0,"test2:send-rev msg fail!");
				i = 1;
				printf("a\n");
			}else{
				flag = 0;   //停止循环接收
				if ((message_rev.type==message_send[i-1].type) && (message_rev.bPara==message_send[i-1].bPara)
					 &&(message_rev.lPara==message_send[i-1].lPara) && (message_rev.wPara==message_send[i-1].wPara)
					 && (ref==0)){
					result = 1;
				}else{
					result = 0;
					i = 1;
				}
			}
		}
	}
	assert(result == 1,"test2:send-revprio msg fail!");
	while(1){

	}
}

void thread_test3()
{
	int ref;
	u8 id,i,flag,prio;
	u8 result = 0;
	msg_t message_rev;

	id = 5;
	u8 timeout = 10;
	printf("rev test thread3 start!\n");

	/*************正常功能测试用例2********************/
	for(i=3;i>0;i--){
		flag = 1;
		while(flag){   //循环接收
			memset(&message_rev, 0 ,sizeof(msg_t));
			ref = msg_recv_prio(id, &message_rev, timeout);       //超时等待
			if (ref == -ERR_TIMEOUT){
				flag = 1;
				sleep(1);
			}else if(ref<0){
				flag = 0;
				assert(ref==0,"test3:send-nonblockrev msg fail!");
				i=1;
			}else{
				flag = 0;   //停止循环接收
				if ((message_rev.type==message_send[i-1].type) && (message_rev.bPara==message_send[i-1].bPara)
					 &&(message_rev.lPara==message_send[i-1].lPara) && (message_rev.wPara==message_send[i-1].wPara)
					 && (ref==0)){
					result = 1;
				}else{
					result = 0;
					i = 1;
					printf("ff\n");
				}
			}
		}
	}
	assert(result == 1,"test3:send-revprio msg fail!");

	//测试用例4性能测试
	sleep(5);
	printf("rev\n");
	id = 11;
	prio = 8;
	for(i=1;i<5;i++){
		memset(&message_rev, 0, sizeof(message_rev));
		ref = msg_recv_prio(id, &message_rev, 0);                          //永久等待
		if (message_rev.type == prio){
			flag = 1;
			prio--;
		}else{
			flag = 0;
		}
		printf("prio_rev = %d\n",message_rev.type);
	}
	assert(flag == 1,"test4:send-rev fail!");
	while(1){

	}
}

int main(void)
{
	int ref;
	u8 id,i;
	msg_t message_rev;
	msg_t message_s={1,2,3,4};

	ref = thread_init();                //模块初始化
	if (ref){
		printf("init thread fail!\n");
		goto error;
	}
	/*********测试用例11*******************/
	id = 2;
	ref = msg_recv_prio(id, &message_rev, 0);
	assert(ref==-ERR_NOINIT,"test11: send-revprio fail!");  //未初始化错误

	ref = msg_init();            		//初始化消息模块
	if (ref){
		printf("init msg fail!\n");
		goto error;
	}
	/*********测试用例1********************/
	u8 prio = 5;
	id = 3;
	for(i=3;i<6;i++){
		ref = msg_send(id, &message_send[i-3], prio);   //发送消息
		if (ref){
			printf("test1:send fail!\n");
		}
		prio++;
	}
	u8 id_test;
	id_test = 6;
	ref = thread_create(id_test, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);   //建立接收线程
	if (ref){
		printf("test1:thread create fail!\n");
		goto error;
	}
	/*********测试用例2********************/
	prio = 5;
	id = 4;
	for(i=0;i<3;i++){
		ref = msg_send(id, &message_send[i], prio);   //发送消息
		if (ref){
			printf("test2:send fail!\n");
		}
		prio++;
	}
	id_test = 7;
	ref = thread_create(id_test, (void *)thread_test2, NULL, THREAD_MODE_NORMAL, 0);   //建立接收线程
	if (ref){
		printf("test2:thread create fail!\n");
		goto error;
	}
	/*********测试用例3********************/
	prio = 5;
	id = 5;
	for(i=0;i<3;i++){
		ref = msg_send(id, &message_send[i], prio);   //发送消息
		if (ref){
			printf("test3:send fail!\n");
			goto error;
		}
		prio++;
	}

	id_test = 8;
	ref = thread_create(id_test, (void *)thread_test3, NULL, THREAD_MODE_NORMAL, 0);   //建立接收线程
	if (ref){
		printf("test3:thread create fail!\n");
		goto error;
	}
	//测试用例4，5，6性能测试

	/*******测试用例7************************/
	prio = 5;
	id = 31;
	for(i=3;i<5;i++){
		ref = msg_send(id, &message_send[i-3], prio);   //发送消息
		if (ref){
			printf("test7:send fail!\n");
			goto error;
		}
		prio+=5;
	}
	/*******测试用例8************************/
	prio = 5;
	id = 0;
	for(i=3;i<5;i++){
		ref = msg_send(id, &message_send[i-3], prio);   //发送消息
		if (ref){
			printf("test8:send fail!\n");
			goto error;
		}
		prio+=5;
	}
	/*******测试用例9************************/
	prio = 0;
	id = 7;
	for(i=3;i<5;i++){
		ref = msg_send(id, &message_send[i-3], prio);   //发送消息
		if (ref){
			printf("test9:send fail!\n");
			goto error;
		}
		prio+=99;
	}

/*************测试用例4（性能测试）***************/

	id = 11;
	prio = 5;
	message_s.type = prio;
	ref = msg_send(id, &message_s, prio);      //发送消息
	if (ref){
		printf("test4:send msg fail!\n");
	}
	prio = 7;
	message_s.type = prio;
	ref = msg_send(id, &message_s, prio);      //发送消息
	if (ref){
		printf("test4:send msg fail!\n");
	}
	prio = 6;
	message_s.type = prio;
	ref = msg_send(id, &message_s, prio);      //发送消息
	if (ref){
		printf("test4:send msg fail!\n");
	}
	prio = 8;
	message_s.type = prio;
	ref = msg_send(id, &message_s, prio);      //发送消息
	if (ref){
		printf("test4:send msg fail!\n");
	}
	printf("send ok!\n");

	//测试用例5性能测试
	id = 6;
	for(i=1;i<65;i++){
		ref = msg_send(id, &message_send[1], 5);      //发送消息--使消息队列满
		if (ref){
			printf("test5:send msg fail!\n");
		}
	}
	ref = msg_send(id, &message_send[1], 5);
	assert(ref==-ERR_NOMEM,"test5:send msg fail!");

	//测试用例6性能测试
	id = 28;
	u8 timeout = 3;
	ref = msg_recv(id, &message_rev, timeout);       //超时等待
	assert(ref== -ERR_TIMEOUT,"test6:send-recv fail!");

	sleep(15);
	finaltest();
error:
	exit(0);

}
