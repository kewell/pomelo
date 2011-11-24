/**
* mdbstat.h -- 终端状态数据
* 
* 
* 创建时间: 2010-5-13
* 最后修改时间: 2010-5-13
*/

#ifndef _MDBSTAT_H
#define _MDBSTAT_H

//F2, 终端日历时钟

//F3, 终端参数状态

//F4, 终端通信状态

//F5, 终端控制设置状态

//F6, 终端控制设置状态

//F7, 终端事件计数器当前值
//F8, 终端事件标志状态

int ReadMdbState(unsigned short metpid, unsigned short itemid, unsigned char *buf, int len);

#endif /*_MDBSTAT_H*/

