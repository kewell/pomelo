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
	文件		：  atmel_tc.c
	描述		：  本文件定义了定时器模块的底层驱动程序接口
	版本		：  0.2
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/
#include "atmel_tc.h"
#include "timerlib.h"

//模块调试开关
#undef	DEBUG
//#define DEBUG
#ifdef DEBUG
#define	DPRINTK( x... )		printk("atmel_tc: " x)
#else
#define DPRINTK( x... )
#endif

/*************************************************
  静态全局变量及宏定义
*************************************************/
static dev_t atmel_tc_dev_t = MKDEV(MAJOR_DEVICE,0);
struct atmel_tc_driver drv_info;
struct atmel_tc tc[6];
static struct class *drv_class;
static irqreturn_t (*atmel_tc_interrupt[6])(int irq, void *dev_id);

/*************************************************
  API
*************************************************/

/******************************************************************************
*	函数:	atmel_tc_probe
*	功能:	定时器设备探测函数，初始化结构体
*	参数:
*	返回:	0		-	成功
 ******************************************************************************/
static int  atmel_tc_probe(int channel)
{
	//struct atmel_tc *tc;
	struct clk	*clk;	
	static char tc_t[20];

	tc[channel].data0 = 0;
	tc[channel].data1 = 1;
	tc[channel].pulse = 0;
	tc[channel].ready = 0;
	tc[channel].irq100 = 0;
	tc[channel].clock_source = -1;		//时钟源默认为-1,方便检查配置。
	//tc[channel].clock_source = SCK32KIHZ;  // default clock
	//tc[channel].clock_f	= 32768;
	
	sprintf(tc_t,"tc%d",channel);
	strcpy(tc[channel].name,tc_t);
	tc[channel].channle = channel;

	//内存映射及中断号定义
	if(channel<3){		//定时器0~2,定时器3~5地址不连续
		tc[channel].block = 0;
		tc[channel].blkregs = ioremap(AT91SAM9260_BASE_TCB0, 0xff);	
		tc[channel].regs = ioremap(AT91SAM9260_BASE_TCB0 + channel*0x40, 0x40);
		tc[channel].irq = AT91SAM9260_ID_TC0+channel;
		}
	else{
		tc[channel].block = 1;
		tc[channel].blkregs = ioremap(AT91SAM9260_BASE_TCB1, 0xff);		
		tc[channel].regs = ioremap(AT91SAM9260_BASE_TCB1 + (channel-3)*0x40, 0x40);
		tc[channel].irq = AT91SAM9260_ID_TC3 + channel - 3;
	}

	//获取时钟
	sprintf(tc_t,"tc%d_clk",channel);
	clk = clk_get(NULL, tc_t);
	if (IS_ERR(clk)) 	
		return -EINVAL;
	tc[channel].clk = clk;
	clk_enable(tc[channel].clk);		

	//初始化等待队列
	init_waitqueue_head( &(tc[channel].wq) );
	
	return 0;
}

/******************************************************************************
*	函数:	atmel_tc0_interrupt
*	功能:	定时器0中断处理函数
*	参数:
*	返回:	IRQ_HANDLED
 ******************************************************************************/
static irqreturn_t  atmel_tc0_interrupt(int irq,void *dev_id)
{
	__raw_readl(tc[0].regs + ATMEL_TC_SR);		/* Read TC0 Status Register to clear it */
	DPRINTK("enter irq=%d...id=%d\n",irq,tc[0].id );

	//心跳模式
	if(tc[0].id == TC_DELAY){
		wake_up_interruptible(&(tc[0].wq));
	}//频率测量
	else if(tc[0].id == TC_MEASURE){
		tc[0].ready = 1;
		tc[0].data0 = __raw_readl(tc[0].regs + ATMEL_TC_RA);
		tc[0].data1 = __raw_readl(tc[0].regs + ATMEL_TC_RB);
		DPRINTK(  "%s:enter data1=%d...data0=%d\n",__FUNCTION__,tc[0].data1,tc[0].data0 );
	}//脉宽调制
	else if(tc[0].id == TC_PWM){
	;
	}
	return IRQ_HANDLED;
	
}

