/**
* diskspace.c -- 分析储存分析剩余空间
* 
* 
* 创建时间: 2010-6-16
* 最后修改时间: 2010-6-16
*/

#include <stdio.h>
#include <sys/vfs.h>

/**
* @brief 计算分区的空间利用率
* @param diskpath 分区名
* @return 成功返回空间利用率(1%), 失败返回-1
*/
int DiskUsage(const char *diskpath)
{
	struct statfs buffer;
	int rtn;

	if(statfs(diskpath, &buffer)) return -1;

	if(buffer.f_blocks <= 0) return -1;

	rtn = (buffer.f_blocks - buffer.f_bavail)*100/buffer.f_blocks;
	return rtn;
}

