/********************************************************************************
 *      Copyright:  (C) 2011 R&D of San Fran Electronics Co., LTD  
 *                  All rights reserved.
 *
 *       Filename:  pio_base.h
 *    Description:  This head file 
 *
 *        Version:  1.0.0(11/12/2011~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "11/12/2011 09:07:55 AM"
 *                 
 ********************************************************************************/
#define I8    signed char
#define U8  unsigned char     /* unsigned 8  bits. */
#define uchar  unsigned char     /* unsigned 8  bits. */
#define I16   signed short    /*   signed 16 bits. */
#define U16 unsigned short    /* unsigned 16 bits. */
#define I32   signed long   /*   signed 32 bits. */
#define U32 unsigned long   /* unsigned 32 bits. */
#define I16P I16              /*   signed 16 bits OR MORE ! */
#define U16P U16              /* unsigned 16 bits OR MORE ! */

#define GUI_LCM_XMAX 160
#define GUI_LCM_YMAX 160

#define AT91_DIS 0x80000000

typedef unsigned char  INT8U;

typedef struct 
{ 
    I16 x0;
    I16 y0;
    I16 x1;
    I16 y1;
}LCD_RECT;

LCD_RECT disp_area = {0, 0, GUI_LCM_XMAX - 1, GUI_LCM_YMAX - 1};
INT8U DispRAM[GUI_LCM_YMAX][(GUI_LCM_XMAX + 2) / 2];

#define write_cmd(Byte)     (*(volatile unsigned char*)disp_base = (Byte))
#define write_command(Byte)     (*(volatile unsigned char*)disp_base = (Byte))
#define write_data(Byte)    (*(volatile unsigned char*)(disp_base + 4) = (Byte))
#define write_data2(Byte)    (*(volatile unsigned char*)(disp_base + 4) = (Byte))
#define LCD_READ_A1()       (*(volatile unsigned char*)(disp_base + 1))
#define LCD_READ_A0()       (*(volatile unsigned char*)disp_base)

#define write(Flag, Byte)   ((0 == Flag) ? write_cmd(Byte) : write_data(Byte))
#define read(Flag)          ((0 == Flag) ? LCD_READ_A0() : LCD_READ_A1())

void *disp_base;
void *sys_base;

void GUI_FillSCR(INT8U dat);
void GUI_CopyRAMToLCD(LCD_RECT);
void Init_Ram_Address(void);
void UC1698_init(void);
void Init_Ram_Address(void);
void GUI_CopyRAMToLCD(LCD_RECT address);
void GUI_FillSCR(INT8U dat);
void GUI_DrawPoint_at(INT8U x, INT8U y);
void GUI_DrawPoint(INT8U x, INT8U y);
void GUI_DrawHLine(INT8U x0,  INT8U x1, INT8U y0);
int disp_init(void);
void disp_exit(void);
void display_WZ();
void writeHZ(U8 h1, U8 h2, U8 h3);
void SW(U8 x);
void coord(U8 col,U8 page);
void character(void);
void words(uchar x,uchar y,uchar type,uchar *p);

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

#define	PIN_PC12	76			//IO口编号 WENJING
#define	PIN_PC13	77			//IO口编号

#define __raw_writel(a, b) (*(volatile unsigned int*)(b) = (a) )

#if 1
static inline unsigned int at91_sys_read(unsigned int reg_offset)
{
    //void __iomem *addr = (void __iomem *)AT91_VA_BASE_SYS;
    //return __raw_readl(addr + reg_offset);
    return *(volatile unsigned long*)(sys_base + reg_offset);
}

static inline void at91_sys_write(unsigned int reg_offset, unsigned long value)
{
	//void __iomem *addr = (void __iomem *)AT91_VA_BASE_SYS;
	//__raw_writel(value, addr + reg_offset);
	*(volatile unsigned long*)(sys_base + reg_offset) = value;
}
#endif

static inline void  *pin_to_controller(unsigned pin)
{
	pin /= 32;
	return sys_base + pin*512 + AT91_GPIO_BASE;
}

static inline unsigned pin_to_mask(unsigned pin)
{
	return 1 << (pin % 32);
}

int at91_set_gpio_input(unsigned pin, int use_pullup)
{
    void *pio = pin_to_controller(pin);
    unsigned    mask = pin_to_mask(pin);

    if (!pio)
        return -1;

    __raw_writel(mask, pio + PIO_IDR);
    __raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
    __raw_writel(mask, pio + PIO_ODR);
    __raw_writel(mask, pio + PIO_PER);
    return 0;
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

int at91_set_GPIO_periph(unsigned pin, int use_pullup)
{
    void *pio = pin_to_controller(pin);
    unsigned    mask = pin_to_mask(pin);

    if (!pio)
        return -1;
    __raw_writel(mask, pio + PIO_IDR);
    __raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
    __raw_writel(mask, pio + PIO_PER);
    return 0;
}

int at91_set_B_periph(unsigned pin, int use_pullup)
{
    void *pio = pin_to_controller(pin);
    unsigned    mask = pin_to_mask(pin);

    if (!pio)
        return -1;

    __raw_writel(mask, pio + PIO_IDR);
    __raw_writel(mask, pio + (use_pullup ? PIO_PUER : PIO_PUDR));
    __raw_writel(mask, pio + PIO_BSR);
    __raw_writel(mask, pio + PIO_PDR);
    return 0;
}
