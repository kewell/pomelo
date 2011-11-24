/**
* diskspace.h -- 分析储存分析剩余空间
* 
* 
* 创建时间: 2010-6-16
* 最后修改时间: 2010-6-16
*/

#ifndef _SYS_DISKSPACE_H
#define _SYS_DISKSPACE_H

/**
* @brief 计算分区的空间利用率
* @param diskpath 分区名
* @return 成功返回空间利用率(1%), 失败返回-1
*/
int DiskUsage(const char *diskpath);

#endif /*_SYS_DISKSPACE_H*/

