/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/*****************************************************************************
	项目名称	：SGE800计量智能终端平台
	文件		：file.c
	描述		：本文件实现了文件操作模块中的API函数
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2009.12
*****************************************************************************/
//库配置头文件
#include "private/config.h"

//模块启用开关
#ifdef CFG_FILE_MODULE

//调试头文件
#include "private/debug.h"

//驱动调用头文件

//C库头文件
#include <stdio.h>						//printf
#include <string.h>
#include <errno.h>
#include <fcntl.h>						//open
#include <unistd.h>						//write
#include <sys/stat.h>					//stat
#include <sys/types.h>

//提供给用户的头文件
#include "include/file.h"
#include "include/error.h"


/*************************************************
  静态全局变量定义
*************************************************/
//各存储区域路径
static const char *const path_ram = "/var/";
static const char *const path_flash_code = "/mnt/local/";
static const char *const path_flash_data = "/mnt/data0/";
static const char *const path_sd = "/mnt/sddisk/";
static const char *const path_u = "/mnt/udisk/";



/*************************************************
  API
*************************************************/
/******************************************************************************
*	函数:	file_open
*	功能:	打开文件
*	参数:	name			-	文件名
 			pos				-	文件存放位置
 			mode			-	文件打开模式
*	返回:	>0				-	文件描述符
 			-ERR_INVAL		-	参数错误
 			-ERR_BUSY		-	文件已存在
*	说明:
 ******************************************************************************/
