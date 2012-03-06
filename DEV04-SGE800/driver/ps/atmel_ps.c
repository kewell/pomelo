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
	描述		：  本文件定义了录波模块的底层驱动程序接口
	版本		：  0.2
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/
#include "atmel_ps.h"
#include "pulselib.h"

//模块调试开关
#undef	DEBUG
//#define DEBUG
#ifdef DEBUG
#define	DPRINTK( x... )		printk("atmel_ps: " x)
#else
#define DPRINTK( x... )
#endif

/*************************************************
  静态全局变量及宏定义
*************************************************/
static dev_t atmel_ps_dev_t = MKDEV(MAJOR_DEVICE,0);
struct atmel_ps_t atmel_ps;
static struct class *drv_class;

/*************************************************
  API
*************************************************/

/******************************************************************************
*	函数:	atmel_tc_interrupt
*	功能:	定时器中断函数,每隔设定的毫秒数中断一次，用来录波
*	参数:
*	返回:	IRQ_HANDLED		-	成功
 ******************************************************************************/
static irqreturn_t  atmel_tc_interrupt(int irq,void *dev_id)
{		
	int i=0;
	__raw_readl(atmel_ps.tc_regs + ATMEL_TC_SR);		/* Read ps0 Status Register to clear it */
	atmel_ps.val_temp_buf = 0x0000;
	for(i = atmel_ps.count-1; i >= 0; i --){
		atmel_ps.val_temp_buf |= (at91_get_gpio_value(atmel_ps.channel[i]) << i);
	}
	DPRINTK("%s:read buf = %x\n",__FUNCTION__,atmel_ps.val_temp_buf);
	BUF_HEAD = atmel_ps.val_temp_buf;		//存放数据到环形缓冲区头，头指针加1
	atmel_ps.head = INCBUF(atmel_ps.head, MAX_PS_BUF/2);
	DPRINTK("%s:read buf = %x,head = %d,tail = %d\n",__FUNCTION__,atmel_ps.val_temp_buf,atmel_ps.head,atmel_ps.tail);
	wake_up_interruptible(&(atmel_ps.wq));			
	return IRQ_HANDLED;	
}

/******************************************************************************
*	函数:	atmel_ps_open
*	功能:	录波设备打开函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_ps_open(struct inode *inode, struct file *filp)
{	
	int retval = 0;	
	//DPRINTK("%s:minor = %d\n",__FUNCTION__,minor);
	if(!atomic_dec_and_test(&atmel_ps.opened)) {	
		retval = -EBUSY;
		goto out;
	}

	//初始化结构体
	atmel_ps.head = atmel_ps.tail = 0;	//初始化环形缓冲区为空
	atmel_ps.count = 0;  				//初始化通道数为0
	atmel_ps.val_temp_buf = 0x0000;

	atmel_ps.val_buf = kzalloc(MAX_PS_BUF, GFP_KERNEL);
	if(!atmel_ps.val_buf){
		retval = -EFAULT;
		goto out;
	}
	//memset(atmel_ps.val_buf,0,MAX_PS_BUF);
	atmel_ps.val_ret = kzalloc(MAX_PS_BUF, GFP_KERNEL);
	if(!atmel_ps.val_ret){
		retval = -EFAULT;
		goto out;
	}
	//memset(atmel_ps.val_ret,0,MAX_PS_BUF);
	
	init_MUTEX(&(atmel_ps.sem));		
	init_waitqueue_head( &(atmel_ps.wq) );
	return retval;
out:
	atomic_inc(&atmel_ps.opened);
	return retval;	
}

/******************************************************************************
*	函数:	atmel_ps_release
*	功能:	录波设备释放函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_ps_release(struct inode *inode,struct file *filp)
{ 		
	iounmap(atmel_ps.tc_regs);	
	atmel_ps.tc_regs = NULL;	
	free_irq(atmel_ps.tc_irq, NULL);
	kfree(atmel_ps.val_ret);
	kfree(atmel_ps.val_buf);
	atomic_inc(&atmel_ps.opened);
	DPRINTK("atmel_ps Release!!\n");
	return 0;
}

/******************************************************************************
*	函数:	atmel_ps_read
*	功能:	录波读取函数
*	参数:
*	返回:	>=0 - 成功读取到的字节数
 ******************************************************************************/
ssize_t atmel_ps_read(struct file *filp,char  *buf,size_t count,loff_t *offset)
{	
	int retval = 0;
	int i=0;
	u16 head_cur;		//当前头指针
	u16 *atmel_ps_ret_t;//搬运数据指针
	
	memset(atmel_ps.val_ret,0,MAX_PS_BUF);
	atmel_ps_ret_t = atmel_ps.val_ret;
	DPRINTK("%s:enter \n",__FUNCTION__);		
retry:	
	//禁用中断，设置当前要读取的头指针
	disable_irq_nosync(atmel_ps.tc_irq);
	head_cur = atmel_ps.head;
	enable_irq(atmel_ps.tc_irq);

	//当前环形缓冲区不为空时，从尾部读取数据，否则睡眠
	if( head_cur != atmel_ps.tail ){
		i=0;		
		do{
			memcpy(atmel_ps_ret_t++,&BUF_TAIL,sizeof atmel_ps_ret_t );		//读走数据
			atmel_ps.tail = INCBUF(atmel_ps.tail, MAX_PS_BUF/2);	//缓冲区尾指针加1
			i++;
		}while(head_cur != atmel_ps.tail);		//直到读走所有缓冲区已有数据
		retval = 2*i;	//返回总共读取到的字节数
		atmel_ps_ret_t = atmel_ps.val_ret;				
		DPRINTK("%s:ret = %d,data = %x ,tail = %d\n",__FUNCTION__,2*i,*(atmel_ps_ret_t),atmel_ps.tail);
		if (copy_to_user(buf, (void*)atmel_ps.val_ret, retval)) {
			return -EFAULT;				
		}			
	}else{			
		if( filp->f_flags & O_NONBLOCK)
			return -EAGAIN;		
		interruptible_sleep_on(&(atmel_ps.wq)); 	
		if(signal_pending(current))
			return -ERESTARTSYS;		
		goto retry;
	}

	return retval;
}

