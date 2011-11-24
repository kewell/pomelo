/*********************************************************************************
 *      Copyright:  (C) 2011 R&D of San Fran Electronics Co., LTD  
 *                  All rights reserved.
 *
 *       Filename:  lcd.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/09/2011~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "11/09/2011 10:17:29 AM"
 *                 
 ********************************************************************************/
//#define PARTIAL_DISPALY
//#define LINE_INVERSION
//#define SCROLL_FIXED_LINE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <memory.h>

#include "version.h"
#include "at91sam9_smc.h"
#include "pio_base.h"
#include "font_12x24.h"

#define COL_ADDR_LSB        0x00    /* CA[3:0] */
#define COL_ADDR_MSB        0x10    /* CA[6:4] */
#define TEMP_COMPENS        0x24    /* TC[1:0] */
#define POWER_CTL           0x28    /*   */
#define ADV_POW_CTL_CMD     0x30    /* ---USELESS---  */
#define SCR_LINE_LSB        0x40    /* SL[3:0] */
#define SCR_LINE_MSB        0x50    /* SL[7:4] */
#define ROW_ADDR_LSB        0x60    /* RA[3:0] */
#define ROW_ADDR_MSB        0x70    /* RA[7:4] */
#define VBIAS_POTENT_CMD    0x81    /* PM[7:0] */
#define PARTIAL_DISP        0x84    /* LC[8] */
#define RAM_ADDR_CTL        0x88    /* AC[2:0] */
#define FIXED_LINE_CMD      0x90    /* FLT, FLB */
#define LINE_RATE           0xA0    /* LC[4:3] */
#define ALL_PIXEL_ON        0xA4    /* DC[1] */
#define INVERSE_DISP        0xA6    /* DC[0] */
#define DISP_ENABLE         0xA8    /* DC[4:2] */
#define MAPPING_CTL         0xC0    /* LC[2:0] */
#define LINE_INVERSE_CMD    0xC8    /* NIV[4:0] */
#define COLOR_PATTERN       0xD0    /* LC[5] */
#define COLOR_MODE          0xD4    /* LC[7:6] */
#define COM_SCAN_FUNC       0xD8    /* CSF[2:0] */
#define RESET_CMD           0xE2    /*  */
#define BIAS_RATIO          0xE8    /* BR[1:0] */
#define COM_END_CMD         0xF1    /* CEN[6:0] */
#define PARTIAL_START_CMD   0xF2    /* DST[6:0] */
#define PARTIAL_END_CMD     0xF3    /* DEN[6:0] */
#define WIN_COL_START_CMD   0xF4    /* WPC0 */
#define WIN_ROW_START_CMD   0xF5    /* WPP0 */
#define WIN_COL_END_CMD     0xF6    /* WPC1 */
#define WIN_ROW_END_CMD     0xF7    /* WPP1 */
#define WIN_MODE            0xF8    /* AC[3] */
#define MTP_OPER_CTL_CMD    0xB8    /* MTPC[4:0]*/
//#define /*  */
void UC1698_init(void)
{
	write_cmd(RESET_CMD);
    usleep(200);

	write_cmd(POWER_CTL | 0x03);    /* PC[1:0] 11b -> internal Vlcd 13nF< LCD < 22nF */
    write_cmd(LINE_RATE | 0x00);    /* LC[4:3] 00b -> 8.5KIps */
    write_cmd(BIAS_RATIO | 0x03);   /* BR[1:0] must between V_lcd and V_bias */
	
    write_cmd(COM_END_CMD | 0x00);  /* set CEN[6:0] Com end */
    write_cmd(0x9F);                /* value 159 ->  duty :1/160 */

    write_cmd(VBIAS_POTENT_CMD);
    write_cmd(0x80);                /* VLCD=(CV0+Cpm*pm)*(1+(T-25)*CT%) */
    
    write_cmd(MAPPING_CTL | 0x04);  /* LC[2:0] MY inversion/MX inversion/Fix line display */
    //write_cmd(MAPPING_CTL | 0x00);
	
    write_cmd(RAM_ADDR_CTL | 0x01); /* AC[2:0] AC[0] -> RA or CA will increment by one step */
	write_cmd(COLOR_PATTERN | 0x01);
	write_cmd(COLOR_MODE | 0x01);   /* when DC[2]=1 and LC[7:6]=01b means 4k-color*/

    write_cmd(COM_SCAN_FUNC | 0x02);/* CSF[2:0] PWM:FRC:LRM */
	write_cmd(DISP_ENABLE | 0x05);  /* DC[4:2]=101b display on, select on/off mode, Green Enhance mode disable */

#ifdef LINE_INVERSION
    write_cmd(LINE_INVERSE_CMD);
	write_cmd(0x18);                /* NIV[4:0] 11000 -> Enable NIV / XOR / 11 Lines */ 
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
    write_cmd(0x25);                /* 37D */

	write_cmd(WIN_COL_END_CMD);
    write_cmd(0x5a);                /* [0x5A-0x2A] --> ensure 54[RGB]X3 per line --> 162Bit */

	write_cmd(WIN_ROW_START_CMD);
	write_cmd(0x00);

	write_cmd(WIN_ROW_END_CMD);
	write_cmd(0x9f);                /* 0x9F->159 */

	write_cmd(WIN_MODE | 0x00);
}

