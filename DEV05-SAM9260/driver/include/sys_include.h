/********************************************************************************
 *      Copyright:  (C) 2012 KEWELL 
 *
 *       Filename:  sys_include.h
 *    Description:  This head file 
 *
 *        Version:  1.0.0(09/26/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "09/26/2012 09:29:48 AM"
 *                  2,
 *                 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *                 
 ********************************************************************************/

#ifndef __SYS_INCLUDE_H
#define __SYS_INCLUDE_H

#include <../arch/arm/include/asm/io.h>

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/string.h>
#include <linux/bcd.h>
#include <linux/miscdevice.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/rtc.h>
#include <linux/spinlock.h>
#include <linux/usb.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/syscalls.h>  /*For sys_access*/
#include <linux/unistd.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/irq.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
#include <mach/hardware.h>
#include <mach/at91_tc.h>
#include <mach/gpio.h>
#include <mach/at91_pmc.h>
#include <asm/irq.h>
#include <mach/at91_rtt.h>
#else                           /*For 2.6.22.1 we used before */
#include <asm-arm/irq.h>
#include <asm/arch/gpio.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/at91_tc.h>
#include <asm/arch/hardware.h>
#include <asm/arch/at91_rtt.h>
#endif

#endif /*__SYS_INCLUDE_H*/

