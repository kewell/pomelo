/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/******************************************************************************
	项目名称	：SGE800计量智能终端平台
	文件		：dbs.c
	描述		：本文件实现了数据库服务模块中的API函数
	版本		：0.1
	作者		：孙锐
	创建日期	：2009.12
******************************************************************************/

//库配置头文件
#include "private/config.h"

//模块启用开关
#ifdef CFG_DBS_MODULE

//调试头文件
#include "private/debug.h"

//驱动调用头文件

//C库头文件
#include <stdio.h>						//printf
#include <fcntl.h>						//open
#include <unistd.h>						//read,write
#include <string.h>						//bzero
#include <errno.h>
#include <db.h>


//提供给用户的头文件
#include "include/error.h"
#include "include/dbs.h"

/*************************************************
  静态全局变量定义
*************************************************/
static DB_ENV *dbenv = NULL;                         //数据库环境句柄
static DB *dbp[CFG_DBS_UNM_MAX];                     //数据库句柄      
/*************************************************
  结构类型定义
*************************************************/
static struct {
	pthread_mutex_t dbp_mutex;  //属性锁  
	u8 dbp_count;               //计数
	u8 dbp_mode;                //打开方式
	u8 dbp_pos;                 //打开位置
}dbp_info[CFG_DBS_UNM_MAX];     //属性

//各存储区域路径
static const char *const path_ram = "/var/";
static const char *const path_flash_code = "/mnt/local/";
static const char *const path_flash_data = "/mnt/data0/";
static const char *const path_sd = "/mnt/sddisk/";
static const char *const path_u = "/mnt/udisk/";

/*******************************API函数实现***********************************/

/******************************************************************************
*	函数:	dbs_init
*	功能:	初始化数据库
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数无效
			-ERR_BUSY		-	已经初始化

*	说明:	初始化数据库环境和数据库变量。
******************************************************************************/
int dbs_init()
{
	int ret = 0;
	int ref;
	int i;
	 
	for (i = 0;i < CFG_DBS_UNM_MAX;i ++){            //初始化数据库句柄
		dbp[i] = NULL;
	}
	
	for (i = 0;i < CFG_DBS_UNM_MAX;i ++){            //初始化数据库打开关闭计数器
		dbp_info[i].dbp_count = 0;
	}
	
	for (i = 0;i < CFG_DBS_UNM_MAX;i ++){            //初始化数据库属性设置锁
		ref = pthread_mutex_init(&(dbp_info[i].dbp_mutex),NULL);
		if (ref){
			ret = -ERR_SYS;
			goto error;
		}
	}
 
	ref = db_env_create(&dbenv, 0);                       //创建数据库环境句柄
	if (ref){
		DBSPRINTF("create db handle error!\n");
		ret = -ERR_SYS;
		goto error;
	}                    
	
	//设置共享内存池空间大小，默认256K
	if (CFG_DBS_MPOOL_SIZE < MPOOL_SIZE_MIN){
		ret = -ERR_INVAL;
		goto error;
	}
	ref = dbenv->set_cachesize(dbenv, 0, CFG_DBS_MPOOL_SIZE, 0);   
	if (ref !=0){
		DBSPRINTF("set cachesize error!\n");
		ret = -ERR_SYS;
		goto error;
	}
	
    //可以用denv->set_timeout 设置锁的超时
    
    //打开环境
	ref = dbenv->open (dbenv, CFG_DBS_ENV_HOME, CFG_DBS_ENV_FLAG, 0);    //初始化数据存储子系统
	if (ref == ENOSPC ){
		DBSPRINTF("env has opened error!ref =%d\n",ref);
		ret = -ERR_BUSY; 
		goto error;
	}else if (ref){
		DBSPRINTF("env initial fail!\n");
		ret = -ERR_SYS;
		goto error;
	}
	DBSPRINTF("env initial success!\n");
	
error:
	return(ret);	
}
/******************************************************************************
*	函数:	dbs_open
*	功能:	打开数据库
*	参数:	id				-	数据库标号
			pose			-	打开位置
			mode			-	打开方式
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没初始化
			-ERR_BUSY		-	已打开/已存在
			-ERR_NOFILE     -   数据库文件不存在
*	说明:	以DBS_MODE_CREAT方式打开数据库，如果数据库已经创建则会报错-ERR_BUSY，
以DBS_MODE_RD方式打开只读数据库，数据库不存在报错-ERR_NOFILE。
 ******************************************************************************/
int dbs_open(u8 id, u8 pos, u8 mode)
{
	int ret,ref;
	u32 flag;
	char path[22] = "";                       //数据库打开路径名
	char name[3] = "";						  //数据库名
	char index[4] = ".db";                    //数据库名后缀
	
	if (dbenv == NULL){                       //未初始化数据库模块：环境未设置
		ret = -ERR_NOINIT;
		return ret;
	}
	
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){  
		ret = -ERR_INVAL;
		return ret;
	}
	
	//获得计数器锁
	ref = pthread_mutex_lock(&(dbp_info[id].dbp_mutex));
	if (ref ==  EINVAL){
		DBSPRINTF("mutex lock fail!\n");
		ret = -ERR_NOINIT;
		return ret;
	}else if (ref){                           //ref == EDEADLK
		ret = -ERR_BUSY;
		return ret;
	}else{
		//打开计数
		if (dbp_info[id].dbp_count > 0){               //已打开
			ret = -ERR_BUSY;
			DBSPRINTF("db has opened!\n");
			goto error;
		}else {         
			if ((dbp_info[id].dbp_pos != 0) && (dbp_info[id].dbp_pos != pos)){
				ret = -ERR_INVAL;
				goto error;
			}
			//打开数据库
    	    if ((id >= 0) && (id < 10)){             //数据库标号为一位数字
    	    	name[0] = '0' + id;
    	    }else if(id >9 && id < CFG_DBS_UNM_MAX){ //数据库标号为两位数字
    	    	name[0] = '0' + id/10;				 //用数据库标号作为数据库名称，取十位数字
    	    	name[1] = '0' + id%10;				 //用数据库标号作为数据库名称，取个位数字
    	    }else{
    	    	ret = -ERR_INVAL;
    	    	goto error;
    	    }

    	    //配置打开位置接口
        	switch (pos){
        		case DBS_POS_RAM:
        			strcat (path, path_ram);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		case DBS_POS_FLASH_CODE:
        			strcat (path, path_flash_code);
        			strcat (path, name);
        			strcat (path, index);
        			break;
    			case DBS_POS_FLASH_DATA:
        			strcat (path, path_flash_data);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		case DBS_POS_SD:
        			strcat (path, path_sd);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		case DBS_POS_U:
        			strcat (path, path_u);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		default:
        			ret = -ERR_INVAL;
        			goto error;
        			break;
        	}
        	
        	//保存数据库打开位置
        	dbp_info[id].dbp_pos = pos;
            
        	//数据库打开方式
        	switch (mode){
        		case DBS_MODE_OPEN:                             //以读写方式打开数据库，数据库不存在不创建新库而是报错
        			flag = DB_THREAD;
        			dbp_info[id].dbp_mode = 0;                  //保存打开方式：读写
        			break;
        		case DBS_MODE_CREAT:                            //创建新的数据库，此标号数据库已存在时报错
        			flag = DB_CREATE | DB_EXCL | DB_THREAD;
        			dbp_info[id].dbp_mode = 0;                  //保存打开方式：读写
        			break;
        		case DBS_MODE_RW:                               //以读写方式打开数据库，数据库不存在时创建新库
        			flag = DB_CREATE| DB_THREAD;
        			dbp_info[id].dbp_mode = 0;                  //保存打开方式：读写
        			break;
        		case DBS_MODE_RD:                               //以只读方式打开只读数据库，数据库不存在时报错
        			flag = DB_RDONLY | DB_THREAD;
        			dbp_info[id].dbp_mode = 1;                  //保存打开方式：只读
        			break;
        		default:
        			ret = -ERR_INVAL;
        			goto error;
        			break;
        	}
		
			//创建参数数据库
			if (db_create(&dbp[id], dbenv, 0)) {
				ret = -ERR_SYS;
				goto error; 
			}
			
			//设置数据库支持多重记录
			if (dbp[id]->set_flags(dbp[id], DB_DUP)){
				ret = -ERR_SYS;
				goto error;
			}
		    
			//打开数据库
			ref = dbp[id]->open(dbp[id], NULL, path, NULL, CFG_DBS_ARITH, flag, 0);
			if (ref == ENOENT ){
				ret = -ERR_NOFILE;                  //此地址没有数据库存在
				goto error;
			}else if (ref == EEXIST){
				ret = -ERR_BUSY;                    //创建新库时已存在
				goto error;
			}else if (ref){                         //DB_LOCK_NOTGRANTED
				ret = -ERR_TIMEOUT;
				goto error;
			}
			DBSPRINTF("db open!id =%x\n",id);
		}
		ret = 0;
		dbp_info[id].dbp_count = 1;
		//解锁
error:
		ref = pthread_mutex_unlock(&(dbp_info[id].dbp_mutex)); //解锁
		if (ref){   
			DBSPRINTF("mutex unlock fail!\n");
			ret = -ERR_SYS;
		}
	}

	return(ret);        

}
/******************************************************************************
*	函数:	dbs_insert
*	功能:	写多重记录数据（在该数据关键字的指定位置插入单条记录）
*	参数:	id				-	数据库标号
			key_size		-	关键字长度
			key             -   关键字
            set             -   多重记录定位
            flag            -   多重记录插入标志
            data_size       -   数据大小
            data            -   数据
*	返回:	0				-	成功
			-ERR_INVA		-   接口参数配置错误；
			-ERR_NOFILE		-	数据库未打开
			-ERR_NOFUN		-	不具备写权限
			-ERR_NODISK		-	数据长度越限
			-ERR_TIMEOUT	-	超时
			-ERR_NORECORD	-	记录不存在
			-ERR_NOMEM		-   内存错误
*	说明:	插入记录--如果该记录不存在，在其后插入的请求将返回-ERR_NORECORD错误
 ******************************************************************************/