void GUI_CopyRAMToLCD(LCD_RECT addr)
{
	INT8U i, j;

    //printf("x0=%.3d x1=%.3d y0=%.3d y1=%.3d\n", addr.x0, addr.x1, addr.y0, addr.y1);

	for(i = 0; i < GUI_LCM_YMAX; i++)
	{
		for(j = 0; j < 27 ; j++)	//保证每行写满81个字节
        {
            write_data(DispRAM[i][j]);
            write_data(DispRAM[i][j]);
            write_data(DispRAM[i][j]);
        }
	}
}

void row(U8 index, U8 status)
{
	U8 i, j;
	for(i = 0; i < GUI_LCM_YMAX; i++)
	{
		for(j = 0; j < 81; j++)
        {
            if(index == i)
                DispRAM[i][j] = status; 
        }
	}
}

void col(U8 index, U8 status)
{
	U8 i, j;
	for(i = 0; i < GUI_LCM_YMAX; i++)
	{
		for(j = 0; j < 81; j++)
        {
            if(index == j)
                DispRAM[i][j] = status;
        }
	}
}

void GUI_FillSCR(U8 dat)
{
	U8 i, j;
	for(i = 0; i < GUI_LCM_YMAX; i++)
	{
		for(j = 0; j < 81; j++)
        {
            DispRAM[i][j] = dat;
        }
	}
}

int disp_init(void)
{
    int fd = -1;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){
		return -1;
	}

	disp_base = mmap(0, 0x0f, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AT91_DIS);
	sys_base = mmap(0, 0x3fff, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AT91_SYS);

	at91_set_B_periph(PIN_PC12, 0);		/* set NCS7 enable */

	/*
	 * Static memory controller timing adjustments.
	 * REVISIT:  these timings are in terms of MCK cycles, so
	 * when MCK changes (cpufreq etc) so must these values...
	 */
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
					|  AT91_SMC_EXNWMODE_DISABLE
					| AT91_SMC_DBW_8
					| AT91_SMC_WRITEMODE
					| AT91_SMC_BAT_WRITE
					| AT91_SMC_TDF_(4)
                );
	UC1698_init();
    Init_Ram_Address();

	return 0;
}

void disp_exit (void)
{
    at91_set_GPIO_periph(PIN_PC12, 0); /* must disable periph function but useless here */
	munmap(disp_base, 0x0f);
	munmap(sys_base, 0x3fff);
}

void words_all (U8 x, U8 y, U8 width, U8 hight, U8 *p)
{
	U8 j, index = 0, compare = 0, byte_count; 
    unsigned long font_index;
    U8 data[160] = {0};
    U8 *font;
                
	x = 37 + x;

    for (j = 0; j < hight; j++) // if (j < 2)
	{
	    byte_count = 0;

        write(0, 0x00 | (x  & 0x0f));
        write(0, 0x10 | ((x & 0xf0) >> 4)); /* Why this four command must be together ??? */

        write(0, 0x60 | ((y + j)  & 0x0f));
		write(0, 0x70 | (((y + j) & 0xf0) >> 4));

        if (0 == width % 8)
        {
            font_index = j * (width / 8); 
        }
        else
        {
            font_index = j * (width / 8 + 1); 
        }

        //printf("[%03d] = %d\n", j, font_index);

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

            if (1 == index % 2)
            {
                write(1, data[index - 1] | data[index]);
            }

            index++;
            if (0 == (index % 8))
            {
                byte_count++;
            }
        }
    }
}

