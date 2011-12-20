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
    项目名称    ：  SGE800计量终端业务平台
    文件名         ：  cyclicdb.c
    描          述    ：  本文件用于业务平台库循环存储数据库功能的实现
    版          本    ：  0.1
    作          者    ：  孙锐
    创建日期    ：  2010.04
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		    //exit
#include <unistd.h>		    //sleep
#include <db.h>
#include <string.h>

//平台库头文件
#include "include/dbs.h"
#include "private/config.h"
#include "include/error.h"

/*************************************************
  结构类型定义
*************************************************/
static struct {
	u32 record_len;
	u8 count;            //初始化计数
}cyclicdb_info[CFG_DBS_UNM_MAX];     //属性

#pragma pack(1)
typedef struct {
	 u8 year;		//年: [0 - 255]自从2000年以来的年
	 u8 mon;		//月: [1 - 12]
	 u8 day;		//日：[1 - 31]
	 u8 hour;		//时：[0 - 23]
	 u8 min;		//分：[0 - 59]
	 u8 sec;		//秒：[0 - 59]
} db_t;

#pragma pack()

/*******************************API函数实现***********************************/

/******************************************************************************
*	函数:	cyclicdb_init
*	功能:	初始化并打开循环数据库
*	参数:	id				-	数据库标号
			pose			-	打开位置
			mode			-	打开方式
			record_len      -   循环存储最大记录数
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没初始化
			-ERR_BUSY		-	已打开/已存在
			-ERR_NOFILE     -   数据库文件不存在
*	说明:	以DBS_MODE_CREAT方式打开数据库，如果数据库已经创建则会报错-ERR_BUSY，
以DBS_MODE_RD方式打开只读数据库，数据库不存在报错-ERR_NOFILE。
******************************************************************************/
int cyclicdb_init(u8 id, u8 pos, u8 mode, u8 record_len)
{
	int ret;

	ret = dbs_init();                          //数据库模块初始化
	if ((ret != -ERR_BUSY) && (ret != 0)){
		goto error;
	}

	ret = dbs_open(id, pos, mode);            //打开数据库
	if ((ret != -ERR_BUSY) && (ret != 0)){
		goto error;
	}

	cyclicdb_info[id].record_len = record_len;
	cyclicdb_info[id].count = 1;

	ret = 0;
error:
	return(ret);
}

/******************************************************************************
 *	函数:	cyclicdb_write
 *	功能:	写循环记录数据（最新的一条记录）
 *	参数:	id				-	数据库标号
 			key_size		-	关键字长度
 			key             -   关键字
            data_size       -   数据大小
            data            -   数据
 *	返回:	0				-	成功
 			-ERR_INVA		-   接口参数配置错误；
 			-ERR_NOFILE		-	数据库未打开
 			-ERR_NOFUN		-	不具备写权限
 			-ERR_NODISK		-	数据长度越限
 			-ERR_TIMEOUT	-	超时
 			-ERR_NORECORD	-	数据纪录不存在
 *	说明:	如果记录数已达到最大记录数，则删除最早的记录，添加最新的第一条记录。
                               如果该关键字下记录不存在，则直接添加第一条新记录
******************************************************************************/
int cyclicdb_write(u8 id, u8 key_size, void *key, u16 data_size, void *data)
{
	int ret;
	u32 record_total;
	dbs_set_t set;
	u8 flag;

	if (cyclicdb_info[id].count != 1){
		ret = -ERR_NOINIT;
		goto error;
	}
//判断是否到达循环存储的最大值
	ret = dbs_count(id, key_size, key, &record_total);
	if (ret == 0){
		if (record_total == cyclicdb_info[id].record_len){     //已经达到最大值
			set.offset = 0;
			set.whence = DBS_SEEK_SET;
			ret = dbs_delete(id, key_size, key, &set);
			if (ret != 0){
				goto error;
			}else{
				set.offset = 0;
				set.whence = DBS_SEEK_END;
				flag = DBS_SEEK_BACKWARD;
				ret = dbs_insert(id, key_size, key, &set, flag, data_size, data);
				if(ret !=0){
					goto error;
				}
			}
		}else if(record_total < cyclicdb_info[id].record_len){    //没有达到最大值
			set.offset = 0;
			set.whence = DBS_SEEK_END;
			flag = DBS_SEEK_BACKWARD;
			ret = dbs_insert(id, key_size, key, &set, flag, data_size, data);
			if(ret !=0){
				goto error;
			}
		}else{
			ret = -ERR_SYS;
			goto error;
		}
	}else if(ret==-ERR_NORECORD){             //无记录时直接插入新记录
		set.offset = 0;
		set.whence = DBS_SEEK_END;
		flag = DBS_SEEK_BACKWARD;
		ret = dbs_insert(id, key_size, key, &set, flag, data_size, data);
		if(ret !=0){
			goto error;
		}
	}else {
		goto error;
	}
	ret = 0;
error:
	return(ret);

}