int dbs_insert(u8 id, u8 key_size, void *key, dbs_set_t *set, u8 flag, u16 data_size, void  *data)
{
	int ret;
	int ref,i;
	
	DBC *cursorp_w;
	DBT key_w;                            //数据库记录结构
	DBT record_w;
	dbs_set_t set_w;
	u32 num_total;	
	u8 data_middle[CFG_DBS_RECORD_MAX];   //中间数据
	
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){  
		ret = -ERR_INVAL;
		goto error1;
	}
	
	if (dbp[id] == NULL){                 //数据库未打开
		ret = -ERR_NOFILE;
		goto error1;
	}

	//判断该标号数据库是否有写权限
	if (dbp_info[id].dbp_mode == 1){      //已只读方式打开
		ret = -ERR_NOFUN;
		goto error1;
	}
	//判断关键字参数是否越限
	if (key_size > CFG_DBS_KEYSIZE_MAX){
		ret = -ERR_INVAL;
		goto error1;
	}

	//判断写数据是否越限
	if (data_size > CFG_DBS_RECORD_MAX){
		ret = -ERR_NODISK;
		goto error1;
	}
	//初始化写游标
	ref = dbp[id]->cursor(dbp[id], NULL, &cursorp_w, DB_WRITECURSOR);        //为写游标
	if (ref){                           
		ret = -ERR_TIMEOUT;
		goto error1;
	}
	memset(data_middle, 0, CFG_DBS_RECORD_MAX);
	memset(&key_w, 0, sizeof(DBT));
	memset(&record_w, 0, sizeof(DBT));
	key_w.data = key;
	key_w.size = key_size;

	//插入（添加）到多重记录的头部
	if ((set->whence == DBS_SEEK_SET) && (set->offset == 0) && (flag == DBS_SEEK_FORWARD) ){
		memset(&key_w, 0, sizeof(DBT));
    	memset(&record_w, 0, sizeof(DBT));
    	key_w.data = key;
    	key_w.size = key_size;
    	record_w.data = data;
    	record_w.size = data_size;
    	ref = cursorp_w->put(cursorp_w, &key_w, &record_w, DB_KEYFIRST);
    	if (ref == EACCES){
    		ret = -ERR_NOFUN;
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
    	}
	//插入（添加）到多重记录的尾部
	}else if ((set->whence == DBS_SEEK_END) && (set->offset == 0) && (flag == DBS_SEEK_BACKWARD)){
		memset(&key_w, 0, sizeof(DBT));
    	memset(&record_w, 0, sizeof(DBT));
    	key_w.data = key;
    	key_w.size = key_size;
    	record_w.data = data;
    	record_w.size = data_size;
    	ref = cursorp_w->put(cursorp_w, &key_w, &record_w, DB_KEYLAST);
    	if (ref == EACCES){
    		ret = -ERR_NOFUN;
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
    	}
	//需定位新的插入位置
	}else{
    	//整理接口
    	if (set->whence == DBS_SEEK_END){
    		ref = dbs_count(id,key_size,key,&num_total);   //多重记录的总记录数
    		if (ref){
    			ret = -ERR_NORECORD;
    			goto error;
    		}
    		set_w.whence = DBS_SEEK_SET;
    		set_w.offset = num_total - 1 - set->offset;
    		if(set_w.offset < 0){
    			ret = -ERR_NORECORD;
    			goto error;
    		}
    	}else if (set->whence == DBS_SEEK_SET){
    		set_w.whence = DBS_SEEK_SET;
    		set_w.offset = set->offset;
    	}else{
    		ret = -ERR_INVAL;
    		goto error;
    	}
    	//定位到新的读写位置
		record_w.data = data_middle;
		record_w.ulen = CFG_DBS_RECORD_MAX;
		record_w.flags = DB_DBT_USERMEM;
		ref = cursorp_w->get(cursorp_w, &key_w, &record_w, DB_SET);
		if (ref == DB_NOTFOUND){                          //未找到符合条件的记录
			ret = -ERR_NORECORD;
			goto error;
		}else if (ref){
			ret = -ERR_TIMEOUT;
			goto error;
		}
    	for (i = 0;i < set_w.offset;i++){                 //顺序偏移到新的读写位置
    		memset(data_middle,0,CFG_DBS_RECORD_MAX);
    		memset(&key_w, 0, sizeof(DBT));
    		memset(&record_w, 0, sizeof(DBT));
    		key_w.data = key;
    		key_w.size = key_size;
    		record_w.data = data_middle;
    		record_w.ulen = CFG_DBS_RECORD_MAX;
			record_w.flags = DB_DBT_USERMEM;
    		ref = cursorp_w->get(cursorp_w, &key_w, &record_w, DB_NEXT_DUP);
    		if (ref == DB_NOTFOUND){                      //未找到符合条件的记录
				ret = -ERR_NORECORD;
				goto error;
    		}else if (ref){
    			ret = -ERR_TIMEOUT;
    			goto error;
    		}
    	}
		//确定在新的读写位置的向前插入
    	if (flag == DBS_SEEK_FORWARD){                    //新的读写位置向前插入
        	memset(&key_w, 0, sizeof(DBT));
        	memset(&record_w, 0, sizeof(DBT));
        	key_w.data = key;
        	key_w.size = key_size;
        	record_w.data = data;
        	record_w.size = data_size;
        	ref = cursorp_w->put(cursorp_w, &key_w, &record_w, DB_BEFORE);
        	if (ref == EACCES){
    			ret = -ERR_NOFUN;
    			goto error;
    		}else if (ref){
    			ret = -ERR_TIMEOUT;
    			goto error;
    		}
    	}else if (flag == DBS_SEEK_BACKWARD){             //新的读写位置向后插入
    		memset(&key_w, 0, sizeof(DBT)); 
        	memset(&record_w, 0, sizeof(DBT));
        	key_w.data = key;
        	key_w.size = key_size;
        	record_w.data = data;
        	record_w.size = data_size;
        	ref = cursorp_w->put(cursorp_w, &key_w, &record_w, DB_AFTER);
        	if (ref == EACCES){
    			ret = -ERR_NOFUN;
    			goto error;
    		}else if (ref){
    			ret = -ERR_TIMEOUT;
    			goto error;
    		}
    	}else{
    		//接口参数设置错误
    		ret = -ERR_INVAL;
    		goto error;
    	}
	}	
	ret = 0;