static irqreturn_t  atmel_tc1_interrupt(int irq,void *dev_id)
{
	
	__raw_readl(tc[1].regs + ATMEL_TC_SR);		/* Read TC0 Status Register to clear it */
	 DPRINTK(  "%s:enter irq=%d\n",__FUNCTION__,irq);
	 	 
	 tc[1].ready = 1;
	 wake_up_interruptible(&(tc[1].wq));
	if(tc[1].data0 == 0)
		tc[1].data0 = 2;
	else
		tc[1].data0 = 0;
	return IRQ_HANDLED;
	
}
static irqreturn_t  atmel_tc2_interrupt(int irq,void *dev_id)
{
	
	__raw_readl(tc[2].regs + ATMEL_TC_SR);		/* Read TC0 Status Register to clear it */
	 DPRINTK(  "%s:enter irq=%d\n",__FUNCTION__,irq);
	 wake_up_interruptible(&(tc[2].wq));
	return IRQ_HANDLED;
}

static irqreturn_t  atmel_tc3_interrupt(int irq,void *dev_id)
{
	
	__raw_readl(tc[3].regs + ATMEL_TC_SR);		/* Read TC0 Status Register to clear it */
	DPRINTK(  "%s:enter irq=%d\n",__FUNCTION__,irq);
	wake_up_interruptible(&(tc[3].wq));
	return IRQ_HANDLED;
	
}

static irqreturn_t  atmel_tc4_interrupt(int irq,void *dev_id)
{	
	__raw_readl(tc[4].regs + ATMEL_TC_SR);		/* Read TC0 Status Register to clear it */
	DPRINTK(  "%s:enter irq=%d\n",__FUNCTION__,irq);
	wake_up_interruptible(&(tc[4].wq));	
	return IRQ_HANDLED;	
}
static irqreturn_t  atmel_tc5_interrupt(int irq,void *dev_id)
{
	
	__raw_readl(tc[5].regs + ATMEL_TC_SR);		/* Read TC0 Status Register to clear it */
	DPRINTK(  "%s:enter irq=%d\n",__FUNCTION__,irq);	
	wake_up_interruptible(&(tc[5].wq));	
	return IRQ_HANDLED;	
}

/******************************************************************************
*	函数:	atmel_tc_open
*	功能:	定时器设备打开函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_tc_open(struct inode *inode, struct file *filp)
{	
	
	int minor;
	int retval = 0;
	minor = iminor(inode);
	//DPRINTK(  "%s:minor = (%d)...\n",__FUNCTION__,minor);
	if(!atomic_dec_and_test(&tc[minor].opened)) {	
		retval = -EBUSY;
		goto out;
	}
	if ((minor > 5) || (minor < 0))		//设备检查
		return -EFAULT;
		
	retval = atmel_tc_probe(minor);
	if(retval!=0)
		return retval;

	filp->private_data = &tc[minor];
	//DPRINTK(  "%s:open %s\n",__FUNCTION__,tc[minor].name);
	return retval;
out:
	atomic_inc(&tc[minor].opened);
	return retval;
}
/******************************************************************************
*	函数:	atmel_tc_release
*	功能:	定时器设备释放函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_tc_release(struct inode *inode,struct file *filp)
{ 
	static int minor;
	minor = iminor(inode);
	__raw_writel(ATMEL_TC_CLKDIS  , tc[minor].regs + ATMEL_TC_CCR); /* Disable the Clock Counter */
	__raw_writel(0xFFFFFFFF, tc[minor].regs + ATMEL_TC_IDR);		/* Disable the irq */
	iounmap(tc[minor].blkregs);
	iounmap(tc[minor].regs);
	tc[minor].blkregs = NULL;
	tc[minor].regs = NULL;
	tc[minor].pulse = 0;
	//disable irq
	if(tc[minor].irq100 == 1)
		free_irq(tc[minor].irq, NULL);
	//disable clock
	clk_disable(tc[minor].clk);
	clk_put(tc[minor].clk);
	atomic_inc(&tc[minor].opened);
	//DPRINTK(  "atmel_tc%d: Release!!\n",minor);
	return 0;
}

