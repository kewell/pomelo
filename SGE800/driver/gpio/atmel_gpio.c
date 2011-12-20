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
	文件		：  atmel_gpio.c
	描述		：  本文件定义了ad转换模块的底层驱动程序接口
	版本		：  0.2
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/
#include "atmel_gpio.h"
#include "gpiolib.h"

//模块调试开关
#undef	DEBUG
//#define DEBUG
#ifdef DEBUG
#define	DPRINTK( x... )		printk("atmel_gpio: " x)
#else
#define DPRINTK( x... )
#endif

/*************************************************
  静态全局变量及宏定义
*************************************************/

struct cdev atmel_gpio_cdev;		//字符设备
struct class *atmel_gpio_class;		//类

/*************************************************
  API
*************************************************/

/******************************************************************************
*	函数:	gpio_open
*	功能:	gpio设备打开函数
*	参数:
*	返回:	0		-	成功
 ******************************************************************************/
int gpio_open(struct inode*inode,struct file *file)
{	
	//unsigned int temp = 0;
	DPRINTK( "atmel_gpio: Open!!  \n");
	return 0;
}

/******************************************************************************
*	函数:	gpio_release
*	功能:	gpio设备释放函数
*	参数:
*	返回:	0		-	成功
 ******************************************************************************/
int gpio_release(struct inode *inode,struct file *file)
{ 
	DPRINTK( "atmel_gpio: Release!!\n");
	return 0;
}

/******************************************************************************
*	函数:	io口控制函数
*	功能:	atmel 获取io口的内存映射控制
*	参数:
*	返回:
 ******************************************************************************/
static inline void __iomem *pin_to_controller_ry(unsigned pin)
{
	void __iomem *sys_base = (void __iomem *) AT91_VA_BASE_SYS;	
	pin -= PIN_BASE;
	pin /= 32;
	
	if (likely(pin < MAX_GPIO_BANKS))
		return sys_base + atmel_gpio[pin];
	return NULL;
}
static inline unsigned pin_to_mask_ry(unsigned pin)
{
	pin -= PIN_BASE;
	return 1 << (pin % 32);
}

/******************************************************************************
*	函数:	gpio_ctl_ioctl
*	功能:	gpio设备控制函数，设置io口的属性及状态
*	参数:
*	返回:	0				-	成功
 ******************************************************************************/
int gpio_ctl_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	static unsigned int temp = 0 ;
	static int retval = 0;
	u32	pdsr = 0, odsr = 0;		//存放io读取值
	u32 pin=0;

	void __iomem *pio ;
	unsigned mask ;
	//io地址映射
	if((cmd == IOGETI) || (cmd == IOGETO)){
		pin = *((u32*)arg);
	}else{
		pin = arg;
	}

	pio = pin_to_controller_ry(pin);
	mask = pin_to_mask_ry(pin);
	if (!pio)
		return -EINVAL;


	DPRINTK( "%s:enter,%d,%d,%d\n",__FUNCTION__,IOGETI,cmd,pin);
	switch(cmd){
		
		case SETI:	//设置输入
			__raw_writel(mask, pio + PIO_IDR);
			__raw_writel(mask, pio + PIO_ODR);
			__raw_writel(mask, pio + PIO_PER);
			//DPRINTK( "atmel_gpio: Ioctl set !! arg=%d\n",arg);
			break;

		case SETO:	//设置输出
			__raw_writel(mask, pio + PIO_IDR);
			__raw_writel(mask, pio + PIO_OER);
			__raw_writel(mask, pio + PIO_PER);
			//DPRINTK( " Ioctl output mode !! arg=%ld\n",arg);
			break;

		case OCLR:	//设置输出为0
			__raw_writel(mask, pio + PIO_CODR);
			//DPRINTK( " Ioctl output clr !! arg=%ld\n",arg);
			break;

		case OSET:	//设置输出为1
			__raw_writel(mask, pio + PIO_SODR);
			//DPRINTK( " Ioctl output set !! arg=%ld\n",arg);
			break;

		case ODE:	//设置OD门使能
			__raw_writel(mask, pio + PIO_MDER);
			break;

		case ODD:	//设置OD门禁用
			__raw_writel(mask, pio +PIO_MDDR);
			break;

		case PUE:	//设置上拉使能
			__raw_writel(mask, pio + PIO_PUER);
			break;

		case PUD:	//设置上拉禁用
			__raw_writel(mask, pio + PIO_PUDR);
			break;

		case IOGETI:	//获取输入值
			DPRINTK( "%s:IOGETI IO = %d\n",__FUNCTION__, pin);
			pdsr = __raw_readl(pio + PIO_PDSR);
			temp = ((pdsr & mask) != 0);
			retval = put_user(temp, ((int *)arg));
			break;

		case IOGETO:	//获取输出值
			odsr = __raw_readl(pio + PIO_ODSR);
			DPRINTK( "%s:IOGETO IO = %d\n",__FUNCTION__, pin);
			DPRINTK( "%s:read osr = %d\n",__FUNCTION__,odsr);
			temp = ((odsr & mask) != 0);
			DPRINTK( "%s:read output IO val = %d\n",__FUNCTION__,temp);
			retval = put_user(temp, ((int *)arg));
			break;
		case IFE:	//设置滤波使能
			__raw_writel(mask, pio + PIO_IFER);
			break;
		case IFD:	//设置滤波禁用
			__raw_writel(mask, pio + PIO_IFDR);
			break;
		default:
			__raw_writel(mask, pio + PIO_IDR);
			__raw_writel(mask, pio + PIO_ODR);
	}

	retval = 0;
	return retval;
}