error:
	//关闭游标
	ref = cursorp_w->close(cursorp_w);
	if (ref){
		ret = -ERR_SYS;
	}
error1:
	return (ret);
	
}
/******************************************************************************
 *	函数:	dbs_write
 *	功能:	写多重记录数据（在该数据关键字的指定位置修改替换单条记录）
 *	参数:	id				-	数据库标号
 			key_size		-	关键字长度
 			key             -   关键字
            set             -   多重记录定位
            data_size       -   数据大小
            data            -   数据
 *	返回:	0				-	成功
 			-ERR_INVA		-   接口参数配置错误；
 			-ERR_NOFILE		-	数据库未打开
 			-ERR_NOFUN		-	不具备写权限
 			-ERR_NODISK		-	数据长度越限
 			-ERR_TIMEOUT	-	超时
 			-ERR_NORECORD	-	数据纪录不存在
 *	说明:	修改记录，但是如果修改第一条记录，但是记录不存在不算成错误，直接添加第一条
******************************************************************************/
int dbs_write(u8 id, u8 key_size, void *key, dbs_set_t *set,u16 data_size, void  *data)
{
	int ret;
	int ref,i;
	
	DBC *cursorp_w;
	DBT key_w;         
	DBT record_w;
	dbs_set_t set_w;
	u32 num_total;
	u8 data_middle[CFG_DBS_RECORD_MAX];
	
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){  
		ret = -ERR_INVAL;
		goto error1;
	}
	
	//判断是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error1;
	}
	//判断该标号数据库是否有写权限
	if (dbp_info[id].dbp_mode == 1){             //已只读方式打开
		ret = -ERR_NOFUN;
		goto error1;
	}
	//判断关键字参数是否越限
	if (key_size > CFG_DBS_KEYSIZE_MAX){
		ret = -ERR_INVAL;
		goto error1;
	}

	//判断写数据是否越限
	if (data_size > CFG_DBS_RECORD_MAX){
		ret = -ERR_NODISK;
		goto error1;
	}
	//初始化写游标
	ref = dbp[id]->cursor(dbp[id], NULL, &cursorp_w, DB_WRITECURSOR);        //为写游标
	if (ref){                           
		ret = -ERR_TIMEOUT;
		goto error1;
	}
	
	memset(data_middle,0,CFG_DBS_RECORD_MAX);
	memset(&key_w, 0, sizeof(DBT));
	memset(&record_w, 0, sizeof(DBT));
	key_w.data = key;
	key_w.size = key_size;
	record_w.data = data_middle;
	record_w.ulen = CFG_DBS_RECORD_MAX;
	record_w.flags = DB_DBT_USERMEM;
	
	//整理接口
	if (set->whence == DBS_SEEK_SET){
		ref = dbs_count(id,key_size,key,&num_total);           //多重记录的总记录数
		if ((ref == -ERR_NORECORD) && (set->offset == 0)){     //第一条记录不存在,允许添加
			memset(&key_w, 0, sizeof(DBT));
        	memset(&record_w, 0, sizeof(DBT));
        	key_w.data = key;
        	key_w.size = key_size;
        	record_w.data = data;
        	record_w.size = data_size;
        	ref = cursorp_w->put(cursorp_w, &key_w, &record_w, DB_KEYFIRST);
        	if (ref == EACCES){
        		ret = -ERR_NOFUN;
        		goto error;
        	}else if (ref){
        		ret = -ERR_NORECORD;
        		goto error;
        	}
        	//关闭游标
        	ref = cursorp_w->close(cursorp_w);
        	if (ref){
        		ret = -ERR_SYS;
        	} 
        	ret = 0;
        	return(ret);
		}else if(ref){
			ret = -ERR_SYS;
			goto error;
		}
		if (num_total > 0){
			set_w.whence = DBS_SEEK_SET;
			set_w.offset = set->offset;
		}else{
			ret = -ERR_NORECORD;
			goto error;
		}
	}else if (set->whence == DBS_SEEK_END){
		ref = dbs_count(id,key_size,key,&num_total);          //多重记录的总记录数
		if (ref){
			ret = -ERR_NORECORD;
			goto error;
		}
		if (num_total > 0){
    		set_w.whence = DBS_SEEK_SET;
    		set_w.offset = num_total - 1 - set->offset;
    		if(set_w.offset < 0){
    			ret = -ERR_NORECORD;
    			goto error;
    		}
		}else{
			ret = -ERR_NORECORD;
    		goto error;
		}
	}else{
		ret = -ERR_INVAL;
		goto error;
	}
	//定位新的读写位置
	if (set_w.whence == DBS_SEEK_SET){
		ref = cursorp_w->get(cursorp_w, &key_w, &record_w,DB_SET);
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
			ret = -ERR_NORECORD;
			goto error;
		}else if (ref){
			ret = -ERR_TIMEOUT;
			goto error;
		}
		if (set_w.offset == 0){          //偏移量为0
			memset(&key_w, 0, sizeof(DBT));
			memset(&record_w,0,sizeof(DBT));
			key_w.data = key;
			key_w.size = key_size;
			record_w.data = data;
			record_w.size = data_size;
			ref = cursorp_w->put(cursorp_w, &key_w, &record_w, DB_CURRENT);
			if (ref == EACCES){
    			ret = -ERR_NOFUN;
    			goto error;
    		}else if (ref){
    			ret = -ERR_TIMEOUT;
    			goto error;
    		}
		}
		else if (set_w.offset > 0){
			//将游标移到偏移量所指的读写位置
			for (i = 0;i < set_w.offset;i ++){
				memset(data_middle,0,CFG_DBS_RECORD_MAX);
				memset(&key_w, 0, sizeof(DBT));
				memset(&record_w, 0, sizeof(DBT));
				key_w.data = key;
				key_w.size = key_size;
				record_w.data = data_middle;
				record_w.ulen = CFG_DBS_RECORD_MAX;
				record_w.flags = DB_DBT_USERMEM;
				ref = cursorp_w->get(cursorp_w, &key_w, &record_w,DB_NEXT_DUP);
				if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
        			ret = -ERR_NORECORD;
        			goto error;
        		}else if (ref){
        			ret = -ERR_TIMEOUT;
        			goto error;
        		}
			}
			//将数据写入新的读写位置
			memset(&key_w, 0, sizeof(DBT));
			memset(&record_w, 0, sizeof(DBT));
			key_w.data = key;
			key_w.size = key_size;
			record_w.data = data;
			record_w.size = data_size;
			ref = cursorp_w->put(cursorp_w, &key_w, &record_w, DB_CURRENT);
			if (ref == EACCES){
    			ret = -ERR_NOFUN;
    			goto error;
    		}else if (ref){
    			ret = -ERR_TIMEOUT;
    			goto error;
    		}
		}
		else{
			DBSPRINTF("interface fail!\n");
			ret = -ERR_INVAL;
			goto error;
		}
	}
	
	ret = 0;
