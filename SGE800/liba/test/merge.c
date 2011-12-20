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
    文件名      ：  merge.c
    描述        ：  本文件用于调试和测试平台库adc
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.05
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

int main(int argn, char **arg)
{
	char str[256];
	char command[20];
	FILE *fp;

	char *mainlib;
	char *addlib;

	if (argn != 3) {
		printf ("should be not 2 file\n");
		return 1;
	}
	mainlib = arg[1];
	fp = fopen (mainlib, "r");
	if (fp == NULL) {
		printf ("error file\n");
		return 1;
	}
	fclose (fp);

	addlib = arg[2];
	fp = fopen (addlib, "r");
	if (fp == NULL) {
		printf ("error file\n");
		return 1;
	}
	fclose (fp);
	printf ("start to expand addlib file\n");
	sprintf (command, "arm-linux-ar -x %s", addlib);
	system (command);
	printf ("start to check the addlib out to a.txt \n");
	  // 用ar -t显示模块名列表,保存在a.txt文件里,如果能保存在内存里就快了
	sprintf (command, "arm-linux-ar -t %s > a.txt", addlib);
	system (command);

	fp = fopen ("a.txt", "r");
	if (fp == NULL) {
		printf ("error\n");
		return 1;
	}
	printf ("start to add %s to %s \n",mainlib,addlib);
	while (-1 != fscanf (fp, "%s", str)) {
		sprintf (command, "arm-linux-ar -r %s %s", mainlib, str);
		//sprintf (command, "arm-linux-ar -d %s %s", mainlib, str);
		printf ("%s\n", command);
		system (command);
		sprintf (command, "rm -f %s ", str);
		system (command);
	}
	system ("rm -f a.txt");
	fclose (fp);
	printf ("done \n");
	return 0;
}
