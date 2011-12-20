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
	描述		：  本文件定义了ad转换模块的底层驱动程序接口的实现
	版本		：  0.2
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/

#include "adclib.h"
#include "atmel_adc.h"

//模块调试开关
#undef	DEBUG
//#define DEBUG
#ifdef DEBUG
#define	DPRINTK( x... )		printk("tlv1504: " x)
#else
#define DPRINTK( x... )
#endif

/*************************************************
  静态全局变量及宏定义
*************************************************/

static struct cdev tlv1504_cdev;
static dev_t tlv1504_dev_t = MKDEV(MAJOR_DEVICE,0);
static struct class *drv_class;

static struct tlv1504_dev_t tlv1504_dev = {
		.name = "tlv1504",
		.opened = ATOMIC_INIT(1),
		.addr	= 0x0,
		.data_max	= 4,

		.baud = SPI_DEFAULT_BAUD,
};

/*************************************************
  API
*************************************************/

/******************************************************************************
*	函数:	tlv1504_config
*	功能:	tlv1504芯片的工作模式配置函数,写cfr
*	参数:
*	返回:
 ******************************************************************************/
void tlv1504_config(void)
{
	//1 1 0 00 00 00 1 00=3076
	__raw_writel( CFR_WRITE	|			//写cfr命令，后面跟12位数据
					INTER_REF	|		//参考电压内部参考
					INT_REF_V2	|		//内部2V参考电压
					SAM_PERIOD12	|	//选择短采样时钟12SCLK
					SELECT_CLK_INT	|	//AD转换时钟源选自内部晶振
					SELECT_MODE_S	|	//AD转换模式选择读取单路结果
					SWEEP_SEQ	|		//无循环读取各路转换
					INT_EN	|			//EOC/INT引脚用作EOC
					FIFO_TRIG_LEVEL0,	//FIFO中断触发设置为满触发
					tlv1504_dev.base + AT91_SPI_TDR);
}

/******************************************************************************
*	函数:	tlv1504_open
*	功能:	ad转换驱动打开函数,配置tlv1504
*	参数:
*	返回:
 ******************************************************************************/
int tlv1504_open(struct inode*inode,struct file *filp)
{	

	int retval = 0;
	void *buf;

//	minor = iminor(inode);
	if(!atomic_dec_and_test(&tlv1504_dev.opened)) {
		retval = -EBUSY;
		goto out;
	}
	//为ad转换结果分配空间
	buf = kmalloc(tlv1504_dev.data_max, GFP_KERNEL);
	if (!buf) {
		retval = -ENOMEM;
		goto out;
	}
	tlv1504_dev.buf = buf;

	filp->private_data = &tlv1504_dev;

	tlv1504_config();

	DPRINTK("tlv1504 adc: Open!! \n");
	return retval;
out:
	atomic_inc(&tlv1504_dev.opened);
	return retval;
	
	
}
/******************************************************************************
*	函数:	tlv1504_release
*	功能:	ad转换驱动释放函数,释放资源
*	参数:
*	返回:
 ******************************************************************************/
int tlv1504_release(struct inode *inode,struct file *filp)
{ 
	struct tlv1504_dev_t *dev = filp->private_data;
	kfree(dev->buf);
	atomic_inc(&dev->opened);

	DPRINTK("tlv1504_adc: Release!!\n");
	return 0;
}
/******************************************************************************
*	函数:	tlv1504_write
*	功能:	ad转换驱动写函数，暂不实现
*	参数:
*	返回:
 ******************************************************************************/
static ssize_t tlv1504_write(struct file *file,const char *bur,size_t count,loff_t *offset)
{	
	return -EBUSY;

}
/******************************************************************************
*	函数:	tlv1504_set_readch
*	功能:	ad转换驱动设置读取功能函数。此函数调用后会马上返回结果到spi接收寄存器
*	参数:	channel 要读取的通道号0~3，或功能 4，5,6-测试电平,7-读取CFR，8-读取FIFO
*	返回:
 ******************************************************************************/