error:
	//关闭游标
	ref = cursorp_w->close(cursorp_w);
	if (ref){
		ret = -ERR_SYS;
	} 
error1:
	return (ret);
}
/******************************************************************************
  *	函数:	dbs_multiple_count
  *	功能:	统计多重记录数据的记录条数
  *	参数:	id				-	数据库标号
  			key_size		-	关键字长度
  			key             -   关键字
            count_num       -	多重记录条数（返回）
  *	返回:	0				-	成功
  			-ERR_INVA		-   接口参数配置错误；
  			-ERR_NOFILE		-	数据库未打开
  			-ERR_NODISK		-	数据长度越限
  			-ERR_TIMEOUT	-	超时
  			-ERR_NORECORD	-	数据纪录不存在
  *	说明:	无
 ******************************************************************************/
int dbs_count(u8 id, u8 key_size, void *key,u32 *count_num)
{
	int ref;
	int ret;
	
	DBC *cursorp_c;
	DBT key_c;            //数据库记录结构
	DBT record_c;
	
	u8 data_middle[CFG_DBS_RECORD_MAX];
	db_recno_t *countp;   //返回记录个数指针
    countp = count_num; 
    
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){  
		ret = -ERR_INVAL;
		goto error1;
	}
	
	//判断是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error1;
	}
	
	//判断关键字参数是否越限
	if (key_size > CFG_DBS_KEYSIZE_MAX){
		ret = -ERR_INVAL;
		goto error1;
	}

	//初始化游标
	if (dbp[id]->cursor(dbp[id], NULL, &cursorp_c, 0)){    //初始化游标
		ret = -ERR_TIMEOUT;
		goto error1;
    }
	
	//将游标定位到所要统计记录个数的KEY
	memset(&key_c, 0, sizeof(DBT));
	memset(&record_c, 0, sizeof(DBT));
	key_c.data = key;
	key_c.size = key_size;
	record_c.data = data_middle;
	record_c.ulen = CFG_DBS_RECORD_MAX;
	record_c.flags = DB_DBT_USERMEM;
	ref = cursorp_c->get(cursorp_c, &key_c, &record_c,DB_SET);
	if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
		ret = -ERR_NORECORD;
		goto error;
	}else if (ref){
		ret = -ERR_TIMEOUT;
		goto error;
	}
	
	//统计记录个数
	ref = cursorp_c->count(cursorp_c,countp, 0);
	if (ref) {
		ret = -ERR_TIMEOUT;
		goto error;
	}
	ret = 0;
error:
	//关闭游标
	ref = cursorp_c->close(cursorp_c);
	if (ref){
		ret = -ERR_SYS;
	} 
error1:
	return (ret);
}

/******************************************************************************
  *	函数:	dbs_read
  *	功能:	读多重记录数据项中的单条记录数据
  *	参数:	id				-	数据库标号
  			key_size		-	关键字长度
  			key             -   关键字
            set             -	多重记录定位
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
  *	说明:	无
 ******************************************************************************/
int dbs_read(u8 id, u8 key_size, void *key, dbs_set_t *set, u16 max_size, u16 *data_size, void  *data)
{
	int ret;
	int ref,i;
	u32 num_total;
	
	DBC *cursorp_r;
	DBT key_r;         //数据库记录结构
	DBT record_r;
	dbs_set_t set_r;
	u8 data_middle[CFG_DBS_RECORD_MAX];
	
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){  
		ret = -ERR_INVAL;
		goto error1;
	}
	
	//判断数据库是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error1;
	}
	
	//判断关键字参数是否越限
	if (key_size > CFG_DBS_KEYSIZE_MAX){
		ret = -ERR_INVAL;
		goto error1;
	}
	
	if ((max_size < 0) || (max_size > CFG_DBS_RECORD_MAX)){
		ret = -ERR_INVAL;
		goto error1;
	}
	
	//初始化游标
	ref = dbp[id]->cursor(dbp[id], NULL, &cursorp_r, 0);
	if (ref){        
		ret = -ERR_TIMEOUT;
		goto error1;
    }
	//整理接口
	if (set->whence == DBS_SEEK_END){
		ref = dbs_count(id, key_size, key, &num_total);   //多重记录的总记录数
		if (ref){
			ret = -ERR_NORECORD;
			goto error;
		}
		set_r.whence = DBS_SEEK_SET;
		set_r.offset = num_total - 1 - set->offset;
		if(set_r.offset < 0){
			ret = -ERR_NORECORD;
			goto error;
		}
	}else if (set->whence == DBS_SEEK_SET){
		set_r.whence = DBS_SEEK_SET;
		set_r.offset = set->offset;
	}else{
		ret = -ERR_INVAL;
		goto error;
	}
	memset(data_middle, 0, CFG_DBS_RECORD_MAX);
	memset(&key_r, 0, sizeof(DBT));
	memset(&record_r, 0, sizeof(DBT));
	key_r.data = key;
	key_r.size = key_size;

	if (set_r.offset == 0){
		//偏移量为0,读起始记录
		record_r.data = data;
		record_r.ulen = max_size;
		record_r.flags = DB_DBT_USERMEM;
		ref = cursorp_r->get(cursorp_r, &key_r, &record_r, DB_SET);
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
    		ret = -ERR_NORECORD;
    		goto error;
    	}else if (ref == DB_BUFFER_SMALL){
    		ret = -ERR_NOMEM;
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
		}else{
			*data_size = record_r.size;
		}
	}else if(set_r.offset > 0){
		//将游标定位到所要读写的KEY
		record_r.data = data_middle;
		record_r.ulen = CFG_DBS_RECORD_MAX;
		record_r.flags = DB_DBT_USERMEM;
		ref = cursorp_r->get(cursorp_r, &key_r, &record_r,DB_SET);
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
    		ret = -ERR_NORECORD;
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
    	}
		for (i = 0;i <(set_r.offset - 1);i++){
			memset(data_middle, 0, CFG_DBS_RECORD_MAX);
			memset(&key_r, 0, sizeof(DBT));
			memset(&record_r, 0, sizeof(DBT));
			key_r.data = key;
			key_r.size = key_size;
			record_r.data = data_middle;
			record_r.ulen = CFG_DBS_RECORD_MAX;
			record_r.flags = DB_DBT_USERMEM;
			ref = cursorp_r->get(cursorp_r, &key_r, &record_r, DB_NEXT_DUP);
			if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
        		ret = -ERR_NORECORD;
        		goto error;
        	}else if (ref){
        		ret = -ERR_TIMEOUT;
        		goto error;
        	}
		}
		//读数据
		memset(&key_r, 0, sizeof(DBT));
		memset(&record_r, 0, sizeof(DBT));
		key_r.data = key;
		key_r.size = key_size;
		record_r.data = data;
		record_r.ulen = max_size;
		record_r.flags = DB_DBT_USERMEM;
		ref = cursorp_r->get(cursorp_r, &key_r, &record_r,DB_NEXT_DUP);
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
    		ret = -ERR_NORECORD;
    		goto error;
    	}else if (ref == DB_BUFFER_SMALL){
    		ret = -ERR_NOMEM;
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
		}else{
			*data_size = record_r.size;
		}
    }
	ret = 0;