int file_open(char *name, u8 pos, u8 mode)
{
    int ret = 0;
    int i;
    int fd;
    int errnum;
    int oflag;
    int namesize;
    char path[32 + CFG_FILE_NAME_MAX] = "";			//32留作路径存储空间

//确定文件打开模式参数
    if (FILE_MODE_OPEN == mode) {
        oflag = O_RDWR;
    }
    else if (FILE_MODE_CREAT == mode) {
        oflag = O_RDWR | O_CREAT | O_EXCL;
    }
    else if (FILE_MODE_RW == mode) {
        oflag = O_RDWR | O_CREAT;
    }
    else if (FILE_MODE_RD == mode) {
        oflag = O_RDONLY;
    }
    else {
        ret = -ERR_INVAL;
        goto error;
    }
    oflag = oflag | O_SYNC;
//确定文件路径
    if (FILE_POS_RAM == pos) {
        strcat (path, path_ram);
    }
    else if (FILE_POS_FLASH_CODE == pos) {
        strcat (path, path_flash_code);
    }
    else if (FILE_POS_FLASH_DATA == pos) {
        strcat (path, path_flash_data);
    }
    else if (FILE_POS_SD == pos) {
        strcat (path, path_sd);
    }
    else if (FILE_POS_U == pos) {
        strcat (path, path_u);
    }
    else {
        ret = -ERR_INVAL;
        goto error;
    }
//添加文件名
    namesize = strlen (name);
    if ((0 == namesize) || (namesize > CFG_FILE_NAME_MAX)) {
        ret = -ERR_INVAL;
        goto error;
    }
    for (i=0; i<namesize; i++) {
    	//文件名只能是数字、大小写字母、下划线
        if (((name[i] > 47) && (name[i] < 58)) || ((name[i] > 64) && (name[i] < 91))
        	|| ((name[i] > 96) && (name[i] < 123)) || (name[i] == 95)) {
            continue;
        }
        else {
            ret = -ERR_INVAL;
            goto error;
        }
    }
    strcat (path, name);
//打开文件
    fd = open (path, oflag, S_IRWXU);
    if (fd < 0) {
        errnum = errno;
        switch (errnum) {
        case EEXIST:
            ret = -ERR_BUSY;
            break;
        case ENOENT:
            ret = -ERR_NOFILE;
            break;
        case ENOMEM:
            ret = -ERR_NOMEM;
            break;
        default:
            ret = -ERR_SYS;
        }
        goto error;
    }
    else {
        ret = fd;
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	file_write
*	功能:	写入文件
*	参数:	fd				-	文件描述符
 			buf				-	数据区（数据传入）
 			count			-	请求字节数
*	返回:	>=0				-	写入字节数
 			-ERR_INVAL		-	参数错误
 			-ERR_NOFUN		-	无此功能
 			-ERR_NOMEM		-	缓冲区错误
 			-ERR_NODISK		-	硬盘空间不足
 			-ERR_SYS		-	系统异常
*	说明:
 ******************************************************************************/
int file_write(int fd, void *buf, u32 count)
{
    int ret = 0;
    int errnum;

    if ((count > CFG_FILE_DATA_MAX) || (count <= 0)) {
        ret = -ERR_INVAL;
        goto error;
    }
    if (fd <= 0) {
        ret = -ERR_INVAL;
        goto error;
    }
    if (buf == NULL){
    	 ret = -ERR_INVAL;
    	 goto error;
    }
    ret = write (fd, buf, count);
    if (ret < 0) {
        errnum = errno;
        switch (errnum) {
        case EBADF:
        case EINVAL:
            ret = -ERR_NOFUN;
            break;
        case EFAULT:
            ret = -ERR_NOMEM;
            break;
        case ENOSPC:
            ret = -ERR_NODISK;
            break;
		default:
            ret = -ERR_SYS;
        }
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	file_read
*	功能:	读取文件
*	参数:	fd				-	文件描述符
 			buf				-	数据区（数据传出）
 			count			-	请求字节数
*	返回:	>=0				-	读取字节数
 			-ERR_INVAL		-	参数错误
 			-ERR_NOMEM		-	缓冲区错误
 			-ERR_SYS		-	系统异常
*	说明:
 ******************************************************************************/
int file_read(int fd, void *buf, u32 count)
{
    int ret = 0;
    int errnum;

    if ((count > CFG_FILE_DATA_MAX) || (count <= 0)) {
        ret = -ERR_INVAL;
        goto error;
    }
    if (fd <= 0) {
        ret = -ERR_INVAL;
        goto error;
    }
    ret = read (fd, buf, count);
    if (ret < 0) {
        errnum = errno;
        switch (errnum) {
        case EBADF:
            ret = -ERR_INVAL;
            break;
        case EFAULT:
            ret = -ERR_NOMEM;
            break;
        default:
            ret = -ERR_SYS;
        }
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	file_seek
*	功能:	移动文件操作指针
*	参数:	fd				-	文件描述符
 			offset			-	偏移值
 			whence			-	偏移参考
*	返回:	>0				-	文件当前的偏移
 			-ERR_INVAL		-	参数错误
 			-ERR_SYS		-	系统异常
*	说明:
 ******************************************************************************/
int file_seek(int fd, int offset, u8 whence)
{
    int ret = 0;
    int errnum;

    if ((fd <= 0) || (whence > FILE_SEEK_END)) {
        ret = -ERR_INVAL;
        goto error;
    }
    ret = lseek (fd, offset, whence);
    if (ret < 0) {
        errnum = errno;
        switch (errnum) {
        case EBADF:
        case EINVAL:
            ret = -ERR_INVAL;
            break;
        default:
            ret = -ERR_SYS;
        }
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	file_close
*	功能:	关闭文件
*	参数:	fd				-	文件描述符
*	返回:	0				-	成功
 			-ERR_INVAL		-	参数错误
 			-ERR_SYS		-	系统异常
*	说明:
 ******************************************************************************/
int file_close(int fd)
{
    int ret = 0;
    int errnum;
    ret = close (fd);
    if (ret < 0) {
        errnum = errno;
        switch (errnum) {
        case EBADF:
            ret = -ERR_INVAL;
            break;
        default:
            ret = -ERR_SYS;
        }
    }
    return ret;
}

/******************************************************************************
*	函数:	file_delete
*	功能:	删除文件
*	参数:	name			-	文件名
 			pos				-	文件存放位置
*	返回:	0				-	成功
 			-ERR_NOFILE		-	无此文件
 			-ERR_NOMEM		-	内存不足
 			-ERR_SYS		-	系统异常
*	说明:
 ******************************************************************************/
int file_delete(char *name, u8 pos)
{
    int ret = 0;
    int i;
    int errnum;
    int namesize;
    char path[32 + CFG_FILE_NAME_MAX] = "";			//32留作路径存储空间
//确定文件路径
    if (FILE_POS_RAM == pos) {
        strcat (path, path_ram);
    }
    else if (FILE_POS_FLASH_CODE == pos) {
        strcat (path, path_flash_code);
    }
    else if (FILE_POS_FLASH_DATA == pos) {
        strcat (path, path_flash_data);
    }
    else if (FILE_POS_SD == pos) {
        strcat (path, path_sd);
    }
    else if (FILE_POS_U == pos) {
        strcat (path, path_u);
    }
    else {
        ret = -ERR_INVAL;
        goto error;
    }
//添加文件名
    namesize = strlen (name);
    if ((0 == namesize) || (namesize > CFG_FILE_NAME_MAX)) {
        ret = -ERR_INVAL;
        goto error;
    }
    for (i=0; i<namesize; i++) {
    	//文件名只能是数字、大小写字母、下划线
        if (((name[i] > 47) && (name[i] < 58)) || ((name[i] > 64) && (name[i] < 91))
        	|| ((name[i] > 96) && (name[i] < 123)) || (name[i] == 95)) {
            continue;
        }
        else {
            ret = -ERR_INVAL;
            goto error;
        }
    }
    strcat (path, name);
//删除文件
    ret = unlink (path);
    if (ret < 0) {
        errnum = errno;
        switch (errnum) {
        case ENOENT:
            ret = -ERR_NOFILE;
            break;
        case ENOMEM:
            ret = -ERR_NOMEM;
            break;
        default:
            ret = -ERR_SYS;
        }
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	file_info
*	功能:	获取文件信息
*	参数:	name			-	文件名
 			pos				-	文件存放位置
 			size			-	文件大小（数据传出）
 			time			-	修改时间（数据传出）
*	返回:	0				-	成功
 			-ERR_NOFILE		-	无此文件
 			-ERR_NOMEM		-	内存不足
 			-ERR_SYS		-	系统异常
*	说明:
 ******************************************************************************/
int file_info(char *name, u8 pos, u32 *size, file_time_t *time)
{
    int ret = 0;
    int i;
    int errnum;
    int namesize;
    char path[32 + CFG_FILE_NAME_MAX] = "";			//32留作路径存储空间
    struct stat st;
    struct tm t;
//确定文件路径
    if (FILE_POS_RAM == pos) {
        strcat (path, path_ram);
    }
    else if (FILE_POS_FLASH_CODE == pos) {
        strcat (path, path_flash_code);
    }
    else if (FILE_POS_FLASH_DATA == pos) {
        strcat (path, path_flash_data);
    }
    else if (FILE_POS_SD == pos) {
        strcat (path, path_sd);
    }
    else if (FILE_POS_U == pos) {
        strcat (path, path_u);
    }
    else {
        ret = -ERR_INVAL;
        goto error;
    }
//添加文件名
    namesize = strlen (name);
    if ((0 == namesize) || (namesize > CFG_FILE_NAME_MAX)) {
        ret = -ERR_INVAL;
        goto error;
    }
    for (i=0; i<namesize; i++) {
    	//文件名只能是数字、大小写字母、下划线
        if (((name[i] > 47) && (name[i] < 58)) || ((name[i] > 64) && (name[i] < 91))
        	|| ((name[i] > 96) && (name[i] < 123)) || (name[i] == 95)) {
            continue;
        }
        else {
            ret = -ERR_INVAL;
            goto error;
        }
    }
    strcat (path, name);
//获取文件信息
    ret = stat (path, &st);
    if (ret < 0) {
        errnum = errno;
        switch (errnum) {
		case ENOENT:
            ret = -ERR_NOFILE;
            break;
		case ENOMEM:
            ret = -ERR_NOMEM;
            break;
		default:
            ret = -ERR_SYS;
        }
    }
    else {
        ret = (int)(gmtime_r(&st.st_mtime, &t));
        if (0 == ret) {
            ret = -ERR_SYS;
            goto error;
        }
        *size = st.st_size;
        time->year = t.tm_year - 100;		//系统读出的年是自从1900年以来的年
        time->mon = t.tm_mon + 1;
        time->day = t.tm_mday;
        time->hour = t.tm_hour;
        time->min = t.tm_min;
        time->sec = t.tm_sec;
        ret = 0;
    }
error:
    return ret;
}

#endif /* CFG_FILE_MODULE */