/******************************************************************************
*	函数:	atmel_tc_read
*	功能:	定时器设备读取函数
*	参数:
*	返回:	tcx - 成功，定时器号
 ******************************************************************************/
static ssize_t atmel_tc_read(struct file *filp,char  *buf,size_t count,loff_t *offset)
{
	int a = 0;
	int retval = 0;
	
	int  tcx = iminor(filp->f_path.dentry->d_inode) ;
	DPRINTK(  "%s:enter  minor is %d\n",__FUNCTION__,tcx);

	//心跳功能
	if(tc[tcx].id == TC_DELAY){
		if( filp->f_flags & O_NONBLOCK )
			return -EAGAIN;
		interruptible_sleep_on(&(tc[tcx].wq));	//睡眠等待定时时间到
		if(signal_pending(current))
			return -ERESTARTSYS;						
		retval = tcx;	//返回定时器号0~5
	}//频率测量功能
	else if(tc[tcx].id == TC_MEASURE){
		if( tc[tcx].ready ){
			a = 32768/(tc[tcx].data1-tc[tcx].data0);
			if (copy_to_user(buf, &a, sizeof(int))) {
				retval = -EFAULT;	
				return retval;
			}
		}
		retval = tcx;
	}
	return retval;
}

/******************************************************************************
*	函数:	atmel_tc_config_delay
*	功能:	心跳功能配置函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_tc_config_delay(int minor,unsigned long arg)
{	
	int retval=0;

	//不支持2s以上定时
	if(arg >= 2000){
		retval = -EFAULT;		
	}

	tc[minor].id = TC_DELAY;	//心跳功能
	//DPRINTK(  "%s:enter,minor = (%d)...\n",__FUNCTION__, minor);

	//配置寄存器
	/* disable hardware  */
	__raw_writel(ATMEL_TC_CLKDIS  , tc[minor].regs + ATMEL_TC_CCR); /* Disable the Clock Counter */
	__raw_writel(0xFFFFFFFF, tc[minor].regs + ATMEL_TC_IDR);		/* Disable the irq */
	__raw_readl(tc[minor].regs + ATMEL_TC_SR);		/* Read TC0 Status Register to clear it */

	/* set clock source */
	DPRINTK("%s:clock source is %d\n",__FUNCTION__, tc[minor].clock_source);
	if(tc[minor].clock_source < 0 )
		return -EACCES;
	__raw_writel(tc[minor].clock_source | ATMEL_TC_CPCTRG, 
			tc[minor].regs + ATMEL_TC_CMR);	/*Select  clock , rc compare mode*/
	__raw_writel(ATMEL_TC_CPCS, 
			tc[minor].regs + ATMEL_TC_IER);	/* Validate the RC compare interrupt */

	/* delay time set*/
	tc[minor].rcreg = (tc[minor].clock_f * arg)/1000;	
	DPRINTK("%s:clock is %x,rcreg is %x,%d\n",__FUNCTION__, 
			tc[minor].clock_f,tc[minor].rcreg,tc[minor].rcreg & 0xffff0000);
	if(tc[minor].rcreg & 0xffff0000 )		//判断16位rc寄存器是否溢出
		return -EOVERFLOW;	
	__raw_writel(tc[minor].rcreg, tc[minor].regs + ATMEL_TC_RC); /* Set delay time */
