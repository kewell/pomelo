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
	文件		：  atmel_ps.c
	描述		：  本文件定义了掉点检测模块的底层驱动程序接口
	版本		：  0.2
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/
#include "atmel_powerd.h"
#include "powerchecklib.h"

//模块调试开关
#undef	DEBUG
//#define DEBUG
#ifdef DEBUG
#define	DPRINTK( fmt,args... )		printk("atmel_powerd: "fmt,## args)
#else
#define DPRINTK( x... )
#endif

/*************************************************
  静态全局变量及宏定义
*************************************************/
static dev_t atmel_powerd_dev_t = MKDEV(MAJOR_DEVICE,0);
struct atmel_powerd_t atmel_powerd = {
		.name 		= DRIVER_NAME,
		.timeout	= 0,
		.opened		= ATOMIC_INIT(1),
		.mode		= POWERCHECK_MODE_BLOCK_UPDOWN,
};
static struct class *drv_class;

/*************************************************
  API
*************************************************/

/******************************************************************************
*	函数:	atmel_powerd_interrupt
*	功能:	io口电平跳变中断
*	参数:
*	返回:	IRQ_HANDLED		-	成功
 ******************************************************************************/
static irqreturn_t  atmel_powerd_interrupt(int irq,void *dev_id)
{
	atmel_powerd.io_level = at91_get_gpio_value(atmel_powerd.ioaddr);
	if(atmel_powerd.mode == POWERCHECK_MODE_BLOCK_UPDOWN){
		wake_up_interruptible(&(atmel_powerd.wq));
	}else if(atmel_powerd.mode == POWERCHECK_MODE_BLOCK_UP){
		if(atmel_powerd.io_level > 0)
			wake_up_interruptible(&(atmel_powerd.wq));
	}else if(atmel_powerd.mode == POWERCHECK_MODE_BLOCK_DOWN){
		if(atmel_powerd.io_level == 0)
			wake_up_interruptible(&(atmel_powerd.wq));
	}
	return IRQ_HANDLED;	
}

/******************************************************************************
*	函数:	atmel_powerd_open
*	功能:	掉点检测设备打开函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_powerd_open(struct inode *inode, struct file *filp)
{	
	int retval = 0;	
	//DPRINTK("%s:minor = %d\n",__FUNCTION__,minor);
	if(!atomic_dec_and_test(&atmel_powerd.opened)) {
		retval = -EBUSY;
		goto out;
	}
	init_MUTEX(&(atmel_powerd.sem));
	init_waitqueue_head( &(atmel_powerd.wq) );
	return retval;
out:
	atomic_inc(&atmel_powerd.opened);
	return retval;	
}

/******************************************************************************
*	函数:	atmel_powerd_release
*	功能:	掉点检测设备释放函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_powerd_release(struct inode *inode,struct file *filp)
{ 		


	free_irq(atmel_powerd.irq, NULL);
	atomic_inc(&atmel_powerd.opened);
	DPRINTK("atmel_powerd Release!!\n");
	return 0;
}

/******************************************************************************
*	函数:	atmel_powerd_read
*	功能:	掉点检测读取函数
*	参数:
*	返回:
 ******************************************************************************/
ssize_t atmel_powerd_read(struct file *filp,char  *buf,size_t count,loff_t *offset)
{	
	u8 level=0xff;
	DPRINTK("%s:enter \n",__FUNCTION__);
	//非阻塞模式立即返回当前接口电平
	if( filp->f_flags & O_NONBLOCK){
		atmel_powerd.io_level = at91_get_gpio_value(atmel_powerd.ioaddr);
		if (copy_to_user(buf, &atmel_powerd.io_level, 1)) {
			return -EFAULT;
		}
	}else{
		if(atmel_powerd.timeout == 0)
			interruptible_sleep_on(&(atmel_powerd.wq));
		else
			interruptible_sleep_on_timeout(&(atmel_powerd.wq),atmel_powerd.timeout);
		if(signal_pending(current))
			return -ERESTARTSYS;
		local_irq_disable();
		level = atmel_powerd.io_level;
		local_irq_enable();
		if (copy_to_user(buf, &level, 1)) {
			return -EFAULT;
		}
		level=0xff;
	}
	return 0;
}

