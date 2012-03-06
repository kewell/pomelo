/*****************************************************************************/
/*许继电气股份有限公司                                     版权：2008-2015   */
/*****************************************************************************/
/* 本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许    */
/* 可不得擅自修改或发布，否则将追究相关的法律责任。                          */
/*                                                                           */
/*                      河南许昌许继股份有限公司                             */
/*                      www.xjgc.com                                         */
/*                      (0374) 321 2924                                      */
/*                                                                           */
/*****************************************************************************/


/******************************************************************************
    项目名称    ：  SGE800计量智能终端平台
    文件名      ：  gpiotest.c
    描述        ：  本文件用于调试和测试平台库gpio
    版本        ：  0.1
    作者        ：  路冉冉
    创建日期    ：  2009.12													 
******************************************************************************/


//C库头文件
#include <stdio.h>		//printf
#include <stdlib.h>		//exit
#include <unistd.h>		//sleep

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <memory.h>

/*************************************************
  LCD一般配置
*************************************************/

#define GUI_LCM_XMAX 160
#define GUI_LCM_YMAX 160

#define I8    signed char
#define U8  unsigned char     /* unsigned 8  bits. */
#define I16   signed short    /*   signed 16 bits. */
#define U16 unsigned short    /* unsigned 16 bits. */
#define I32   signed long   /*   signed 32 bits. */
#define U32 unsigned long   /* unsigned 32 bits. */
#define I16P I16              /*   signed 16 bits OR MORE ! */
#define U16P U16              /* unsigned 16 bits OR MORE ! */

typedef struct { I16 x0,y0,x1,y1; } LCD_RECT;
typedef unsigned char  INT8U;


INT8U DispRAM[GUI_LCM_YMAX][(GUI_LCM_XMAX+2)/2];
void * disp_base;
void * sys_base;
int fd;
LCD_RECT disp_area={0,0,159,159};

#define LCD_XSIZE          (160)       /* X-resolution of LCD, Logical coor. */
#define LCD_YSIZE          (160)       /* Y-resolution of LCD, Logical coor. */
#define LCD_CONTROLLER     (uc1698)
#define LCD_BITSPERPIXEL   (4)

#define AT91_DIS 0x60000000

#define LCD_WRITE_A0(Byte) (*(volatile unsigned char*)disp_base = (Byte) ) //__raw_writeb(Byte, lcd_base);   //LCD_X_Write01(Byte)
#define LCD_WRITE_A1(Byte) (*(volatile unsigned char*)(disp_base+1) = (Byte) )   //LCD_X_Write00(Byte)
#define LCD_READ_A1()      (*(volatile unsigned char*)(disp_base+1) )//__raw_readb(lcd_base+1);         //LCD_X_Read01()
#define LCD_READ_A0()      (*(volatile unsigned char*)disp_base )//__raw_readb(lcd_base);          //LCD_X_Read01()



void delay_1(INT8U t)
{
	sleep(t);
}
void init_uc1698(void)
{
	LCD_WRITE_A0(0xe2);   //system reset
	sleep(1);
//	LCD_WRITE_A0(0x31);	//?	//roy
//	LCD_WRITE_A0(0x08);	//?	//roy
	LCD_WRITE_A0(0x2b); // internal power control
	//LCD_WRITE_A0(0x25);  //set TC=-0.05%
	LCD_WRITE_A0(0xA0);  //set line rate
	LCD_WRITE_A0(0xeb);  //0XEA//set bias
	LCD_WRITE_A0(0xf1);   //set com end
	LCD_WRITE_A0(0x9f);   //set duty :1/160
	LCD_WRITE_A0(0x81);   //set VLCD value
	LCD_WRITE_A0(0x80); //VLCD=(CV0+Cpm*pm)*(1+(T-25)*CT%)
	LCD_WRITE_A0(0xc4);  //0XC4 //set LCD mapping control  //scan dirction com0~127
	LCD_WRITE_A0(0x89); //set RAM address control
	LCD_WRITE_A0(0xd1); // R_G_B
	LCD_WRITE_A0(0xd5);   //4k color
	LCD_WRITE_A0(0xc8);	//?
	LCD_WRITE_A0(0x18);	//?
//	LCD_WRITE_A0(0xde);	//roy

	LCD_WRITE_A0(0xad);   //0xad//display  on
//	LCD_WRITE_A0(0xa4);

}