/******************************************************************************
  *	函数:	cyclicdb_read
  *	功能:	读取循环记录数据
  *	参数:	id				-	数据库标号
  			key_size		-	关键字长度
  			key             -   关键字
            set             -	记录定位
            max_size        -	data元素指向大小（即返回data缓冲区大小）
 			data_size       -	实际返回数据的长度(输出)
 			data            -	数据
  *	返回:	0				-	成功
  			-ERR_INVA		-   接口参数配置错误；
  			-ERR_NOINIT		-	数据库模块未初始化
  			-ERR_NOFILE		-	数据库未打开
  			-ERR_NOMEM		-	内存错误，数据长度超过接口参数的最大长度
  			-ERR_TIMEOUT	-	超时
  			-ERR_NORECORD	-	数据纪录不存在
  *	说明:	无
 ******************************************************************************/
int cyclicdb_read(u8 id, u8 key_size, void *key, dbs_set_t *set, u16 max_size, u16 *data_size, void  *data)
{
	int ret;
	ret = dbs_read(id, key_size, key, set, max_size, data_size, data);
	if (ret != 0){
		goto error;
	}
	ret = 0;
error:
	return(ret);
}
/******************************************************************************
 *	函数:	cyclicdb_write_t
 *	功能:	写带时间标示的循环记录数据（最新的一条记录）
 *	参数:	id				-	数据库标号
 			key_size		-	关键字长度
 			key             -   关键字
 			time			-   指定时间
            data_size       -   数据大小
            data            -   数据
 *	返回:	0				-	成功
 			-ERR_INVA		-   接口参数配置错误；
 			-ERR_NOFILE		-	数据库未打开
 			-ERR_NOFUN		-	不具备写权限
 			-ERR_NODISK		-	数据长度越限
 			-ERR_TIMEOUT	-	超时
 			-ERR_NORECORD	-	数据纪录不存在
 *	说明:	如果记录数已达到最大记录数，则删除最早的记录，添加最新的第一条记录。
                                如果该关键字下记录不存在，则直接添加第一条新记录
            ！该记录时刻应该大于上一写数据库记录的时刻
******************************************************************************/
int cyclicdb_write_t(u8 id, u8 key_size, void *key, db_t *time, u16 data_size, void  *data)
{
	int ret;
	u32 record_total;
	dbs_set_t set;
	u8 flag;
	struct{
		db_t record_time;
		u8 record_data[data_size];
	}record;

	if (cyclicdb_info[id].count != 1){
		ret = -ERR_NOINIT;
		goto error;
	}
	record.record_time.day = time->day;
	record.record_time.sec = time->sec;
	record.record_time.hour = time->hour;
	record.record_time.min = time->min;
	record.record_time.mon = time->mon;
	record.record_time.year = time->year;

	memcpy(&record.record_data, data, data_size);
	//判断是否达到循环存储的最大值
	ret = dbs_count(id, key_size, key, &record_total);
	if (ret == 0){
		if (record_total == cyclicdb_info[id].record_len){           //达到最大值
			set.offset = 0;
			set.whence = DBS_SEEK_SET;
			ret = dbs_delete(id, key_size, key, &set);
			if (ret != 0){
				goto error;
			}else{
				set.offset = 0;
				set.whence = DBS_SEEK_END;
				flag = DBS_SEEK_BACKWARD;
				ret = dbs_insert(id, key_size, key, &set, flag, (data_size + sizeof(db_t)), &record);
				if(ret !=0){
					goto error;
				}
			}
		}else if(record_total < cyclicdb_info[id].record_len){        //没有达到最大值
			set.offset = 0;
			set.whence = DBS_SEEK_END;
			flag = DBS_SEEK_BACKWARD;
			ret = dbs_insert(id, key_size, key, &set, flag, (data_size + sizeof(db_t)), &record);
			if(ret !=0){
				goto error;
			}
		}else{
			ret = -ERR_SYS;
			goto error;
		}
	}else if (ret == -ERR_NORECORD){                  //无记录插入新记录
		set.offset = 0;
		set.whence = DBS_SEEK_END;
		flag = DBS_SEEK_BACKWARD;
		ret = dbs_insert(id, key_size, key, &set, flag, (data_size + sizeof(db_t)), &record);
		if(ret !=0){
			goto error;
		}
	}else{
		goto error;
	}
	ret = 0;
error:
	return(ret);

}
/******************************************************************************
  *	函数:	cyclicdb_read_t
  *	功能:	按指定时间读取循环记录数据库中的记录
  *	参数:	id				-	数据库标号
  			key_size		-	关键字长度
  			key             -   关键字
  			time			-	指定时间
            max_size        -	data元素指向大小（即返回data缓冲区大小）
 			data_size       -	实际返回数据的长度(输出)
 			data            -	数据
  *	返回:	0				-	成功
  			-ERR_INVA		-   接口参数配置错误；
  			-ERR_NOINIT		-	数据库模块未初始化
  			-ERR_NOFILE		-	数据库未打开
  			-ERR_NOMEM		-	内存错误，数据长度超过接口参数的最大长度
  			-ERR_TIMEOUT	-	超时
  			-ERR_NORECORD	-	数据纪录不存在
  *	说明:	无
 ******************************************************************************/