void words(U8 x,U8 y,U8 type,U8 *p)			//type=1,ascii;type=2,Chinese character
{
	U8 i,k,j,m,n,l,x0,dat0,dat1,dat2,dat3,dat4,dat5,dat6;
	x=37+x;
	x0=0x00|(x&0x0f);
	x=0x10|((x&0xf0)>>4);

    for(i=0;i<3;i++) //if (i == 0)
	{
	    n=i*12*type; // 0 24 48
		
		// 每次循环写8行: write data 8 * (4 * 3)times --> 8*24 = 192 pixs
		for(j=0;j<8;j++)//if (j == 0)
		{
		    m=i*8+j;

            //write(0,0x89);
			
			write(0,x0);
			write(0,x);
			write(0,0x60|((y+m)&0x0f));
			write(0,0x70|(((y+m)&0xf0)>>4));

			// 每次循环写一行: write data 4*3 times --> 24 pixs
			for(k = 0; k < 2 * type; k++) //if(k == 0)
			{         
				l = k*6 + n;

                dat6=0x01<<j; // j = 0-7
                
				dat0=(*(p+0+l))&dat6;
				dat0=dat0>>j;
				dat0<<=7;
				
				dat1=(*(p+1+l))&dat6;
				dat1=dat1>>j;
				dat1<<=3;

				dat2=*(p+2+l)&dat6;
				dat2=dat2>>j;
				dat2<<=7;

				dat3=(*(p+3+l))&dat6;
				dat3=dat3>>j;
				dat3<<=3;

				dat4=(*(p+4+l))&dat6;
				dat4=dat4>>j;
				dat4<<=7;

				dat5=(*(p+5+l))&dat6;
				dat5=dat5>>j;
				dat5<<=3;

				write(1,dat0|dat1);
				write(1,dat2|dat3);
				write(1,dat4|dat5);
			}
            //write(0,0x88);
		}
	}
	//write(0,0x89);
}

void character(void)
{
    //U8 i,j,temp[3],table[6],*q;

    //words_all(0, 0, 48, 96, width_48);sleep(1);
    words_all(0, 0, 80, 160, half_width);
    
#if 0

    for(i=0;i<1;i++)
	{
		q=hanzi+i*72;
		j=8*i;

        words_all(j,0,24,24,q);
    }
    

	for(i=0;i<11;i++)
	{
		q=zimu+i*48;
		j=4*i;
		words_all(j,40,12,24,q);
    }
	
	for(i=11;i<18;i++)
	{
		q=zimu+i*48;
		j=4*(i-11);
		words_all(j,70,12,24,q);
     }	

	for(i=18;i<25;i++)
	{
		q=zimu+i*48;
		j=4*(i-18);
		words_all(j,100,12,24,q);
     }

	temp[0]=read(0);
	temp[1]=read(0);
	temp[2]=read(0);
	table[0]=(temp[0]&0xf0)>>4;
	table[1]=temp[0]&0x0f;
	q=ascii+table[0]*36;		
	words(29,100,1,q);
	q=ascii+table[1]*36;		
	words(33,100,1,q);

	table[2]=(temp[1]&0xf0)>>4;
	table[3]=temp[1]&0x0f;
	q=ascii+table[2]*36;		
	words(37,100,1,q);
	q=ascii+table[1]*36;		
	words(41,100,1,q);

	table[4]=(temp[2]&0xf0)>>4;
	table[5]=temp[2]&0x0f;
	q=ascii+table[4]*36;		
	words(45,100,1,q);
	q=ascii+table[5]*36;		
	words(49,100,1,q);
#endif
}

//coord(0x25,i+16*k);
void coord(U8 col,U8 page)
{
    write_command(((page&0xf0)>>4)|0x70);    //page(row) high
    write_command((page&0x0f)|0x60);         //page(row) low
    write_command(((col&0x70)>>4)|0x10);     //colomn high
    write_command(col&0x0f);                 //colomn low
}

void SW(U8 x)
{
    switch(x)
    {
        case 0x00:  {write_data2(0x00); write_data2(0x00);  break;}
        case 0x01:  {write_data2(0x00); write_data2(0x1f);  break;}
        case 0x02:  {write_data2(0x07); write_data2(0xE0);  break;}
        case 0x03:  {write_data2(0x07); write_data2(0xff);  break;}
        case 0x04:  {write_data2(0xf8); write_data2(0x00);  break;}
        case 0x05:  {write_data2(0xF8); write_data2(0x1F);  break;}
        case 0x06:  {write_data2(0xff); write_data2(0xe0);  break;}
        case 0x07:  {write_data2(0xff); write_data2(0xFF);  break;}

        default: break;
    }
}