void inline tlv1504_set_readch(int channel)
{		
	int ch_s=0;	
	switch(channel){
		case 0:
			ch_s = SELECT_CH0;
			break;
		case 1:
			ch_s = SELECT_CH1;
			break;
		case 2:
			ch_s = SELECT_CH2;
			break;
		case 3:
			ch_s = SELECT_CH3;
			break;
		case 4:				/*for test voltage=refm */
			ch_s = SELECT_TEST0;
			break;
		case 5:				/*for test voltage=(refm+refp)/2 */
			ch_s = SELECT_TEST1;
			break;
		case 6:				/*for test voltage=refp */
			ch_s = SELECT_TEST2;
			break;
		case 7:				/*read cfr */
			ch_s = CFR_READ;
			break;
		case 8:			/*read FIFO */
			ch_s = FIFO_READ;
		break;
		default:
			break;
	}			
	__raw_writel(ch_s, tlv1504_dev.base + AT91_SPI_TDR);
	
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
		sr = __raw_readl(tlv1504_dev.base + AT91_SPI_SR);
		//DPRINTK("sr=%x--",sr);
		/*
		if(sr & 0x01c0) {
			return -1;
		}
		*/
	} while (!(sr & bit) && (--loop_cntr > 0));

	return (loop_cntr > 0);
}

/******************************************************************************
*	函数:	tlv1504_read
*	功能:	ad转换驱动查询读取ad结果函数，并做初步处理
*	参数:
*	返回:
 ******************************************************************************/
static ssize_t tlv1504_read(struct file *filp,char  *buf,size_t count,loff_t *offset)
{
	int retval = 0;	
	u32 *pbuf;
	int fun = count;	
	u8 read_count = 0;
	
	struct tlv1504_dev_t *dev = filp->private_data;
	//DPRINTK("%s:enter FUNC = %d\n",__FUNCTION__,count);		
	pbuf = dev->buf;
	if ((count < 0) | (count > 8)) {	//参数有效性判断
		retval = -EINVAL;
		goto out;
	}

//查询读取转换数据，循环3次
read:
	tlv1504_set_readch(fun);
	retval = at91_poll_status(AT91_SPI_RDRF);
	DPRINTK("\n%s:retval=%d\n",__FUNCTION__,retval);
	if (retval > 0) {
		*pbuf  = __raw_readl(tlv1504_dev.base + AT91_SPI_RDR);
		DPRINTK("%s:poll:pbuf=%x\n",__FUNCTION__,*pbuf);
		if(read_count++ < 2)		//读取3次
			goto read;
		read_count = 0;
	}else if (retval == 0) {
		retval = -ETIMEDOUT;
	}else {
		retval = -EFAULT;
	}

//转换读取到的数据
	if(fun == 7)					/*读CFR,数据位D11-D0有效 */
		*pbuf &= 0xfff;
	else
		*pbuf = ((*pbuf)>>6)&0x3ff;	/*读取通道数据, 数据位 D15-D6 有效，最小值为0，最大值为0x3ff*/
	
	DPRINTK("%s:pbuf=%x\n",__FUNCTION__,*pbuf);

	retval = sizeof(pbuf);
	if (copy_to_user(buf, pbuf, sizeof(pbuf))) {
			retval = -EFAULT;			
	}

out:
	return retval;
}
/******************************************************************************
*	函数:	tlv1504_ioctl
*	功能:	ad转换驱动控制函数，咱无实现
*	参数:
*	返回:
 ******************************************************************************/
int tlv1504_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int retval=0;
	
	DPRINTK("%s: enter!!\n",__FUNCTION__);
	switch(cmd){		
//	case DS_ADC_CHS:
	case DS_ADC_IRQ_QUERY:
		break;		
//	case DS_ADC_START:		
	case 1:	
		break;		
//	case DS_ADC_STOP:
	case 2:			
		break;
	default:
		break;
	}
	return retval;
}

static struct file_operations tlv1504_fops = { 
	.owner	= THIS_MODULE, 
	.open	= tlv1504_open, 
	.write	= tlv1504_write, 
	.read	= tlv1504_read, 
	.ioctl	= tlv1504_ioctl,
	.release = tlv1504_release, 
}; 

/******************************************************************************
*	函数:	tlv1504_init
*	功能:	ad转换驱动初始化函数，字符设备初始化，spi总线初始化
*	参数:
*	返回:
 ******************************************************************************/
