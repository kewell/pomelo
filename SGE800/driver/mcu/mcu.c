/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  mcu.c
	描述		：  本文件定义了单片机扩展功能驱动程序
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
#include "mcu.h"

//模块调试开关
//#undef	DEBUG
#define DEBUG
#ifdef DEBUG
#define	DPRINTK( x... )		printk("mcu: " x)
#else
#define DPRINTK( x... )
#endif

/*************************************************
  静态全局变量及宏定义
*************************************************/

static struct cdev mcu_cdev;
static dev_t mcu_dev_m = MKDEV(MAJOR_DEVICE,0);
static struct class *drv_class;

static struct mcu_dev_t mcu_dev = {
		.name = "mcu",
		.opened = ATOMIC_INIT(1),
		.addr	= 0,
		.data_size	= MCU_BUF_SIZE,
		.baud = SPI_DEFAULT_BAUD,
		.irq = AT91SAM9260_ID_IRQ2,  //??????????
		.rw_flag = MCU_FRAME_IDEAL,
};

/*************************************************
  API
*************************************************/
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
		sr = __raw_readl(mcu_dev.base + AT91_SPI_SR);
	} while (!(sr & bit) && (--loop_cntr > 0));

	return (loop_cntr > 0);
}
/******************************************************************************
*	函数:	mcu_spi_write
*	功能:	spi 写缓冲
*	参数:
*	返回:
 ******************************************************************************/
static void inline mcu_spi_write(u32 data)
{
	__raw_writel(data, mcu_dev.base + AT91_SPI_TDR);
}
/******************************************************************************
*	函数:	mcu_spi_read
*	功能:	spi 写缓冲
*	参数:
*	返回:
 ******************************************************************************/
static int inline mcu_spi_reade(void)
{
	int retval = -1;
	__raw_writel(0xff, mcu_dev.base + AT91_SPI_TDR);
	retval = at91_poll_status(AT91_SPI_RDRF);
	if(retval < 0)
		return retval;
	return __raw_readl(mcu_dev.base + AT91_SPI_RDR);
}
/******************************************************************************
*	函数:	atmel_spi_hardinit
*	功能:	spi 硬件初始化
*	参数:
*	返回:
 ******************************************************************************/
static void inline atmel_spi_hardinit(void)
{
	u32 cdiv;
	//set pin
	at91_set_A_periph(AT91_PIN_PB0, 0);	//set SPI1 MISO
	at91_set_A_periph(AT91_PIN_PB1, 0);	//set SPI1 MOSI
	at91_set_A_periph(AT91_PIN_PB2, 0);	//set SPCK
	//at91_set_A_periph(AT91_PIN_PB3, 0);	//set NPCS0		//为了节省io口，片选没有用到
	//enable spi clock
	mcu_dev.clk = clk_get(NULL, "spi1_clk");
	clk_enable(mcu_dev.clk);

	//ioremap
	mcu_dev.base = ioremap(AT91SAM9260_BASE_SPI1, 0x40);
	//spi hardware init
	__raw_readl(mcu_dev.base + AT91_SPI_SR); // clear irq flag
	__raw_writel(0xffffffff, mcu_dev.base + AT91_SPI_IDR);		//disable all spi interrupt
	__raw_writel(AT91_SPI_SWRST, mcu_dev.base + AT91_SPI_CR);	//reset spi
	//set mode
	__raw_writel(AT91_SPI_MSTR | AT91_SPI_PS_VARIABLE , mcu_dev.base + AT91_SPI_MR);
	//set baudrate
	cdiv = (clk_get_rate(mcu_dev.clk) >> 6) / mcu_dev.baud + 1;	// /(64 * drv_info.baud)
	if (cdiv > 255) {
		cdiv = 255;
	}
	//very important!! 设置spi的片选芯片的时序及数据位数
	__raw_writel((cdiv << 8) | AT91_SPI_BITS_8 | AT91_SPI_NCPHA, mcu_dev.base + AT91_SPI_CSR(mcu_dev.addr));
	__raw_writel(AT91_SPI_SPIEN, mcu_dev.base + AT91_SPI_CR);	//enable spi

}
/******************************************************************************
*	函数:	mcu_interrupt
*	功能:	定时器中断函数,每隔设定的毫秒数中断一次，用来录波
*	参数:
*	返回:	IRQ_HANDLED		-	成功
 ******************************************************************************/
