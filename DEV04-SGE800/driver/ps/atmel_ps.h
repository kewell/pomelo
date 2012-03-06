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
	文件		：  atmel_ps.h
	描述		：  本文件定义了录波模块的底层驱动程序接口
	版本		：  0.2
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/


#ifndef ATMEL_PS_H
#define ATMEL_PS_H

#include <linux/compiler.h>
//#include <linux/list.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
//#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>	//dev_t
#include <linux/cdev.h>		//char device register
#include <linux/kernel.h>	//container of macro
#include <linux/slab.h>		//memory manage
#include <linux/errno.h>	//error code
#include <linux/ioctl.h>	//ioctl system call
#include <linux/device.h>	//class_create
#include <linux/interrupt.h> //irqaction
#include <linux/wait.h>		//waitqueue
#include <linux/clk.h>		//clock func
//#include <linux/delay.h>	//udelay
 #include <linux/sched.h>
#include <asm/io.h>			//ioremap
#include <asm/uaccess.h>	//copy_to_user
#include <asm/atomic.h>		//atomic

#include <mach/hardware.h>	//寄存器操作
#include <mach/gpio.h>	//gpio
#include <mach/at91sam9260.h>
#include <mach/at91_aic.h> //aic aboat tc interupt

/*************************************************
  宏定义
*************************************************/
#define SOFTWARE_VERSION	"XJ atmel_ps V10.1.0"
#define DRIVER_NAME 		"atmel_ps"

#define MAJOR_DEVICE	106 
#define MAX_PS_BUF	2048				// 分配的缓冲区存储空间，单位byte，
										// 每路可以连续保存1024次数据

//寄存器定义
#define		ATMEL_TC_CCR	0x00		/* Channel Control Register */
#define		ATMEL_TC_CLKEN	(1 << 0)	/* clock enable */
#define		ATMEL_TC_CLKDIS	(1 << 1)	/* clock disable */
#define		ATMEL_TC_SWTRG	(1 << 2)	/* software trigger */
#define		ATMEL_TC_SR	0x20		/* status (read-only) */


#ifndef  PIN_BASE
#define PIN_BASE		32
#define MAX_GPIO_BANKS		3
/* these pin numbers double as IRQ numbers, like AT91xxx_ID_* values */

#define	AT91_PIN_PA0	(PIN_BASE + 0x00 + 0)
#define	AT91_PIN_PA1	(PIN_BASE + 0x00 + 1)
#define	AT91_PIN_PA2	(PIN_BASE + 0x00 + 2)
#define	AT91_PIN_PA3	(PIN_BASE + 0x00 + 3)
#define	AT91_PIN_PA4	(PIN_BASE + 0x00 + 4)
#define	AT91_PIN_PA5	(PIN_BASE + 0x00 + 5)
#define	AT91_PIN_PA6	(PIN_BASE + 0x00 + 6)
#define	AT91_PIN_PA7	(PIN_BASE + 0x00 + 7)
#define	AT91_PIN_PA8	(PIN_BASE + 0x00 + 8)
#define	AT91_PIN_PA9	(PIN_BASE + 0x00 + 9)
#define	AT91_PIN_PA10	(PIN_BASE + 0x00 + 10)
#define	AT91_PIN_PA11	(PIN_BASE + 0x00 + 11)
#define	AT91_PIN_PA12	(PIN_BASE + 0x00 + 12)
#define	AT91_PIN_PA13	(PIN_BASE + 0x00 + 13)
#define	AT91_PIN_PA14	(PIN_BASE + 0x00 + 14)
#define	AT91_PIN_PA15	(PIN_BASE + 0x00 + 15)
#define	AT91_PIN_PA16	(PIN_BASE + 0x00 + 16)
#define	AT91_PIN_PA17	(PIN_BASE + 0x00 + 17)
#define	AT91_PIN_PA18	(PIN_BASE + 0x00 + 18)
#define	AT91_PIN_PA19	(PIN_BASE + 0x00 + 19)
#define	AT91_PIN_PA20	(PIN_BASE + 0x00 + 20)
#define	AT91_PIN_PA21	(PIN_BASE + 0x00 + 21)
#define	AT91_PIN_PA22	(PIN_BASE + 0x00 + 22)
#define	AT91_PIN_PA23	(PIN_BASE + 0x00 + 23)
#define	AT91_PIN_PA24	(PIN_BASE + 0x00 + 24)
#define	AT91_PIN_PA25	(PIN_BASE + 0x00 + 25)
#define	AT91_PIN_PA26	(PIN_BASE + 0x00 + 26)
#define	AT91_PIN_PA27	(PIN_BASE + 0x00 + 27)
#define	AT91_PIN_PA28	(PIN_BASE + 0x00 + 28)
#define	AT91_PIN_PA29	(PIN_BASE + 0x00 + 29)
#define	AT91_PIN_PA30	(PIN_BASE + 0x00 + 30)
#define	AT91_PIN_PA31	(PIN_BASE + 0x00 + 31)

