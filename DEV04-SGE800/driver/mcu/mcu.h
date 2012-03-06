/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  mcu.h
	描述		：  本文件定义了单片机扩展功能驱动程序
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
#ifndef _MCU_H
#define _MCU_H

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
#include <linux/interrupt.h> 	//irqaction
#include <linux/wait.h>		//waitqueue
#include <linux/clk.h>			//clock func
#include <linux/delay.h>		//udelay
#include <linux/sched.h>

#include <asm/io.h>			//ioremap
#include <asm/uaccess.h>		//copy_to_user
#include <asm/atomic.h>		//atomic

#include <mach/hardware.h>	//寄存器操作
#include <mach/gpio.h>		//gpio
#include <mach/at91_tc.h>	//TC
#include <mach/at91_pmc.h>	//PMC
#include <mach/at91_spi.h>	//SPI
#include <mach/at91sam9260.h>
#include <mach/at91_aic.h> //aic aboat SPI interupt

#define SOFTWARE_VERSION	"XJ mcu serial and adc 1.0"

#define MAJOR_DEVICE	104
#define SPI_DEFAULT_BAUD 10000

#define SPITEMP 0
#define SPIOTHER 1<<12

#define SPI_READ_STAT 0
#define SPI_WRITE_STAT 1
#define MCU_BUF_SIZE 1024			//缓冲区大小

#define MCU_FRAME_IDEAL	 0
#define MCU_FRAME_READ	 1
#define MCU_FRAME_WRITE 2

#define MCU_FRAME_DATA_MAX 240		//帧数据最大字节数
#define MCU_FRAME_HEAD 0x68
#define MCU_FRAME_TAIL 0x16
#define MCU_FRAME_NEXT (1<<5)
struct mcu_dev_t {
	//spi相关
	u8 *name;
	u8 *buf;				// 设备读缓冲区
	u16 cur;				//数据当前指针，初始化为0,中断增加，read清零
	atomic_t opened;		// 设备打开标志
	u8 addr;				// 设备片选地址
	u16 data_size;			// 一次传输最大字节数
	u32 baud;
	int irq;
	void __iomem *base;
	struct clk *clk;
	wait_queue_head_t wq;

	u8 rw_flag;				//当前spi数据传输状态,完整帧读写状态
	struct semaphore sem;
	struct semaphore rwsem;
};

#endif
