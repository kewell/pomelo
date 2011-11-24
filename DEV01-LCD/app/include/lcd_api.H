/*//////////////////////////////////////////////////////////////////////////
Orgnization:	Emtronix Incorporated
Filename:	LCD_API.H
Authors:	ZHL & CS
Date:		Nov, 2007
Revised:	...
Description:	defintions of API functions for LCD module
///////////////////////////////////////////////////////////////////////////*/
//#ifndef  _LCD_API_H
//#define  _LCD_API_H

//#ifdef	__cplusplus
//extern "C" {
//#endif

#define  BK_COLOR		0			// LCD 背景显示颜色
#define  TEXT_COLOR		1			// LCD 文本显示颜色

#define  COPY_WRITE		0			// 复制模式写
#define  XOR_WRITE		1           // 异或模式写

#define LCD_AUTO		0			// LCD 自动识别模式
#define LCD_12864		1			// LCD 128X64（分辨率）  KS0108控制器
#define LCD_240128		2			// LCD 240X128（分辨率） T6963C控制器
#define LCD_320240		3			// LCD 320X240（分辨率） SED1335控制器
#define LCD_19264		4			// LCD 192X64（分辨率）  KS0108控制器
#define LCD_160160		5			// LCD 160X160（分辨率） UC1698U控制器

// 功能描述:   启动对LCD屏的操作功能，并初始化LCD显示屏。
// 输入参数:   LCD的类型。
// 返回值 > 0: 启动LCD成功，并返回LCD的类型。
//       = -1: 启动LCD失败。
int  LCD_Init( int LCDType );

// 功能描述:   释放对LCD显示屏的操作功能。
// 输入参数:   无。
// 返回值 > 0: 初始化LCD成功，并返回LCD的类型。
//       = -1: 初始化LCD失败。
int  LCD_DeInit( );

// 功能描述:   对LCD显示屏的清屏操作。
// 输入参数:   无。
// 返回值 = 0: LCD清屏成功。
//        <0 : LCD清屏失败。
int  LCD_Clearup( );

// 功能描述:  获取LCD显示屏的X方向最大分辨率。
// 输入参数:  无。
// 返回值  :  LCD显示屏的X方向最大分辨率。。
int  LCD_GetMaxX( );

// 功能描述:  获取LCD显示屏的Y方向最大分辨率。
// 输入参数:  无。
// 返回值  :  LCD显示屏的Y方向最大分辨率。。
int  LCD_GetMaxY( );

// 功能描述:  设置LCD显示屏的写模式。
// 输入参数:  Mode = 0 : COPY 写模式
//            Mode = 1 : XOR 写模式
// 返回值 0:  成功
int  LCD_SetMode( int Mode );

// 功能描述:  设置LCD显示汉字字模：16点阵、12点阵。
// 输入参数:  FontWidth = 16 : 16点阵汉字
//           FontWidth = 12 : 12点阵汉字
// 返回值 :  字模的类型16或12
int  LCD_SetFont( int FontWidth );


// 功能描述:  对LCD显示屏进行描点操作。
// 输入参数:
//	    int x0: 描点的列坐标(x坐标)，范围0～(MaxX-1)，不可大于或等于MaxX
//      int y0: 描点的行坐标(y坐标)，范围0～(MaxY-1)，不可大于或等于MaxY
//   int color: 描点的颜色：BK_COLOR或者TEXT_COLOR
// 返回值 0:  成功。
int  LCD_PutPixel( int x0, int y0, int color );

// 功能描述:  对LCD显示屏进行写字节操作。
// 输入参数:
//	     int x: 写字节的列坐标(x坐标)，范围0～(MaxX-1)，并要求为8的倍数
//       int y: 写字节的行坐标(y坐标)，范围0～(MaxY-1)，并要求为8的倍数
//  UCHAR byte: 写的字节
// 返回值 0:  成功。
int	 LCD_WriteByte( int x0, int y0, unsigned char byte );

// 功能描述:  对LCD显示屏进行写多个字节操作。
// 输入参数:
//	               int x: 写字节的列坐标(x坐标)，范围0～(MaxX-1)，并要求为8的倍数
//                 int y: 写字节的行坐标(y坐标)，范围0～(MaxY-1)，并要求为8的倍数
//  unsigned char* hfont: 需要写的字节组
//        int NumOfBytes: 需要写的字节数
// 返回值 0:  成功。
int  LCD_WriteByteBlock( int x0, int y0, unsigned char* hfont, int NumOfBytes );

// 功能描述:  对LCD显示屏进行读字节操作。
// 输入参数:
//	     int x: 写字节的列坐标(x坐标)，范围0～(MaxX-1)，并要求为8的倍数
//       int y: 写字节的行坐标(y坐标)，范围0～(MaxY-1)，并要求为8的倍数
// 返回值 :  读取的字节。
int  LCD_ReadByte( int x0, int y0 );

// 功能描述:  对LCD显示屏画直线操作。
// 输入参数:
//	     int x0: 直线开始列坐标(x坐标)，范围0～(MaxX-1)，不可大于或等于MaxX
//       int y0: 直线开始行坐标(y坐标)，范围0～(MaxY-1)，不可大于或等于MaxY
//	     int x1: 直线结束列坐标(x坐标)，范围0～(MaxX-1)，不可大于或等于MaxX
//       int y1: 直线结束行坐标(y坐标)，范围0～(MaxY-1)，不可大于或等于MaxY
//    int color: 颜色设置：BK_COLOR或者TEXT_COLOR
int  LCD_DrawLine( int x0, int y0, int x1, int y1, int color );

// 功能描述:  对LCD显示屏画Bar条操作。
// 输入参数:
//	     int x0: 直线开始列坐标(x坐标)，范围0～(MaxX-1)，不可大于或等于MaxX
//       int y0: 直线开始行坐标(y坐标)，范围0～(MaxY-1)，不可大于或等于MaxY
//	     int x1: 直线结束列坐标(x坐标)，范围0～(MaxX-1)，不可大于或等于MaxX
//       int y1: 直线结束行坐标(y坐标)，范围0～(MaxY-1)，不可大于或等于MaxY
//    int color: 颜色设置：BK_COLOR或者TEXT_COLOR
int  LCD_FillBar( int x0, int y0, int x1, int y1, int color );

// 功能描述:  对LCD显示屏进行写字符串的操作。
// 输入参数:
//	    int x0: 字符写入的列坐标(x坐标)，范围0～(MaxX-1)，并要求为8的倍数
//      int y0: 字符写入的行坐标(y坐标)，范围0～(MaxY-1)，并要求为8的倍数
//   int color: 字符写入的颜色设置：BK_COLOR或者TEXT_COLOR
// 返回值 0:  成功。
int  LCD_WriteString( int x0, int y0, char* pStr, int color );

//#ifdef	__cplusplus
//}
//#endif

//#endif
