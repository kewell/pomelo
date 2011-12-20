/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  gpio.c
	描述		：  本文件定义了IO操作模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2009.12
******************************************************************************/
//库配置头文件
#include "private/config.h"

//模块启用开关
#ifdef CFG_GPIO_MODULE

//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//ioctl
#include <string.h> 	//memset
#include <unistd.h>		//close
#include <pthread.h>	//pthread库函数
#include <sys/ioctl.h>

//提供给用户的头文件
#include "include/gpio.h"
#include "include/error.h"

//驱动调用头文件
#include "private/drvlib/gpiolib.h"

/*************************************************
  静态全局变量及宏定义
*************************************************/
#define PINBASE 32			//传入到驱动的io口地址基址
#define MAX_PIN 96			//最大io口数量
static int fd;				//保存gpio驱动文件描述符

/*************************************************
  结构类型定义
*************************************************/
static struct io_attr{
	u8 active;		//记录gpio的激活状态
	u8 mode;		//工作模式（1输入或0输出）
	u8 od;			//OD门使能标志，1表示使能
	u8 pu;			//上拉电阻使能标志，1表示使能
	pthread_mutex_t mutex;	//互斥锁
}gpio[MAX_PIN];	//各个gpio口属性设置	

static u8 gpio_count = 0;		//模快打开计数

/*************************************************
  API
*************************************************/
/******************************************************************************
*	函数:	gpio_init
*	功能:	GPIO模块初始化
*	参数:	无
*	返回:	0				-	成功
			-ERR_NOFILE		-	没有此路径
			-ERR_BUSY		-	设备忙，已经打开
			-ERR_SYS		-	初始化锁失败，如内存不足；已经初始化化，但还没销毁；互斥锁地址无效等
*	说明:	无
 ******************************************************************************/
int gpio_init(void)
{
	int ret = -1;
	int i;
	if(gpio_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}
	
	fd = open("/dev/atmel_gpio", O_RDWR | O_NOCTTY);
	if (fd < 0){
   		ret = -ERR_NOFILE;		//没有此路径
   		goto err;
	}
	gpio_count = 1;
	memset(gpio, 0, MAX_PIN*sizeof( struct io_attr));
	//初始化互斥锁
	for(i = 0; i < MAX_PIN; i ++){
		if (pthread_mutex_init(&gpio[i].mutex, NULL)) {
			ret = -ERR_SYS;
			goto err;
		}
	}

	ret = 0;
err:
	return ret;
}


/******************************************************************************
*	函数:	gpio_set
*	功能:	单个IO设置
*	参数:	io				-	IO口编号
			mode			-	工作模式（输入或输出）
			od				-	OD门使能标志，1表示使能
			pu				-	上拉电阻使能标志，1表示使能
*	返回:	0				-	成功
			-ERR_INVAL 	 	-	参数错误
			-ERR_NODEV 	 	-	无此设备
			-ERR_SYS 	 	-	系统错误，ioctl调用失败
			-ERR_NOINIT  	-	锁没有初始化化
*	说明:	无
 ******************************************************************************/
int gpio_set(u8 io, u8 mode, u8 od, u8 pu)
{
	int ret = -1;	
	
	if(gpio_count == 0)
		return -ERR_NOINIT;
	if((io < 0) || (io >= MAX_PIN)){		//gpio地址范围
		return -ERR_NODEV;		
	}
	//获得互斥锁
	if (pthread_mutex_lock (&gpio[io].mutex)) {
		return  -ERR_NOINIT;		
	}
	
	if(mode == GPIO_OUT || mode == GPIO_IN)		//设置输入输出
		gpio[io].mode = mode;		
	else{
		ret = -ERR_INVAL;
		goto err;
	}

	if(od == GPIO_ODD || od == GPIO_ODE)		//设置od门使能
		gpio[io].od = od;			
	else{
		ret = -ERR_INVAL;
		goto err;
	}
	
	if(pu == GPIO_PUD || pu == GPIO_PUE)		//设置上拉电阻使能
		gpio[io].pu = 0;		
	else{
		ret = -ERR_INVAL;
		goto err;
	}
	
	gpio[io].active = 1;
	
	if(mode == GPIO_OUT){		//设置输入输出		
		ret = ioctl(fd, SETO, io + PINBASE); 
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}
	else if(mode == GPIO_IN){		
		ret = ioctl(fd, SETI, io + PINBASE); 
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}
	
	if(od == GPIO_ODD){		//设置od门使能		
		ret = ioctl(fd, ODD, io + PINBASE); 
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}
	else if(od == GPIO_ODE){		
		ret = ioctl(fd, ODE, io + PINBASE); 
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}
	
	if(pu == GPIO_PUD){		//设置上拉电阻使能		
		ret = ioctl(fd, PUD, io + PINBASE); 
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}
	else if(pu == GPIO_PUE){		
		ret = ioctl(fd, PUE, io + PINBASE); 
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}
	ret = 0;
err:	//解锁
	if (pthread_mutex_unlock (&gpio[io].mutex)) {
		ret = -ERR_OTHER;		
	}
	return ret;
}


/******************************************************************************
*	函数:	gpio_output_set
*	功能:	输出模式下设置输出电平状态
*	参数:	io				-	IO口编号
			val				-	输出状态
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_NODEV		-	无此设备
			-ERR_NOFUN		-	无此功能
			-ERR_NOCFG		-	没有设置
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	锁没有初始化化
			-ERR_OTHER		-	其他关于线程互斥锁的错误
*	说明:	无
 ******************************************************************************/
