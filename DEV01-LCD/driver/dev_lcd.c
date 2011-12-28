/*********************************************************************************
 *      Copyright:  (C) 2011 R&D of San Fran Electronics Co., LTD  
 *                  All rights reserved.
 *
 *       Filename:  dev_lcd.c
 *    Description:
 *                 
 *        Version:  1.0.0(11/10/2011~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "11/10/2011 04:54:56 PM"
 *                 
 ********************************************************************************/
#include "include/plat_driver.h"
#include "include/dev_lcd.h"

#define DRV_AUTHOR                "WENJING <WENJING0101@gmail.com>"
#define DRV_DESC                  "AT91SAM9XXX lcd driver"

#define DEV_NAME                  DEV_LCD_NAME

#ifndef DEV_MAJOR
#define DEV_MAJOR                 0 /*dynamic major by default */
#endif

static struct cdev dev_cdev;
static struct class *dev_class = NULL;

static int debug = DISABLE;
static int dev_major = DEV_MAJOR;
static int dev_minor = 0;

module_param(debug, int, S_IRUGO);
module_param(dev_major, int, S_IRUGO);
module_param(dev_minor, int, S_IRUGO);

#define dbg_print(format,args...) if(DISABLE!=debug) \
        {printk("[kernel] ");printk(format, ##args);}    

typedef unsigned char     U8;

/**********************************************************/
#define GUI_LCM_XMAX        160
#define GUI_LCM_YMAX        160
U8  gui_disp_buf[GUI_LCM_YMAX][GUI_LCM_XMAX / 8];

void __iomem *lcd_base;

void  GUI_FillSCR(U8 dat);

struct display_format 
{   
    unsigned char x;            /* Start pixel at COL */
    unsigned char y;            /* Start pixel at ROW */
    unsigned char gapX;         /* Blanks between each COL as pixel */
    unsigned char gapY;         /* Blanks between each ROW as pixel */
    unsigned char height;       /* Font Height as pixel */
    unsigned char width;        /* Font Width as pixel */
    unsigned long size;         /* Each W/H Font taks how many $SIZE inside data arrary */
    unsigned char invs;         /* inverse flags, default is 0 means nerver inverse */
    unsigned char unused;
};

struct font_format 
{ 
    unsigned char height; 
    unsigned char width;  
    unsigned char size;
    //unsigned char gap;
    unsigned char font[1];
};

inline void write_data(unsigned char dat) 
{
    *(unsigned char *)(lcd_base + 4) = dat;
}

inline void write_cmd(unsigned char cmd)
{
    *(unsigned char *)(lcd_base) = cmd;
}

/*
 * 取模方式:逐行式
 * 取模走向:顺向,高位在前
 *
 * Get data by ROWs, the first bit at the high level
 * 
 * Font 12 * 24 G
 *
 * 0000,1111,0100 -> Line 6 -> 0x0F,0x40
 * 0001,1000,1100 -> Line 7 -> 0x18,0xC0
 *
 */
void dis_normal (U8 xMirror, U8 yMirror, U8 width, U8 height, U8 *p)
{
    U8 i, j = 0, compare = 0, byte_count; 
    unsigned long font_index, each_row_byte;
    U8 data[160] = {0};
    U8 *font, write_data_count = 0, compare_index;
                
    xMirror = 37 + xMirror;

    each_row_byte = width / 8 + ((0 == width % 8) ? 0 : 1);

    for (i = 0; i < width; i++)
    {
        write_cmd(0x00 | (xMirror  & 0x0f));
        write_cmd(0x10 | ((xMirror & 0xf0) >> 4)); /* Why this four command must be together ??? */

        write_cmd(0x60 | ((yMirror + i)  & 0x0f));
        write_cmd(0x70 | (((yMirror + i) & 0xf0) >> 4));

        write_data_count = 0;
        compare_index = 7 - i % 8; //7,6,5,,,1,0,7,6,5,,,1,0
        byte_count = i / 8;

        for (j = 0; j < height;) /* wirte one row */ 
        {
            data[j] = 0x00;
            font_index = j * each_row_byte;
            compare = 0x01 << compare_index;
            font = p + font_index + byte_count;

            if ((compare == ((*font) & compare)))
            {
                if(0 == j % 2)
                {
                    data[j] = 0x80;
                }
                else
                {
                    data[j] = 0x08;
                }
            }
            
            if (1 == j % 2) // if width is odd number , then will lost one number here !!!
            {
                write_data(data[j - 1] | data[j]);
                write_data_count++;
            }
            j++;
        }
        
        if (2 == write_data_count % 3)
        {
            write_data(0x00);
        }
        else if (1 == write_data_count % 3)
        {
            write_data(0x00);
            write_data(0x00);
        }
    }
}

void dis_inverse(U8 xMirror, U8 yMirror, U8 width, U8 height, U8 *p)
{
    U8 i, compare = 0; 
    unsigned long each_row_byte;
    int j = 0, byte_count = 0;
    U8 data[160] = {0};
    U8 *font, write_data_count = 0;

    xMirror = 37 + xMirror;

    each_row_byte = width / 8 + ((0 == width % 8) ? 0 : 1);
    
    for (i = 0; i < height; i++)
    {
        write_cmd(0x00 | (xMirror  & 0x0f));
        write_cmd(0x10 | ((xMirror & 0xf0) >> 4)); /* Why this four command must be together ??? */

        write_cmd(0x60 | ((yMirror + i)  & 0x0f));
        write_cmd(0x70 | (((yMirror + i) & 0xf0) >> 4));

        write_data_count = 0;
        byte_count = 0;

        for (j = 0; j < width;) /* wirte one row */ 
        {
            data[j] = 0x00;
            compare = 0x80 >> (j % 8);
            font = p + i * each_row_byte + byte_count;

            if ((compare == ((*font) & compare)))
            {
                if(0 == j % 2)
                {
                    data[j] = 0x80;
                }
                else
                {
                    data[j] = 0x08;
                }
            }

            if (1 == j % 2)
            {
                write_data(data[j - 1] | data[j]);
                write_data_count++;
            }

            j++;
            if (0 == (j % 8))
            {
                byte_count++;
            }
        }

        if (2 == write_data_count % 3)
        {
            write_data(0x00);
        }
        else if (1 == write_data_count % 3)
        {
            write_data(0x00);
            write_data(0x00);
        }
    }
}

void dis_reverse (U8 xMirror, U8 yMirror, U8 width, U8 height, U8 *p)
{
    U8 i, compare = 0; 
    unsigned long each_row_byte;
    int j = 0, byte_count = 0;
    U8 data[160] = {0};
    U8 *font, write_data_count = 0;

    xMirror = 37 + xMirror;

    each_row_byte = width / 8 + ((0 == width % 8) ? 0 : 1);

    write_cmd(0x00 | (xMirror  & 0x0f));
    write_cmd(0x10 | ((xMirror & 0xf0) >> 4)); /* Why this four command must be together ??? */
    for (i = 0; i < height; i++)
    {
        write_cmd(0x00 | (xMirror  & 0x0f));
        write_cmd(0x10 | ((xMirror & 0xf0) >> 4)); /* Why this four command must be together ??? */

        write_cmd(0x60 | ((yMirror + i)  & 0x0f));
        write_cmd(0x70 | (((yMirror + i) & 0xf0) >> 4));

        write_data_count = 0;
        byte_count = each_row_byte - 1;

        for (j = width - 1; j >= 0; j--) /* wirte one row */ 
        {
            data[j] = 0x00;
            compare = 0x80 >> (j - 8 * (byte_count));
            font = p + i * each_row_byte + byte_count;

            if ((compare == ((*font) & compare)))
            {
                if(0 == j % 2)
                {
                    data[j] = 0x08;
                }
                else
                {
                    data[j] = 0x80;
                }
            }

            if (0 == j % 2)
            {
                write_data(data[j + 1] | data[j]);
                write_data_count++;
            }

            if (0 == (j % 8))
            {
                byte_count--;
            }
        }

        if (2 == write_data_count % 3)
        {
            write_data(0x00);
        }
        else if (1 == write_data_count % 3)
        {
            write_data(0x00);
            write_data(0x00);
        }
    }
}

static void display_string(struct display_format *fmt, U8 *font, int len)
{
    int row = 0, pos = 0, col = 0;
    int xMirror, yMirror, xMirNext, yMirNext, xTmp;

    //if(format->invs) lcd_invs = 1;else lcd_invs = 0;
    
    while (pos < len)
    {
        /* 
         * When column address(WPC) increase by one,three pixel will be displayed .
         * So, segment must be 3 integral multiples in window program
         */
        xTmp = (160 - fmt->width) - (fmt->x + (col * (fmt->gapX + fmt->width)));
        xMirror  = xTmp / 3;
        //xMirror  = xTmp / 3 + ((2 == xTmp % 3) ? 1 : 0);

        col++;
        xTmp = (fmt->x + (col * (fmt->gapX + fmt->width)));
        xMirNext = xTmp / 3;
        //xMirNext = xTmp / 3 + ((2 == xTmp % 3) ? 1 : 0);

        if (xMirNext >= 53) /* 160 / 3 = 53*/
        {
            row++;
            col = 0;
            continue;
        }

        yMirror  = fmt->y + row * (fmt->gapY + fmt->height);
        yMirNext = fmt->y + (row + 1) * (fmt->gapY + fmt->height);

        if (yMirNext >= 159)
        {
            printk("ERROR\n");
            break;
        }
        dis_reverse( xMirror, yMirror, fmt->width, fmt->height, font);
        
        #if 0
        dis_normal( xMirror, yMirror, fmt->width, fmt->height, font);        
        msleep(500);
        
        write_cmd(MAPPING_CTL | 0x04);
        dis_inverse( xMirror, yMirror, fmt->width, fmt->height, font);
        #endif

        font += fmt->size;
        pos += fmt->size;
    }
    //lcd_invs = 0;
}

void pixle(int x, int y, U8 status)
{
    //if (0 > x || 159 < x || 0 > y || 159 < y) return;
    
    U8 index_x, alive_bit;

    alive_bit = x % 8;
    index_x = x / 8;
    alive_bit = 0x80 >> alive_bit;

    if (1 == status)
        gui_disp_buf[y][index_x] |= alive_bit;
    else
        gui_disp_buf[y][index_x] &= (0xff - alive_bit);
}

#if 0

void set_row(int index, int start, int end, U8 status)
{
    int j = 0;
    //if (0 > index || 159 < index)return;

    for(j = start; j < end; j++)
    {
        pixle(index, j, status);
    }
}

void set_col(int index, int start, int end, U8 status)
{
    int j = 0;
    //if (0 > index || 159 < index)return;

    for(j = start; j < end; j++)
    {
        pixle(j, index, status);
    }
}

#else
void set_row(int index, int start, int end, U8 status)
{
    //if (0 > index || 159 < index)return;
    
    int i, j, startBit, endBit;

    startBit = start % 8;
    endBit = end % 8;
    
    start = start / 8;
    end = end / 8;
    
    for(j = start; j <= end; j++)
    {
        if ((end == start))
        {
            for(i = startBit; i <= endBit; i++)
            {
                if (1 == status)
                    gui_disp_buf[index][j] |= 0x80 >> i;
                else
                    gui_disp_buf[index][j] &= (0xff - (0x80 >> i));
            }
        }
        else if (j == start)
        {            
            if (1 == status)
                gui_disp_buf[index][j] |= (0xff >> startBit);
            else
                gui_disp_buf[index][j] &= (0xff << (8 - startBit));
        }

        else if (j == end)
        {
            if (1 == status)
                //gui_disp_buf[index][j] |= (0xff << (8 - endBit));
                gui_disp_buf[index][j] |= (0xff << (7 - endBit)); /*  */
            else
                //gui_disp_buf[index][j] &= (0xff >> (endBit));
                gui_disp_buf[index][j] &= (0xff >> (endBit + 1)); /*  */
        }
        else
        {
            if (1 == status)
                gui_disp_buf[index][j] = 0xff;
            else
                gui_disp_buf[index][j] = 0x00;
        }
    }    
}

void set_col(int index, int start, int end, U8 status)
{
    //if (0 > index || 159 < index)return;
    
    int i, index_x, alive_bit;

    alive_bit = index % 8;
    index_x = index / 8;
    alive_bit = 0x80 >> alive_bit;

    for(i = start; i < end; i++)
    {
        if (1 == status)
            gui_disp_buf[i][index_x] |= alive_bit;
        else
            gui_disp_buf[i][index_x] &= 0xff - alive_bit;
    }
}
#endif

void LCD_SetWindowProgram(U8 x1, U8 y1, U8 x2, U8 y2)
{
    write_cmd(0xf4);
    write_cmd(0x25 + x1 / 3);
    write_cmd(0xf6 );
    write_cmd(0x25+ (x2 + 2) / 3);
    write_cmd(0xf5);
    write_cmd(y1);
    write_cmd(0xf7);
    write_cmd(y2);
}

void LCD_SetAddress(U8 x, U8 y)
{
    x = 0x25 + x / 2;//计算出该点所在的列地址，注意该液晶每3个点共有一个地址
    write_cmd(0x60 | (y & 0x0f));       //写行地址低位
    write_cmd(0x70 | (y >> 4));     //写行地址高位
    write_cmd(x & 0x0f);        //写列地址低位
    write_cmd(0x10 | (x >> 4)); //写列地址高位
}

void GUI_RefreshSCR(void)
{
    U8 i, j, k;
    U8 TempData = 0;
    LCD_SetWindowProgram(0, 0, 159, 159);
    LCD_SetAddress(0, 0) ;

    for (i = 0; i < GUI_LCM_YMAX; i++)
    {
        for (j = 0; j < 20; j++)
        {
            for (k = 0; k < 8;)
            {
                TempData=0;

                if (gui_disp_buf[i][j] & (0x80 >> k))
                {
                    TempData = 0xf0;
                }
                
                if (gui_disp_buf[i][j] & (0x80 >> (k + 1)))
                {
                    TempData |= 0x0f;
                }
                
                write_data(TempData);
                k += 2;
            }
        }
        write_data(0x00);
    }
}

void  GUI_FillSCR(U8 dat)
{  
    U8 i, j;

    for(i = 0; i < GUI_LCM_YMAX; i++)
    {  
        for(j = 0; j < GUI_LCM_XMAX / 8; j++)
        {  
            gui_disp_buf[i][j] = dat;
        }
    }
    GUI_RefreshSCR();
}

void init_uc1698 (void)
{
    write_cmd(RESET_CMD);
    mdelay(1);

    write_cmd(POWER_CTL | 0x03);    /* PC[1:0] 11b -> internal Vlcd 13nF< LCD < 22nF */
    write_cmd(LINE_RATE | 0x00);    /* LC[4:3] 00b -> 8.5KIps */
    write_cmd(BIAS_RATIO | 0x03);   /* BR[1:0] must between V_lcd and V_bias */
    
    write_cmd(COM_END_CMD | 0x00);  /* set CEN[6:0] Com end */
    write_cmd(0x9F);                /* value 159 ->  duty :1/160 */

    write_cmd(VBIAS_POTENT_CMD);
    write_cmd(0x80);                /* VLCD=(CV0+Cpm*pm)*(1+(T-25)*CT%) */ 
     
    write_cmd(COLOR_PATTERN | 0x01);
    write_cmd(COLOR_MODE | 0x01);   /* when DC[2]=1 and LC[7:6]=01b means 4k-color*/

    write_cmd(COM_SCAN_FUNC | 0x02);/* CSF[2:0] PWM:FRC:LRM */
    write_cmd(DISP_ENABLE | 0x05);  /* DC[4:2]=101b display on, select on/off mode, Green Enhance mode disable */ 

#ifdef LINE_INVERSION
    write_cmd(LINE_INVERSE_CMD);
    write_cmd(0x18);                /* NIV[4:0] 11 000 -> Enable NIV / XOR / 11 Lines */ 
#endif

#ifdef PARTIAL_DISPALY   
    write_cmd(PARTIAL_START_CMD);
    write_cmd(0x00); 
    write_cmd(PARTIAL_END_CMD);
    write_cmd(0x5f);                /* 0x9f -> 159 */
    write_cmd(PARTIAL_DISP | 0x01);
#endif

#ifdef SCROLL_FIXED_LINE
    write_cmd(SCR_LINE_LSB | 0x00); /* SL[3:0] */
    write_cmd(SCR_LINE_MSB | 0x00); /* SL[7:4]  */
    write_cmd(FIXED_LINE_CMD);      /* FLT, FLB */
    write_cmd(0x00);
#endif
}

void Init_Ram_Address(void)
{
    write_cmd(ROW_ADDR_MSB | 0x00); /* RA[7:4] */
    write_cmd(ROW_ADDR_LSB | 0x00); /* RA[3:0] */

    write_cmd(COL_ADDR_MSB | 0x02); /* CA[6:4] */
    write_cmd(COL_ADDR_LSB | 0x05); /* CA[3:0] 0100101B --> 37D*/
#if 0
    write_cmd(WIN_COL_START_CMD);
    write_cmd(0x25);                /* 37D ---> SEG 112*/

    write_cmd(WIN_COL_END_CMD);
    write_cmd(0x5a);                /* [0x5A-0x2A] --> SEG 271 -->ensure 54[RGB]X3 per line --> 162Bit */

    write_cmd(WIN_ROW_START_CMD);
    write_cmd(0x00);

    write_cmd(WIN_ROW_END_CMD);
    write_cmd(0x9f);                /* 0x9F->159 */

    write_cmd(WIN_MODE | 0x00);
#endif
}

void simple_test (void)
{
#if 1
    struct display_format *format;
    unsigned long len;

    //write_cmd(MAPPING_CTL | 0x00);
    //Init_Ram_Address();

    write_cmd(WIN_MODE | 0x00);
    
    format = kmalloc(sizeof(struct display_format), GFP_KERNEL);
    
    format->x= 0;
    format->y= 0;
    format->gapX= 0;
    format->gapY = 0;
    format->width = 16;
    format->height = 32;
    format->size = 32*2;

    len = format->size * 10;
    display_string(format, width_16_BL, len);

#else

    int i, j = 0;

    //Init_Ram_Address(); 

    for(j = 0; j < 1; j++)
    {
        for(i = 0; i < 27; i++)
        {
            write_data(0xff);
            write_data(0xff);
            write_data(0xff);       
        }
        msleep(1500);
    }
#endif

    //GUI_FillSCR(0x00);
}
static int initialize (void)
{
    at91_set_B_periph(AT91_PIN_PC12, 0); /* Enable CS7 */
    lcd_base = ioremap(0x80000000, 0x0f);
    
    at91_sys_write(AT91_SMC_SETUP(7),
                  AT91_SMC_NWESETUP_(1)
                | AT91_SMC_NCS_WRSETUP_(1)              //tcssa=5ns
                | AT91_SMC_NRDSETUP_(1)
                | AT91_SMC_NCS_RDSETUP_(1)
    );
    at91_sys_write(AT91_SMC_PULSE(7),
                      AT91_SMC_NWEPULSE_(7)
                    | AT91_SMC_NCS_WRPULSE_(11)         //tcy=100ns
                    | AT91_SMC_NRDPULSE_(7)
                    | AT91_SMC_NCS_RDPULSE_(11)
        );
    at91_sys_write(AT91_SMC_CYCLE(7),
                      AT91_SMC_NWECYCLE_(12)
                    | AT91_SMC_NRDCYCLE_(12)
        );
    at91_sys_write(AT91_SMC_MODE(7),
                      AT91_SMC_READMODE
                    | AT91_SMC_EXNWMODE_DISABLE
                    | AT91_SMC_DBW_8
                    | AT91_SMC_WRITEMODE
                    | AT91_SMC_BAT_WRITE
                    | AT91_SMC_TDF_(4)
        );

    init_uc1698();
    GUI_FillSCR(0x00);
    simple_test();
    return 0;
}

static int lcd_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int lcd_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t lcd_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct display_format *format;
    char *kbuf;
    unsigned char *font;
    int font_len;

    if ((count < sizeof(*format)) || (count > 0x800)) 
        return -EFAULT;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf) {
        printk("kmalloc is null\n");
        return -ENOMEM;
    }

    if(copy_from_user(kbuf, buf, count)) 
    {
        kfree(kbuf);   
        return -EFAULT;
    }

    format = (struct display_format *)kbuf; 

    if(count == sizeof(*format)) {
        kfree(kbuf);
        return -EFAULT;
    }

    font = kbuf; 
    font += sizeof(*format);
    font_len = count - sizeof(*format);
    
    display_string(format, font, font_len);  

    kfree(kbuf);
    return count;  
}