static int __init tlv1504_init(void)
{	
	int retval;
	u32 cdiv;	
	DPRINTK(SOFTWARE_VERSION" build time:" __TIME__ "\n");	
	//DPRINTK("%s:enter,major = (%d)...\n",__FUNCTION__, MAJOR(tlv1504_dev_t));

//注册字符设备驱动并创建供udev试用的设备类
	retval = register_chrdev_region (tlv1504_dev_t, 1, tlv1504_dev.name);
	if (retval)
		return retval;	
	cdev_init(&tlv1504_cdev, &tlv1504_fops);

	tlv1504_cdev.owner = THIS_MODULE;
	retval = cdev_add(&tlv1504_cdev, tlv1504_dev_t, 1);
	if (retval)
		return retval;

	drv_class = class_create(THIS_MODULE, tlv1504_dev.name);
	if (drv_class == NULL)
		return -ENOMEM;
	
	device_create(drv_class, NULL,tlv1504_dev_t, NULL, tlv1504_dev.name);

//硬件设备初始化，spi总线初始化
	//set pin
	at91_set_A_periph(AT91_PIN_PB0, 0);	//set SPI1 MISO
	at91_set_A_periph(AT91_PIN_PB1, 0);	//set SPI1 MOSI
	at91_set_A_periph(AT91_PIN_PB2, 0);	//set SPCK
	at91_set_A_periph(AT91_PIN_PB3, 0);	//set NPCS0
	//enable spi clock
	tlv1504_dev.clk = clk_get(NULL, "spi1_clk");
	clk_enable(tlv1504_dev.clk);
	
	//ioremap
	tlv1504_dev.base = ioremap(AT91SAM9260_BASE_SPI1, 0x40);
	//spi hardware init
	__raw_readl(tlv1504_dev.base + AT91_SPI_SR); // clear irq flag
	__raw_writel(0xffffffff, tlv1504_dev.base + AT91_SPI_IDR);		//disable all spi interrupt
	__raw_writel(AT91_SPI_SWRST, tlv1504_dev.base + AT91_SPI_CR);	//reset spi
	//set mode
	__raw_writel(AT91_SPI_MSTR | AT91_SPI_PS_VARIABLE , tlv1504_dev.base + AT91_SPI_MR);
	//set baudrate
	cdiv = (clk_get_rate(tlv1504_dev.clk) >> 6) / tlv1504_dev.baud + 1;	// /(64 * drv_info.baud)
	if (cdiv > 255) {
		cdiv = 255;
	}
	/*very important!! 设置spi的片选芯片的时序及数据位数
	  */
	__raw_writel((cdiv << 8) | AT91_SPI_BITS_16 | AT91_SPI_NCPHA, tlv1504_dev.base + AT91_SPI_CSR(tlv1504_dev.addr));

	__raw_writel(AT91_SPI_SPIEN, tlv1504_dev.base + AT91_SPI_CR);	//enable spi
	
	DPRINTK(SOFTWARE_VERSION " init\n\n");
	return 0;
}
/******************************************************************************
*	函数:	tlv1504_exit
*	功能:	ad转换驱动注销函数
*	参数:
*	返回:
 ******************************************************************************/
static void __exit tlv1504_exit(void)
{	

	//disable spi master
	__raw_writel(AT91_SPI_SPIDIS, tlv1504_dev.base + AT91_SPI_CR);
	//iounmap
	iounmap(tlv1504_dev.base);
	
	//disable clock
	clk_disable(tlv1504_dev.clk);
	clk_put(tlv1504_dev.clk);
	//set pin input
	at91_set_gpio_input(AT91_PIN_PB0, 1);
	at91_set_gpio_input(AT91_PIN_PB1, 1);
	at91_set_gpio_input(AT91_PIN_PB2, 1);
	at91_set_gpio_input(AT91_PIN_PB3, 1);
//delete device file and unregister char device
	device_destroy(drv_class, MKDEV(MAJOR_DEVICE,0));
	class_destroy(drv_class);		
	cdev_del(&tlv1504_cdev);
	unregister_chrdev_region(tlv1504_dev_t, 1);
	DPRINTK(SOFTWARE_VERSION " exit\n");

}

MODULE_AUTHOR("Roy <luranran@gmail.com>");
MODULE_DESCRIPTION("tlv1504 adc driver");
MODULE_LICENSE("GPL");
module_init(tlv1504_init);
module_exit(tlv1504_exit);

