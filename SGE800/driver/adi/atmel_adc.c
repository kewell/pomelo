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
	文件		：  adclib.c
	描述		：  本文件定义了9260内部ad转换模块的底层驱动程序接口的实现
	版本		：  0.2
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/

#include "adilib.h"
#include "atmel_adc.h"

//模块调试开关
#undef	DEBUG
//#define DEBUG
#ifdef DEBUG
#define	DPRINTK( x... )		printk("atmel_adc: " x)
#else
#define DPRINTK( x... )
#endif

/*************************************************
  静态全局变量及宏定义
*************************************************/

static struct cdev atmel_adc_cdev;
static dev_t atmel_adc_dev_t = MKDEV(MAJOR_DEVICE,0);
static struct class *drv_class;

static struct atmel_adc_dev_t atmel_adc_dev = {
		.name = "atmel_adc",
		.opened = ATOMIC_INIT(1),
};

/*************************************************
  API
*************************************************/



/******************************************************************************
*	函数:	atmel_adc_open
*	功能:	ad转换驱动打开函数,配置atmel_adc
*	参数:
*	返回:
 ******************************************************************************/
int atmel_adc_open(struct inode*inode,struct file *filp)
{	

	int retval = 0;

//	minor = iminor(inode);
	if(!atomic_dec_and_test(&atmel_adc_dev.opened)) {
		retval = -EBUSY;
		goto out;
	}

	filp->private_data = &atmel_adc_dev;

	DPRINTK("atmel_adc adc: Open!! \n");
	return retval;
out:
	atomic_inc(&atmel_adc_dev.opened);
	return retval;
	
	
}
/******************************************************************************
*	函数:	atmel_adc_release
*	功能:	ad转换驱动释放函数,释放资源
*	参数:
*	返回:
 ******************************************************************************/
int atmel_adc_release(struct inode *inode,struct file *filp)
{ 
	struct atmel_adc_dev_t *dev = filp->private_data;
	atomic_inc(&dev->opened);
	//ad通道作为输入
	if((dev->ch_en & (1 << 0)) == 1){
		at91_set_A_periph(AT91_PIN_PC0,0); //Mux ADC0 to GPIO
	}
	if((dev->ch_en  & (1 << 1)) == 1){
		at91_set_A_periph(AT91_PIN_PC1,0); //Mux ADC1 to GPIO
	}
	if((dev->ch_en  & (1 << 2)) == 1){
		at91_set_A_periph(AT91_PIN_PC2,0); //Mux ADC2 to GPIO
	}
	if((dev->ch_en  & (1 << 3)) == 1){
		at91_set_A_periph(AT91_PIN_PC3,0); //Mux ADC3 to GPIO
	}
	__raw_writel(0, atmel_adc_dev.base + AT91_ADC_CHDR);	//禁用通道
	DPRINTK("atmel_adc_adc: Release!!\n");
	return 0;
}
/******************************************************************************
*	函数:	atmel_adc_write
*	功能:	ad转换驱动写函数，暂不实现
*	参数:
*	返回:
 ******************************************************************************/
static ssize_t atmel_adc_write(struct file *file,const char *bur,size_t count,loff_t *offset)
{	
	return -EBUSY;

}

/******************************************************************************
*	函数:	at91_poll_status
*	功能:	查询spi状态寄存器，等待读取spi数据。每10us查询一次，共计1000次后若无数据则报错
*	参数:
*	返回:	>0 - 成功，有数据
*			0  - 超时
 ******************************************************************************/
static int at91_poll_status(unsigned long bit)
{
	int loop_cntr = 100;	//最大10ms
	int sr;
	do {
		udelay(100);	//每次延时100us，否则<10us读不上数据
		sr = __raw_readl(atmel_adc_dev.base + AT91_ADC_SR);

	} while (!(sr & bit) && (--loop_cntr > 0));

	return (loop_cntr > 0);
}

/* Function to read last conversion of channels*/
static inline u32 at91_channel_read(unsigned int chan)
{
	__raw_writel(AT91_ADC_START, atmel_adc_dev.base + AT91_ADC_CR); //Start the ADC
	if( at91_poll_status(AT91_ADC_EOC(chan)) <= 0)
		return -ETIMEDOUT;
	return __raw_readl(atmel_adc_dev.base + AT91_ADC_CHR(chan) ); //Read & Return the conversion
}

/******************************************************************************
*	函数:	atmel_adc_read
*	功能:	ad转换驱动查询读取ad结果函数，并做初步处理
*	参数:
*	返回:
 ******************************************************************************/
static ssize_t atmel_adc_read(struct file *filp,char  *buf,size_t count,loff_t *offset)
{
	int retval = 0;
	int ch = 0;
	u32 adc_data = 0;


	DPRINTK("%s:enter channel = %d\n",__FUNCTION__,count-4);

	if ((count < 4) | (count > 7)) {	//参数有效性判断
		return -EINVAL;
	}
	ch = count - 4;

	adc_data = at91_channel_read(ch);
	if(adc_data < 0)
		return -ETIMEDOUT;

	adc_data &= 0x03ff;
	DPRINTK("%s:adc_data=%x\n",__FUNCTION__,adc_data);

	retval = sizeof(adc_data);
	if (copy_to_user(buf, &adc_data, sizeof(adc_data))) {
			retval = -EFAULT;			
	}
	return retval;
}
/******************************************************************************
*	函数:	atmel_adc_ioctl
*	功能:	ad转换驱动控制函数，咱无实现
*	参数:
*	返回:
 ******************************************************************************/