//* compatible with kernel version >=2.6.38 */
static long lcd_ioctl(struct file *inode, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static struct file_operations lcd_fops = {
    .owner = THIS_MODULE,
    .open = lcd_open,
    .write = lcd_write,
    .release = lcd_release,
    .unlocked_ioctl = lcd_ioctl,
};

static int __init lcd_init (void)
{
    int result;
    dev_t devno;

    /* Alloc for the device for driver */
    if (0 != dev_major)
    {
        devno = MKDEV(dev_major, dev_minor);
        result = register_chrdev_region(devno, 1, DEV_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&devno, dev_minor, 1, DEV_NAME);
        dev_major = MAJOR(devno);
    }

    /*Alloc for device major failure */
    if (result < 0)
    {
        printk("%s driver can't get major %d\n", DEV_NAME, dev_major);
        return result;
    }
    
    /*Initialize cdev structure and register it*/
    cdev_init (&dev_cdev, &lcd_fops);
    dev_cdev.owner  = THIS_MODULE;
    
    result = cdev_add (&dev_cdev, devno , 1);
    if (result)
    {
        printk (KERN_NOTICE "error %d add lcd device", result);
        goto ERROR;
    }

    dev_class = class_create(THIS_MODULE, DEV_NAME);
    if (IS_ERR(dev_class)) 
    {           
        printk("%s driver create class failture\n",DEV_NAME);           
        result =  -ENOMEM;  
        goto ERROR;   
    }       

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)     
    device_create(dev_class, NULL, devno, NULL, DEV_NAME);
#else
    device_create (dev_class, NULL, devno, DEV_NAME);
#endif

    printk("AT91 %s driver version %d.%d.%s <%s> initiliazed.\n", 
        DEV_NAME, DRV_MAJOR_VER, DRV_MINOR_VER, DRV_REVER_VER, __DATE__);

    initialize();
    return 0;

ERROR:
    cdev_del(&dev_cdev);
    unregister_chrdev_region(devno, 1);
    return result;
}

static void __exit lcd_exit(void)
{
    dev_t devno = MKDEV(dev_major, dev_minor);

    device_destroy(dev_class, devno);   
    class_destroy(dev_class);

    cdev_del(&dev_cdev);

    unregister_chrdev_region(devno, 1);
    printk("%s driver removed\n", DEV_NAME);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);