//	DPRINTK(  "%s:clock is %d,rcreg is %lld\n",__FUNCTION__,tc[minor].clock_f,temp);

	//10ms及以上定时才注册中断函数，10ms以下定时用作他用。32768HZ最低能产生125ms无误差心跳
	if(tc[minor].pulse == 0){
		retval = request_irq(tc[minor].irq, atmel_tc_interrupt[minor],
								IRQF_DISABLED, tc[minor].name, NULL);
		tc[minor].irq100 = 1;	
	}
		
	return retval;
	//__raw_writel(ATMEL_TC_CLKEN, tc[minor].regs + ATMEL_TC_CCR); /* Enable the Clock Counter */			
}

/******************************************************************************
*	函数:	atmel_tc_config_measure
*	功能:	频率测量功能配置函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_tc_config_measure(int minor,unsigned long arg)
{
	int retval=0;

	tc[minor].id = TC_MEASURE;	/* Record times ' function */
	//DPRINTK(  "%s:enter,minor = (%d)...\n",__FUNCTION__, minor);

	//配置寄存器
	/* disable hardware  */
	__raw_writel(ATMEL_TC_CLKDIS  , tc[minor].regs + ATMEL_TC_CCR); /* Disable the Clock Counter */
	__raw_writel(0xFFFFFFFF, tc[minor].regs + ATMEL_TC_IDR);		/* Disable the irq */
	__raw_readl(tc[minor].regs + ATMEL_TC_SR);		/* Read TC0 Status Register to clear it */

	/* Set  TIOA rising edge trigger, captrue model,  RA RB loading edge is rising, */	
	if(tc[minor].clock_source < 0 )
		return -1;
	__raw_writel(tc[minor].clock_source | 
					ATMEL_TC_ABETRG | 				/* Set  TIOA falling edge trigger */
					ATMEL_TC_ETRGEDG_FALLING |
					ATMEL_TC_LDRA_FALLING| 			/* RA RB loading edge is falling */
					ATMEL_TC_LDRB_FALLING, 
					tc[minor].regs + ATMEL_TC_CMR);	/*Select default clock , rc compare mode*/	

	//注册中断函数
	retval = request_irq(tc[minor].irq, 
								atmel_tc_interrupt[minor], 
								IRQF_DISABLED, 
								tc[minor].name, 
								NULL);			
	tc[minor].irq100 = 1;	

	return retval;
}

