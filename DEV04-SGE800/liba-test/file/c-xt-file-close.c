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
    文件名         ：  c-tx-file-close.c
    描述	       ：  本文件用于平台库file_close函数的测试
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
	int ret;
	//环境的初始化
	inittest();

	char *name = "file1";
	u8 pos,mode;
	int fd;
	//测试环境，打开数据库
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	fd = file_open(name, pos, mode);
	if(fd<=0){
		printf("file open error!\n");
		goto error;
	}
	/*********测试用例1**************/
	ret = file_close(fd);
	assert(ret==0,"test1:file close error!");

	/*********测试用例2**************/
	ret = file_close(-1);
	assert(ret==-ERR_INVAL,"test2:file close error!");

	/*********测试用例3**************/
	ret = file_close(100);
	assert(ret==-ERR_INVAL,"test3:file close error!");



	finaltest();
error:
	exit(0);
}
