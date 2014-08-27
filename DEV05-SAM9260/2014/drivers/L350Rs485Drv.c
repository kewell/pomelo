#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/arch/hardware.h>
#include <asm/system.h>
#include <linux/delay.h>
#include <asm/arch/gpio.h>

/************************* include file*************************/
#include "L350Rs485Drv.h"
#include "L350IoctlPara.h"

int rs485_open (struct inode *inode, struct file *file)
{
	return 0;
}

int rs485_ioctl (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int	minor = MINOR (inode->i_rdev);
	
	switch (minor)
	{
	case 0:
		if (0 == cmd)
			at91_set_gpio_output (RS485_1_CRTL, 0);	// Send mode
		else
			at91_set_gpio_output (RS485_1_CRTL, 1);	// Recv mode
		
		return 0;
	
	case 1:
		if (0 == cmd)
			at91_set_gpio_output (RS485_2_CRTL, 0);	// Send mode
		else
			at91_set_gpio_output (RS485_2_CRTL, 1);	// Recv mode
		
		return 0;
	}

    return -ENOIOCTLCMD;
}

struct file_operations fops_rs485 = {
	.owner	= THIS_MODULE,
	.open	= rs485_open,
	.ioctl	= rs485_ioctl
};

int init_module (void)
{
	if (0 > register_chrdev (MODULE_ID , MODULE_NAME, &fops_rs485))
		return -ENODEV;

    return 0;
}

void cleanup_module (void)
{
	unregister_chrdev (MODULE_ID, MODULE_NAME);
}

MODULE_LICENSE ("GPL");