/*******************************************************************************************
**函数名称：void  GUI_HLine(INT8U x0, INT8U y0, INT8U x1)
**函数功能：
**入口参数：
**返 回 值：
**说    明：
*******************************************************************************************/

void Init_Ram_Address(void)
{
	LCD_WRITE_A0(0xf4);  //set column start address
	LCD_WRITE_A0(0x25);  //  start address
	LCD_WRITE_A0(0xf6);  //  set column end address
	LCD_WRITE_A0(0x5a); //(0x4f+1)*3=240  5a   //90	//保证54个RGB

	LCD_WRITE_A0(0xf5);  //set row start address
	LCD_WRITE_A0(0x00);  //start address=0x00
	LCD_WRITE_A0(0xf7); //set row end address
	LCD_WRITE_A0(0x9f); //row end address=9f
	LCD_WRITE_A0(0x70);  //set row MSB address
	LCD_WRITE_A0(0x60);  //set row LSB address
	LCD_WRITE_A0(0x12);  //set column MSB address  //    12
	LCD_WRITE_A0(0x05);  //set column LSB address  //37  05
	LCD_WRITE_A0(0xf8); //inside mode
}

//运行中初始化LCD
void Init_LCD_Running(void)
{
	LCD_WRITE_A0(0x2b); // internal power control
	//LCD_WRITE_A0(0x25);  //set TC=-0.05%
	LCD_WRITE_A0(0xA0);  //set line rate
	LCD_WRITE_A0(0xeb);  //0XEA//set bias
	LCD_WRITE_A0(0xf1);   //set com end
	LCD_WRITE_A0(0x9f);   //set duty :1/160
	LCD_WRITE_A0(0x81);   //set VLCD value
	LCD_WRITE_A0(0x80); //VLCD=(CV0+Cpm*pm)*(1+(T-25)*CT%)
	LCD_WRITE_A0(0xc4);  //0XC4 //set LCD mapping control  //scan dirction com0~127
	LCD_WRITE_A0(0x89); //set RAM address control
	LCD_WRITE_A0(0xd1); // R_G_B
	LCD_WRITE_A0(0xd5);   //4k color
	LCD_WRITE_A0(0xc8);	//?
	LCD_WRITE_A0(0x18);	//?
	//LCD_WRITE_A0(0xde);
	//LCD_WRITE_A0(0xad);   //0xad//display  on
	LCD_WRITE_A0(0xad);   //0xad//display  on

	Init_Ram_Address();

}
//此函数不完善,要求address.x0=0; address.x1=159
void GUI_CopyRAMToLCD(LCD_RECT address)
{
	INT8U i,j;
	static INT8U protect=0;	//函数保护值

	while(protect)
	{
		sleep(1);
	}
	protect=1;

	LCD_WRITE_A0(0x70|(address.y0>>4)); 	//set row MSB address
	LCD_WRITE_A0(0x60|(address.y0&0x0F));   //set row LSB address
	LCD_WRITE_A0(0x12);  //set column MSB address  //    12
	LCD_WRITE_A0(0x05);  //set column LSB address  //37  05

	for(i=address.y0;i<=address.y1;i++)
	{
		for(j=address.x0;j<(address.x1+3)/2;j++)	//保证每行写满81个字节
			LCD_WRITE_A1(DispRAM[i][j]);
	}

	protect=0;
}
/*******************************************************************************************
**函数名称：GUI_FillSCR()
**函数功能：全屏填充。直接使用数据填充显示缓冲区。
**入口参数：dat		填充的数据(对于黑白色LCM，为0的点灭，为1的点显示)
**返 回 值：无
**说    明：
*******************************************************************************************/
void GUI_FillSCR(INT8U dat)
{
	INT8U i,j;

	for(i=0;i<GUI_LCM_YMAX;i++)
	{
		for(j=0;j<(GUI_LCM_XMAX+2)/2;j++)
		  DispRAM[i][j]=dat;
	}

}