error:
	//关闭游标
	ref = cursorp_r->close(cursorp_r);
	if (ref){
		ret = -ERR_SYS;
	} 
error1:
	return (ret);
}

/******************************************************************************
   *	函数:	dbs_read_bulk
   *	功能:	读多重记录数据项中的多条记录数据
   *	参数:	id				-	数据库标号
				key_size		-	关键字长度
				key             -   关键字
				from            -	读取多重记录一组记录起始定位
				to              -	结束定位
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
   *	说明:	数据返回结构是长度+数据的形式
   				from.whence =DBS_SEEK_SET表示顺序读取多重记录
   				from.whence =DBS_SEEK_END表示逆序读取多重记录
 ******************************************************************************/
int dbs_read_bulk
(u8 id, u8 key_size, void *key, dbs_set_t *from, dbs_set_t *to, u16 max_size, u16 *data_size, void *data)
{
	int ret;
	int ref,i;
	
	DBC *cursorp_r;
	DBT key_r;                   //数据库记录结构
	DBT record_r;
	u8 data_middle[CFG_DBS_RECORD_MAX];
	u8 flag;
	
	u16 data_length;             //单条记录的数据长度
	u32 num_total = 0;           //key下记录总条数
	u16 record_length = 0;       //数据纪录总长度
	dbs_set_t set;               //定位的起始位置
	u16 record_total = 0;        //要读取的记录条数
	
	void *point;
	point = data;
	memset(&set,0,sizeof(dbs_set_t));
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){ 
		ret = -ERR_INVAL;
		goto error1;
	}
	//判断数据库是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error1;
	}
	
	//判断关键字参数是否越限
	if (key_size > CFG_DBS_KEYSIZE_MAX){
		ret = -ERR_INVAL;
		goto error1;
	}
	
	if ((max_size < 0) || (max_size > CFG_DBS_RECORD_MAX)){
		ret = -ERR_INVAL;
		goto error1;
	}
	
	//初始化游标
	ref = dbp[id]->cursor(dbp[id], NULL, &cursorp_r, 0);
	if (ref){        
		ret = -ERR_TIMEOUT;
		goto error1;
    }
	
	memset(&key_r, 0, sizeof(DBT));
	memset(&record_r, 0, sizeof(DBT));
	key_r.data = key;
	key_r.size = key_size;
	
	if (max_size < 0){
		ret = -ERR_INVAL;
		goto error;
	} 
	
	//判断接口是否正确---整理接口
	ref = dbs_count(id, key_size, key, &num_total);                      			//多重记录的总记录数
	if (ref){
		ret = -ERR_NORECORD;
		goto error;
	}
	
	if ((from->offset >= num_total) || (to->offset >= num_total)){      			//读记录位置超出总记录数
		ret = -ERR_NORECORD;
		goto error;
	}
	if ((from->whence == DBS_SEEK_SET) && (to->whence == DBS_SEEK_SET)){   
		if (from->offset > to->offset){								  			//起始位置与结束位置颠倒
			ret = -ERR_INVAL;
			goto error;
		}
		set.whence = DBS_SEEK_SET;
		set.offset = from->offset;
		flag = DB_NEXT_DUP;
		record_total = to->offset - from->offset + 1;
	}
	else if ((from->whence == DBS_SEEK_END) && (to->whence == DBS_SEEK_END)){   
		if (from->offset > to->offset){                              			//起始位置与结束位置颠倒
			ret = -ERR_INVAL;
			goto error;
		}
		set.whence = DBS_SEEK_SET;
		set.offset = num_total - 1 - from->offset;
		flag = DB_PREV_DUP;
		record_total = to->offset - from->offset + 1;
	}
	else if ((from->whence == DBS_SEEK_SET) && (to->whence == DBS_SEEK_END)){   //起始位置与结束位置颠倒
		if ((from->offset + to->offset) >= num_total){
			ret = -ERR_INVAL;
			goto error;
		}
		set.whence = DBS_SEEK_SET;
		set.offset = from->offset;
		flag = DB_NEXT_DUP;
		record_total = num_total - to->offset - from->offset;
	}
	else if ((from->whence == DBS_SEEK_END) && (to->whence == DBS_SEEK_SET)){   //起始位置与结束位置颠倒
		if ((from->offset + to->offset) >= num_total){
			ret = -ERR_INVAL;
			goto error;
		}
		set.whence = DBS_SEEK_SET;
		set.offset = num_total - 1 - from->offset;
		flag = DB_PREV_DUP;
		record_total = num_total - to->offset - from->offset;
	}
	else{
		ret = -ERR_INVAL;
		goto error;
	}
	//读单条且为第一条记录
	if((set.offset == 0) && (record_total-1 == 0)){
		memset(&key_r, 0, sizeof(DBT));
		memset(&record_r, 0, sizeof(DBT));
		key_r.data = key;
		key_r.size = key_size;
		record_r.data = point + 2;               //数据长度两个字节
		record_r.ulen = max_size - record_length;
		record_r.flags = DB_DBT_USERMEM;
		ref = cursorp_r->get(cursorp_r, &key_r, &record_r,DB_SET);   //读第一条
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
			ret = -ERR_NORECORD;
			goto error;
		}else if (ref == DB_BUFFER_SMALL){
			ret = -ERR_NOMEM;
			goto error;
		}else if (ref){
			ret = -ERR_TIMEOUT;
			goto error;
		}else{
			data_length = record_r.size;
			memcpy(point, &data_length, 2);		//数据长度两个字节
			point += 2 + data_length;			//数据长度两个字节
			record_length += 2 + data_length;	//数据长度两个字节
		}
	}
	//非起始位置数据将游标定位到起始位置
	else{
		record_r.data = data_middle;
		record_r.ulen = CFG_DBS_RECORD_MAX;
		record_r.flags = DB_DBT_USERMEM;
		ref = cursorp_r->get(cursorp_r, &key_r, &record_r,DB_SET);
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
			ret = -ERR_NORECORD;
			goto error;
		}else if (ref){
			ret = -ERR_TIMEOUT;
			goto error;
		}
		if (set.offset > 0){
			for (i = 0;i < set.offset;i ++){
				memset(data_middle,0,CFG_DBS_RECORD_MAX);
				memset(&key_r, 0, sizeof(DBT));
				memset(&record_r, 0, sizeof(DBT));
				key_r.data = key;
				key_r.size = key_size;
				record_r.data = data_middle;
				record_r.ulen = CFG_DBS_RECORD_MAX;
				record_r.flags = DB_DBT_USERMEM;
				ref = cursorp_r->get(cursorp_r, &key_r, &record_r,DB_NEXT_DUP);
				if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
					ret = -ERR_NORECORD;
					goto error;
				}else if (ref){
					ret = -ERR_TIMEOUT;
					goto error;
				}
			}
		}
		//读数据
		memset(&key_r, 0, sizeof(DBT));
		memset(&record_r, 0, sizeof(DBT));
		key_r.data = key;
		key_r.size = key_size;
		record_r.data = point + 2;               //数据长度两个字节
		record_r.ulen = max_size - record_length;
		record_r.flags = DB_DBT_USERMEM;
		ref = cursorp_r->get(cursorp_r, &key_r, &record_r,DB_CURRENT);   //读第一条
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
			ret = -ERR_NORECORD;
			goto error;
		}else if (ref == DB_BUFFER_SMALL){
			ret = -ERR_NOMEM;
			goto error;
		}else if (ref){
			ret = -ERR_TIMEOUT;
			goto error;
		}else{
			data_length = record_r.size;
			memcpy(point, &data_length, 2);		//数据长度两个字节
			point += 2 + data_length;			//数据长度两个字节
			record_length += 2 + data_length;	//数据长度两个字节
		}
		if(record_total-1 > 0){
			for (i = 0;i< record_total-1;i ++){
				memset(&key_r, 0, sizeof(DBT));
				memset(&record_r, 0, sizeof(DBT));
				key_r.data = key;
				key_r.size = key_size;
				record_r.data = point + 2;               //数据长度两个字节
				record_r.ulen = max_size - record_length;
				record_r.flags = DB_DBT_USERMEM;
				ref = cursorp_r->get(cursorp_r, &key_r, &record_r,flag);
				if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
					ret = -ERR_NORECORD;
					goto error;
				}else if (ref == DB_BUFFER_SMALL){
					ret = -ERR_NOMEM;
					goto error;
				}else if (ref){
					ret = -ERR_TIMEOUT;
					goto error;
				}else{
					data_length = record_r.size;
					memcpy(point, &data_length, 2);		//数据长度两个字节
					point += 2 + data_length;			//数据长度两个字节
					record_length += 2 + data_length;	//数据长度两个字节
				}
			}
		}
	}
	*data_size = record_length;
	ret = 0;
