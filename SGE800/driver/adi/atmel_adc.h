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
	文件		：  atmel_adc.h
	描述		：  本文件定义了at91sam9260内部ad转换模块的底层驱动程序接口底层
				宏定义，结构体、寄存器等
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/

#ifndef _ATMEL_ADC_H
#define _ATMEL_ADC_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>		//dev_t
#include <linux/cdev.h>		//char device register
#include <linux/kernel.h>		//container of macro
#include <linux/slab.h>		//memory manage
#include <linux/errno.h>		//error code
#include <linux/ioctl.h>		//ioctl system call
#include <linux/device.h>		//class_create

#include <linux/wait.h>		//waitqueue
#include <linux/clk.h>			//clock func
#include <linux/delay.h>		//udelay

#include <asm/io.h>			//ioremap
#include <asm/uaccess.h>		//copy_to_user
#include <asm/atomic.h>		//atomic

#include <mach/hardware.h>	//寄存器操作
#include <mach/gpio.h>		//gpio
#include <mach/at91_adc.h>	//TC
#include <mach/at91_pmc.h>	//PMC

#include <mach/at91sam9260.h>
#include <mach/at91_aic.h> //aic aboat SPI interupt

#define SOFTWARE_VERSION	"XJ atmel_adc adc 1.0"

#define MAJOR_DEVICE	109

struct atmel_adc_dev_t {
	char *name;
	void *buf;				// 设备读缓冲区
	atomic_t opened;		// 设备打开标志
	u8 ch_nr;				// 通道个数
	u8 ch_en;				//各个通道使能
	void __iomem *base;
	struct clk *clk;

};
#define CHANNEL0	4		//读取内部模拟通道0
#define CHANNEL1	5		//读取内部模拟通道1
#define CHANNEL2	6		//读取内部模拟通道2
#define CHANNEL3	7		//读取内部模拟通道3

#endif //_ATMEL_ADC_H
