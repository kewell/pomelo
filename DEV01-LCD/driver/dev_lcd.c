
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
#include "include/version.h"
#include "include/dev_lcd.h"

#undef  CONFIG_PROC_FS
#define DRV_AUTHOR                "kewell <wenjing0101@gmail.com>"
#define DRV_DESC                  "AT91SAM9XXX lcd driver"

/*Driver version*/
#define DRV_MAJOR_VER             1
#define DRV_MINOR_VER             0
#define DRV_REVER_VER             0

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
/* 定义颜色数据类型(可以是数据结构) */
#define GUI_LCM_XMAX		160	/* 定义液晶x轴的点数 */
#define GUI_LCM_YMAX		160	/* 定义液晶y轴的点数 */
U8  gui_disp_buf[GUI_LCM_YMAX][GUI_LCM_XMAX / 8];	// 声明GUI显示缓冲区

void __iomem *lcd_base;

struct display_format 
{   
    unsigned char x;   
    unsigned char y;   

    unsigned char gapX;
    unsigned char gapY;

    unsigned char height; 
    unsigned char width;

    unsigned char size;

    unsigned char invs;
    

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
void display_word3 (U8 x, U8 y, U8 hight, U8 width,  U8 *p)
{
    U8 j, index = 0, compare = 0, byte_count; 
    unsigned long font_index;
    U8 data[160] = {0};
    U8 *font, write_count = 0;
                
	x = 37 + x;

    for (j = 0; j < hight; j++)
	{
	    write_count = 0;
        byte_count = 0;

        write_cmd(0x00 | (x  & 0x0f));
        write_cmd(0x10 | ((x & 0xf0) >> 4)); /* Why this four command must be together ??? */

        write_cmd(0x60 | ((y + j)  & 0x0f));
		write_cmd(0x70 | (((y + j) & 0xf0) >> 4));

        if (0 == (width % 8))
        {
            font_index = j * (width / 8); // if width == 7 DIE!!!
        }
        else
        {
            font_index = j * (width / 8 + 1); 
        }

        for (index = 0; index < width;) /* wirte one row */ 
        {
            data[index] = 0x00;

            compare = 0x01 << (index - 8 * byte_count);
            font = p + font_index + byte_count;

            if ((compare == ((*font) & compare)))
            {
                if(0 == index % 2)
                {
                    data[index] = 0x80;
                }
                else
                {
                    data[index] = 0x08;
                }
            }

            if (1 == index % 2) // if width is odd number , then will lost one number here !!!
            {
                write_data(data[index - 1] | data[index]);
                write_count++;
            }

            index++;
            if (0 == (index % 8))
            {
                byte_count++;
            }
        }
        
        if (2 == write_count % 3)
        {
            write_data(0x00);
        }
        else if (1 == write_count % 3)
        {
            write_data(0x00);
            write_data(0x00);
        }
    }
}


void display_word2 (U8 x, U8 y, U8 width, U8 hight, U8 *p)
{
    U8 j, index = 0, compare = 0, byte_count; 
    unsigned long font_index;
    U8 data[160] = {0};
    U8 *font, write_count = 0;
                
	x = 37 + x;

    for (j = 0; j < hight; j++)
	{
	    write_count = 0;
        byte_count = 0;

        write_cmd(0x00 | (x  & 0x0f));
        write_cmd(0x10 | ((x & 0xf0) >> 4)); /* Why this four command must be together ??? */

        write_cmd(0x60 | ((y + j)  & 0x0f));
		write_cmd(0x70 | (((y + j) & 0xf0) >> 4));

        if (0 == (width % 8))
        {
            font_index = j * (width / 8); // if width == 7 DIE!!!
        }
        else
        {
            font_index = j * (width / 8 + 1); 
        }

        for (index = 0; index < width;) /* wirte one row */ 
        {
            data[index] = 0x00;

            compare = 0x01 << (index - 8 * byte_count);
            font = p + font_index + byte_count;

            if ((compare == ((*font) & compare)))
            {
                if(0 == index % 2)
                {
                    data[index] = 0x80;
                }
                else
                {
                    data[index] = 0x08;
                }
            }

            if (1 == index % 2) // if width is odd number , then will lost one number here !!!
            {
                write_data(data[index - 1] | data[index]);
                write_count++;
            }

            index++;
            if (0 == (index % 8))
            {
                byte_count++;
            }
        }
        
        if (2 == write_count % 3)
        {
            write_data(0x00);
        }
        else if (1 == write_count % 3)
        {
            write_data(0x00);
            write_data(0x00);
        }
    }
}


void display_word (U8 x, U8 y, U8 width, U8 hight, U8 *p)
{
    U8 j, compare = 0; 
    unsigned long font_count;
    int index = 0, byte_count = 0;
    U8 data[160] = {0};
    U8 *font, write_count = 0;

    U8 width_rest = width % 8;

    x = 37 + x;

    if (0 == width_rest)
    {
        font_count = width / 8;
    }
    else
    {
        font_count = width / 8 + 1;
    }
    
    for (j = 0; j < hight; j++)
	{
	    write_count = 0;
        byte_count = font_count - 1;

        write_cmd(0x00 | (x  & 0x0f));
        write_cmd(0x10 | ((x & 0xf0) >> 4)); /* Why this four command must be together ??? */

        write_cmd(0x60 | ((y + j)  & 0x0f));
		write_cmd(0x70 | (((y + j) & 0xf0) >> 4));

        for (index = width - 1; index >= 0; index--) /* wirte one row */ 
        {
            data[index] = 0x00;

            compare = 0x80 >> (index - 8 * (byte_count));
            font = p + j * font_count + byte_count;

            if ((compare == ((*font) & compare)))
            {
                if(0 == index % 2)
                {
                    data[index] = 0x08;
                }
                else
                {
                    data[index] = 0x80;
                }
            }

            if (0 == index % 2)
            {
                write_data(data[index + 1] | data[index]);
                write_count++;
            }

            if (0 == (index % 8))
            {
                byte_count--;
            }
        }

        if (2 == write_count % 3)
        {
            write_data(0x00);
        }
        else if (1 == write_count % 3)
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
         When column address(WPC) increase by one,three pixel will be displayed .
         So,segment must be 3 integral multiples in window program
        */
        xTmp = (fmt->x + (col * (fmt->gapX + fmt->width)));
        xMirror  = xTmp / 3;// + ((0 == xTmp % 3) ? 0 : 1);

        col++;
        xTmp = (fmt->x + (col * (fmt->gapX + fmt->width)));
        xMirNext = xTmp / 3;// + ((0 == xTmp % 3) ? 0 : 1);

        if (xMirNext >= 53)
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
        
        //void display_word (U8 x, U8 y, U8 width, U8 hight, U8 *p)
        display_word( xMirror, 
            yMirror,
            fmt->width, 
            fmt->height, 
            font);

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
    //printk("%d\n", 0x25+ (x2 + 2) / 3);
	write_cmd(0xf5);
	write_cmd(y1);
	write_cmd(0xf7);
	write_cmd(y2);
}

void LCD_SetAddress(U8 x, U8 y)
{
    x = 0x25 + x / 2;//计算出该点所在的列地址，注意该液晶每3个点共有一个地址
    write_cmd(0x60 | (y & 0x0f));		//写行地址低位
    write_cmd(0x70 | (y >> 4));		//写行地址高位
    write_cmd(x & 0x0f);		//写列地址低位
    write_cmd(0x10 | (x >> 4));	//写列地址高位
}

//=============================================================================
//函 数 名: GUI_RefreshSCR()
//功 能：	刷新全屏，将显示缓冲区全部数据送到模组显示
//入口参数： 无
//出口参数： 无
//返 回 值： 无
//=============================================================================
void GUI_RefreshSCR(void)
{
    U8 i, j, k;
    U8 TempData = 0;
    LCD_SetWindowProgram(0, 0, 159, 159);
    LCD_SetAddress(0, 0) ;

    for (i = 0; i < GUI_LCM_YMAX; i++) // 历遍所有列
    {
        for (j = 0; j < 20; j++) // 历遍所有行
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
        write_data(0x00); //补全每行末尾的数据，使总点数能被三整除
    }
}

//=============================================================================
//函 数 名: GUI_FillSCR()
//功 能：	全屏填充。直接使用数据填充显示缓冲区。
//入口参数： dat 填充的数据(对于黑白色LCM，为0的点灭，为1的点显示)
//出口参数： 无
//返 回 值： 无
//=============================================================================
void  GUI_FillSCR(U8 dat)
{  
    U8 i, j;

    for(i = 0; i < GUI_LCM_YMAX; i++)		// 历遍所有行
    {  
        for(j = 0; j < GUI_LCM_XMAX / 8; j++)	// 历遍所有行
        {  
            gui_disp_buf[i][j] = dat;		// 填充数据
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
    
    write_cmd(MAPPING_CTL | 0x04);  /* LC[2:0] MY inversion/MX inversion/Fix line display */
    
    write_cmd(RAM_ADDR_CTL | 0x01); /* AC[2:0] AC[0] -> RA or CA will increment by one step */
	write_cmd(COLOR_PATTERN | 0x01);
	write_cmd(COLOR_MODE | 0x01);   /* when DC[2]=1 and LC[7:6]=01b means 4k-color*/

    write_cmd(COM_SCAN_FUNC | 0x02);/* CSF[2:0] PWM:FRC:LRM */
	write_cmd(DISP_ENABLE | 0x05);  /* DC[4:2]=101b display on, select on/off mode, Green Enhance mode disable */

    //write_cmd(INVERSE_DISP | 0x01);

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
	write_cmd(SCR_LINE_MSB | 0x00);	/* SL[7:4]  */
	write_cmd(FIXED_LINE_CMD);		/* FLT, FLB */
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

static int initialize (void)
{
    int i;

    at91_set_B_periph(AT91_PIN_PC12, 0); /* Enable CS7 */
    lcd_base = ioremap(0x80000000, 0x0f);
    
	at91_sys_write(AT91_SMC_SETUP(7),
				  AT91_SMC_NWESETUP_(1)
				| AT91_SMC_NCS_WRSETUP_(1)				//tcssa=5ns
				| AT91_SMC_NRDSETUP_(1)
				| AT91_SMC_NCS_RDSETUP_(1)
	);
	at91_sys_write(AT91_SMC_PULSE(7),
					  AT91_SMC_NWEPULSE_(7)
					| AT91_SMC_NCS_WRPULSE_(11)			//tcy=100ns
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
#if 0
    write_cmd(MAPPING_CTL | 0x04);

    //GUI_FillSCR(0x00);
    //Init_Ram_Address();

    //write_cmd(WIN_MODE | 0x00);

    for(i = 0; i < 0; i++)
    {
        //display_word(i*16, 16*i, 8, 16, width_8_ENG);
        display_word(i * 7, 0, 20, 40, width_20_time + i * 120);
        display_word(i * 5, 40, 16, 16, width_16_chinese + i * 32);
    }

    return 0;

    struct display_format *format = kmalloc(sizeof(struct display_format), GFP_KERNEL);
    unsigned long len;

    format->x= 0;
    format->y= 0;
    format->gapX= 0;
    format->gapY = 0;
    format->width = 8;
    format->height = 16;
    format->size = 16;

    len = format->size * 26;
    
    display_string(format, width_8_ENG + 0 * format->size, len);
    format->y= format->height * 2;
    display_string(format, width_8_ENG + 0 * format->size, len);
    format->y= format->height * 4;
    display_string(format, width_8_ENG + 0 * format->size, len);

    format->width = 16;
    format->height = 16;
    format->y= format->height * 5;
    format->size = 32;

    len = format->size * 12;
    display_string(format, width_16_chinese, len);
    

    write_cmd(ROW_ADDR_MSB | (i & 0xf0)); /* RA[7:4] */
    write_cmd(ROW_ADDR_LSB | (i & 0x0f)); /* RA[3:0] */

    write_cmd(COL_ADDR_MSB | 0x02); /* CA[6:4] */
    write_cmd(COL_ADDR_LSB | 0x05); /* CA[3:0] 0100101B --> 37D*/
        
#endif    
#if 1

    GUI_FillSCR(0x00);
    Init_Ram_Address();

    write_cmd(MAPPING_CTL | 0x00);
    write_cmd(RAM_ADDR_CTL | 0x01); /* AC[2:0] AC[0] -> RA or CA will increment by one step */

    for(i = 0; i > 16 * 1; i++)
    {
        write_data(0xff);
        write_data(0xff);
        write_data(0xff);
        write_data(0xff);
        write_data(0x0);
        write_data(0xff);
        write_data(0xff);
        write_data(0xff);
        write_data(0xff);
        msleep(100);
        write_data(0xff);
        write_data(0xff);
        write_data(0xff);
        write_data(0xff);
        write_data(0x0);
        write_data(0xff);
        write_data(0xff);
        write_data(0xff);
        write_data(0xff);
        msleep(100);        
    }

    for(i = 0; i < 4; i++)
    {
        //display_word(0, 0, 12, 24, width_12_ENG_R2L + 5 * 48);
        display_word(i*4, 0, 12, 24, width_12_ENG_R2L + (12 - i )* 48);
        display_word(i*8, 24, 24, 24, width_24_sft_R2L + i * 72);
        display_word3(i*10, 48, 16, 32, width_16_ENG_COL + i * 64);
        //display_word(i, 40 * i, 20, 40, width_20_time + i * 120);
        //display_word(i * 5, 40, 16, 16, width_16_chinese + i * 32);
    }
        
#endif

    //I_FillSCR(0x00);
    
    return 0;
}

#ifdef CONFIG_PROC_FS

#define PROC_IOCTL       "ioctl"

static int ioctl_proc_read(char *page, char **start, off_t offset, int count, int *eof, void *data)
{
    char *p = page;
    if (0 != offset)
    {
        *eof = 1;
        return 0;
    }

    p += sprintf(p, "Current driver support ioctl command:\n");
    {
        p += sprintf(p, "========================== Commone ioctl =============================\n");
        p += sprintf(p, "Enable/Disable Debug    : %u\n", SET_DRV_DEBUG);
        p += sprintf(p, "Get driver version      : %u\n", GET_DRV_VER);
        p += sprintf(p, "\n");

        p += sprintf(p, "=========================== lcd driver ==============================\n");
        p += sprintf(p, "\n");
    }

    return (p - page);
}

#endif

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


//static int lcd_ioctl(struct inode *inode, struct file *file, u_int cmd, u_long arg)
static int lcd_ioctl(struct file *inode, unsigned int cmd, unsigned long arg)
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
	dev_cdev.owner 	= THIS_MODULE;
	
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

#ifdef CONFIG_PROC_FS    
    create_proc_read_entry(PROC_IOCTL, 0444, NULL, ioctl_proc_read, NULL);
    //printk("Initializ proc file \"/proc/%s\"\n", PROC_IOCTL);
#endif

    printk("%s driver version %d.%d.%d initiliazed [%s]\n", DEV_NAME, DRV_MAJOR_VER, DRV_MINOR_VER, DRV_REVER_VER, MODIFY_TIME);

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

#ifdef CONFIG_PROC_FS    
    remove_proc_entry(PROC_IOCTL, NULL);
#endif

    device_destroy(dev_class, devno);   
    class_destroy(dev_class);

    cdev_del(&dev_cdev);

    unregister_chrdev_region(devno, 1);
    //printk("%s driver removed\n", DEV_NAME);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);

