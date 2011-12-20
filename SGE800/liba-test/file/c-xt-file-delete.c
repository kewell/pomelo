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
    文件名         ：  c-tx-file-delete.c
    描述	       ：  本文件用于平台库file_delete函数的测试
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
	//测试环境，打开数据库----并且建立测试文件运行一下x-xt-file-open
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	fd = file_open(name, pos, mode);
	if(fd<=0){
		printf("file open error!\n");
		goto error;
	}
	ret = file_close(fd);
	if (ret){
		printf("file close error!\n");
		goto error;
	}
	/*********测试用例1**************/
	pos = FILE_POS_RAM;
	ret = file_delete(name, pos);
	assert(ret==0,"test1:file delete error!");

	/*********测试用例2**************/
	pos = FILE_POS_RAM;
	ret = file_delete(name, -1);
	assert(ret==-ERR_INVAL,"test2:file delete error!");

	/*********测试用例3**************/
	pos = FILE_POS_RAM;
	ret = file_delete(name, 5);
	assert(ret==-ERR_INVAL,"test3:file delete error!");

	/*********测试用例4**************/
	pos = FILE_POS_RAM;
	ret = file_delete("file2", pos);
	assert(ret==-ERR_NOFILE,"test4:file delete error!");

	/*********测试用例5**************/
	pos = FILE_POS_RAM;
	ret = file_delete("file.c", pos);
	assert(ret==-ERR_INVAL,"test5:file delete error!");

	/*********测试用例6**************/
	char file_name[10]="";
	memset(file_name, 0, 10);
	strcat (file_name, "file");
	pos = FILE_POS_FLASH_CODE;
	ret = file_delete(file_name, pos);
	assert(ret==0,"test6:file delete error!");
	p_err(ret);

	/*********测试用例7**************/
	pos = FILE_POS_FLASH_DATA;
	ret = file_delete(file_name, pos);
	assert(ret==0,"test7:file delete error!");
	p_err(ret);

	/*********测试用例8**************/
	pos = FILE_POS_SD;  //无SD卡
	ret = file_delete(file_name, pos);
	assert(ret==0,"test8:file delete error!");
	p_err(ret);

	/*********测试用例9**************/
	pos = FILE_POS_U;   //只读
	ret = file_delete(file_name, pos);
	assert(ret==0,"test9:file delete error!");
	p_err(ret);
	/*********测试用例10**************/
	pos = FILE_POS_RAM;
	ret = file_delete("fi/s", pos);
	assert(ret==-ERR_INVAL,"test10:file delete error!");

	/*********测试用例11**************/
	char *file_long = "123filejfd334567jfkdhkdjf34567jjkd";
	pos = FILE_POS_RAM;
	ret = file_delete(file_long, pos);
	assert(ret==-ERR_INVAL,"test11:file delete error!");

	/*********测试用例12**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FILE");
	pos = FILE_POS_RAM;
	ret = file_delete(file_name, pos);
	assert(ret==0,"test12:file delete error!");
	p_err(ret);

	/*********测试用例13**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE");
	pos = FILE_POS_RAM;
	ret = file_delete(file_name, pos);
	assert(ret==0,"test13:file delete error!");
	p_err(ret);

	/*********测试用例15**************/
	memset(file_name, 0, 10);
	strcat (file_name, "Fi_lE678");
	pos = FILE_POS_RAM;
	ret = file_delete(file_name, pos);
	assert(ret==0,"test15:file delete error!");
	p_err(ret);

	/*********测试用例16**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE123");
	pos = FILE_POS_RAM;
	ret = file_delete(file_name, pos);
	assert(ret==0,"test16:file delete error!");
	p_err(ret);


	/*********测试用例17**************/
	pos = FILE_POS_RAM;
	ret = file_delete("fi$%", pos);
	assert(ret==-ERR_INVAL,"test17:file delete error!");

	/*********测试用例18**************/
	pos = FILE_POS_RAM;
	ret = file_delete("fi;ss", pos);
	assert(ret==-ERR_INVAL,"test18:file delete error!");

	/*********测试用例19**************/
	pos = FILE_POS_RAM;
	ret = file_delete("fi[ss]", pos);
	assert(ret==-ERR_INVAL,"test19:file delete error!");

	/*********测试用例20**************/
	pos = FILE_POS_RAM;
	ret = file_delete("fi}ss", pos);
	assert(ret==-ERR_INVAL,"test20:file delete error!");


	finaltest();
error:
	exit(0);
}