#define	AT91_PIN_PB0	(PIN_BASE + 0x20 + 0)
#define	AT91_PIN_PB1	(PIN_BASE + 0x20 + 1)
#define	AT91_PIN_PB2	(PIN_BASE + 0x20 + 2)
#define	AT91_PIN_PB3	(PIN_BASE + 0x20 + 3)
#define	AT91_PIN_PB4	(PIN_BASE + 0x20 + 4)
#define	AT91_PIN_PB5	(PIN_BASE + 0x20 + 5)
#define	AT91_PIN_PB6	(PIN_BASE + 0x20 + 6)
#define	AT91_PIN_PB7	(PIN_BASE + 0x20 + 7)
#define	AT91_PIN_PB8	(PIN_BASE + 0x20 + 8)
#define	AT91_PIN_PB9	(PIN_BASE + 0x20 + 9)
#define	AT91_PIN_PB10	(PIN_BASE + 0x20 + 10)
#define	AT91_PIN_PB11	(PIN_BASE + 0x20 + 11)
#define	AT91_PIN_PB12	(PIN_BASE + 0x20 + 12)
#define	AT91_PIN_PB13	(PIN_BASE + 0x20 + 13)
#define	AT91_PIN_PB14	(PIN_BASE + 0x20 + 14)
#define	AT91_PIN_PB15	(PIN_BASE + 0x20 + 15)
#define	AT91_PIN_PB16	(PIN_BASE + 0x20 + 16)
#define	AT91_PIN_PB17	(PIN_BASE + 0x20 + 17)
#define	AT91_PIN_PB18	(PIN_BASE + 0x20 + 18)
#define	AT91_PIN_PB19	(PIN_BASE + 0x20 + 19)
#define	AT91_PIN_PB20	(PIN_BASE + 0x20 + 20)
#define	AT91_PIN_PB21	(PIN_BASE + 0x20 + 21)
#define	AT91_PIN_PB22	(PIN_BASE + 0x20 + 22)
#define	AT91_PIN_PB23	(PIN_BASE + 0x20 + 23)
#define	AT91_PIN_PB24	(PIN_BASE + 0x20 + 24)
#define	AT91_PIN_PB25	(PIN_BASE + 0x20 + 25)
#define	AT91_PIN_PB26	(PIN_BASE + 0x20 + 26)
#define	AT91_PIN_PB27	(PIN_BASE + 0x20 + 27)
#define	AT91_PIN_PB28	(PIN_BASE + 0x20 + 28)
#define	AT91_PIN_PB29	(PIN_BASE + 0x20 + 29)
#define	AT91_PIN_PB30	(PIN_BASE + 0x20 + 30)
#define	AT91_PIN_PB31	(PIN_BASE + 0x20 + 31)

