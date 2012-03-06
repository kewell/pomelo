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
    文件名         ：  c-tx-file-seek.c
    描述	       ：  本文件用于平台库file_seek函数的测试
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

int main()
{
	int ret,ref;
	//环境的初始化
	inittest();

	system("rm /var/fi*");
	char *name = "file1";
	u8 pos,mode;
	int fd;
	//测试用例1环境要求存在file1文件(可读写)，内容为：hello
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	fd = file_open(name,pos,mode);
	if(fd<=0){
		printf("file open error!\n");
		goto error;
	}
	char *buf_write = "hello";
	ret = file_write(fd, buf_write, 5);
	if(ret<=0){
		printf("write error!\n");
		goto error;
	}

	/*********测试用例1**************/
	ret = file_seek(fd, 1, FILE_SEEK_SET);
	if (ret<0){
		assert(ret==1,"test1:file seek error!");
	}else{
		char buf[10];
		memset(buf,0,10);
		ref = file_read(fd, buf, 10);
		if(ref<=0){
			assert(ret>0,"test1:file seek point error!");
			goto error;
		}
		assert((ret==1) && (strcmp(buf,"ello")==0),"test1:file seek error!");
	}

	/*********测试用例2**************/
	ret = file_seek(fd, 1, 5);
	assert(ret==-ERR_INVAL,"test2:file seek error!");

	/*********测试用例3**************/
	ret = file_seek(-1, 1, FILE_SEEK_SET);
	assert(ret==-ERR_INVAL,"test3:file seek error!");

	finaltest();
error:
	exit(0);
}