int atmel_adc_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int retval=0;
	struct atmel_adc_dev_t *dev = file->private_data;
	dev->ch_en = arg;

	DPRINTK("%s: enter!! arg = %lx\n",__FUNCTION__,arg);
	switch(cmd){		

	case DS_ADI_CH:
		if((arg & (1 << 0)) == 1){
			at91_set_A_periph(AT91_PIN_PC0,0); //Mux ADC0 to GPIO
		}
		if((arg & (1 << 1)) == 1){
			at91_set_A_periph(AT91_PIN_PC1,0); //Mux ADC1 to GPIO
		}
		if((arg & (1 << 2)) == 1){
			at91_set_A_periph(AT91_PIN_PC2,0); //Mux ADC2 to GPIO
		}
		if((arg & (1 << 3)) == 1){
			at91_set_A_periph(AT91_PIN_PC3,0); //Mux ADC3 to GPIO
		}
		__raw_writel(arg, atmel_adc_dev.base + AT91_ADC_CHER);     //Enable Channels
		break;		
//	case DS_ADC_STOP:
	case 2:			
		break;
	default:
		break;
	}
	return retval;
}

static struct file_operations atmel_adc_fops = {
	.owner	= THIS_MODULE, 
	.open	= atmel_adc_open,
	.write	= atmel_adc_write,
	.read	= atmel_adc_read,
	.ioctl	= atmel_adc_ioctl,
	.release = atmel_adc_release,
}; 

/******************************************************************************
*	函数:	atmel_adc_init
*	功能:	ad转换驱动初始化函数，字符设备初始化，spi总线初始化
*	参数:
*	返回:
 ******************************************************************************/
static int __init atmel_adc_init(void)
{	
	int retval;
	DPRINTK(SOFTWARE_VERSION" build time:" __TIME__ "\n");	
	//DPRINTK("%s:enter,major = (%d)...\n",__FUNCTION__, MAJOR(atmel_adc_dev_t));

//注册字符设备驱动并创建供udev试用的设备类
	retval = register_chrdev_region (atmel_adc_dev_t, 1, atmel_adc_dev.name);
	if (retval)
		return retval;	
	cdev_init(&atmel_adc_cdev, &atmel_adc_fops);

	atmel_adc_cdev.owner = THIS_MODULE;
	retval = cdev_add(&atmel_adc_cdev, atmel_adc_dev_t, 1);
	if (retval)
		return retval;

	drv_class = class_create(THIS_MODULE, atmel_adc_dev.name);
	if (drv_class == NULL)
		return -ENOMEM;
	
	device_create(drv_class, NULL,atmel_adc_dev_t, NULL, atmel_adc_dev.name);

//硬件设备初始化
//	//set pin
//	at91_set_A_periph(AT91_PIN_PC0,0); //Mux ADC0 to GPIO
//	at91_set_A_periph(AT91_PIN_PC1,0); //Mux ADC1 to GPIO
//	at91_set_A_periph(AT91_PIN_PC2,0); //Mux ADC2 to GPIO
//	at91_set_A_periph(AT91_PIN_PC3,0); //Mux ADC3 to GPIO
	//enable adc clock
	atmel_adc_dev.clk = clk_get(NULL, "adc_clk");
	clk_enable(atmel_adc_dev.clk);
	//ioremap
	atmel_adc_dev.base = ioremap(AT91SAM9260_BASE_ADC, 0xff);

	//adc hardware init
	__raw_writel(AT91_ADC_SWRST, atmel_adc_dev.base + AT91_ADC_CR); //Reset the ADC
	__raw_writel((	AT91_ADC_SHTIM_(3) |
					AT91_ADC_STARTUP_(7) |
					AT91_ADC_PRESCAL_(9) |
					AT91_ADC_SLEEP |
					AT91_ADC_LOWRES |
					AT91_ADC_TRGSEL_TC1 |
					AT91_ADC_TRGEN), atmel_adc_dev.base + AT91_ADC_MR); //Mode setup
 	//__raw_writel(0x0f, atmel_adc_dev.base + AT91_ADC_CHER);     //Enable Channels
	//__raw_writel(CH_DIS, atmel_adc_dev.base + ADC_CHDR);    //Disable Channels
	
	DPRINTK(SOFTWARE_VERSION " init\n");
	return 0;
}
/******************************************************************************
*	函数:	atmel_adc_exit
*	功能:	ad转换驱动注销函数
*	参数:
*	返回:
 ******************************************************************************/
static void __exit atmel_adc_exit(void)
{	

	//disable spi master
//	__raw_writel(0, atmel_adc_dev.base + AT91_ADC_CHDR);
	//iounmap
	iounmap(atmel_adc_dev.base);
	
	//disable clock
	clk_disable(atmel_adc_dev.clk);
	clk_put(atmel_adc_dev.clk);

//delete device file and unregister char device
	device_destroy(drv_class, MKDEV(MAJOR_DEVICE,0));
	class_destroy(drv_class);		
	cdev_del(&atmel_adc_cdev);
	unregister_chrdev_region(atmel_adc_dev_t, 1);
	DPRINTK(SOFTWARE_VERSION " exit\n");

}

MODULE_AUTHOR("Roy <luranran@gmail.com>");
MODULE_DESCRIPTION("atmel_adc driver");
MODULE_LICENSE("GPL");
module_init(atmel_adc_init);
module_exit(atmel_adc_exit);

