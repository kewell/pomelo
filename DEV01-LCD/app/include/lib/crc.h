/**
* crc.h -- 计算CRC函数头文件
* 
* 
* 创建时间: 2010-4-1
* 最后修改时间: 2010-4-1
*/

#ifndef _LIB_CRC_H
#define _LIB_CRC_H

unsigned short CalculateCRC(const unsigned char *buffer, int count);
void CalculateCRCStep(const unsigned char *buffer, int count, unsigned short *pcrc);

#endif /*_LIB_CRC_H*/

