/**
* scr_lcd.h -- 液晶显示头文件
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#ifndef  _SCR_LCD_H_
#define  _SCR_LCD_H_

#define  LCD_BUFFER_SIZE		3200
#define  BIGASCII_HZK_SIZE	0x2800
#define  GB2312_HZK_SIZE		0x40000
#define  ASCII_SIZE			4096

#define  TOP_LINE_ROW            16
#define  BTM_LINE_ROW            143
#define  ONE_LINE_BYTE           20          

void  DisplayNormal(const unsigned char *str,unsigned char len,unsigned char x,unsigned char y);
void  ClearLcdScreen(void);
void  LcdBakLight(int ison);
void  DrawMenuLine(void);
char  LcdGetKey(void);
char GetLcdBakLightState(void);
void CloseLcdBakLightState(void);
int   InitLcd();
void  DisplayLcdBuffer(void);
void  SetInvs(unsigned char flag);
void  DisplayOneAscii(unsigned char *str,unsigned char x,unsigned char y);
void  CleanPartOfScreen(unsigned char x ,unsigned char len);
void  SetLineInvs(unsigned char x);
//void  SetLineInvs(unsigned char x,unsigned char y,unsigned char len)
void  CleanPageNum(void);
void  ShowTime();
void  DisplayPageNumber(int number,unsigned char x);
void  ShowTopFlag(void);
void  CleanLoopMetNum(void);
void ClearTopBuffer(void);
void ClearMenuBuffer(void);
void ScreenSetHeadFlag(unsigned char flag,unsigned int arg);
void DisplayReadMeter(int number,unsigned char x);
#endif /*_SCR_LCD_H_*/

