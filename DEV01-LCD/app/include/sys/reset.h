/**
* syslock.c -- 系统复位函数头文件
* 
* 
* 创建时间: 2010-5-5
* 最后修改时间: 2010-5-5
*/

#ifndef _RESET_H
#define _RESET_H

/**
* @brief 复位系统
*/
void SysRestart(void);
/**
* @brief 系统掉电
*/
void SysPowerDown(void);

#endif /*_RESET_H*/