/*
writeHZ(font[2*i+1+32*1],  font[2*i+0+32*2],  font[2*i+1+32*2]);     Font_2 font[32]--font[63]
writeHZ(font[33], font[64], font[65]);
writeHZ(font[35], font[66], font[67]);
writeHZ(font[37], font[68], font[69]);
...
writeHZ(font[61], font[92], font[93]);
writeHZ(font[63], font[94], font[95]);

            


writeHZ(font[2*i+0+32*0],  font[2*i+1+32*0],  font[2*i+0+32*1]);    Font_1 font[0]--font[31]

writeHZ(font[0], font[1], font[32]);
writeHZ(font[2], font[3], font[34]);
writeHZ(font[4], font[5], font[36]);
...
writeHZ(font[28], font[29], font[60]);
writeHZ(font[30], font[31], font[62]);

*/
void writeHZ(U8 h1, U8 h2, U8 h3) // 8 SW() * 2 = 16 bytes
{
    SW((h1>>5)&0x07);
    SW((h1>>2)&0x07);
    SW(((h1<<1)&0x06)|((h2>>7)&0x01));
    
    SW((h2>>4)&0x07);
    SW((h2>>1)&0x07);
    SW(((h2<<2)&0x04)|((h3>>6)&0x03));
    
    //SW((h3>>3)&0x07); // & 0111
    //SW((h3>>0)&0x07);
}

/*
U8 code  font[]={
//--  文字:  欢
//--  宋体12;  此字体下对应的点阵为：宽x高=16x16
0x00,0x80,0x00,0x80,0xFC,0x80,0x05,0xFE,0x85,0x04,0x4A,0x48,0x28,0x40,0x10,0x40,
0x18,0x40,0x18,0x60,0x24,0xA0,0x24,0x90,0x41,0x18,0x86,0x0E,0x38,0x04,0x00,0x00,
*/
void display_WZ()
{
    U8 i,k;
    //write_command(0x81);
    //write_command(195);

    for(k=0; k<1; k++)
    {
        for(i=0;i<16;i++)//font size 16x16
        {
            coord(0x25,i+16*k); // from font top row 0 to 15, and will write 7 strings each line here

            writeHZ(font[2*i+0+32*0],  font[2*i+1+32*0],  font[2*i+0+32*1]);            
            //writeHZ(font[2*i+1+32*1],  font[2*i+0+32*2],  font[2*i+1+32*2]);
            //writeHZ(font[2*i+0+32*3],  font[2*i+1+32*3],  font[2*i+0+32*4]);
            //writeHZ(font[2*i+1+32*4],  font[2*i+0+32*5],  font[2*i+1+32*5]);
            //writeHZ(font[2*i+0+32*6],  font[2*i+1+32*6],  font[2*i+0+32*7]);
            //writeHZ(font[2*i+1+32*7],  font[2*i+0+32*8],  font[2*i+1+32*8]);
            //writeHZ(font[2*i+0+32*9],  font[2*i+1+32*9],  font[2*i+0+32*9]);
        }
    }
    //key_wait_adjust(30);
    //sleep(3);
}

int main (void)
{
	int ret = 0;

#ifdef MODIFY_TIME
	//printf("+----------------------------------------+\n");
	printf("| 	 Modify  time : %s\n", MODIFY_TIME);
	//printf("+----------------------------------------+\n");
#endif

	if((ret = disp_init()) != 0)
		goto error;

    GUI_FillSCR(0x00);GUI_CopyRAMToLCD(disp_area);
#if 0
    int i = 0;
    while(i < 3)
    {
        GUI_FillSCR(0x00);
        GUI_CopyRAMToLCD(disp_area);

        sleep(1);
        GUI_FillSCR(0xff);
        /*
        disp_area.y0 += 10;
        disp_area.y1 += 10;
        if(disp_area.y1 > 159){
            disp_area.y0 = 0;
            disp_area.y1 = 10;
        }
        */
        GUI_CopyRAMToLCD(disp_area);
        sleep(1);
        i++;
    }
#endif

    //display_WZ();
    //sleep(1);
    character();

	disp_exit();
    //printf("LINE:%03d FUNC:%s()\n", __LINE__, __func__);
    return ret;

error:
	printf ("test error %d\n",ret);
	return 0;
}

