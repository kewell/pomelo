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
	文件		：  atmel_gpio.c
	描述		：  本文件定义了ad转换模块的底层驱动程序接口底层宏定义，结构体、寄存器等
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/

#ifndef _TLV1504_H
#define _TLV1504_H

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

#define SOFTWARE_VERSION	"XJ tlv1504 adc 1.0"

#define MAJOR_DEVICE	102

#define SPI_DEFAULT_BAUD 10000

#define SPITEMP 0
#define SPIOTHER 1<<12

#define SELECT_CH0 0x0000 // Select analog input channel 0 
#define SELECT_CH1 0x1000	// Select analog input channel 1
#define SELECT_CH2 0x4000	// Select analog input channel 2
#define SELECT_CH3 0x6000	// Select analog input channel 3

#define SW_POWER_DOWN 0x8000	// SW power down (analog + reference)
#define CFR_READ 0x9000		// Read CFR register data shown as SDO D(11–0)
#define CFR_WRITE 0xa000	/* Write CFR followed by 12-bit data, 
							 * e.g., 0A100h means external reference,
							 * short sampling, SCLK/4, single shot, INT
							 */
#define INTER_REF	(1 << 11)	// Reference select  1: internal						
#define EXT_REF		(0 << 11)	// Reference select  0: External 	
// Internal reference voltage select 
#define INT_REF_V4	(0<<10)	// 0: Internal ref = 4 V 								
#define INT_REF_V2	(1 << 10)	// 1: internal ref = 2 V
// ample period select
#define SAM_PERIOD12	(0 << 9)	// 0: Short sampling 12 SCLKs (1x sampling time)		 
#define SAM_PERIOD24	(1 << 9)	// 1: Long sampling 24 SCLKs (2x sampling time)								 
								 
// D(8–7) Conversion clock source select								 
#define SELECT_CLK_INT	(0 << 7)	// 00: Conversion clock = internal OSC
#define SELECT_CLK_S	(1 << 7)	// 01: Conversion clock = SCLK
#define SELECT_CLK_S4	(2 << 7)	// 10: Conversion clock = SCLK/4
#define SELECT_CLK_S2	(3 << 7)	// 11: Conversion clock = SCLK/2
//Conversion mode select
#define SELECT_MODE_S	(0 << 5)	// 00: Single shot mode 
									//(single conversion on selected channel)
#define SELECT_MODE_R	(1 << 5)	// 01: Repeat mode 
									//(repeats conversion on selected channel)
#define SELECT_MODE_SW	(2 << 5)	// 10: Sweep mode 
									//(single sweep of selected channels)
#define SELECT_MODE_RS	(3 << 5)	// 11: Repeat sweep mode 
									//(repeats sweep of selected channels)

//Sweep auto sequence select
#define SWEEP_SEQ	(0 << 3)	// 00: N/A
#define SWEEP_SE0123	(0 << 3)	// 01: 0–1–2–3–0–1–2–3
#define SWEEP_SEQ0011	(0 << 3)	// 10: 0–0–1–1–2–2–3–3
#define SWEEP_SEQ0101	(0 << 3)	// 11: 0–1–0–1–0–1–0–1
//EOC/INT – pin function select
#define EOC_EN	(0 << 2)	// 0: Pin used as INT
#define INT_EN	(1 << 2)	// 1: Pin used as EOC
//FIFO trigger level (sweep sequence length)
#define FIFO_TRIG_LEVEL0 (0 << 0) // 00: Full 
									//(INT generated after FIFO level 7 filled)
#define FIFO_TRIG_LEVEL1 (1 << 0) // 01: 3/4 
									//(INT generated after FIFO level 5 filled)
#define FIFO_TRIG_LEVEL2 (2 << 0) // 10: 1/2 
									//(INT generated after FIFO level 3 filled)
#define FIFO_TRIG_LEVEL3 (3 << 0)	// 11: 1/4 
									//(INT generated after FIFO level 1 filled)


#define SELECT_TEST0 0xc000	// Select test, voltage = REFM = 0
#define SELECT_TEST1 0xb000	// Select test, voltage = (REFP+REFM)/2
#define SELECT_TEST2 0xd000	// Select test, voltage = REFP
#define FIFO_READ 0xe000	/* FIFO read, FIFO contents shown as SDO D(15–6), 
							 * D(5–0) = XXXXXX
							 */


struct tlv1504_dev_t {
	char *name;
	void *buf;				// 设备读缓冲区
	atomic_t opened;		// 设备打开标志
	u8 addr;				// 设备片选地址
	u8 data_max;			// 一次传输最大字节数

	u32 baud;
	void __iomem *base;
	struct clk *clk;

};

#endif