struct file_operations atmel_gpio_fops= {
	.owner 		= THIS_MODULE,
	.ioctl 		= gpio_ctl_ioctl,
	.open  		= gpio_open,
	.release 	= gpio_release,
};

/******************************************************************************
*	函数:	atmel_gpio_exit
*	功能:	gpio设备关闭函数
*	参数:
*	返回:	0	-	成功
 ******************************************************************************/
void atmel_gpio_exit(void)
{
	dev_t dev = 0;
	dev = MKDEV(IOPORT_MAJOR, 0);
	cdev_del(&atmel_gpio_cdev);
	unregister_chrdev_region(dev, 1);
	
    device_destroy(atmel_gpio_class, dev);
    class_destroy(atmel_gpio_class);
	
    DPRINTK(KERN_WARNING "atmel_gpio: exit!!\n");
}

/******************************************************************************
*	函数:	atmel_gpio_init
*	功能:	gpio设备初始化函数
*	参数:
*	返回:	0	-	成功
 ******************************************************************************/
int __init atmel_gpio_init(void)
{

	int ret;

	dev_t dev = 0;
	dev = MKDEV(IOPORT_MAJOR, 0);
	ret = register_chrdev_region(dev, 1, "atmel_gpio");
	if(ret < 0 )	{
		DPRINTK( "init: can't get major %d!!\n", IOPORT_MAJOR);
		return ret;
	}
	//字符设备初始化，添加。
	cdev_init(&atmel_gpio_cdev, &atmel_gpio_fops);
	atmel_gpio_cdev.owner = THIS_MODULE;
	atmel_gpio_cdev.ops = &atmel_gpio_fops;
	ret = cdev_add(&atmel_gpio_cdev,dev,1);
	if(ret)
		DPRINTK( "init: Error %d adding atmel_gpio!!\n", ret);

	//创建类，在sysfs中注册设备，以备udev创建设备节点
    atmel_gpio_class = class_create(THIS_MODULE, "atmel_gpio");
    if(IS_ERR(atmel_gpio_class)) {
    	DPRINTK("Err: failed in creating class.\n");
        return -1;
    }
    device_create(atmel_gpio_class, NULL, dev, NULL, "atmel_gpio");

	DPRINTK("init: init done!!\n");
	return 0;
}
module_init(atmel_gpio_init);
module_exit(atmel_gpio_exit);

MODULE_AUTHOR("roy <luranran@gmail.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("GPIO driver For AT91SAM9260");
