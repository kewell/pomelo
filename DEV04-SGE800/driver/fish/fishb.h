#ifndef _FISHB_H_
#define _FISHB_H_ 1

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>	//dev_t
#include <linux/cdev.h>		//char device register
#include <linux/kernel.h>	//container of macro
#include <linux/slab.h>		//memory manage
#include <asm/uaccess.h>	//copy_to_user
#include <linux/errno.h>	//error code
#include <linux/ioctl.h>	//ioctl system call
#include <asm/io.h>			//ioremap
#include <linux/device.h>



#endif