/*****************************************************************************************
**函数名称：INT8U  GUI_DrawPoint(INT8U x, INT8U y)
**函数功能：
**入口参数：
**返 回 值：
**说    明：
******************************************************************************************
*/
void GUI_DrawPoint_at(INT8U x, INT8U y)
{
   if(x%2==0)		//x为偶数
   {
		DispRAM[y][x/2]&=0x0F;	//清高半字节
		DispRAM[y][x/2]|=0xF0;	//置高半字节
   }
	else	//x为奇数
	{
		DispRAM[y][x/2]&=0xF0;	//清高半字节
		DispRAM[y][x/2]|=0x0F;	//置高半字节
	}
   LCD_WRITE_A0(0x60|(y&0x0f));		//写行地址低位
   LCD_WRITE_A0(0x70|(y>>4));		//写行地址高位
   LCD_WRITE_A0(x&0x0f);		//写列地址低位
   LCD_WRITE_A0(0x10|(x>>4));	//写列地址高位
   LCD_WRITE_A1(DispRAM[y][x/2]);
}
void GUI_DrawPoint(INT8U x, INT8U y)
{
   if(x>=GUI_LCM_XMAX) return;
   if(y>=GUI_LCM_YMAX) return;

   if(x%2==0)		//x为偶数
   {
		DispRAM[y][x/2]&=0x0F;	//清高半字节
		DispRAM[y][x/2]|=0xF0;	//置高半字节
   }
	else	//x为奇数
	{
		DispRAM[y][x/2]&=0xF0;	//清高半字节
		DispRAM[y][x/2]|=0x0F;	//置高半字节
	}
}

/*****************************************************************************************
**函数名称：void  GUI_DrawHLine(INT8U x0, INT8U y0, INT8U x1)
**函数功能：
**入口参数：
**返 回 值：
**说    明：
******************************************************************************************
*/
void  GUI_DrawHLine(INT8U x0,  INT8U x1, INT8U y0)
{
   INT8U bak;

   if(x0>x1) 						// 对x0、x1大小进行排列，以便画图
   {  bak = x1;
	  x1 = x0;
	  x0 = bak;
   }

   for(; x0<=x1; x0++)
   {
		GUI_DrawPoint(x0, y0);
   }
}


/*************************************************
  操作底层所用到的寄存器
*************************************************/

//GPIO
#define PIO_PER		0x00	/* Enable Register */
#define PIO_PDR		0x04	/* Disable Register */
#define PIO_PSR		0x08	/* Status Register */
#define PIO_OER		0x10	/* Output Enable Register */
#define PIO_ODR		0x14	/* Output Disable Register */
#define PIO_OSR		0x18	/* Output Status Register */
#define PIO_IFER	0x20	/* Glitch Input Filter Enable */
#define PIO_IFDR	0x24	/* Glitch Input Filter Disable */
#define PIO_IFSR	0x28	/* Glitch Input Filter Status */
#define PIO_SODR	0x30	/* Set Output Data Register */
#define PIO_CODR	0x34	/* Clear Output Data Register */
#define PIO_ODSR	0x38	/* Output Data Status Register */
#define PIO_PDSR	0x3c	/* Pin Data Status Register */
#define PIO_IER		0x40	/* Interrupt Enable Register */
#define PIO_IDR		0x44	/* Interrupt Disable Register */
#define PIO_IMR		0x48	/* Interrupt Mask Register */
#define PIO_ISR		0x4c	/* Interrupt Status Register */
#define PIO_MDER	0x50	/* Multi-driver Enable Register */
#define PIO_MDDR	0x54	/* Multi-driver Disable Register */
#define PIO_MDSR	0x58	/* Multi-driver Status Register */
#define PIO_PUDR	0x60	/* Pull-up Disable Register */
#define PIO_PUER	0x64	/* Pull-up Enable Register */
#define PIO_PUSR	0x68	/* Pull-up Status Register */
#define PIO_ASR		0x70	/* Peripheral A Select Register */
#define PIO_BSR		0x74	/* Peripheral B Select Register */
#define PIO_ABSR	0x78	/* AB Status Register */
#define PIO_OWER	0xa0	/* Output Write Enable Register */
#define PIO_OWDR	0xa4	/* Output Write Disable Register */
#define PIO_OWSR	0xa8	/* Output Write Status Register */
#define AT91_SYS			(0xffffc000 )
#define AT91_SMC			(0xffffec00 - AT91_SYS )
#define AT91_MATRIX			(0xffffee00 - AT91_SYS )
#define AT91_GPIO_BASE		(0xFFFFF400 - AT91_SYS)

