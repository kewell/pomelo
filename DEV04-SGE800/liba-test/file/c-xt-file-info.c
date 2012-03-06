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
    文件名         ：  c-tx-file-info.c
    描述	       ：  本文件用于平台库file_info函数的测试
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
	//测试环境初始化，运行c-xt-file-open
	char *name = "file1";
	u8 pos;
	file_time_t file_time;
	u32 size;

	/************测试用例1*****************/
	pos = FILE_POS_RAM;
	ret = file_info(name, pos, &size, &file_time);
	assert(ret == 0,"test1:file info get error");
	printf("file time:year=%d,month=%d,day=%d,hour=%d,min=%d,sec=%d\n",
			file_time.year,file_time.mon,file_time.day,file_time.hour,file_time.min,file_time.sec);
	printf("size = %d\n",size);
	system("ls -l /var/file1");

	/************测试用例2*****************/
	ret = file_info(name, -1, &size, &file_time);
	assert(ret == -ERR_INVAL,"test2:file info get error");

	/************测试用例3*****************/
	ret = file_info(name, 5, &size, &file_time);
	assert(ret == -ERR_INVAL,"test3:file info get error");

	/*********测试用例4**************/
	pos = FILE_POS_RAM;
	ret = file_info("file2", pos, &size, &file_time);
	assert(ret==-ERR_NOFILE,"test4:file info get error!");

	/*********测试用例5**************/
	pos = FILE_POS_RAM;
	ret = file_info("file.c", 5, &size, &file_time);
	ret = file_delete("file.c", pos);
	assert(ret==-ERR_INVAL,"test5:file info get error!");

	/*********测试用例6**************/
	pos = FILE_POS_RAM;
	u32 *size1;
	ret = file_info(name, 5, size1, &file_time);
	assert(ret==-ERR_INVAL,"test6:file info get error!");

	/*********测试用例7**************/
	pos = FILE_POS_RAM;
	file_time_t *file_time1;
	ret = file_info(name, 5, &size, file_time1);
	assert(ret==-ERR_INVAL,"test7:file info get error!");


	/*********测试用例8**************/
	char file_name[10]="";
	memset(file_name, 0, 10);
	strcat (file_name, "file");
	pos = FILE_POS_FLASH_CODE;
	ret = file_info(file_name, pos, &size, &file_time);
	assert(ret==0,"test8:file info get error!");
	p_err(ret);

	/*********测试用例9**************/
	pos = FILE_POS_FLASH_DATA;
	ret = file_info(file_name, pos, &size, &file_time);
	assert(ret==0,"test9:file info get error!");
	p_err(ret);

	/*********测试用例10**************/
	pos = FILE_POS_SD;  //无SD卡
	ret = file_info(file_name, pos, &size, &file_time);
	assert(ret==0,"test10:file info get error!");
	p_err(ret);

	/*********测试用例11**************/
	pos = FILE_POS_U;   //只读
	ret = file_info(file_name, pos, &size, &file_time);
	assert(ret==0,"test11:file info get error!");
	p_err(ret);
	/*********测试用例12**************/
	pos = FILE_POS_RAM;
	ret = file_info("fi/s", pos, &size, &file_time);
	assert(ret==-ERR_INVAL,"test12:file info get error!");

	/*********测试用例13**************/
	char *file_long = "123filejfd334567jfkdhkdjf34567jjkd";
	pos = FILE_POS_RAM;
	ret = file_info(file_long, pos, &size, &file_time);
	assert(ret==-ERR_INVAL,"test13:file info get error!");

	/*********测试用例14**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FILE");
	pos = FILE_POS_RAM;
	ret = file_info(file_name, pos, &size, &file_time);
	assert(ret==0,"test14:file info get error!");
	p_err(ret);

	/*********测试用例15**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE");
	pos = FILE_POS_RAM;
	ret = file_info(file_name, pos, &size, &file_time);
	assert(ret==0,"test15:file info get error!");
	p_err(ret);

	/*********测试用例17**************/
	memset(file_name, 0, 10);
	strcat (file_name, "Fi_lE678");
	pos = FILE_POS_RAM;
	ret = file_info(file_name, pos, &size, &file_time);
	assert(ret==0,"test17:file info get error!");
	p_err(ret);

	/*********测试用例18**************/
	memset(file_name, 0, 10);
	strcat (file_name, "FilE123");
	pos = FILE_POS_RAM;
	ret = file_info(file_name, pos, &size, &file_time);
	assert(ret==0,"test18:file info get error!");
	p_err(ret);

	/*********测试用例19**************/
	pos = FILE_POS_RAM;
	ret = file_info("fi$%", pos, &size, &file_time);
	assert(ret==-ERR_INVAL,"test19:file info get error!");

	/*********测试用例20**************/
	pos = FILE_POS_RAM;
	ret = file_info("fi;ss", pos, &size, &file_time);
	assert(ret==-ERR_INVAL,"test20:file info get error!");

	/*********测试用例21**************/
	pos = FILE_POS_RAM;
	ret = file_info("fi[ss]", pos, &size, &file_time);
	assert(ret==-ERR_INVAL,"test21:file info get error!");

	/*********测试用例22**************/
	pos = FILE_POS_RAM;
	ret = file_info("fi}ss", pos, &size, &file_time);
	assert(ret==-ERR_INVAL,"test22:file info get error!");


	finaltest();

	exit(0);
}
