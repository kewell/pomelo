/**
* sigin.h -- 输入信号驱动接口
* 
* 
* 创建时间: 2010-5-16
* 最后修改时间: 2010-5-16
*/

#ifndef _SYS_SIGIN_H
#define _SYS_SIGIN_H

#define SIGIN_NUM		2
#define SIGIN_MASK		((1<<SIGIN_NUM)-1)

/**
* @brief 读取输入量状态
* @return 输入量状态,每BIT代表一个
*/
unsigned int SiginReadState(void);
/**
* @brief 读取输入按键
* @return 输入按键信息,返回0表示没有输入
*/
char SiginGetChar(void);

#endif /*_SYS_SIGIN_H*/

