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
#include <asm-arm/arch-at91/at91sam926x_mc.h>
#include <asm-arm/arch-at91/at91sam9260_matrix.h>

//模块调试开关
//#undef	DEBUG
#define DEBUG
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

//======================================== 变量定义 ===========================
/* 定义颜色数据类型(可以是数据结构) */
#define  INT8U				u8
#define  TCOLOR				INT8U
#define  GUI_LCM_XMAX		160	/* 定义液晶x轴的点数 */
#define  GUI_LCM_YMAX		160	/* 定义液晶y轴的点数 */
TCOLOR  gui_disp_buf[GUI_LCM_YMAX][GUI_LCM_XMAX/8];	// 声明GUI显示缓冲区

u8 const  DCB_HEX_TAB[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};



/*********************************************************************
*
*                   General configuration of LCD
*
**********************************************************************
*/

#define LCD_XSIZE          (160)       /* X-resolution of LCD, Logical coor. */
#define LCD_YSIZE          (160)       /* Y-resolution of LCD, Logical coor. */
#define LCD_CONTROLLER     (uc1698)
#define LCD_BITSPERPIXEL   (4)
void __iomem *lcd_base;
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

//=============================================================================
//函 数 名:  WriteCommandUC1698U()
//功 能：	写命令到UC1698U
//入口参数： CmdData:命令码
//出口参数： 无
//返 回 值： 无
//=============================================================================
void WriteCommandUC1698U(u8 cmddata)
{
	u32 temp;
	temp = cmddata ;
	__raw_writel(temp, lcd_base);
}
//=============================================================================
//函 数 名:  WriteDataUC1698U()
//功 能：	写数据到UC1698U
//入口参数： Data:写入的数据
//出口参数： 无
//返 回 值： 无
//=============================================================================
void WriteDataUC1698U(u8 data)
{
	u32 temp;
	temp = data ;
	__raw_writel(temp, lcd_base+1);

}
void  LCD_Initialize(void)
{

#define BR		0X01	   	//Set LCD Bias Ratio:1/10 bias
#define PC1     0X03        //power control set as internal power
#define TC      0x00        //set temperate compensation as 0%
#define PM      192         //Set Vbias Potentiometer
#define LC_210  0X05        //set LCD Control
#define LC_43   0x03
#define LC_5    0X01
#define LC_76   0x01
#define LC_8    0X00
#define NIV     0X10        //n-line inversion
#define CSF     0X02        //23:enable FRC,PWM,LRM sequence
#define WPC0    0x25        //Starting Column Address
#define WPP0    0           //Starting Row Address
#define WPC1	0x5A		//Ending Column Address
#define WPP1	159		    //Ending Row Address
#define	AC    	0x01		//Address  Control
#define CEN 	159 		//COM scanning end (last COM with full line cycle,0 based index)



//	at91_set_gpio_output(AT91_PIN_PC13,0);		 //硬件复位
   	msleep(1);
//	at91_set_gpio_output(AT91_PIN_PC13,1);
	msleep(100);

//	WriteCommandUC1698U(0xE8|(BR&0X03));
//	WriteCommandUC1698U(0x28|(PC1&0X03));
//	WriteCommandUC1698U(0x24|(TC&0x03));
//	WriteCommandUC1698U(0x81);
//	WriteCommandUC1698U(PM );
//	WriteCommandUC1698U(0xc0|(LC_210&0X07) );
//	WriteCommandUC1698U(0xa0|(LC_43 &0X03) );
//	WriteCommandUC1698U(0xd0|(LC_5  &0X01) );
//	WriteCommandUC1698U(0xd4|(LC_76 &0X03) );
//	WriteCommandUC1698U(0x84|(LC_8  &0X01) );
//	WriteCommandUC1698U(0xc8 );
//	WriteCommandUC1698U( NIV );
//	WriteCommandUC1698U(0xd8|(CSF&0X07) );
//	WriteCommandUC1698U(0xf4 );
//	WriteCommandUC1698U(WPC0 );
//	WriteCommandUC1698U(0xf6 );
//	WriteCommandUC1698U(WPC1 );
//	WriteCommandUC1698U(0xf5 );
//	WriteCommandUC1698U(WPP0 );
//	WriteCommandUC1698U(0xf7 );
//	WriteCommandUC1698U(WPP1 );
//	WriteCommandUC1698U(0xf8|((AC>>3)&0x01) );
//	WriteCommandUC1698U(0x88| (0x07&AC) );
//	WriteCommandUC1698U(0xf1 );
//	WriteCommandUC1698U(CEN );
//	WriteCommandUC1698U(0xad );		//18:display on,select on/off mode.Green Enhance mode disable


	WriteCommandUC1698U(0x2b); // internal power control
	//LCD_WRITE_A0(0x25);  //set TC=-0.05%
	WriteCommandUC1698U(0xA0);  //set line rate
	WriteCommandUC1698U(0xeb);  //0XEA//set bias
	WriteCommandUC1698U(0xf1);   //set com end
	WriteCommandUC1698U(0x9f);   //set duty :1/160
	WriteCommandUC1698U(0x81);   //set VLCD value
	WriteCommandUC1698U(0x80); //VLCD=(CV0+Cpm*pm)*(1+(T-25)*CT%)
	WriteCommandUC1698U(0xc4);  //0XC4 //set LCD mapping control  //scan dirction com0~127
	WriteCommandUC1698U(0x89); //set RAM address control
	WriteCommandUC1698U(0xd1); // R_G_B
	WriteCommandUC1698U(0xd5);   //4k color
	WriteCommandUC1698U(0xc8);	//?
	WriteCommandUC1698U(0x18);	//?
	//WriteCommandUC1698U(0xde);
	//WriteCommandUC1698U(0xad);   //0xad//display  on
	WriteCommandUC1698U(0xad);   //0xad//display  on

}
void LCD_SetWindowProgram(INT8U x1,INT8U y1,INT8U x2,INT8U y2)
{
 	WriteCommandUC1698U(0xf4 );
	WriteCommandUC1698U(0x25+x1/3 );
	WriteCommandUC1698U(0xf6 );
	WriteCommandUC1698U(0x25+(x2+2)/3);
	WriteCommandUC1698U(0xf5 );
	WriteCommandUC1698U(y1);
	WriteCommandUC1698U(0xf7 );
	WriteCommandUC1698U(y2 );
}
void LCD_SetAddress(INT8U x,INT8U y)
{
    x = 0x25+x/2;//计算出该点所在的列地址，注意该液晶每3个点共有一个地址
    WriteCommandUC1698U(0x60|(y&0x0f));		//写行地址低位
    WriteCommandUC1698U(0x70|(y>>4));		//写行地址高位
    WriteCommandUC1698U(x&0x0f);		//写列地址低位
    WriteCommandUC1698U(0x10|(x>>4));	//写列地址高位
}
//=============================================================================
//                   与LCM相关的GUI接口函数
//=============================================================================
//=============================================================================
//函 数 名: GUI_RefreshSCR()
//功 能：	刷新全屏，将显示缓冲区全部数据送到模组显示
//入口参数： 无
//出口参数： 无
//返 回 值： 无
//=============================================================================
void GUI_RefreshSCR(void)
{

	INT8U  i,j;
	INT8U  TempData = 0;
	LCD_SetWindowProgram(0,0,159,159);
	LCD_SetAddress(0,0)	;
	for(i=0; i<GUI_LCM_YMAX; i++)		// 历遍所有列
   {  for(j=0; j<GUI_LCM_XMAX/8; j++)	// 历遍所有行
      {
	   	TempData=0;
		if(gui_disp_buf[i][j] & 0x80)
       		TempData=0xf0;
   		if(gui_disp_buf[i][j] & 0x40)
       		TempData|=0x0f;
   		WriteDataUC1698U(TempData);

		TempData=0;
   		if(gui_disp_buf[i][j] & 0x20)
       		TempData=0xf0;
   		if(gui_disp_buf[i][j] & 0x10)
       		TempData|=0x0f;
   		WriteDataUC1698U(TempData);

		TempData=0;
   		if(gui_disp_buf[i][j] & 0x08)
       		TempData=0xf0;
   		if(gui_disp_buf[i][j] & 0x04)
       		TempData|=0x0f;
   		WriteDataUC1698U(TempData);

		TempData=0;
   		if(gui_disp_buf[i][j] & 0x02)
       		TempData=0xf0;
   		if(gui_disp_buf[i][j] & 0x01)
       		TempData|=0x0f;
  		WriteDataUC1698U(TempData);
	  }
	  WriteDataUC1698U(0x00);  //补全每行末尾的数据，使总点数能被三整除
   }

}
//=============================================================================
//函 数 名: GUI_FillSCR()
//功 能：	全屏填充。直接使用数据填充显示缓冲区。
//入口参数： dat 填充的数据(对于黑白色LCM，为0的点灭，为1的点显示)
//出口参数： 无
//返 回 值： 无
//=============================================================================
void  GUI_FillSCR(INT8U dat)
{  INT8U  i, j;

   for(i=0; i<GUI_LCM_YMAX; i++)		// 历遍所有行
   {  for(j=0; j<GUI_LCM_XMAX/8; j++)	// 历遍所有行
      {  gui_disp_buf[i][j] = dat;		// 填充数据
	  }
   }
   GUI_RefreshSCR();
}
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
	
    class_device_destroy(atmel_gpio_class, dev);
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


	unsigned long csa;

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
    class_device_create(atmel_gpio_class, NULL, dev, NULL, "atmel_gpio");

    //测试LCD总线初始化
    at91_set_gpio_output(AT91_PIN_PC13,1);	//背光亮
    at91_set_A_periph(AT91_PIN_PC9, 0);		//set NCS5
    //ioremap
    lcd_base = ioremap(0x60000000, 8);

    csa = at91_sys_read(AT91_MATRIX_EBICSA);
    at91_sys_write(AT91_MATRIX_EBICSA, csa | AT91_MATRIX_CS5A_SMC);
	/*
	 * Static memory controller timing adjustments.
	 * REVISIT:  these timings are in terms of MCK cycles, so
	 * when MCK changes (cpufreq etc) so must these values...
	 */
	at91_sys_write(AT91_SMC_SETUP(5),
				  AT91_SMC_NWESETUP_(1)
				| AT91_SMC_NCS_WRSETUP_(1)				//tcssa=5ns
				| AT91_SMC_NRDSETUP_(1)
				| AT91_SMC_NCS_RDSETUP_(1)
	);
	at91_sys_write(AT91_SMC_PULSE(5),
					  AT91_SMC_NWEPULSE_(5)
					| AT91_SMC_NCS_WRPULSE_(11)			//tcy=100ns
					| AT91_SMC_NRDPULSE_(5)
					| AT91_SMC_NCS_RDPULSE_(11)
		);
	at91_sys_write(AT91_SMC_CYCLE(5),
					  AT91_SMC_NWECYCLE_(12)
					| AT91_SMC_NRDCYCLE_(12)
		);
	at91_sys_write(AT91_SMC_MODE(5),
					  AT91_SMC_READMODE
					|  AT91_SMC_EXNWMODE_DISABLE
					| AT91_SMC_DBW_8
					| AT91_SMC_WRITEMODE
					| AT91_SMC_BAT_WRITE
					| AT91_SMC_TDF_(4)

		);
//   __raw_writel(0xff000000, lcd_base);
//	__raw_readl(lcd_base);
//	__raw_writel(0x01030700, lcd_base);

	LCD_Initialize();
	GUI_FillSCR(0x00);
	DPRINTK("init: init done!!\n");
	return 0;
}
module_init(atmel_gpio_init);
module_exit(atmel_gpio_exit);

MODULE_AUTHOR("roy <luranran@gmail.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("GPIO driver For AT91SAM9260");