error:
	//关闭游标
	ref = cursorp_r -> close(cursorp_r);
	if (ref){
		ret = -ERR_SYS;
	} 
error1:
	return (ret);
}

/******************************************************************************
   *	函数:	dbs_delete
   *	功能:	删除多重记录数据的单条记录
   *	参数:	id				-	数据库标号
				key_size		-	关键字长度
				key             -   关键字
				set             -	多重记录定位
   *	返回:	0				-	成功
				-ERR_INVA		-   接口参数配置错误；
				-ERR_NOINIT		-	数据库模块未初始化
				-ERR_NOFILE		-	数据库未打开（或已关闭）
				-ERR_NOFUN		-	没有此功能（不能删除）
				-ERR_TIMEOUT	-	超时
				-ERR_NORECORD	-	数据纪录不存在
   *	说明:	无
 ******************************************************************************/
int  dbs_delete(u8 id, u8 key_size, void *key, dbs_set_t *set)
{
	int ret;
	int ref,i;
	
	DBC *cursorp_d;
	DBT key_d;         //数据库记录结构
	DBT record_d;
	dbs_set_t set_d;
	u8 data_middle[CFG_DBS_RECORD_MAX];
	u32 num_total;
	
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){  
		ret = -ERR_INVAL;
		goto error1;
	}
	//判断数据库是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error1;
	}

	//判断关键字参数是否越限
	if (key_size > CFG_DBS_KEYSIZE_MAX){
		ret = -ERR_INVAL;
		goto error1;
	}
	//初始化游标
	ref = dbp[id]->cursor(dbp[id], NULL, &cursorp_d, DB_WRITECURSOR);
	if (ref){        
		ret = -ERR_TIMEOUT;
		goto error1;
    }
	
	//整理接口
	if (set->whence == DBS_SEEK_END){
		ref = dbs_count(id,key_size,key,&num_total);   //多重记录的总记录数
		if (ref){
			ret = -ERR_NORECORD;
			goto error;
		}
		set_d.whence = DBS_SEEK_SET;
		set_d.offset = num_total - 1 - set->offset;
		if(set_d.offset < 0){
			ret = -ERR_NORECORD;
			goto error;
		}
	}else if (set->whence == DBS_SEEK_SET){
		set_d.whence = DBS_SEEK_SET;
		set_d.offset = set->offset;
	}else{
		ret = -ERR_INVAL;
		goto error;
	}
	memset(&key_d, 0, sizeof(DBT));
	memset(&record_d, 0, sizeof(DBT));
	key_d.data = key;
	key_d.size = key_size;
	
	//将游标移到偏移量所指的读写位置
	record_d.data = data_middle;
	record_d.ulen = CFG_DBS_RECORD_MAX;
	record_d.flags = DB_DBT_USERMEM;
	ref = cursorp_d->get(cursorp_d, &key_d, &record_d,DB_SET);
	if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
		ret = -ERR_NORECORD;
		goto error;
	}else if (ref){
		ret = -ERR_TIMEOUT;
		goto error;
	}
	for (i = 0;i < set_d.offset;i ++){
		memset(data_middle,0,CFG_DBS_RECORD_MAX);
		memset(&key_d, 0, sizeof(DBT));
		memset(&record_d, 0, sizeof(DBT));
		key_d.data = key;
		key_d.size = key_size;
		record_d.data = data_middle;
		record_d.ulen = CFG_DBS_RECORD_MAX;
		record_d.flags = DB_DBT_USERMEM;
		ref = cursorp_d->get(cursorp_d, &key_d, &record_d,DB_NEXT_DUP);
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
    		ret = -ERR_NORECORD;
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
    	}
	}
	//删除数据
	memset(&key_d, 0, sizeof(DBT));
	memset(&record_d, 0, sizeof(DBT));
	key_d.data = key;
	key_d.size = key_size;
	ref = cursorp_d->del(cursorp_d, 0);
	if (ref == DB_KEYEMPTY ){
		ret = -ERR_NORECORD;
		goto error;
	}
	else if (ref){
		ret = -ERR_SYS;
		goto error;
	}
	
	ret = 0;
	DBSPRINTF("delete record success!\n");
error:
	//关闭游标
	ref = cursorp_d->close(cursorp_d);
	if (ref){
		ret = -ERR_SYS;
	}
error1:
	return (ret);
}

/******************************************************************************
    *	函数:	dbs_delete_bulk
    *	功能:	删除多重记录数据的多条记录
    *	参数:	id				-	数据库标号
 				key_size		-	关键字长度
 				key             -   关键字
 				from            -	读取多重记录一组记录起始定位
 				to              -	结束定位
    *	返回:	0				-	成功
 				-ERR_INVA		-   接口参数配置错误；
 				-ERR_NOINIT		-	数据库模块未初始化
 				-ERR_NOFILE		-	数据库未打开
 				-ERR_NOFUN      -   没有此功能（不能删除）
 				-ERR_TIMEOUT	-	超时
 				-ERR_NORECORD	-	数据纪录不存在
    *	说明:	无
 ******************************************************************************/