static irqreturn_t  atmel_mcu_spi_interrupt(int irq,void *dev_id)
{
	if( mcu_dev.rw_flag == MCU_FRAME_IDEAL){
		if(down_trylock(&(mcu_dev.rwsem)) == 0){
			mcu_dev.rw_flag = MCU_FRAME_READ;
			wake_up_interruptible(&(mcu_dev.wq));
		}
	}
	if(mcu_dev.rw_flag == MCU_FRAME_READ ){
		wake_up_interruptible(&(mcu_dev.wq));
	}
	return IRQ_HANDLED;
}


/******************************************************************************
*	函数:	mcu_open
*	功能:	单片机扩展驱动打开函数,配置mcu
*	参数:
*	返回:
 ******************************************************************************/
int mcu_open(struct inode*inode,struct file *filp)
{

	int retval = 0;
	void *buf;

//	minor = iminor(inode);
	if(!atomic_dec_and_test(&mcu_dev.opened)) {
		retval = -EBUSY;
		goto out;
	}
	//为ad转换结果分配空间
	buf = kmalloc(mcu_dev.data_size, GFP_KERNEL);
	if (!buf) {
		retval = -ENOMEM;
		goto out;
	}
	mcu_dev.buf = buf;
	mcu_dev.cur = 0;
	filp->private_data = &mcu_dev;
	//初始化等待队列
	init_waitqueue_head( &(mcu_dev.wq) );
	//pc14 irq2
	retval = request_irq(mcu_dev.irq,
							atmel_mcu_spi_interrupt,
							IRQF_DISABLED,
							mcu_dev.name,
							NULL);

	init_MUTEX(&(mcu_dev.sem));
	init_MUTEX(&(mcu_dev.rwsem));
	DPRINTK("mcu: Open!! \n");
	return retval;
out:
	atomic_inc(&mcu_dev.opened);
	return retval;


}
/******************************************************************************
*	函数:	mcu_release
*	功能:	ad转换驱动释放函数,释放资源
*	参数:
*	返回:
 ******************************************************************************/
int mcu_release(struct inode *inode,struct file *filp)
{
	struct mcu_dev_t *dev = filp->private_data;
	kfree(dev->buf);
	atomic_inc(&dev->opened);
	free_irq(mcu_dev.irq, NULL);
	DPRINTK("mcu_adc: Release!!\n");
	return 0;
}


static int inline mcu_data_analays(void)
{
	int ret = -1;
	int i = 0;

#define MCU_FRAME_NULL 0xffff
	static u16 head = MCU_FRAME_NULL,tail = MCU_FRAME_NULL;		//帧头，帧尾
	u16 tail_t;
	u16 cur_pos = mcu_dev.cur;
	u8 sum_check;

	if(cur_pos > MCU_BUF_SIZE/2){
		head = MCU_FRAME_NULL;			//初始化缓冲区
		tail = MCU_FRAME_NULL;
		memset(mcu_dev.buf, 0 , MCU_BUF_SIZE);
		mcu_dev.cur = 0;
		ret = -2;		//缓冲区空间不足
		return ret;
	}
	//找到数据头
	if(head == MCU_FRAME_NULL){
		for(i = 0; i < cur_pos; i ++){
			if(mcu_dev.buf[i] == MCU_FRAME_HEAD){
				head = i;
				break;
			}
		}
	}

	//找到数据尾
	if((tail == MCU_FRAME_NULL) && (head != MCU_FRAME_NULL)){
		tail_t = head + 2 + mcu_dev.buf[head + 2] + 2;
		if(mcu_dev.buf[tail_t] == MCU_FRAME_TAIL)
			tail = tail_t;
	}

	//帧校验
	if((tail != MCU_FRAME_NULL) && (head != MCU_FRAME_NULL)){
		sum_check = 0;
		for(i = head; i < tail - 1; i ++){
			sum_check += mcu_dev.buf[i];
		}
		if(sum_check == mcu_dev.buf[tail - 1]){
			ret = head;
		}else{
			memset(mcu_dev.buf, 0 , MCU_BUF_SIZE);
			mcu_dev.cur = 0;
			ret = -3; 		//校验出错
		}
		head = MCU_FRAME_NULL;			//初始化缓冲区
		tail = MCU_FRAME_NULL;
	}

	return ret;
}
/******************************************************************************
*	函数:	mcu_read
*	功能:	ad转换驱动查询读取ad结果函数，并做初步处理
*	参数:
*	返回:
 ******************************************************************************/
