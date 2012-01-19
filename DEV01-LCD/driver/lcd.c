//#include <common.h>
#include <asm/io.h>
#include <asm/arch/at91sam9_smc.h>
#include <asm/arch/at91_common.h>
#include <asm/arch/gpio.h>

typedef unsigned char     U8;

#define GUI_LCM_XMAX		160
#define GUI_LCM_YMAX		160
U8  gui_disp_buf[GUI_LCM_YMAX][GUI_LCM_XMAX / 8];

void __iomem *lcd_base;

inline void write_data(unsigned char dat) 
{
	*(unsigned char *)(lcd_base + 4) = dat;
}

inline void write_cmd(unsigned char cmd)
{
	*(unsigned char *)(lcd_base) = cmd;
}

/*
 * Get data by ROWs, the first bit at the high level
 * 
 * Font 12 * 24 G
 *
 * 0000,1111,0100 -> Line 6 -> 0x0F,0x40
 * 0001,1000,1100 -> Line 7 -> 0x18,0xC0
 *
 */
void dis_reverse (U8 xMirror, U8 yMirror, U8 width, U8 height, U8 *p)
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
    
    write_cmd(MAPPING_CTL | 0x00);  /* LC[2:0] MY inversion/MX inversion/Fix line display */
    
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
    simple_test();
    return 0;
}