int dbs_delete_bulk(u8 id, u8 key_size, void *key, dbs_set_t *from, dbs_set_t *to)
{
	int ret;
	int ref,i;
	
	DBC *cursorp_d;
	DBT key_d;           //数据库记录结构
	DBT record_d;
	u8 data_middle[CFG_DBS_RECORD_MAX];
	
	u32 num_total = 0;           //key下记录总条数
	dbs_set_t set;                    //定位的起始位置
	u8 flag;
	u16 record_total = 0;        //要读取的记录条数
	memset(&set,0,sizeof(dbs_set_t));
	
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){ 
		ret = -ERR_INVAL;
		goto error1;
	}
	
	//判断数据库是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error1;
	}
	
	//判断关键字参数是否越限
	if (key_size > CFG_DBS_KEYSIZE_MAX){
		ret = -ERR_INVAL;
		goto error1;
	}

	//初始化游标
	ref = dbp[id]->cursor(dbp[id], NULL, &cursorp_d, DB_WRITECURSOR);
	if (ref){        
		ret = -ERR_SYS;
		goto error1;
    }
	
	memset(&key_d, 0, sizeof(DBT));
	memset(&record_d, 0, sizeof(DBT));
	key_d.data = key;
	key_d.size = key_size;
	
	
	//判断接口是否正确---整理接口
	ref = dbs_count(id, key_size,key,&num_total);             //多重记录的总记录数
	if (ref == -ERR_NORECORD){
		ret = -ERR_NORECORD;
		goto error;
	}
	else if (ref){
		ret = -ERR_SYS;
		goto error;
	}
	if ((from->offset >= num_total) || (to->offset >= num_total)){            //读记录位置超出总记录数
		ret = -ERR_NORECORD;
		goto error;
	}
	if ((from->whence == DBS_SEEK_SET) && (to->whence == DBS_SEEK_SET)){   
		if (from->offset > to->offset){                                    //起始位置与结束位置颠倒
			ret = -ERR_INVAL;
			goto error;
		}
		set.whence = DBS_SEEK_SET;
		set.offset = from->offset;
		flag = DB_NEXT_DUP;
		record_total = to->offset - from->offset + 1;
	}
	else if ((from->whence == DBS_SEEK_END) && (to->whence == DBS_SEEK_END)){   //起始位置与结束位置颠倒
		if (from->offset > to->offset){
			ret = -ERR_INVAL;
			goto error;
		}
		set.whence = DBS_SEEK_SET;
		set.offset = num_total - to->offset - 1;
		flag = DB_PREV_DUP;
		record_total = to->offset - from->offset + 1;
	}
	else if ((from->whence == DBS_SEEK_SET) && (to->whence == DBS_SEEK_END)){   //起始位置与结束位置颠倒
		if ((from->offset + to->offset) >= num_total){
			ret = -ERR_INVAL;
			goto error;
		}
		set.whence = DBS_SEEK_SET;
		set.offset = from->offset;
		flag = DB_NEXT_DUP;
		record_total = num_total - to->offset - from->offset;
	}
	else if ((from->whence == DBS_SEEK_END) && (to->whence == DBS_SEEK_SET)){   //起始位置与结束位置颠倒
		if ((from->offset + to->offset) >= num_total){
			ret = -ERR_INVAL;
			goto error;
		}
		set.whence = DBS_SEEK_SET;
		set.offset = to->offset;
		flag = DB_PREV_DUP;
		record_total = num_total - to->offset - from->offset;
	}
	else{
		ret = -ERR_INVAL;
		goto error;
	}
	
	//将游标定位到起始位置
	if(set.offset == 0){                                //偏移量为0
		record_d.data = data_middle;
		record_d.ulen = CFG_DBS_RECORD_MAX;
		record_d.flags = DB_DBT_USERMEM;
		ref = cursorp_d->get(cursorp_d, &key_d, &record_d,DB_SET);
		if (ref == DB_NOTFOUND){                        //未找到符合条件的记录
    		ret = -ERR_NORECORD; 
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
    	}else{
			record_total --;
			//删除记录
			ref = cursorp_d->del(cursorp_d, 0);        //删除该游标所指的记录
    		if (ref == EPERM ){                        //以只读方式打开的数据库
    			DBSPRINTF("del has no right!\n");
    			ret = -ERR_NOFUN;
    			goto error;
    		}
    		else if (ref){
       			DBSPRINTF("del record fail!\n");
       			ret = -ERR_SYS;
       			goto error;
    		}else{
    			DBSPRINTF("del success!\n");
    			//goto hull;
			}
		}
	}else if (set.offset > 0){
		//将游标移到偏移量所指的读写位置
		record_d.data = data_middle;
		record_d.ulen = CFG_DBS_RECORD_MAX;
		record_d.flags = DB_DBT_USERMEM;
		ref = cursorp_d->get(cursorp_d, &key_d, &record_d,DB_SET);
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
    		ret = -ERR_NORECORD;
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
    	}
		for (i = 0;i < (set.offset - 1);i ++){
			memset(data_middle, 0, CFG_DBS_RECORD_MAX);
			memset(&key_d, 0, sizeof(DBT));
			memset(&record_d, 0, sizeof(DBT));
			key_d.data = key;
			key_d.size = key_size;
			record_d.data = data_middle;
			record_d.ulen = CFG_DBS_RECORD_MAX;
			record_d.flags = DB_DBT_USERMEM;
			ref = cursorp_d->get(cursorp_d, &key_d, &record_d, DB_NEXT_DUP);
			if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
        		ret = -ERR_NORECORD;
        		goto error;
        	}else if (ref){
        		ret = -ERR_TIMEOUT;
        		goto error;
        	}
		}
	}else{
		ret = -ERR_INVAL;
		goto error;
	}
	//删除数据
	for (i = 0;i< record_total;i ++){
		memset(&key_d, 0, sizeof(DBT));
		memset(&record_d, 0, sizeof(DBT));
		memset(data_middle, 0, CFG_DBS_RECORD_MAX);
		key_d.data = key;
		key_d.size = key_size;
		record_d.data = data_middle;
		record_d.ulen = CFG_DBS_RECORD_MAX;
		record_d.flags = DB_DBT_USERMEM;
		ref = cursorp_d->get(cursorp_d, &key_d, &record_d, DB_NEXT_DUP);
		if (ref == DB_NOTFOUND){                 //未找到符合条件的记录
    		ret = -ERR_NORECORD;
    		goto error;
    	}else if (ref){
    		ret = -ERR_TIMEOUT;
    		goto error;
    	}else{
			//删除记录
			ref = cursorp_d->del(cursorp_d, 0);  //删除该游标所指的记录
    		if (ref == EPERM ){                        //以只读方式打开的数据库
    			DBSPRINTF("del has no right!\n");
    			ret = -ERR_NOFUN;
    			goto error;
    		}
    		else if (ref){
       			DBSPRINTF("del record fail!\n");
       			ret = -ERR_SYS;
       			goto error;
    		}else{
    			DBSPRINTF("del success!\n");
    			//goto hull;
			}
		}
	}
	ret = 0;
error:
	//关闭游标
	ref = cursorp_d -> close(cursorp_d);
	if (ref){
		ret = -ERR_SYS;
		goto error;
	} 
