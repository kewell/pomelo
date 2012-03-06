
#ifndef _TIMERLIB_H
#define _TIMERLIB_H

//define ioctl command 
#define TIMER_IOC_MAGIC 0xE2
 
#define SET_DELAY		_IO(TIMER_IOC_MAGIC,  0)	//设置TIMER为延时功能
#define SET_MEASURE		_IO(TIMER_IOC_MAGIC,  1)	//设置TIMER为频率测量功能
#define SET_PWM			_IO(TIMER_IOC_MAGIC,  2)	//设置TIMER为脉宽调制功能
#define SET_PULSE		_IO(TIMER_IOC_MAGIC,  3)	//设置TIMER为配合开入功能

#define SET_CLOCK		_IO(TIMER_IOC_MAGIC,  4)	//设置timer时钟
#define TCSTART			_IO(TIMER_IOC_MAGIC,  5)	//启动TIMER
#define TCSTOP			_IO(TIMER_IOC_MAGIC,  6)	//停止TIMER

//命令SETCLOCK 对应arg参数, 即时钟频率选择
#define MCKD2		0 		//定时器时钟输入频率MCK/2		
#define MCKD8		1		//定时器时钟输入频率MCK/8		
#define MCKD32		2		//定时器时钟输入频率MCK/32		
#define MCKD128		3		//定时器时钟输入频率MCK/128		
#define SCK32KIHZ	4		//定时器时钟输入频率32.768K
#define ETCLK0		5		//定时器时钟输入外部0
#define ETCLK1		6		//定时器时钟输入外部1
#define ETCLK2		7		//定时器时钟输入外部2

//命令TCSTART 对应arg参数, 即定时器号
#define TIMER0		0		//端口0
#define TIMER1		1		//端口1
#define TIMER2		2		//端口2
#define TIMER3		3		//端口3
#define TIMER4		4		//端口4
#define TIMER5		5		//端口5

#endif  /* _TIMERLIB_H */