int cyclicdb_read_t(u8 id, u8 key_size, void *key, db_t *time, u16 max_size, u16 *data_size, void  *data)
{
	int ret;
	dbs_set_t set;
	u32 record_total,mid,top,bot;
	u8 find;
	struct rec{
		db_t record_time;
		u8 record_data[max_size];
	}record;
	u16 record_len;

	if (cyclicdb_info[id].count != 1){
		ret = -ERR_NOINIT;
		goto error;
	}
	ret = dbs_count(id, key_size, key, &record_total);
	if(ret != 0){
		goto error;
	}
	top = 0;
	bot = record_total-1;
	find = 0;
	do              //循环查找符合指定时间的记录
	{
		mid = (top + bot)/2;
		set.offset = mid;
		set.whence = DBS_SEEK_SET;
		ret = dbs_read(id, key_size, key, &set, sizeof(db_t)+max_size, &record_len, &record);
		if (ret == 0){
			if (record.record_time.year < time->year){
				top = mid + 1;
			}else if (record.record_time.year > time->year){
				bot = mid - 1;
			}else{
				if (record.record_time.mon < time->mon){
					top = mid + 1;
				}else if (record.record_time.mon > time->mon){
					bot = mid - 1;
				}else{
					if (record.record_time.day < time->day){
						top = mid + 1;
					}else if (record.record_time.day > time->day){
						bot = mid - 1;
					}else{
						if (record.record_time.hour < time->hour){
							top = mid + 1;
						}else if (record.record_time.hour > time->hour){
							bot = mid - 1;
						}else {
							if (record.record_time.min < time->min){
								top = mid + 1;
							}else if (record.record_time.min > time->min){
								bot = mid - 1;
							}else{
								if (record.record_time.sec < time->sec){
									top = mid + 1;
								}else if (record.record_time.sec > time->sec){
									bot = mid - 1;
								}else{
									find = 1;
									record_len = record_len - sizeof(db_t);
									memcpy(data_size, &record_len, sizeof(record_len));
									memcpy(data,&record.record_data, record_len);
								}
							}
						}
					}
				}
			}
		}else{
			goto error;
		}

	}
	while((find == 0) && (top <= bot));
	if (find != 1){
		ret = -ERR_NORECORD;
		goto error;
	}
	ret = 0;
error:
	return(ret);

}