#define	AT91_PIN_PC0	(PIN_BASE + 0x40 + 0)
#define	AT91_PIN_PC1	(PIN_BASE + 0x40 + 1)
#define	AT91_PIN_PC2	(PIN_BASE + 0x40 + 2)
#define	AT91_PIN_PC3	(PIN_BASE + 0x40 + 3)
#define	AT91_PIN_PC4	(PIN_BASE + 0x40 + 4)
#define	AT91_PIN_PC5	(PIN_BASE + 0x40 + 5)
#define	AT91_PIN_PC6	(PIN_BASE + 0x40 + 6)
#define	AT91_PIN_PC7	(PIN_BASE + 0x40 + 7)
#define	AT91_PIN_PC8	(PIN_BASE + 0x40 + 8)
#define	AT91_PIN_PC9	(PIN_BASE + 0x40 + 9)
#define	AT91_PIN_PC10	(PIN_BASE + 0x40 + 10)
#define	AT91_PIN_PC11	(PIN_BASE + 0x40 + 11)
#define	AT91_PIN_PC12	(PIN_BASE + 0x40 + 12)
#define	AT91_PIN_PC13	(PIN_BASE + 0x40 + 13)
#define	AT91_PIN_PC14	(PIN_BASE + 0x40 + 14)
#define	AT91_PIN_PC15	(PIN_BASE + 0x40 + 15)
#define	AT91_PIN_PC16	(PIN_BASE + 0x40 + 16)
#define	AT91_PIN_PC17	(PIN_BASE + 0x40 + 17)
#define	AT91_PIN_PC18	(PIN_BASE + 0x40 + 18)
#define	AT91_PIN_PC19	(PIN_BASE + 0x40 + 19)
#define	AT91_PIN_PC20	(PIN_BASE + 0x40 + 20)
#define	AT91_PIN_PC21	(PIN_BASE + 0x40 + 21)
#define	AT91_PIN_PC22	(PIN_BASE + 0x40 + 22)
#define	AT91_PIN_PC23	(PIN_BASE + 0x40 + 23)
#define	AT91_PIN_PC24	(PIN_BASE + 0x40 + 24)
#define	AT91_PIN_PC25	(PIN_BASE + 0x40 + 25)
#define	AT91_PIN_PC26	(PIN_BASE + 0x40 + 26)
#define	AT91_PIN_PC27	(PIN_BASE + 0x40 + 27)
#define	AT91_PIN_PC28	(PIN_BASE + 0x40 + 28)
#define	AT91_PIN_PC29	(PIN_BASE + 0x40 + 29)
#define	AT91_PIN_PC30	(PIN_BASE + 0x40 + 30)
#define	AT91_PIN_PC31	(PIN_BASE + 0x40 + 31)
#endif


/*************************************************
  结构类型定义
*************************************************/
struct atmel_ps_t{	
	char  name[10];
	int		id;	
	u32		tc_channel;		//定时器通道号
	void __iomem	*tc_regs;
	int		tc_irq;

	u32		count;  		//录波的io通道数
	u32		channel[16];	//存储录波的io地址，最大16路
	
	u16		val_temp_buf;	//中断处理中用到的临时读取io口的变量
	u16		*val_buf;		//环形缓冲区指针
	u16		*val_ret;		//保存从缓冲区读来的数据，返回应用层
	u16 head,tail;			//环形缓冲区头尾的位置
	atomic_t opened;		
	int data0;
	int data1;
	wait_queue_head_t wq;
	struct cdev cdev;
	struct semaphore sem;
};

//环形缓冲区操作宏定义
#define BUF_HEAD 		(atmel_ps.val_buf[atmel_ps.head])  //缓冲区头
#define BUF_TAIL 		(atmel_ps.val_buf[atmel_ps.tail])  //缓冲区尾
#define INCBUF(x,mod) 	((++(x)) & ((mod) - 1))		//mod 必须是2的幂，


#endif
