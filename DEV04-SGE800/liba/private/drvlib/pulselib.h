
#ifndef _PSLIB_H
#define _PSLIB_H

#define PS_IOC_MAGIC 0xE3
 
#define SET_TC		_IO(PS_IOC_MAGIC,  0)	//选择用那个定时器作为5ms定时
#define SET_COUNT	_IO(PS_IOC_MAGIC,  1)	//设置TIMER为频率测量功能
#define ADD_IO		_IO(PS_IOC_MAGIC,  2)	//设置TIMER为脉宽调制功能
#define PSTART		_IO(PS_IOC_MAGIC,  3)	//设置timer时钟



#endif  /* _PSLIB_H */