#define	PIN_PC9		73			//IO口编号
#define	PIN_PC13	77			//IO口编号

//SMC寄存器
#define AT91_SMC_SETUP(n)	(AT91_SMC + 0x00 + ((n)*0x10))	/* Setup Register for CS n */
#define		AT91_SMC_NWESETUP	(0x3f << 0)			/* NWE Setup Length */
#define			AT91_SMC_NWESETUP_(x)	((x) << 0)
#define		AT91_SMC_NCS_WRSETUP	(0x3f << 8)			/* NCS Setup Length in Write Access */
#define			AT91_SMC_NCS_WRSETUP_(x)	((x) << 8)
#define		AT91_SMC_NRDSETUP	(0x3f << 16)			/* NRD Setup Length */
#define			AT91_SMC_NRDSETUP_(x)	((x) << 16)
#define		AT91_SMC_NCS_RDSETUP	(0x3f << 24)			/* NCS Setup Length in Read Access */
#define			AT91_SMC_NCS_RDSETUP_(x)	((x) << 24)

#define AT91_SMC_PULSE(n)	(AT91_SMC + 0x04 + ((n)*0x10))	/* Pulse Register for CS n */
#define		AT91_SMC_NWEPULSE	(0x7f <<  0)			/* NWE Pulse Length */
#define			AT91_SMC_NWEPULSE_(x)	((x) << 0)
#define		AT91_SMC_NCS_WRPULSE	(0x7f <<  8)			/* NCS Pulse Length in Write Access */
#define			AT91_SMC_NCS_WRPULSE_(x)((x) << 8)
#define		AT91_SMC_NRDPULSE	(0x7f << 16)			/* NRD Pulse Length */
#define			AT91_SMC_NRDPULSE_(x)	((x) << 16)
#define		AT91_SMC_NCS_RDPULSE	(0x7f << 24)			/* NCS Pulse Length in Read Access */
#define			AT91_SMC_NCS_RDPULSE_(x)((x) << 24)

#define AT91_SMC_CYCLE(n)	(AT91_SMC + 0x08 + ((n)*0x10))	/* Cycle Register for CS n */
#define		AT91_SMC_NWECYCLE	(0x1ff << 0 )			/* Total Write Cycle Length */
#define			AT91_SMC_NWECYCLE_(x)	((x) << 0)
#define		AT91_SMC_NRDCYCLE	(0x1ff << 16)			/* Total Read Cycle Length */
#define			AT91_SMC_NRDCYCLE_(x)	((x) << 16)

