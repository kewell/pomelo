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
void set_addr(U8 x, U8 y);
inline void get_data(int lenght, U8 *data);
//void set_area(U8 startX, U8 startY, U8 endX, U8 endY, U8 status);

/**********************************************************/
#define GUI_LCM_XMAX        160
#define GUI_LCM_YMAX        160

void __iomem *lcd_base;

struct dis_fmt 
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

U8 read_data(void) 
{
    udelay(1);
    return *(unsigned char *)(lcd_base + 4);
}

inline void get_data(int length, U8 *data) 
{
    U8 temp0, temp1;//, temp2, temp3;
    int i;
    
    /*
    read 2 times will get 3 pixel data
    R4 R3 R2 R1	R0 G5 G4 G3	| G2 G1	G0 B4 B3 B2 B1 B0
        |--------|        |----------|     |--------|

    D[7:0] 
    write 3 times will display 6 pixel
    R3 R2 R1 R0	G3 G2 G1 G0
    B3 B2 B1 B0	R3 R2 R1 R0
    G3 G2 G1 G0	B3 B2 B1 B0
    */
    
    read_data();

    for (i = 0; i < length;)
    {
        temp0 = read_data();
        temp1 = read_data();
        //temp2 = read_data();
        //temp3 = read_data();

        //data[i] = ((temp0 & 0x78) << 1) + ((temp0 & 0x01) << 3) + (temp1 >> 5);
        //data[i] = (temp1 << 4) + ((temp2 & 0x78) >> 3);
        //data[i] = ((temp2 & 0x01) << 7) + ((temp3 & 0xe0) >> 1) + (temp3 & 0x0f);

        data[i] = ((temp0 & 0x78) >> 3);
        i++;
        data[i] = ((temp0 & 0x01) << 3) + (temp1 >> 5);
        i++;
        data[i] = (temp1 & 0xf);
        i++;
    }
}

void set_addr(U8 x, U8 y)
{
    x = 0x25 + x / 3;
    write_cmd(COL_ADDR_LSB | (x & 0x0f));
    write_cmd(COL_ADDR_MSB | (x >> 4));
    write_cmd(ROW_ADDR_LSB | (y & 0x0f));
    write_cmd(ROW_ADDR_MSB | (y >> 4));
}

/*
 * Get data by ROWs, the first bit at the high level
 *
 * Font 12 * 24 G
 * 0000,1111,0100 -> Line 6 -> 0x0F,0x40
 * 0001,1000,1100 -> Line 7 -> 0x18,0xC0
 */