/******************************************************************************
   *	函数:	cyclicdb_read_p
   *	功能:	读指定一段时间内循环存储数据库中的记录数据
   *	参数:	id				-	数据库标号
				key_size		-	关键字长度
				key             -   关键字
				from            -	读取记录起始时间
				to              -	读取记录结束时间
				max_size        -	data元素指向大小（即返回data缓冲区大小）
				data_size       -	实际返回数据的长度(输出)
				data            -	数据（包括记录的长度和数据记录）
   *	返回:	0				-	成功
				-ERR_INVA		-   接口参数配置错误；
				-ERR_NOINIT		-	数据库模块未初始化
				-ERR_NOFILE		-	数据库未打开
				-ERR_NOMEM		-	内存错误，数据长度超过接口参数的最大长度
				-ERR_TIMEOUT	-	超时
				-ERR_NORECORD	-	数据纪录不存在
   *	说明:	数据返回结构是记录长度（时间+数据）+ 时间+记录数据的形式，为低字节在前
******************************************************************************/
int cyclicdb_read_p(u8 id, u8 key_size, void *key, db_t *from, db_t *to, u16 max_size, u16 *size, void *data)
{
	int ret;
	dbs_set_t set;
	u32 record_total,mid,top,bot;
	u8 find;
	u8 offset_f,offset_t;
	struct rec{
		db_t record_time;
		u8 record_data[max_size];
	}record;
	u16 record_len;

	if (cyclicdb_info[id].count != 1){
		ret = -ERR_NOINIT;
		goto error;
	}

	//增加判断开始时刻早于结束时刻
	ret = dbs_count(id, key_size, key, &record_total);
	if(ret != 0){
		goto error;
	}
	top = 0;
	bot = record_total-1;
	find = 0;
	do             //循环查找符合指定起始时间的记录
	{
		mid = (top + bot)/2;
		set.offset = mid;
		set.whence = DBS_SEEK_SET;
		ret = dbs_read(id, key_size, key, &set, sizeof(db_t)+max_size, &record_len, &record);
		if (ret == 0){
			if (record.record_time.year < from->year){
				top = mid + 1;
			}else if (record.record_time.year > from->year){
				bot = mid - 1;
			}else{
				if (record.record_time.mon < from->mon){
					top = mid + 1;
				}else if (record.record_time.mon > from->mon){
					bot = mid - 1;
				}else{
					if (record.record_time.day < from->day){
						top = mid + 1;
					}else if (record.record_time.day > from->day){
						bot = mid - 1;
					}else{
						if (record.record_time.hour < from->hour){
							top = mid + 1;
						}else if (record.record_time.hour > from->hour){
							bot = mid - 1;
						}else {
							if (record.record_time.min < from->min){
								top = mid + 1;
							}else if (record.record_time.min > from->min){
								bot = mid - 1;
							}else{
								if (record.record_time.sec < from->sec){
									top = mid + 1;
								}else if (record.record_time.sec > from->sec){
									bot = mid - 1;
								}else{
									find = 1;
									offset_f = mid;
								}
							}
						}
					}
				}
			}
		}else{
			goto error;
		}

	}
	while((find == 0) && (top <= bot));
	if (find != 1){
		ret = -ERR_NORECORD;
		goto error;
	}
	top = 0;
	bot = record_total-1;
	find = 0;
	do                     //循环查找符合结束时间的记录
	{
		mid = (top + bot)/2;
		set.offset = mid;
		set.whence = DBS_SEEK_SET;
		ret = dbs_read(id, key_size, key, &set, sizeof(db_t)+max_size, &record_len, &record);
		if (ret == 0){
			if (record.record_time.year < to->year){
				top = mid + 1;
			}else if (record.record_time.year > to->year){
				bot = mid - 1;
			}else{
				if (record.record_time.mon < to->mon){
					top = mid + 1;
				}else if (record.record_time.mon > to->mon){
					bot = mid - 1;
				}else{
					if (record.record_time.day < to->day){
						top = mid + 1;
					}else if (record.record_time.day > to->day){
						bot = mid - 1;
					}else{
						if (record.record_time.hour < to->hour){
							top = mid + 1;
						}else if (record.record_time.hour > to->hour){
							bot = mid - 1;
						}else {
							if (record.record_time.min < to->min){
								top = mid + 1;
							}else if (record.record_time.min > to->min){
								bot = mid - 1;
							}else{
								if (record.record_time.sec < to->sec){
									top = mid + 1;
								}else if (record.record_time.sec > to->sec){
									bot = mid - 1;
								}else{
									find = 1;
									offset_t = mid;
								}
							}
						}
					}
				}
			}
		}else{
			goto error;
		}

	}
	while((find == 0) && (top <= bot));
	if (find != 1){
		ret = -ERR_NORECORD;
		goto error;
	}

	//批量读取数据
	dbs_set_t read_f;
	dbs_set_t read_t;
	read_f.whence = DBS_SEEK_SET;
	read_f.offset = 2;
	read_t.whence = DBS_SEEK_SET;
	read_t.offset = 5;
	ret = dbs_read_bulk(id, key_size, key, &read_f, &read_t, max_size, size, data);
	if(ret != 0){
		goto error;
	}
	ret = 0;
error:
	return(ret);
}

/******************************************************************************
    *	函数:	cyclicdb_close
    *	功能:	关闭循环记录数据库
    *	参数:	id				-	数据库标号
    *	返回:	0				-	成功
 				-ERR_INVA		-   接口参数配置错误
 				-ERR_NOINIT		-	数据库模块未初始化
 				-ERR_NOFILE		-	数据库未打开
 				-ERR_BUSY		-	未关闭
				-ERR_SYS		-	系统错误
    *	说明:	无
******************************************************************************/
int  cyclicdb_close(u8 id)
{
	int ret;
	ret = dbs_close(id);
	if(ret != 0){
		goto error;
	}
	ret = 0;
error:
	return(ret);
}

/******************************************************************************
      *	函数:	cyclicdb_remove
      *	功能:	删除循环记录数据库
      *	参数:	id				-	数据库标号
      *	返回:	0				-	成功
   				-ERR_INVA		-   接口参数配置错误
   				-ERR_NOINIT		-	数据库模块未初始化
   				-ERR_NOFILE		-	数据库未打开
   				-ERR_BUSY		-	未关闭
 				-ERR_SYS		-	系统错误
      *	说明:	无
 ******************************************************************************/
int  cyclicdb_remove(u8 id)
{
	int ret;
	ret = dbs_remove(id);
	if (ret != 0){
		goto error;
	}
	ret = 0;
error:
	return(ret);

}