/******************************************************************************
*	函数:	atmel_tc_config_pwm
*	功能:	脉宽调制功能配置函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_tc_config_pwm(int minor,u64 arg)
{
	u32 f_t,duty_c,duty_m;	//频率，占空比分子，分母
	
	tc[minor].id = TC_PWM;	/* Record times ' function */
	
	//设置脉冲输出引脚
	if(minor==0)
		at91_set_A_periph(AT91_PIN_PA26,1);	 //tioa0
		//at91_set_B_periph(AT91_PIN_PC9,1);	//tiob0
	else if(minor==1)
		at91_set_A_periph(AT91_PIN_PA27,1);	//tioa1
	//at91_set_A_periph(AT91_PIN_PC7,1);	//tiob1
	else if(minor==2)
		at91_set_A_periph(AT91_PIN_PA28,1);	//tioa2
	//at91_set_A_periph(AT91_PIN_PC6,1);	//tiob2
	else if(minor==3)
		at91_set_B_periph(AT91_PIN_PB0,1);	//tioa3
	//at91_set_B_periph(AT91_PIN_PB1,1);	//tiob3
	else if(minor==4)
		at91_set_B_periph(AT91_PIN_PB2,1);	//tioa4
	//at91_set_B_periph(AT91_PIN_PB18,1);	//tiob4
	else if(minor==5)
		at91_set_B_periph(AT91_PIN_PB3,1);	//tioa5
		//at91_set_B_periph(AT91_PIN_PB19,1);	//tiob5
			
	//配置寄存器
	/* disable hardware  */
	__raw_writel(ATMEL_TC_CLKDIS  , tc[minor].regs + ATMEL_TC_CCR); /* Disable the Clock Counter */
	__raw_writel(0xFFFFFFFF, tc[minor].regs + ATMEL_TC_IDR);		/* Disable the irq */
	__raw_readl(tc[minor].regs + ATMEL_TC_SR);		/* Read TC Status Register to clear it */

	/* Set mode register */
	if(tc[minor].clock_source < 0 )
		return -1;
	__raw_writel(tc[minor].clock_source |					 
					ATMEL_TC_WAVE | 			/* Set TC waveform mode */
					ATMEL_TC_WAVESEL_UP_AUTO | 	/* Set TC waveform type */		
					ATMEL_TC_ACPA_SET|		/* Set RA compare changes TIOA */	
					//ATMEL_TC_BCPB_TOGGLE |
					ATMEL_TC_ACPC_TOGGLE,		/* RC compare changes TIOA */
					//ATMEL_TC_BCPC_TOGGLE
					tc[minor].regs + ATMEL_TC_CMR); 
	f_t = (arg>>32);
	tc[minor].rcreg = tc[minor].clock_f/f_t;	
	duty_c = (arg>>16) & 0xffff;	
	duty_m = arg & 0xffff;
	
	DPRINTK(  "%s:f = %d duty_c = %d duty_m = %d arg = %llx\n",__FUNCTION__,f_t,duty_c,duty_m,arg);
	tc[minor].rareg = tc[minor].rcreg-(tc[minor].rcreg)*duty_c/duty_m;
	if(tc[minor].rareg==0)
		tc[minor].rareg = 1;

	DPRINTK(  "%s:Rc = %d Ra = %d\n",__FUNCTION__,tc[minor].rcreg,tc[minor].rareg);

	__raw_writel(tc[minor].rcreg, tc[minor].regs + ATMEL_TC_RC); /* Set the delay time */
	__raw_writel(tc[minor].rareg, tc[minor].regs + ATMEL_TC_RA); /* Set the delay time */

	return 0;
}

/******************************************************************************
*	函数:	atmel_tc_config_setclk
*	功能:	定时器时钟源及时钟频率配置函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_tc_config_setclk(int minor,unsigned long arg)
{
	int retval = 0;
	
	if(arg == MCKD2)
		tc[minor].clock_f = MCK/2;
	else if(arg == MCKD8)
		tc[minor].clock_f = MCK/8;
	else if(arg == MCKD32)
		tc[minor].clock_f = MCK/32;
	else if(arg == MCKD128)
		tc[minor].clock_f = MCK/128;
	else if(arg == SCK32KIHZ)
		tc[minor].clock_f = 32768;
	else if(arg == ETCLK0)
		tc[minor].clock_f = eTCLK0;
	else if(arg == ETCLK1)
		tc[minor].clock_f = eTCLK1;
	else if(arg == ETCLK2)
		tc[minor].clock_f = eTCLK2;
	else
		retval= -EFAULT;
		
	tc[minor].clock_source = arg;
	DPRINTK("set clock done source is %d\n",tc[minor].clock_source);
	return retval;
}

/******************************************************************************
*	函数:	atmel_tc_ioctl
*	功能:	定时器控制函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
int atmel_tc_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	int minor;	
	u64 arg_t;

	retval = down_interruptible(&(drv_info.sem));
	if (retval) {
		retval = -EBUSY;
		return retval;	
	}
	minor = iminor(inode);		
	DPRINTK(  "%s:enter,cmd = (%d)(%d)...arg = %lx\n",__FUNCTION__, cmd,SET_CLOCK,arg);
	switch (cmd) {
		//设置心跳功能
		case SET_DELAY:	
			retval = atmel_tc_config_delay( minor, arg);
			break;
		//设置频率测量功能
		case SET_MEASURE:
			retval = atmel_tc_config_measure( minor, arg);
			break;
		//设置脉宽调制功能
		case SET_PWM:	
			if((u64*)arg==NULL)
				retval = -1;
			arg_t = *((u64*)arg);
			retval = atmel_tc_config_pwm(minor,arg_t);			
			break;
		//设置定时器模块为配合开入模块模式
		case SET_PULSE:
			tc[minor].pulse = 1;
			retval = 0;
			break;
		//设置时钟源
		case SET_CLOCK:
			retval = atmel_tc_config_setclk( minor, arg);
			break;
		//启动定时器
		case TCSTART:
			__raw_writel(ATMEL_TC_CLKEN, tc[minor].regs + ATMEL_TC_CCR); /* Enable the Clock Counter */		
			__raw_writel(ATMEL_TC_SWTRG, tc[minor].regs + ATMEL_TC_CCR); /* Enable the Clock Counter */
			break;
		
		default:
			retval = -EFAULT;
			
	}
	up(&(drv_info.sem));		
	return retval;		
}

