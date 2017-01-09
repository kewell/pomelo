/* GPIO Push Button driver by ivan wang */
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
//#include <asm/hardware.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <linux/delay.h>

#include <mach/platform.h>
#include "hgpio.h"

#define  IOWRITE(reg,value)       iowrite8(value, IO_ADDRESS(reg))
#define init_MUTEX(x) sema_init(x,1)

GPIO_INFO g_stGpioInfo;

unsigned int BoardID = 0x900c;

//#define IOWRITE8(address,value) iowrite8(value,IO_ADDRESS(address))
#define REG_BASE_SCTL 0x20050000

static int gpio_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int gpio_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = gpio_ioctl,
    .open = gpio_open,
    .release = gpio_release,
};

static struct miscdevice gpio_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "hi_gpio",
    .fops = &gpio_fops,
};

static int __init gpioi2c_init(void)
{
    char flag;
    int ret, i, j = 0;  

    ret = misc_register(&gpio_miscdev);
    if (ret < 0) 
    {
        printk(KERN_ERR "tlclk: misc_register returns %d.\n", ret);
        return ret;
    }
    
    g_stGpioInfo.gpio_ker_vir_addr[0]=(unsigned long)ioremap_nocache(GPIO0_BASE, 0x10000 * GPIO_GROUP_MAX);
    g_stGpioInfo.ddrc_ker_vir_addr[0]=(unsigned long)ioremap_nocache(DDRC0_BASE, 0x10000 * DDRC_GROUP_MAX);

    if(!g_stGpioInfo.gpio_ker_vir_addr[0])
    {
        misc_deregister(&gpio_miscdev);
        printk("ioremap gpio group0 failed!\n");
        return -1;
    }
    
    for(i = 1; i < GPIO_GROUP_MAX; i++)
    {
        g_stGpioInfo.gpio_ker_vir_addr[i] = g_stGpioInfo.gpio_ker_vir_addr[0] + 0x10000*i;
    }

    for(i = 1; i < DDRC_GROUP_MAX; i++)
    {
        g_stGpioInfo.ddrc_ker_vir_addr[i] = g_stGpioInfo.ddrc_ker_vir_addr[0] + 0x10000*i;
    }

    for(i = 0; i < GPIO_GROUP_MAX; i++)
    {
        flag = ioread8(g_stGpioInfo.gpio_ker_vir_addr[i] + GPIO_DIR);
        flag = 0xff;
        iowrite8(flag, g_stGpioInfo.gpio_ker_vir_addr[i] + GPIO_DIR); 
    }

    for(i = 0; i < GPIO_GROUP_MAX; i++)
    {
        for(j = 0; j < 8; j++)
        {
            WRITE_GPIO_DATA(i, j, 0);
        }
    }
    msleep(1000);

    for(i = 0; i < GPIO_GROUP_MAX; i++)
    {
        for(j = 0; j < 8; j++)
        {
            WRITE_GPIO_DATA(i, j, 1);
        }
    }

    msleep(1000);
    for(i = 0; i < GPIO_GROUP_MAX; i++)
    {
        for(j = 0; j < 8; j++)
        {
            WRITE_GPIO_DATA(i, j, 0);
        }
    }

    printk("GPIO driver init OK\n");
    g_stGpioInfo.gpio_status = 0xff;
    return 0;
}

static void __exit gpioi2c_exit(void)
{
    iounmap((void*)g_stGpioInfo.gpio_ker_vir_addr[0]);
    iounmap((void*)g_stGpioInfo.ddrc_ker_vir_addr[0]);
    misc_deregister (&gpio_miscdev);
    g_stGpioInfo.gpio_status = 0x00;
    printk("Gpioi2c driver exit\n");
}

module_init(gpioi2c_init);
module_exit(gpioi2c_exit);

module_param(BoardID, uint, S_IRUGO);
MODULE_PARM_DESC(BoardID,"0x401c,0x900b,0x900c");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hi3531 Developer");
MODULE_DESCRIPTION("GPIO driver for Hi3520 new board with HDMI (Small)");