/******************************************************************************
*	函数:	atmel_powerd_ioctl
*	功能:	掉电检测控制函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_powerd_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int retval = -1;
	retval = down_interruptible(&(atmel_powerd.sem));
	if (retval) {
		retval = -EBUSY;
		return retval;	
	}	

	DPRINTK("%s:enter,cmd = (%d)...arg = %lx\n",__FUNCTION__, cmd,arg);
	switch (cmd) {
		
		case PWRD_SET_IO:
			atmel_powerd.ioaddr = arg;
			atmel_powerd.irq = arg;
			at91_set_gpio_input(atmel_powerd.ioaddr, 0);
			at91_set_deglitch(atmel_powerd.ioaddr,1);
			retval = request_irq(	atmel_powerd.irq,
									atmel_powerd_interrupt,
									IRQF_DISABLED,
									atmel_powerd.name,
									NULL);

			if(retval < 0)
				return -EFAULT;
			break;

		case PWRD_TIMEOUT:
			atmel_powerd.timeout = arg;
			retval = 0;
			break;
	
		case PWRD_SET_MODE:
			atmel_powerd.mode = arg;
			retval = 0;
			break;
		default:
			retval = -EFAULT;
			break;
	}
	up(&(atmel_powerd.sem));

	return retval;		
}

static struct file_operations atmel_powerd_fops = {
	.owner	= THIS_MODULE, 
	.open	= atmel_powerd_open,
	.read	= atmel_powerd_read,
	.ioctl	= atmel_powerd_ioctl,
	.release = atmel_powerd_release,
}; 

/******************************************************************************
*	函数:	atmel_powerd_init
*	功能:	掉点检测初始化函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
static int __init atmel_powerd_init(void)
{
	static int retval=0;	
	strcpy(atmel_powerd.name ,DRIVER_NAME);
	DPRINTK(SOFTWARE_VERSION" build time:" __TIME__ "\n");
	DPRINTK("%s:enter,major = (%d)...\n",__FUNCTION__, MAJOR(atmel_powerd_dev_t));
	
	//字符设备初始化，创建类，在sysfs中注册设备，以备udev创建设备节点
	retval = register_chrdev_region (atmel_powerd_dev_t, 1, atmel_powerd.name);
	if (retval)
		return retval;		
	cdev_init(&(atmel_powerd.cdev), &atmel_powerd_fops);
	atmel_powerd.cdev.owner = THIS_MODULE;
	retval = cdev_add(&atmel_powerd.cdev, atmel_powerd_dev_t, 6);
	if (retval)
		return retval;	

	drv_class = class_create(THIS_MODULE, atmel_powerd.name);
	if (drv_class == NULL)
		return -ENOMEM;			
	device_create(drv_class, NULL,atmel_powerd_dev_t, NULL,atmel_powerd.name);

//	//初始化打开计数
//	atomic_set(&atmel_powerd.opened, 1);    //initialate device open count
	DPRINTK("%s: done!\n",__FUNCTION__);	
	return 0;
}

static void __exit atmel_powerd_exit(void)
{		
	device_destroy(drv_class, MKDEV(MAJOR_DEVICE,0));
	class_destroy(drv_class);	
	cdev_del(&atmel_powerd.cdev);
	unregister_chrdev_region(atmel_powerd_dev_t, 1);
	DPRINTK("exit!\n");
}

MODULE_AUTHOR("Roy <luranran@gmail.com>");
MODULE_DESCRIPTION("atmel_powerd driver");
MODULE_LICENSE("GPL");
module_init(atmel_powerd_init);
module_exit(atmel_powerd_exit);
