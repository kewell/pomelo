/*********************************************************************************
 *      Copyright:  (C) 2011 R&D of San Fran Electronics Co., LTD  
 *                  All rights reserved.
 *
 *       Filename:  dev_gprs.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(12/21/2011~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "12/21/2011 01:58:12 PM"
 *                 
 ********************************************************************************/
#include "include/plat_driver.h"

#define DRV_AUTHOR                "WENJING <wenjing0101@gmail.com>"
#define DRV_DESC                  "AT91SAM9XXX GPRS driver"

#define DEV_NAME                  DEV_GPRS_NAME

#ifndef DEV_MAJOR
#define DEV_MAJOR                 0 /*dynamic major by default */
#endif

static struct cdev dev_cdev;
static struct class *dev_class = NULL;

static int debug = DISABLE;
static int dev_major = DEV_MAJOR;
static int dev_minor = 0;

module_param(debug, int, S_IRUGO);
module_param(dev_major, int, S_IRUGO);
module_param(dev_minor, int, S_IRUGO);

#define dbg_print(format,args...) if(DISABLE!=debug)\
    {printk("[kernel] ");printk(format, ##args);}    
        
static unsigned char g_ucHW_Init = 0;

static void gprs_hw_init (void)
{
	unsigned int *US_CR;
	
	at91_set_A_periph (GPRS_CTS_PIN, DISPULLUP);
	at91_set_A_periph (GPRS_RTS_PIN, DISPULLUP);

	at91_set_gpio_output (GPRS_DTR_PIN, LOWLEVEL);
	at91_set_gpio_value (GPRS_DTR_PIN, LOWLEVEL);
	
	at91_set_gpio_input (GPRS_DSR_PIN, DISPULLUP);
	
	/* configure GPRS_RI_PIN interruput mode */
	at91_set_gpio_input (GPRS_RI_PIN, DISPULLUP);
    
	at91_set_gpio_input (GPRS_DCD_PIN, DISPULLUP);

	at91_set_gpio_input (GPRS_CHK_SIM1_PIN, DISPULLUP);
	at91_set_gpio_input (GPRS_CHK_SIM2_PIN, DISPULLUP);
	
	at91_set_A_periph (GPRS_RXD_PIN, DISPULLUP);
	at91_set_A_periph (GPRS_TXD_PIN, ENPULLUP);

	at91_sys_write (AT91_PMC_PCER, 1 << AT91SAM9260_ID_US1);
	US_CR = ioremap (AT91SAM9260_BASE_US1 + 0x00, 0x04);
	*US_CR = AT91C_US_RXEN;
	*US_CR = AT91C_US_TXEN;
	*US_CR = AT91C_US_DTREN;
	
	// Set the default sim to slot 1
	at91_set_gpio_output (GPRS_SELECT_SIM_PIN, HIGHLEVEL);
	at91_set_gpio_value (GPRS_SELECT_SIM_PIN, HIGHLEVEL);

	at91_set_gpio_output (GPRS_ON_PIN, HIGHLEVEL);		
	at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);		
	at91_set_gpio_output (GPRS_RESET_PIN, HIGHLEVEL);
	at91_set_gpio_value (GPRS_RESET_PIN, HIGHLEVEL);
}

// return 0 = have sim, else means dont have sim
static int gprs_chk_sim (int sim)
{
	if (1 == sim)
	{
        return at91_get_gpio_value (GPRS_CHK_SIM1_PIN);
    }
	else
	{
        return at91_get_gpio_value (GPRS_CHK_SIM2_PIN);
    }
}

// return 0 = sim 2, else means sim 1
static int gprs_get_sim (void)
{
	if (LOWLEVEL == at91_get_gpio_value (GPRS_SELECT_SIM_PIN))
	{
        return 2;
    }
	else
	{
        return 1;
    }
}

static void gprs_set_sim (int sim)
{
	if (2 == sim)
		at91_set_gpio_output (GPRS_SELECT_SIM_PIN, LOWLEVEL);
	else
		at91_set_gpio_output (GPRS_SELECT_SIM_PIN, HIGHLEVEL);

	SLEEP (10);
}

static void gprs_powerup (int sim)
{
	// 0 = use back current sim slot
	if (0 != sim)
	{
		// Check current selected sim slot with wanted sim slot
		if (sim != gprs_get_sim ())
		{
			gprs_set_sim (sim);
		}
	}
	
	at91_set_gpio_value (GPRS_ON_PIN, LOWLEVEL);		
	SLEEP(700);//GTM900B, 700s for Fibcom G600
	at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);
}

static void gprs_powerdown (void)
{
	at91_set_gpio_value (GPRS_ON_PIN, LOWLEVEL);
	SLEEP (2500);//GTM900B		
	at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);						
}