ssize_t mcu_read(struct file *filp,char  *buf,size_t count,loff_t *offset)
{
	int retval = 0;
	u8 read_count = 0;
	u8 next_frame;

	struct mcu_dev_t *dev = filp->private_data;
	//DPRINTK("%s:enter FUNC = %d\n",__FUNCTION__,count);
	//解析现有数据
read:
	retval = mcu_data_analays();
	if(retval >= 0){
		read_count = 2 + mcu_dev.buf[retval + 2] ;
		next_frame = dev->buf[retval + 1] & MCU_FRAME_NEXT;
		if (copy_to_user(buf, &dev->buf[retval + 1], read_count)) {
			retval = -EFAULT;
			return retval;
		}
		//初始化
		memset(mcu_dev.buf, 0 , MCU_BUF_SIZE);
		mcu_dev.cur = 0;
		if(next_frame == 0){
			mcu_dev.rw_flag = MCU_FRAME_IDEAL;
			up(&(mcu_dev.rwsem));
		}
		retval = read_count;
		return retval;
	}else if(retval == -3){//校验出错
		mcu_dev.rw_flag = MCU_FRAME_IDEAL;
		up(&(mcu_dev.rwsem));
	}
	interruptible_sleep_on(&(dev->wq));	//睡眠等待定时时间到
	if(signal_pending(current))
		return -ERESTARTSYS;

	mcu_dev.buf[mcu_dev.cur ++] = (u8) mcu_spi_reade();
	printk("%x ",mcu_dev.buf[mcu_dev.cur -1]);


	goto read;
}

/******************************************************************************
*	函数:	mcu_write
*	功能:	单片机扩展驱动写帧函数
*	参数:
*	返回:
 ******************************************************************************/
static void inline mcu_make_frame(u16 size)
{
	int i;
	u8 sum_check = 0;
	u16 write_size;

	mcu_dev.buf[0] = MCU_FRAME_HEAD;
	for(i = 0; i < size + 1; i++ ){
		sum_check += mcu_dev.buf[i];
	}

	mcu_dev.buf[i++] = sum_check;

	mcu_dev.buf[i++] = MCU_FRAME_TAIL;
	write_size = i ;

	for(i = 0; i < write_size; i ++){
		at91_poll_status(AT91_SPI_TXBUFE);
		mcu_spi_write(mcu_dev.buf[i]);
	}

}
/******************************************************************************
*	函数:	mcu_write
*	功能:	单片机扩展驱动写帧函数
*	参数:
*	返回:
 ******************************************************************************/
