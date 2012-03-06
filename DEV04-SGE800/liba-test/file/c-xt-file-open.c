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
    文件名         ：  c-tx-file-open.c
    描述	       ：  本文件用于平台库file_open函数的测试
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
	u8 pos,mode;
	/*********测试用例28**************/
	char *name = "file1";
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	ret = file_open(name, pos, mode);
	assert(ret>0,"test28:file1 open error!");
	p_err(ret);

	/*********测试用例29**************/
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RD;
	ret = file_open(name, pos, mode);
	assert(ret>0,"test29:file1 open error!");
	p_err(ret);

	while(1){

	}

}

void thread_test2()
{
	int ret;
	u8 pos,mode;
	/*********测试用例28**************/
	char *name = "file1";
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	ret = file_open(name, pos, mode);
	assert(ret>0,"test28:file2 open error!");
	p_err(ret);

	/*********测试用例29**************/
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RD;
	ret = file_open(name, pos, mode);
	assert(ret>0,"test29:file2 open error!");
	p_err(ret);

	while(1){

	}
}

void thread_test3()
{
	int ret;
	u8 pos,mode;

	/*********测试用例28**************/
	char *name = "file1";
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	ret = file_open(name, pos, mode);
	assert(ret>0,"test28:file3 open error!");
	file_close(ret);
	p_err(ret);

	/*********测试用例29**************/
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RD;
	ret = file_open(name, pos, mode);
	assert(ret>00,"test29:file3 open error!");
	p_err(ret);

	while(1){

	}

}

int main()
{
	int ret;
	//环境的初始化
	inittest();
	system("rm /var/f*");
	system("rm /var/F*");
	system("rm /mnt/local/fi*");
	char *name = "file1";
	u8 pos,mode;

	/*********测试用例1（正常测试）**************/
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	ret = file_open(name, pos, mode);
	assert(ret>0,"test1:file open error!");
	file_close(ret);

	/*********测试用例2（边界测试）**************/
	pos = -1;   //非法
	mode = FILE_MODE_OPEN;
	ret = file_open(name, pos, mode);
	assert(ret==-ERR_INVAL,"test2:border test1 fail!");

	/*********测试用例3（边界测试）**************/
	pos = 5;
	mode = FILE_MODE_OPEN;
	ret = file_open(name, pos, mode);
	assert(ret==-ERR_INVAL,"test2:border test2 fail!");

	/*********测试用例4（边界测试）**************/
	pos = FILE_POS_RAM;
	mode = -1;
	ret = file_open(name, pos, mode);
	assert(ret==-ERR_INVAL,"test4:border test3 fail!");

	/*********测试用例5（边界测试）**************/
	pos = FILE_POS_RAM;
	mode = 4;
	ret = file_open(name, pos, mode);
	assert(ret==-ERR_INVAL,"test5:border test4 fail!");

	/*********测试用例6（错误测试）**************/
	char err_name[5]="";
	strcat (err_name, "file");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_OPEN;
	ret = file_open(err_name, pos, mode);
	assert(ret==-ERR_NOFILE,"test6:error test fail!");

	/*********测试用例7**************/
	err_name[3]= '.';
	pos = FILE_POS_RAM;
	mode = FILE_MODE_OPEN;
	ret = file_open(err_name, pos, mode);
	assert(ret==-ERR_INVAL,"test7:error test fail!");

	/*********测试用例8**************/
	pos = 5;
	mode = FILE_MODE_OPEN;
	ret = file_open(name, pos, mode);
	assert(ret==-ERR_INVAL,"test8:error test fail!");

	/*********测试用例9**************/
	pos = FILE_POS_RAM;
	mode = -6;
	ret = file_open(name, pos, mode);
	assert(ret==-ERR_INVAL,"test9:error test fail!");

	/*********测试用例10**************/
	char *err_n;
	u8 name_err = 2;
	err_n = &name_err;    //错误的函数输入
	pos = FILE_POS_RAM;
	mode = FILE_MODE_OPEN;
	ret = file_open(err_n, pos, mode);
	assert(ret==-ERR_INVAL,"test10:error test fail!");

	/*********测试用例11**************/
	char file_name[10]="";
	memset(file_name, 0, 10);
	strcat (file_name, "file");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test11:file open error!");
	p_err(ret);

	/*********测试用例12**************/
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RW;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test12:file open error!");
	p_err(ret);

	/*********测试用例13**************/
	pos = FILE_POS_RAM;
	mode = FILE_MODE_RD;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test13:file open error!");

	/*********测试用例14**************/
	pos = FILE_POS_FLASH_CODE;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test14:file open error!");

	/*********测试用例15**************/
	pos = FILE_POS_FLASH_DATA;         //只读
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test15:file open error!");
	p_err(ret);

	/*********测试用例16**************/
	pos = FILE_POS_SD;  //无SD卡
	mode = FILE_MODE_RW;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test16:file open error!");
	p_err(ret);

	/*********测试用例17**************/
	pos = FILE_POS_U;   //只读
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test17:file open error!");
	p_err(ret);

	/*********测试用例18**************/
	char *file_long = "123filejfd334567jfkdhkdjf34567jjkd";
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_long, pos, mode);
	assert(ret==-ERR_INVAL,"test18:file open error!");
	p_err(ret);

	/*********测试用例19**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FILE");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test19:file open error!");
	p_err(ret);

	/*********测试用例20**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test20:file open error!");
	p_err(ret);

	/*********测试用例21**************/
	memset(file_name, 0, 10);
	strcat (file_name, "Fi_lE678");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test21:file open error!");
	p_err(ret);

	/*********测试用例22**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE123");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret>0,"test23:file open error!");
	p_err(ret);

	/*********测试用例23**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE/12");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret==-ERR_INVAL,"test24:file open error!");

	/*********测试用例24**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE$%12");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret==-ERR_INVAL,"test25:file open error!");

	/*********测试用例25**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE;12");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret==-ERR_INVAL,"test26:file open error!");

	/*********测试用例26**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE[12]");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret==-ERR_INVAL,"test27:file open error!");

	/*********测试用例27**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE12}");
	pos = FILE_POS_RAM;
	mode = FILE_MODE_CREAT;
	ret = file_open(file_name, pos, mode);
	assert(ret==-ERR_INVAL,"test28:file open error!");

	//性能测试-多线程只读打开文件
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