static void gprs_reset (void)
{
	at91_set_gpio_value( GPRS_RESET_PIN, LOWLEVEL );
	SLEEP( 100 );
	at91_set_gpio_value( GPRS_RESET_PIN, HIGHLEVEL );
	
}

static int gprs_open( struct inode *inode, struct file *file )
{
	if (0x00 == g_ucHW_Init)
	{
		g_ucHW_Init = 0x01;
		gprs_hw_init ();
	}
	
	return 0;
}

static int gprs_release( struct inode *inode, struct file *file )
{
	return 0;
}

//compatible with kernel version 2.6.38 
static long gprs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int flag;
	
	switch (cmd)
	{
	case GPRS_POWERON:
		if (!(0 == arg || 1 == arg || 2 == arg))
			return -1;

		// power up with selected sim
		gprs_powerup (arg);

		dbg_print("gprs powered up module with sim %ld\n", arg);
		return 0;
			
	case GPRS_POWERDOWN:
		gprs_powerdown ();

		dbg_print("gprs powered down module\n");
		return 0;
		
	case GPRS_RESET:
		if (!(0 == arg || 1 == arg || 2 == arg))
			return -1;

		if (0 != arg)
			gprs_set_sim (arg);
		
		gprs_reset ();
		
		dbg_print("gprs reset module\n");
		return 0;
	
	case CHK_WORK_SIMSLOT:
		flag = gprs_get_sim ();
		
		return flag;

	case SET_WORK_SIMSLOT:
		dbg_print("\n\nSET_SIM_SLOT: arg=%ld\n\n", arg);

		// 1, 11 = Select SIM 1
		// 2, 12 = Select SIM 2
		if (!(1 == arg || 2 == arg))
			return -1;

		// power up with selected sim
		gprs_powerup (arg);

		dbg_print("gprs selected sim %ld\n", arg);
		return 0;
		
	case GPRS_CHK_SIMDOOR:
		// Check the sim slot have sim or not
		if (!(1 == arg || 2 == arg))
			return -1;

		flag = gprs_chk_sim (arg);
		
		if (0 == flag)
			return 0;
		else
			return 1;

	case GPRS_SET_DTR:
		if (0 == arg)
			at91_set_gpio_value (GPRS_DTR_PIN, LOWLEVEL);
		else
			at91_set_gpio_value (GPRS_DTR_PIN, HIGHLEVEL);
		
		return 0;
		
	case SET_DRV_DEBUG:
		if (0 == arg)
			debug = 0x00;
		else
			debug = 0x01;
		
		return 0;
	}
	
	return -ENOIOCTLCMD;
}

static struct file_operations	gprs_fops =
{
	.owner = THIS_MODULE,
	.open = gprs_open,
	.release = gprs_release,
	.unlocked_ioctl = gprs_ioctl, //compatible with kernel version 2.6.38
};

static int __init gprs_init (void)
{
    int result;
    dev_t devno;

    /* Alloc for the device for driver */
    if (0 != dev_major)
    {
        devno = MKDEV(dev_major, dev_minor);
        result = register_chrdev_region(devno, 1, DEV_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&devno, dev_minor, 1, DEV_NAME);
        dev_major = MAJOR(devno);
    }

    /*Alloc for device major failure */
    if (result < 0)
    {
        printk("%s driver can't get major %d\n", DEV_NAME, dev_major);
        return result;
    }
	
    /*Initialize cdev structure and register it*/
	cdev_init (&dev_cdev, &gprs_fops);
	dev_cdev.owner 	= THIS_MODULE;
	
	result = cdev_add (&dev_cdev, devno , 1);
	if (result)
	{
	    printk (KERN_NOTICE "error %d add gprs device", result);
		goto ERROR;
	}

    dev_class = class_create(THIS_MODULE, DEV_NAME);
    if(IS_ERR(dev_class)) 
    {           
        printk("%s driver create class failture\n",DEV_NAME);           
        result =  -ENOMEM;  
        goto ERROR;   
    }       

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)     
    device_create(dev_class, NULL, devno, NULL, DEV_NAME);
#else
    device_create (dev_class, NULL, devno, DEV_NAME);
#endif  

    //gprs_hw_init ();

    printk("AT91 %s driver version %d.%d.%s <%s> initiliazed.\n", DEV_NAME, DRV_MAJOR_VER, 
        DRV_MINOR_VER, DRV_REVER_VER, __DATE__);

	return 0;

ERROR:
    cdev_del(&dev_cdev);
    unregister_chrdev_region(devno, 1);
    return result;
}


static void __exit gprs_exit (void)
{
    dev_t devno = MKDEV(dev_major, dev_minor);

    device_destroy(dev_class, devno);   
    class_destroy(dev_class);

    cdev_del(&dev_cdev);
    unregister_chrdev_region(devno, 1);

    printk("%s driver removed\n", DEV_NAME);
}


module_init (gprs_init);
module_exit (gprs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);