void dis_normal (U8 xMirror, U8 yMirror, U8 width, U8 height, U8 *p)
{
    U8 i, j = 0, compare = 0, byte_count; 
    unsigned long font_index, each_row_byte;
    U8 data[160] = {0};
    U8 *font, write_data_count = 0, compare_index;
                
    each_row_byte = width / 8 + ((0 == width % 8) ? 0 : 1);

    for (i = 0; i < width; i++)
    {
        set_addr(xMirror, yMirror + i);

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
                    write_data_count = 0;
                    data[j] = 0xf0;
                }
                else
                {
                    data[j] = 0x0f;
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

    each_row_byte = width / 8 + ((0 == width % 8) ? 0 : 1);
    
    for (i = 0; i < height; i++)
    {
        set_addr(xMirror, yMirror + i);

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
                    data[j] = 0xf0;
                }
                else
                {
                    data[j] = 0x0f;
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

    each_row_byte = width / 8 + ((0 == width % 8) ? 0 : 1);
    
    for (i = 0; i < height; i++)
    {
        set_addr(xMirror, yMirror + i);

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
                    data[j] = 0x0f;
                }
                else
                {
                    data[j] = 0xf0;
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

static void display_string(struct dis_fmt *fmt, U8 *font, int len)
{
    int row = 0, pos = 0, col = 0;
    int xMirror, yMirror;

    //if(fmt->invs) lcd_invs = 1;else lcd_invs = 0;
    
    while (pos < len)
    {
        /* 
         * When column address(WPC) increase by one, three pixel will be displayed.
         * So, segment must be 3 integral multiples in window program
         */
#if 1
        xMirror = fmt->x + col * (fmt->gapX + fmt->width);
        col++;
        
        if ((xMirror + fmt->width) > 160)
        {
            row++;
            col = 0;
            continue;
        }

        yMirror  = fmt->y + row * (fmt->gapY + fmt->height);

        if ((yMirror + fmt->height) > 160)
        {
            break;
        }

        dis_reverse(xMirror, yMirror, fmt->width, fmt->height, font);        
        //dis_inverse(xMirror, yMirror, fmt->width, fmt->height, font);write_cmd(MAPPING_CTL | 0x04);
#else
        yMirror  = fmt->y + row * (fmt->gapX + fmt->width);
        row++;

        if ((yMirror + fmt->width) > 160)
        {
            col++;
            row = 0;
            continue;
        }
        
        xMirror = (fmt->x + (col * (fmt->gapY + fmt->height)));
        if ((xMirror + fmt->height) > 160)
        {
            break;
        }
        
        dis_normal(xMirror, yMirror, fmt->width, fmt->height, font);
#endif

        font += fmt->size;
        pos += fmt->size;
    }
    //lcd_invs = 0;
}

void set_pixel(U8 x, U8 y, U8 status)
{
    U8 data[3];
    
    set_addr(x, y);
    get_data(3, data);

    status = status & 0x0f;

    if (0 == x % 3)
        data[0] = status;
    else if (1 == x % 3)
        data[1] = status;
    else
        data[2] = status;

    set_addr(x, y);
    write_data(data[0] << 4 | data[1]);
    write_data(data[2] << 4);
}

void set_col(U8 index, U8 status)
{
    int i = 0;

    for (i = 0; i < 160; i++)
    {
        set_pixel(index, i, status);
    }
}

void set_row(U8 index, U8 status)
{
    int i = 0;
    set_addr(0, index);
    for (i = 0; i < 27; i++)
    {
        write_data(status);
        write_data(status);
        write_data(status);
    }
}

void set_area(U8 startX, U8 startY, U8 endX, U8 endY, U8 status)
{
    U8 i, j;
    U8 count;
    count = (endX - startX) / 2 + (endX - startX) % 2;
    
    for (i = startY; i <= endY; i++)
    {
        set_addr(startX, i);

        for (j = 0; j < count; j++)
        {
            write_data(status);
        }

        if (1 == (j % 3))
        {
            write_data(status);
            write_data(status);
        }
        else if (2 == (j % 3))
        {
            write_data(status);
        }
        
    }
}

void inverse_area(U8 startX, U8 startY, U8 endX, U8 endY)
{
    int i, j, length;
    U8 *data;

    length = endX - startX + 1;

    if (1 == length % 3)
        length += 2;
    else if (2 == length % 3)
        length += 1;

    data = kmalloc(length, GFP_KERNEL); /* Should be global variable cuz 160*160=25K size */

    for (j = startY; j <= endY; j++)
    {
        set_addr(startX, j);
        get_data(length, data);

        set_addr(startX, j);

        for (i = 0; i < length; i++)
        {
            if (1 == i % 2)
            {
                write_data((~(data[i-1] << 4) & 0xf0) | (~data[i] & 0x0f));
            }
        }
    }
    kfree(data);
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

    write_cmd(WIN_COL_START_CMD);
    write_cmd(0x25);                /* 37D ---> SEG 112*/

    write_cmd(WIN_COL_END_CMD);
    write_cmd(0x5a);                /* [0x5A-0x2A] --> SEG 271 -->ensure 54[RGB]X3 per line --> 162Bit */

    write_cmd(WIN_ROW_START_CMD);
    write_cmd(0x00);

    write_cmd(WIN_ROW_END_CMD);
    write_cmd(0x9f);                /* 0x9F->159 */

    write_cmd(WIN_MODE | 0x00);
}

void simple_test (void)
{
    int i = 0, j = 0;

    #if 0

    struct dis_fmt *fmt;
    unsigned long len;  
    
    fmt = kmalloc(sizeof(struct dis_fmt), GFP_KERNEL);
    
    fmt->x= 0;
    fmt->y= 0;
    fmt->gapX= 0;
    fmt->gapY = 0;
    fmt->width = 13;
    fmt->height = 26;
    fmt->size = fmt->height * (fmt->width / 8 + ((fmt->width % 8) ? 1 : 0));
    len = fmt->size * 25;

    display_string(fmt, width_13_EN, len);
    #else
    
    set_area(0, 0, 159, 159, 0xff);mdelay(1000);

    set_row(33, 0);set_row(55, 0);
    set_col(33, 0);set_col(55, 0);

    set_area(30, 30, 130, 130, 0);
    set_area(60, 60, 90, 90, 0xff);

    mdelay(1000);

    inverse_area(20, 20, 110, 140);

    #endif
    printk("-----------------------------\n\n");
}

static int initialize (void)
{
    at91_set_B_periph(AT91_PIN_PC12, 0);    /* Enable CS7 */
    lcd_base = ioremap(0x80000000, 0x0f);
    
    at91_sys_write(AT91_SMC_SETUP(7),
        AT91_SMC_NWESETUP_(1)
        | AT91_SMC_NCS_WRSETUP_(1)      //tcssa=5ns
        | AT91_SMC_NRDSETUP_(1)
        | AT91_SMC_NCS_RDSETUP_(1)
    );
    at91_sys_write(AT91_SMC_PULSE(7),
        AT91_SMC_NWEPULSE_(7)
        | AT91_SMC_NCS_WRPULSE_(11)     //tcy=100ns
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
    Init_Ram_Address();    
    
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
    struct dis_fmt *format;
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

    format = (struct dis_fmt *)kbuf; 

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