static struct file_operations atmel_tc_fops = { 
	.owner	= THIS_MODULE, 
	.open	= atmel_tc_open,  
	.read	= atmel_tc_read, 
	.ioctl	= atmel_tc_ioctl,
	.release = atmel_tc_release, 
}; 

/******************************************************************************
*	函数:	atmel_tc_init
*	功能:	定时器初始化函数
*	参数:
*	返回:	0 - 成功
 ******************************************************************************/
static int __init atmel_tc_init(void)
{	
	static int i=0;
	static int retval=0;
	static char tc_t[20];
	
	strcpy(drv_info.name ,DRIVER_NAME);
	DPRINTK(  "%s:enter,major = (%d)...\n",__FUNCTION__, MAJOR(atmel_tc_dev_t));
	
	//字符设备初始化，创建类，在sysfs中注册设备，以备udev创建设备节点
	retval = register_chrdev_region (atmel_tc_dev_t, MINOR_MAX, drv_info.name);
	if (retval)
		return retval;
	cdev_init(&(drv_info.cdev), &atmel_tc_fops);
	drv_info.cdev.owner = THIS_MODULE;
	retval = cdev_add(&drv_info.cdev, atmel_tc_dev_t, 6);
	if (retval)
		return retval;
	drv_class = class_create(THIS_MODULE, drv_info.name);
	if (drv_class == NULL)
		return -ENOMEM;

	for(i=0; i<MINOR_MAX; i++) {		
		atmel_tc_dev_t = MKDEV(MAJOR_DEVICE,i);		
		sprintf(tc_t,"%s%d",drv_info.name,i);			
		device_create(drv_class, NULL,atmel_tc_dev_t, NULL, tc_t);
		atomic_set(&tc[i].opened, 1);    //initialate device open count
	}

	//初始化信号量，设置中断函数
	init_MUTEX(&(drv_info.sem));
	atmel_tc_interrupt[0]=atmel_tc0_interrupt;
	atmel_tc_interrupt[1]=atmel_tc1_interrupt;
	atmel_tc_interrupt[2]=atmel_tc2_interrupt;
	atmel_tc_interrupt[3]=atmel_tc3_interrupt;
	atmel_tc_interrupt[4]=atmel_tc4_interrupt;
	atmel_tc_interrupt[5]=atmel_tc5_interrupt;

	DPRINTK("%s:init done!\n",__FUNCTION__);
	return 0;
}

static void __exit atmel_tc_exit(void)
{	
	static int i=0;	
	for(i=0; i<MINOR_MAX; i++) {
		device_destroy(drv_class, MKDEV(MAJOR_DEVICE,i));
	}
	class_destroy(drv_class);	
	cdev_del(&drv_info.cdev);
	atmel_tc_dev_t = MKDEV(MAJOR_DEVICE,0);   //add this code! or can't remove device
	unregister_chrdev_region(atmel_tc_dev_t, MINOR_MAX);	
	DPRINTK("exit!\n");
}

MODULE_AUTHOR("Roy <luranran@gmail.com>");
MODULE_DESCRIPTION("atmel_tc driver");
MODULE_LICENSE("GPL");
module_init(atmel_tc_init);
module_exit(atmel_tc_exit);
