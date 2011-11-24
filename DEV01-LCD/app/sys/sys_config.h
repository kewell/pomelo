/**
* sys_config.h -- 系统抽象模块配置文件
* 
* 
* 创建时间: 2010-3-30
* 最后修改时间: 2010-3-30
*/

#ifndef _SYS_CONFIG_H
#define _SYS_CONFIG_H

#define MAX_PTHREAD		32  //软件最大线程(任务)数

//最大时钟定时器数
#define MAX_RTIMER		63
//最大相对定时器数
#define MAX_CTIMER   	63

#define MAX_SYSLOCK		16


#endif /*_SYS_CONFIG_H*/