int gpio_output_set(u8 io, u8 val)
{
	int ret = -1;
	
	if(gpio_count == 0)
		return -ERR_NOINIT;

	if((io < 0) || (io >= MAX_PIN))		//gpio地址范围有效性判断
		return -ERR_NODEV;
	
	//获得互斥锁
	if (pthread_mutex_lock (&gpio[io].mutex)) {
		return  -ERR_NOINIT;		
	}
	
//	if(gpio[io].active == 0){				//此端口没有设置
//		ret = -ERR_NOCFG ;
//		goto err;
//	}
//	if(gpio[io].mode != GPIO_OUT){			//此端口没有设置为输出功能
//		ret = -ERR_NOFUN;
//		goto err;
//	}
	
	if(val == 0){			//设置输出	
		ret = ioctl(fd, OCLR, io + PINBASE); 
		if(ret < 0){			
			ret = -ERR_SYS;
			goto err;
		}
	}
	else if(val == 1){		
		ret = ioctl(fd, OSET, io + PINBASE); 
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else{
		ret = -ERR_INVAL;
		goto err;
	}
	ret = 0;
err:	//解锁
	if (pthread_mutex_unlock (&gpio[io].mutex)) {
		ret = -ERR_OTHER;		
	}
	return ret;
}


/******************************************************************************
*	函数:	gpio_output_get
*	功能:	输出模式下获取输出电平状态
*	参数:	io				-	IO口编号
			val				-	输出状态（数据传出）
*	返回:	0				-	成功
			-ERR_NODEV		-	无此设备
			-ERR_NOFUN		-	无此功能
			-ERR_NOCFG		-	没有设置
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	锁没有初始化化
			-ERR_OTHER		-	其他关于线程互斥锁的错误
*	说明:	无
 ******************************************************************************/
int gpio_output_get(u8 io, u8 *val)
{
	int ret = -1;
	u32 tmp;
	u32 *arg = NULL;
	arg = &tmp;
	tmp = io + PINBASE;
	
	if(gpio_count == 0)
		return -ERR_NOINIT;

	if((io < 0) || (io >= MAX_PIN))		//gpio地址范围
		return -ERR_NODEV;
	//获得互斥锁
	if (pthread_mutex_lock (&gpio[io].mutex)) {
		return  -ERR_NOINIT;		
	}

	if(gpio[io].active == 0){		//此端口没有设置
		ret = -ERR_NOCFG;
		goto err;
	}
	if(gpio[io].mode != GPIO_OUT){		//此端口没有设置为输出功能
		ret = -ERR_NOFUN;
	}
	
	ret = ioctl(fd, IOGETO, arg); 
	if(ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
	if((*arg < 0) || (*arg > 1)){
		ret = -ERR_SYS;
		goto err;
	}
	*val = *arg;
	ret = 0;
err:	//解锁
	if (pthread_mutex_unlock (&gpio[io].mutex)) {
		ret = -ERR_OTHER;		
	}
	return ret;
}


/******************************************************************************
*	函数:	gpio_input_get
*	功能:	输入模式下获取输入电平状态
*	参数:	io 				-	IO口编号
			val				-	输入电平状态（数据传出）
*	返回:	0				-	成功
			-ERR_NODEV		-	无此设备
			-ERR_NOFUN		-	无此功能
			-ERR_NOCFG		-	没有设置
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	锁没有初始化化
*	说明:	无
 ******************************************************************************/
int gpio_input_get(u8 io, u8 *val)
{
	
	int ret;
	int tmp;
	int *arg = NULL;
	arg = &tmp;	
	
	if((io < 0) || (io >= MAX_PIN)){		//gpio地址范围
		return -ERR_NODEV;		
	}
	
	//获得互斥锁
	if (pthread_mutex_lock (&gpio[io].mutex)) {
		return  -ERR_NOINIT;		
	}
	
	if(gpio[io].active == 0){				//此端口没有设置
		ret = -ERR_NOCFG;
		goto err;
	}
	if(gpio[io].mode != GPIO_IN){			//此端口没有设置为输入功能
		ret = -ERR_NOFUN;
		goto err;
	}
	
	tmp = io + PINBASE;
	ret = ioctl(fd, IOGETI, arg); 
	if(ret < 0){
		ret = -ERR_SYS;
		goto err;
	}

	if((*arg < 0) || (*arg > 1)){
		ret = -ERR_SYS;
		goto err;
	}
	*val = *arg;
	ret = 0;
err:	//解锁
	if (pthread_mutex_unlock (&gpio[io].mutex)) {
		ret = -ERR_OTHER;	
	}	
	return ret;
}

/******************************************************************************
*	函数:	gpio_close
*	功能:	GPIO模块关闭函数
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	模块未初始化
			-ERR_OTHER		-	当前线程不拥有互斥锁，或锁未初始化
*	说明:	无
 ******************************************************************************/
int gpio_close(void)
{
	int ret = -1;
	int i;
	if(gpio_count == 0)
		return -ERR_NOINIT;
	ret = close(fd);
	if(ret < 0)
		return -ERR_SYS;
	gpio_count = 0;
	//销毁互斥锁
	for(i = 0; i < MAX_PIN; i ++){
		if (pthread_mutex_destroy(&gpio[i].mutex)) {
			ret = -ERR_OTHER;
		}
	}
	ret = 0;
	return ret;
}


#endif /* CFG_GPIO_MODULE */
