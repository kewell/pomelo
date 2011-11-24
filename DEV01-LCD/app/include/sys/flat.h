/**
* flat.h -- 平滑储存
* 
* 
* 创建时间: 2010-5-15
* 最后修改时间: 2010-5-15
*/

#ifndef _SYS_FLAT_H
#define _SYS_FLAT_H

/**
* @brief 读取FLAT文件数据
* @param sector 文件扇区号(0~3)
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际读取长度, 失败返回-1
*/
int ReadFlatFile(unsigned int sector, unsigned char *buf, int len);

/**
* @brief 写入FLAT文件数据
* @param sector 文件扇区号(0~3)
* @param buf 缓存区指针
* @param len 缓存区长度
* @return 成功返回实际写入长度, 失败返回-1
*/
int WriteFlatFile(unsigned int sector, const unsigned char *buf, int len);

#endif /*_SYS_FLAT_H*/