#define AT91_SMC_MODE(n)	(AT91_SMC + 0x0c + ((n)*0x10))	/* Mode Register for CS n */
#define		AT91_SMC_READMODE	(1 <<  0)			/* Read Mode */
#define		AT91_SMC_WRITEMODE	(1 <<  1)			/* Write Mode */
#define		AT91_SMC_EXNWMODE	(3 <<  4)			/* NWAIT Mode */
#define			AT91_SMC_EXNWMODE_DISABLE	(0 << 4)
#define			AT91_SMC_EXNWMODE_FROZEN	(2 << 4)
#define			AT91_SMC_EXNWMODE_READY		(3 << 4)
#define		AT91_SMC_BAT		(1 <<  8)			/* Byte Access Type */
#define			AT91_SMC_BAT_SELECT		(0 << 8)
#define			AT91_SMC_BAT_WRITE		(1 << 8)
#define		AT91_SMC_DBW		(3 << 12)			/* Data Bus Width */
#define			AT91_SMC_DBW_8			(0 << 12)
#define			AT91_SMC_DBW_16			(1 << 12)
#define			AT91_SMC_DBW_32			(2 << 12)
#define		AT91_SMC_TDF		(0xf << 16)			/* Data Float Time. */
#define			AT91_SMC_TDF_(x)		((x) << 16)
#define		AT91_SMC_TDFMODE	(1 << 20)			/* TDF Optimization - Enabled */
#define		AT91_SMC_PMEN		(1 << 24)			/* Page Mode Enabled */
#define		AT91_SMC_PS		(3 << 28)			/* Page Size */
#define			AT91_SMC_PS_4			(0 << 28)
#define			AT91_SMC_PS_8			(1 << 28)
#define			AT91_SMC_PS_16			(2 << 28)
#define			AT91_SMC_PS_32			(3 << 28)

