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
    文件名      ：  c-jl-dbs-capmem.c
    描述        ：  本文件用于平台库内存泄露的测试
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2010.03
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep
#include <db.h>

//平台库头文件
#include "../include/dbs.h"
#include "../include/xj_assert.h"
#include "../include/error.h"

#define MAXKEY 100
#define MAXDATA 100

int main()
{
	int ret;
	u32 i,j;
	u32 *key = &i;
	u32 *data = &j;
	u16 data_size;
	dbs_set_t set;
	u32 data_r;



	//环境的初始化
	inittest();
	ret = dbs_init();
	p_err(ret);

	//环境的初始化-读写方式，ram中打开
	ret = dbs_open(0, DBS_POS_RAM, DBS_MODE_RW);
	p_err(ret);

	while(1){
		//写MAXKEY×MAXDATA条记录
		for(i=0; i < MAXKEY; i++){
			for(j=0; j< MAXDATA; j++){
				set.whence = DBS_SEEK_SET;
				set.offset = 0;
				ret = dbs_insert(0, 4, key, &set, DBS_SEEK_FORWARD, 4, data);
				assert(ret==0, "dbs_insert error");
			}
		}
		printf("insert %d record\n",i*j);
//		finaltest();

		//读取MAXKEY×MAXDATA条记录
		for(i=0; i < MAXKEY; i++){
			for(j=0; j< MAXDATA; j++){
				set.whence = DBS_SEEK_SET;
				set.offset = j;
				ret = dbs_read(0, 4, key, &set, 8, &data_size, &data_r);
				assert(ret==0, "dbs_read error");
				p_err(ret);
			}
		}
		printf("read %d record\n",i*j);
//		finaltest();

		//删除MAXKEY×MAXDATA条记录
		for(i=0; i < MAXKEY; i++){
			for(j=0; j< MAXDATA; j++){
				set.whence = DBS_SEEK_SET;
				set.offset = 0;
				ret = dbs_delete(0, 4, key, &set);
				assert(ret==0, "dbs_delete error");
			}
		}
		printf("delete %d record\n",i*j);
//		finaltest();
	}

	ret = dbs_close(0);		//关闭并移除数据库文件
	p_err(ret);
	ret = dbs_remove(0);
	p_err(ret);

	exit(0);
}

