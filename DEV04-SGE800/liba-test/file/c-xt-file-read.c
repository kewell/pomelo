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
    文件名         ：  c-tx-file-read.c
    描述	       ：  本文件用于平台库file_read函数的测试
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
#include "../include/file.h"
#include "../include/xj_assert.h"
#include "../include/error.h"
#include "../include/thread.h"

void thread_test1()
{
	int ret;
	char *name = "file1";
	u8 pos,mode;
	int fd;

	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	fd = file_open(name,pos,mode);
	if(fd<=0){
		printf("file opern error!\n");
	}
	char buf[10];
	memset(buf,0,10);

	ret = file_read(fd, buf, 10);
	assert((ret>0) && (strcmp(buf,"hello")==0),"file read error!");
	while(1){

	}

}

void thread_test2()
{
	int ret;
	char *name = "file1";
	u8 pos,mode;
	int fd;

	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	fd = file_open(name,pos,mode);
	if(fd<=0){
		printf("file opern error!\n");
	}
	char buf[10];
	memset(buf,0,10);

	ret = file_read(fd, buf, 10);
	assert((ret>0) && (strcmp(buf,"hello")==0),"file read error!");

	while(1){

	}
}

void thread_test3()
{
	int ret;
	char *name = "file1";
	u8 pos,mode;
	int fd;

	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	fd = file_open(name,pos,mode);
	if(fd<=0){
		printf("file opern error!\n");
	}
	char buf[10];
	memset(buf,0,10);
    ret = file_read(fd, buf, 10);
	assert((ret>0) && (strcmp(buf,"hello")==0),"file read error!");

	while(1){

	}
}

int main()
{
	int ret;
	//环境的初始化
	inittest();
	system("rm /var/fi*");
	char *name = "file1";
	u8 pos,mode;
	int fd;

	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	fd = file_open(name,pos,mode);
	if(fd<=0){
		printf("file opern error!\n");
		goto error;
	}
	//测试环境要求存在file1文件(可读写)，内容为：hello
	char *buf_write = "hello";
	ret = file_write(fd, buf_write, 5);
	if(ret<=0){
		printf("write error!\n");
		goto error;
	}
	ret = file_seek(fd, 0, FILE_SEEK_SET);
	if(ret<0){
		printf("seek error!\n");
		goto error;
	}
	/*********测试用例1**************/
	char buf[10];
	memset(buf,0,10);
	ret = file_read(fd, buf, 10);
	assert((ret>0) && (strcmp(buf,"hello")==0),"test1:file read error!");

	/*********测试用例2**************/
	ret = file_read(fd, buf, 4097);
	assert(ret==-ERR_INVAL, "test2:board test fail");

	/*********测试用例3**************/
	ret = file_read(-2, buf, 5);
	assert(ret==-ERR_INVAL, "test3:board test fail");

	/*********测试用例4**************/
	ret = file_read(fd, buf, 0);
	assert(ret==-ERR_INVAL, "test4:error test fail");

	//性能测试--多线程同时读取文件
	ret = thread_init();                //线程模块初始化
	if (ret){
		printf("init thread fail!\n");
		goto error;
	}
	u8 id = 3;
	ret = thread_create(id, (void *)thread_test1, NULL, THREAD_MODE_NORMAL, 0);
	if (ret){
		printf("thread create fail!\n");
		goto error;
	}
	id = 5;
	ret = thread_create(id, (void *)thread_test2, NULL, THREAD_MODE_NORMAL, 0);
	if (ret){
		printf("thread create fail!\n");
		goto error;
	}
	id = 7;
	ret = thread_create(id, (void *)thread_test3, NULL, THREAD_MODE_NORMAL, 0);
	if (ret){
		printf("thread create fail!\n");
		goto error;
	}
	sleep(20);

	finaltest();
error:
	exit(0);
}
