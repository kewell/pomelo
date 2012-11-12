/*********************************************************************************
 *      Copyright:  (C) 2012 KEWELL
 *
 *       Filename:  dev_beep.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(09/26/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "09/26/2012 09:41:13 AM"
 *                  2, Fix Bug about wrong definition about all PMC E/D/SR register 2012-11-12 08:46:50 
 *                 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 ********************************************************************************/

#include "include/plat_driver.h"

/*Driver version*/
#define DRV_MAJOR_VER             2
#define DRV_MINOR_VER             0
#define DRV_REVER_VER             0

#define DEV_NAME                  DEV_BEEP_NAME

#ifndef DEV_MAJOR
#define DEV_MAJOR                 0/*dynamic major by default */
#endif

static struct cdev *dev_cdev = NULL;
static struct class *dev_class = NULL;
static struct semaphore lock;

static int debug = DISABLE;
static int dev_major = DEV_MAJOR;
static int dev_minor = 0;

module_param(debug, int, S_IRUGO);
module_param(dev_major, int, S_IRUGO);
module_param(dev_minor, int, S_IRUGO);

volatile unsigned int *PMC_SCER;
volatile unsigned int *PMC_SCDR;
volatile unsigned int *PMC_PCK1;
volatile unsigned int *PMC_SR;

static unsigned int m_uiPrescaler = 0x5;

#define dbg_print(format,args...) if(DISABLE!=debug) \
{printk("[kernel] ");printk(format, ##args);}

static int beep_open(struct inode *inode, struct file *file)
{
    if (!down_trylock(&lock)) 
        return 0;   
    else        
        return -EBUSY;
}

static int beep_release(struct inode *inode, struct file *file)
{
    up(&lock);
    return 0;
}

//compatible with kernel version 2.6.38 
static long beep_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned long prescaler;

    switch (cmd)
    {
        case SET_DRV_DEBUG:
            dbg_print("%s driver debug now.\n", DISABLE == arg ? "Disable" : "Enable");

            if (0 == arg)
                debug = DISABLE;
            else
                debug = ENABLE;

            break;

        case GET_DRV_VER:
            print_version(DRV_VERSION);
            return DRV_VERSION;

        case BEEP_ENALARM:
            at91_set_B_periph(BEEP_PIN, DISPULLUP);  //pck1 
            *PMC_SCER |= (0x1 << 9); //enable pck1
            break;

        case BEEP_DISALARM:
            *PMC_SCDR |= (0x1 << 9);  //disable pck1
            at91_set_gpio_output(BEEP_PIN, HIGHLEVEL);
            break;

        case SET_DEFAULT_BEEP_FREQ:
            prescaler = arg;
            if (prescaler > 0x6 || prescaler < 0x0)
            {
                printk("BEEP DRV: ioctl SET_DEFAULT_BEEP_FREQ parameter error\n");
                return -EFAULT;
            }
            *PMC_SCDR |= (0x1 << 9);
            m_uiPrescaler = (unsigned int)prescaler;
            *PMC_PCK1 = ((m_uiPrescaler & 0xff) << 2); //select sclk all long
            while ((0x1 << 9) != ((*PMC_SR) & (0x1 << 9)));

            break;

        default:
            printk("%s driver don't support ioctl command = %d, And Enable = %d, Disable = %d\n", DEV_NAME, cmd, BEEP_ENALARM, BEEP_DISALARM);
            return -1;
    }

    return 0;
}

static struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .release = beep_release,
    .unlocked_ioctl = beep_ioctl,  //compatible with kernel version 2.6.38 
};

#if 0
----------------linux-3.6-rc4------------------
/////////////////////////////////////////////
#define AT91_PMC    0xfffffc00

#define AT91_PMC_SCER       0x00            /*  System Clock Enable Register */
#define AT91_PMC_SCDR       0x04            /*  System Clock Disable Register */
    
#define AT91_PMC_SCSR       0x08            /*  System Clock Status Register */
/////////////////////////////////////////////

----------------linux-3.0    ------------------
/////////////////////////////////////////////
#define AT91_PMC    (0xfffffc00 - AT91_BASE_SYS)

#define AT91_PMC_SCER       (AT91_PMC + 0x00)   /*  System Clock Enable Register */
#define AT91_PMC_SCDR       (AT91_PMC + 0x04)   /*  System Clock Disable Register */
    
#define AT91_PMC_SCSR       (AT91_PMC + 0x08)   /*  System Clock Status Register */
/////////////////////////////////////////////

#endif    
static void beep_hw_init(void)
{
    PMC_SCER = ioremap(AT91_PMC + AT91_BASE_SYS + AT91_PMC_SCER, 0x04);    //PMC System Clock Enable Register,control pck/pck0/pck1...
    PMC_SCDR = ioremap(AT91_PMC + AT91_BASE_SYS + AT91_PMC_SCDR, 0x04);    //PMC System Clock Disable Register 
    PMC_PCK1 = ioremap(AT91_PMC + AT91_BASE_SYS + AT91_PMC_PCKR(1), 0x04); //PMC Programmable Clock Register,pck1 register
    PMC_SR   = ioremap(AT91_PMC + AT91_BASE_SYS + AT91_PMC_SR, 0x04);

    at91_set_gpio_output(BEEP_PIN, HIGHLEVEL);
}

static void beep_hw_term(void)
{
    *PMC_SCDR |= (0x1 << 9);  //disable pck1
    at91_set_gpio_output(BEEP_PIN, HIGHLEVEL);

    iounmap(PMC_SCER);
    iounmap(PMC_SCDR);
    iounmap(PMC_PCK1);
    iounmap(PMC_SR);
}

static void beep_cleanup(void)
{
    dev_t devno = MKDEV(dev_major, dev_minor);

    beep_hw_term();

    device_destroy(dev_class, devno);   
    class_destroy(dev_class);

    cdev_del(dev_cdev);
    unregister_chrdev_region(devno, 1);

    printk("%s driver removed\n", DEV_NAME);
}

static int __init beep_init(void)
{
    int result;
    dev_t devno;

    /*Init beep hardware*/
    beep_hw_init();
    udelay(100);

    /*init semaphore for mutes*/
    sema_init(&lock, 1);

    /*Alloc for the device for driver */
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

    /*Alloc cdev structure */
    dev_cdev = cdev_alloc();;
    if (NULL == dev_cdev)
    {
        unregister_chrdev_region(devno, 1);
        printk("%s driver can't alloc for beep_cdev\n", DEV_NAME);
        return -ENOMEM;
    }

    /*Initialize cdev structure and register it */
    dev_cdev->owner = THIS_MODULE;
    dev_cdev->ops = &beep_fops;
    result = cdev_add(dev_cdev, devno, 1);
    if (0 != result)
    {
        printk("%s driver can't alloc for beep_cdev\n", DEV_NAME);
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

    printk("%s driver version %d.%d.%d initiliazed\n", DEV_NAME, DRV_MAJOR_VER, DRV_MINOR_VER,
            DRV_REVER_VER);

    return 0;

ERROR:
    cdev_del(dev_cdev);
    unregister_chrdev_region(devno, 1);
    return result;
}

module_init(beep_init);
module_exit(beep_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);