ssize_t mcu_write(struct file *filp,const char *buf,size_t count,loff_t *offset)
{
	int retval = -1;
	int i;
//	struct mcu_dev_t *dev = filp->private_data;

	if(count > MCU_FRAME_DATA_MAX){
		retval = -EINVAL;
		return retval;
	}

	retval = down_interruptible(&(mcu_dev.sem));
	if (retval) {
		retval = -EBUSY;
		return retval;
	}

	retval = down_interruptible(&(mcu_dev.rwsem));
	if (retval) {
		retval = -EBUSY;
		goto err2;
	}
	mcu_dev.rw_flag = MCU_FRAME_WRITE;

	//获取用户数据并组帧
	if (copy_from_user(&mcu_dev.buf[1], buf, count)) {
		retval = -EFAULT;
		mcu_dev.rw_flag = MCU_FRAME_IDEAL;
		goto err1;
	}
	mcu_make_frame(count);
	DPRINTK("%s: count = %d,write:",__FUNCTION__,count + 3);
	for(i = 0; i < count + 3; i++){
		printk("%x ",mcu_dev.buf[i]);
	}
	printk("\n");
	mcu_dev.rw_flag = MCU_FRAME_READ;
	retval = count;
	up(&(mcu_dev.sem));
	return retval;
err1:
	up(&(mcu_dev.rwsem));
err2:
	up(&(mcu_dev.sem));
	return retval;
}
/******************************************************************************
*	函数:	mcu_ioctl
*	功能:	控制函数
*	参数:
*	返回:
 ******************************************************************************/
int mcu_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int retval=0;

	DPRINTK("%s: enter!!\n",__FUNCTION__);
	switch(cmd){
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}
	return retval;
}

static struct file_operations mcu_fops = {
	.owner	= THIS_MODULE,
	.open	= mcu_open,
	.write	= mcu_write,
	.read	= mcu_read,
	.ioctl	= mcu_ioctl,
	.release = mcu_release,
};

/******************************************************************************
*	函数:	mcu_init
*	功能:	ad转换驱动初始化函数，字符设备初始化，spi总线初始化
*	参数:
*	返回:
 ******************************************************************************/
static int __init mcu_init(void)
{
	int retval;

	DPRINTK(SOFTWARE_VERSION" build time:" __TIME__ "\n");
	//DPRINTK("%s:enter,major = (%d)...\n",__FUNCTION__, MAJOR(mcu_dev_t));

//注册字符设备驱动并创建供udev试用的设备类
	retval = register_chrdev_region (mcu_dev_m, 1, mcu_dev.name);
	if (retval)
		return retval;
	cdev_init(&mcu_cdev, &mcu_fops);

	mcu_cdev.owner = THIS_MODULE;
	retval = cdev_add(&mcu_cdev, mcu_dev_m, 1);
	if (retval)
		return retval;

	drv_class = class_create(THIS_MODULE, mcu_dev.name);
	if (drv_class == NULL)
		return -ENOMEM;

	device_create(drv_class, NULL,mcu_dev_m, NULL, mcu_dev.name);

//硬件设备初始化，spi总线初始化
	atmel_spi_hardinit();
	DPRINTK(SOFTWARE_VERSION " init\n\n");
	return 0;
}
/******************************************************************************
*	函数:	mcu_exit
*	功能:	ad转换驱动注销函数
*	参数:
*	返回:
 ******************************************************************************/
static void __exit mcu_exit(void)
{

	//disable spi master
	__raw_writel(AT91_SPI_SPIDIS, mcu_dev.base + AT91_SPI_CR);
	//iounmap
	iounmap(mcu_dev.base);

	//disable clock
	clk_disable(mcu_dev.clk);
	clk_put(mcu_dev.clk);
	//set pin input
	at91_set_gpio_input(AT91_PIN_PB0, 1);
	at91_set_gpio_input(AT91_PIN_PB1, 1);
	at91_set_gpio_input(AT91_PIN_PB2, 1);
	at91_set_gpio_input(AT91_PIN_PB3, 1);
	//delete device file and unregister char device
	device_destroy(drv_class, MKDEV(MAJOR_DEVICE,0));
	class_destroy(drv_class);
	cdev_del(&mcu_cdev);
	unregister_chrdev_region(mcu_dev_m, 1);
	DPRINTK(SOFTWARE_VERSION " exit\n");

}

MODULE_AUTHOR("Roy <luranran@gmail.com>");
MODULE_DESCRIPTION("mcu adc driver");
MODULE_LICENSE("GPL");
module_init(mcu_init);
module_exit(mcu_exit);