error1:
	return (ret);
	
}
/******************************************************************************
    *	函数:	dbs_close
    *	功能:	关闭数据库
    *	参数:	id				-	数据库标号
    *	返回:	0				-	成功
 				-ERR_INVAL		-   接口参数配置错误
 				-ERR_NOINIT		-	数据库模块未初始化
 				-ERR_NOFILE		-	数据库未打开
 				-ERR_BUSY		-	未关闭
				-ERR_SYS		-	系统错误
    *	说明:	无
******************************************************************************/
int  dbs_close(u8 id)
{
	int ret,ref;
	if (dbenv == NULL){         //未初始化数据库模块：环境未设置
		ret = -ERR_NOINIT;
		goto error;
	}
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){ 
		ret = -ERR_INVAL;
		goto error;
	}
	//判断数据库是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error;
	}
	
	//获得打开计数器锁
	ref = pthread_mutex_lock(&(dbp_info[id].dbp_mutex));
	if (ref ==  EINVAL){
		DBSPRINTF("mutex lock fail!\n");
		ret = -ERR_NOINIT;
		goto error;
	}else if (ref){                           //ref == EDEADLK
		ret = -ERR_BUSY;
		goto error;
	}else{
		//打开计数
		if (dbp_info[id].dbp_count > 0){      //已打开
			dbp_info[id].dbp_count = 0;       
		}else{
			ret = -ERR_NOFILE;                //数据库未打开
			goto error1;
		}
		//关闭数据库
		ref = dbp[id]->close(dbp[id], 0);
		if (ref){
			DBSPRINTF("close db fail!\n");
			dbp_info[id].dbp_count = 1;
			ret = -ERR_SYS;
			goto error1;
		}
		dbp[id] = NULL;
		DBSPRINTF("close db success!\n");
		ret = 0;
error1:
	//解锁
	ref = pthread_mutex_unlock(&(dbp_info[id].dbp_mutex)); //解锁
	if (ref){   
		DBSPRINTF("mutex unlock fail!\n");
		ret = -ERR_SYS;
	}
	}
error:
	return(ret);
}
/******************************************************************************
      *	函数:	dbs_remove
      *	功能:	删除数据库
      *	参数:	id				-	数据库标号
      *	返回:	0				-	成功
   				-ERR_INVA		-   接口参数配置错误
   				-ERR_NOINIT		-	数据库模块未初始化
   				-ERR_NOFILE		-	数据库未打开
   				-ERR_BUSY		-	未关闭
 				-ERR_SYS		-	系统错误
      *	说明:	无
 ******************************************************************************/
int  dbs_remove(u8 id)
{
	int ret,ref;
	char path[22] = "";         //数据库路径
	char name[3]= "";			//数据库名
	char index[4] = ".db";      //数据库名后缀
	
	if (dbenv == NULL){         //未初始化数据库模块：环境未设置
		ret = -ERR_NOINIT;
		goto error;
	}
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){ 
		ret = -ERR_INVAL;
		goto error;
	}
	
	//获得打开计数器锁
	if (pthread_mutex_lock(&(dbp_info[id].dbp_mutex))){
		DBSPRINTF("mutex lock fail!\n");
		ret = -ERR_SYS;
		goto error;
	}
	else{
		if (dbp_info[id].dbp_count > 0){    //已打开,不能删除数据库
			DBSPRINTF("db has opened!\n");
			ret = -ERR_BUSY;
			goto error1;
		}
		else{         //打开数据库（先打开库才能删除） 
        	
        	//数据库标号
        	if (id >= 0 && id < 10){              //数据库名个位数
    	    	name[0] = '0' + id;
    	    }else if(id >9 && id < CFG_DBS_UNM_MAX){			//数据库名两位数
    	    	name[0] = '0' + id/10;          //用数据库标号作为数据库名称，取十位数字
    	    	name[1] = '0' + id%10;			//用数据库标号作为数据库名称，取个位数字
    	    }else{
    	    	ret = -ERR_INVAL;
    	    	goto error1;
    	    }
    	    //打开位置		
			switch(dbp_info[id].dbp_pos){
        		case DBS_POS_RAM:
        			strcat (path, path_ram);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		case DBS_POS_FLASH_CODE:
        			strcat (path, path_flash_code);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		case DBS_POS_FLASH_DATA:
        			strcat (path, path_flash_data);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		case DBS_POS_SD:
        			strcat (path, path_sd);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		case DBS_POS_U:
        			strcat (path, path_u);
        			strcat (path, name);
        			strcat (path, index);
        			break;
        		default:
        			ret = -ERR_INVAL;
        			goto error1;
        			break;
        	}
			//创建数据库
			if (db_create(&dbp[id], dbenv, 0)) {
				ret = -ERR_SYS;
				goto error1; 
			}
			//删除（移除）数据库
			ref = dbp[id]->remove(dbp[id],path,NULL,0);
			if (ref == EINVAL){
				ret = -ERR_BUSY;
				goto error1;
			}else if (ref){
				DBSPRINTF("remove db fail!ref = %d\n",ref);
				ret = -ERR_SYS;
				goto error1;
			}
			dbp[id] = NULL;
			memset(&dbp_info[id], 0, sizeof(dbp_info));
		}
		ret = 0;
		//解锁
error1:
		if (pthread_mutex_unlock(&(dbp_info[id].dbp_mutex))){   //解锁
			DBSPRINTF("mutex unlock fail!\n");
			ret = -ERR_SYS;
			goto error;
		}
	}
error:
	return(ret);        //系统错误
}
/******************************************************************************
       *	函数:	dbs_reset
       *	功能:	复位数据库（清空）
       *	参数:	id				-	数据库标号
       *	返回:	0				-	成功
    				-ERR_INVA		-   接口参数配置错误
    				-ERR_NOFILE		-	数据库未打开
    				-ERR_BUSY		-	未关闭
					-ERR_SYS		-	系统错误
       *	说明:	无
 ******************************************************************************/
int  dbs_reset(u8 id)
{
	int ret,ref;
	u_int32_t mount;
	
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){  
		ret = -ERR_INVAL;
		goto error;
	}
	//判断数据库是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error;
	}
	
	ref = dbp[id]->truncate(dbp[id], NULL, &mount, 0);
	if (ref == EINVAL ){  //有打开的游标
		ret = -ERR_BUSY;
		goto error;
	}else if(ref){
		DBSPRINTF("sync failed!\n");
		ret = -ERR_SYS;
		goto error;
	}
	ret = 0;
error:
	return (ret);
}
/******************************************************************************
        *	函数:	dbs_sync
        *	功能:	同步数据库
        *	参数:	id				-	数据库标号
        *	返回:	0				-	成功
     				-ERR_INVA		-   接口参数配置错误
     				-ERR_NOFILE		-	数据库未打开
 					-ERR_SYS		-	系统错误
        *	说明:	无
******************************************************************************/
int  dbs_sync(u8 id)
{
	int ret,ref;
	
	//判参数接口
	if ((id >= CFG_DBS_UNM_MAX) || (id < 0)){ 
		ret = -ERR_INVAL;
		goto error;
	}
	
	//判断数据库是否打开
	if (dbp[id] == NULL){
		ret = -ERR_NOFILE;
		goto error;
	}
	
	ref = dbp[id]->sync(dbp[id],0);
	if (ref){
		DBSPRINTF("sync failed!\n");
		ret = -ERR_SYS;
		goto error;
	}
	ret = 0;
error:
	return (ret);
}


#endif    /* CFG_DBS_MODULE */