/******************************************************************************
*	函数:	atmel_ps_ioctl
*	功能:	录波控制函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_ps_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int retval = 0;		
	retval = down_interruptible(&(atmel_ps.sem));
	if (retval) {
		retval = -EBUSY;
		return retval;	
	}	
	DPRINTK("%s:enter,cmd = (%d)...arg = %lx\n",__FUNCTION__, cmd,arg);
	switch (cmd) {
		
		case SET_TC:	
			if( (arg<0) | (arg>5) ){
				retval = -EFAULT;
				break;
			}
			atmel_ps.tc_channel = arg;
			if(atmel_ps.tc_channel < 3){
				atmel_ps.tc_regs = ioremap(AT91SAM9260_BASE_TCB0 + atmel_ps.tc_channel*0x40, 0x40);
				atmel_ps.tc_irq = AT91SAM9260_ID_TC0+atmel_ps.tc_channel;
			}else{		
				atmel_ps.tc_regs = ioremap(AT91SAM9260_BASE_TCB1 + (atmel_ps.tc_channel-3)*0x40, 0x40);
				atmel_ps.tc_irq = AT91SAM9260_ID_TC3 + atmel_ps.tc_channel - 3;
			}
			
			retval = request_irq(atmel_ps.tc_irq, 
									atmel_tc_interrupt,
									IRQF_DISABLED,
									atmel_ps.name,
									NULL);
			break;

		case ADD_IO:			
			atmel_ps.channel[atmel_ps.count] = (u32)arg;			
			retval = at91_set_gpio_input(atmel_ps.channel[atmel_ps.count], 0);
			DPRINTK("%s:ADD_IO,retval = %d;io = %x;count= %d\n",__FUNCTION__,
					retval,atmel_ps.channel[atmel_ps.count],atmel_ps.count);
			if(retval < 0)
				break;
			atmel_ps.count++;				
			break;
	
		case PSTART:
			__raw_writel(ATMEL_TC_CLKEN, atmel_ps.tc_regs + ATMEL_TC_CCR); /* Enable the Clock Counter */		
			__raw_writel(ATMEL_TC_SWTRG, atmel_ps.tc_regs + ATMEL_TC_CCR); /* Enable the Clock Counter */
			retval = 0;
			break;
		
		default:
			retval = -EFAULT;			
	}
	up(&(atmel_ps.sem));		
	return retval;		
}

static struct file_operations atmel_ps_fops = { 
	.owner	= THIS_MODULE, 
	.open	= atmel_ps_open,  
	.read	= atmel_ps_read, 
	.ioctl	= atmel_ps_ioctl,
	.release = atmel_ps_release, 
}; 

/******************************************************************************
*	函数:	atmel_ps_init
*	功能:	录波初始化函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
static int __init atmel_ps_init(void)
{
	static int retval=0;	
	strcpy(atmel_ps.name ,DRIVER_NAME);
	DPRINTK(SOFTWARE_VERSION" build time:" __TIME__ "\n");
	DPRINTK("%s:enter,major = (%d)...\n",__FUNCTION__, MAJOR(atmel_ps_dev_t));
	
	//字符设备初始化，创建类，在sysfs中注册设备，以备udev创建设备节点
	retval = register_chrdev_region (atmel_ps_dev_t, 1, atmel_ps.name);
	if (retval)
		return retval;		
	cdev_init(&(atmel_ps.cdev), &atmel_ps_fops);	
	atmel_ps.cdev.owner = THIS_MODULE;
	retval = cdev_add(&atmel_ps.cdev, atmel_ps_dev_t, 6);
	if (retval)
		return retval;	

	drv_class = class_create(THIS_MODULE, atmel_ps.name);
	if (drv_class == NULL)
		return -ENOMEM;			
	device_create(drv_class, NULL,atmel_ps_dev_t, NULL,atmel_ps.name);

	//初始化打开计数
	atomic_set(&atmel_ps.opened, 1);    //initialate device open count	
	DPRINTK("%s: done!\n",__FUNCTION__);	
	return 0;
}

static void __exit atmel_ps_exit(void)
{		
	device_destroy(drv_class, MKDEV(MAJOR_DEVICE,0));
	class_destroy(drv_class);	
	cdev_del(&atmel_ps.cdev);
	unregister_chrdev_region(atmel_ps_dev_t, 1);
	DPRINTK("exit!\n");
}

MODULE_AUTHOR("Roy <luranran@gmail.com>");
MODULE_DESCRIPTION("atmel_ps driver");
MODULE_LICENSE("GPL");
module_init(atmel_ps_init);
module_exit(atmel_ps_exit);