/*************************************************
  操作底层所用到的函数
*************************************************/
static inline void at91_sys_write(unsigned int reg_offset, unsigned long value)
{
	//void __iomem *addr = (void __iomem *)AT91_VA_BASE_SYS;
	*(volatile unsigned long*)(sys_base + reg_offset) = value;
	//__raw_writel(value, addr + reg_offset);
}
#define __raw_writel(a,b) (*(volatile unsigned int*)(b) = (a) )
static inline void  *pin_to_controller(unsigned pin)
{
	pin /= 32;
	return sys_base + pin*512 + AT91_GPIO_BASE;

}
static inline unsigned pin_to_mask(unsigned pin)
{
	return 1 << (pin % 32);
}
int at91_set_gpio_output(unsigned pin, int value)
{
	void *pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -1;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + PIO_PUDR);
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
	__raw_writel(mask, pio + PIO_OER);
	__raw_writel(mask, pio + PIO_PER);
	return 0;
}
int at91_set_A_periph(unsigned pin, int use_pullup)
{
	void *pio = pin_to_controller(pin);
	unsigned	mask = pin_to_mask(pin);

	if (!pio)
		return -1;

	__raw_writel(mask, pio + PIO_IDR);
	__raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
	__raw_writel(mask, pio + PIO_ASR);
	__raw_writel(mask, pio + PIO_PDR);
	return 0;
}
int disp_init(void)
{
	int ret = -1;
	int num = 0;
	unsigned int *gpioa_out,*gpiob_out,*gpioc_out;

	if((fd=open("/dev/mem",O_RDWR|O_SYNC))==-1){
		ret = -1;
		return ret;
	}
	disp_base = mmap(0,0x0f, PROT_READ|PROT_WRITE, MAP_SHARED, fd,AT91_DIS);
	sys_base = mmap(0,0x3fff, PROT_READ|PROT_WRITE, MAP_SHARED, fd,AT91_SYS);
#if 1
#define PMC_BASE (0xfffffc00 - 0xffffc000)
	//输出时钟状态寄存器
	printf("clock status reg:");
	printf("%x: %.8x \n",AT91_SYS + PMC_BASE  + 0x18,*(volatile unsigned int *)(sys_base + PMC_BASE  + 0x18));
//测试各个io口的输出电平
	gpioa_out = (volatile unsigned int *)(sys_base + 0x3400 );
	gpiob_out = (volatile unsigned int *)(sys_base + 0x3600 );
	gpioc_out = (volatile unsigned int *)(sys_base + 0x3800 );
	//*(volatile u nsigned long *)(sys_base + 0xe800 + 0x38); //PIOA的输出
	printf("gpio register read:");
	for(num =0; num <= 0x6c; ){
		if(num % 16 == 0){
			printf("\n");
			printf("%x: ",0xfffff400 + num);
		}
		printf("%.8x ",*(volatile unsigned int *)(sys_base + 0x3400 + num));
		num += 4;
	}
	for(num =0; num <= 0x6c; ){
		if(num % 16 == 0){
			printf("\n");
			printf("%x: ",0xfffff600 + num);
		}
		printf("%.8x ",*(volatile unsigned int *)(sys_base + 0x3600 + num));
		num += 4;
	}
	for(num =0; num <= 0x6c; ){
		if(num % 16 == 0){
			printf("\n");
			printf("%x: ",0xfffff800 + num);
		}
		printf("%.8x ",*(volatile unsigned int *)(sys_base + 0x3800 + num));
		num += 4;
	}
	printf("\n");
//测试结束
#endif
	at91_set_A_periph(PIN_PC9, 0);		//set NCS5
	/*
	 * Static memory controller timing adjustments.
	 * REVISIT:  these timings are in terms of MCK cycles, so
	 * when MCK changes (cpufreq etc) so must these values...
	 */
	at91_sys_write(AT91_SMC_SETUP(5),
				  AT91_SMC_NWESETUP_(1)
				| AT91_SMC_NCS_WRSETUP_(1)				//tcssa=5ns
				| AT91_SMC_NRDSETUP_(1)
				| AT91_SMC_NCS_RDSETUP_(4)
	);
	at91_sys_write(AT91_SMC_PULSE(5),
					  AT91_SMC_NWEPULSE_(10)
					| AT91_SMC_NCS_WRPULSE_(10)			//tcy=100ns
					| AT91_SMC_NRDPULSE_(10)
					| AT91_SMC_NCS_RDPULSE_(10)
		);
	at91_sys_write(AT91_SMC_CYCLE(5),
					  AT91_SMC_NWECYCLE_(20)
					| AT91_SMC_NRDCYCLE_(20)
		);
	at91_sys_write(AT91_SMC_MODE(5),
					  AT91_SMC_READMODE
					|  AT91_SMC_EXNWMODE_DISABLE
					| AT91_SMC_DBW_8
					| AT91_SMC_WRITEMODE
					| AT91_SMC_BAT_WRITE
					| AT91_SMC_TDF_(1)

		);
	init_uc1698();
	Init_Ram_Address();
	ret = 0;
	return ret;

}
void disp_exit(void)
{
	at91_set_gpio_output(PIN_PC13,0);	//背光
	munmap(disp_base,0x0f);//解除映射关系
	munmap(sys_base,0x3fff);//解除映射关系
	close(fd);
}

int main()
{
	int ret=0,i=0;


	printf("+----------------------------------------+\n");
	printf("| 	 ARM  Linux  display test		|\n");
	printf("| 	  Write on 2010.10.8 ROY		|\n");
	printf("+----------------------------------------+\n");

	if((ret = disp_init()) != 0)
		goto error;

	at91_set_gpio_output(PIN_PC13,1);	//背光亮
	while(i<50){

		GUI_FillSCR(0x00);
		GUI_CopyRAMToLCD(disp_area);

		GUI_FillSCR(0xff);
		disp_area.y0 += 4;
		disp_area.y1 += 4;
		if(disp_area.y1 > 159){
			disp_area.y0 = 0;
			disp_area.y1 = 4;
		}
		GUI_CopyRAMToLCD(disp_area);
		sleep(1);
		i++;
	}
	ret = 0;
	printf ("test done %d\n",ret);
	disp_exit();
	exit(1);

error:
	printf ("test error %d\n",ret);
	return 0;
}
