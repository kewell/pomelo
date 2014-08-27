#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/arch/hardware.h>
#include <asm/arch/gpio.h>
#include <asm/arch/at91_pmc.h>

#include "L350ModemDrv.h"
#include "L350IoctlPara.h"

int modem_reset (void)
{
	at91_set_gpio_value (MODEM_PIN_RESET, LOWLEVEL);
	msleep (300);
	at91_set_gpio_value (MODEM_PIN_RESET, HIGHLEVEL);
	msleep (30);
	
	return 0;
}

int modem_dtr (int flag)
{
	if (LOWLEVEL != flag && HIGHLEVEL != flag)
		return -1;
	
	at91_set_gpio_value (MODEM_PIN_DTR, flag);
	
	return 0;
}

int modem_ring (void)
{
	return (1 == at91_get_gpio_value (MODEM_PIN_RI)) ? 0 : 1;
}

int modem_link (void)
{
	return at91_get_gpio_value (MODEM_PIN_DCD);
}

static int modem_pin_init (void)
{
	at91_set_A_periph (MODEM_PIN_RXD, DISPULLUP);
	at91_set_A_periph (MODEM_PIN_TXD, ENPULLUP);	
	at91_set_A_periph (MODEM_PIN_DTR, DISPULLUP);
	at91_set_A_periph (MODEM_PIN_DCD, DISPULLUP);
	at91_set_A_periph (MODEM_PIN_CTS, DISPULLUP);
	at91_set_A_periph (MODEM_PIN_RTS, DISPULLUP);

	at91_set_gpio_output (MODEM_PIN_RESET, HIGHLEVEL);
	at91_set_gpio_output (MODEM_PIN_DTR, LOWLEVEL);

	at91_set_gpio_input (MODEM_PIN_DCD, LOWLEVEL);
	at91_set_gpio_input (MODEM_PIN_RI, LOWLEVEL);

	return 0;
}

static int modem_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
{
	switch (cmd)
	{
	case MODEM_IOCTL_POWERON:
	case MODEM_IOCTL_RESET:
		return modem_reset ();
		
	case MODEM_IOCTL_DTR:
		return modem_dtr (arg);
	
	case MODEM_IOCTL_RING:
		return modem_ring ();
	
	case MODEM_IOCTL_LINK:
		return modem_link ();
	
	default:
		return -1;
	}

	return 0;
}

static int modem_open( struct inode *inode, struct file *file )
{
	return 0;
}

static struct file_operations	modem_fops =
{
	.owner = THIS_MODULE,
	.ioctl = modem_ioctl,
	.open = modem_open,
};

int modem_init (void)
{
	if ((register_chrdev (MODULE_ID, MODULE_NAME, &modem_fops)) < 0)
	{
		return -ENODEV;
	}

	modem_pin_init ();
	
	printk ("PSTN modem registered\n");
	return 0;
}

void modem_exit (void)
{
	unregister_chrdev (MODULE_ID, MODULE_NAME);
	printk ("PSTN modem unregistered\n");
}

module_init (modem_init);
module_exit (modem_exit);

MODULE_LICENSE ("GPL");

