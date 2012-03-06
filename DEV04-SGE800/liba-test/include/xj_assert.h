/**************************************************************
** Copyight (C) 2002-2006 基础技术开发部
** All right reserved
***************************************************************
**
** 项目名称：XJDN 公共代码库
** 功能简介：XJ_ASSERT 简单测试框架
**
** 原始作者：周劲羽  Email：zhoujingyu@xjgc.com zjy@cnpack.org
** 组织部门：许继集团 基础技术开发部
** 备    注：
** 建立时间：2006-11-30
** 完成时间：2006-11-30
** 20090919 增加printbuffer,打印输出缓冲区内容，用于调试 包伟
** 版    本：1.0
***************************************************************
** 单元标识：$Id: xj_assert.h,v 1.1 2009/12/20 11:17:05 yjb Exp $
** 版本历史：
** 修 改 者：
** 最近修改：
**************************************************************/

#ifndef XJ_ASSERT_H
#define XJ_ASSERT_H

#ifdef XJ_ASSERT_DECLARE

/* 初始化测试环境 */
void inittest();

/* 清除测试环境 */
void finaltest();

/* 测试断言，exp 为要测试的表达式结果，msg 为表达式结果为 0 时的错误消息 */
void assert( int exp, const char * msg );

//输出缓冲区内容，用于调试测试用例
void printbuffer( unsigned char buffer[], int len, char split );

#else

#include <stdio.h>
//#include <string.h>

static int _TestCount  = 0;  /* 测试计数 */
static int _ErrorCount = 0;  /* 错误计数 */

/* 初始化测试环境 */
void inittest()
{
    _TestCount  = 0;
    _ErrorCount = 0;
}

//输出缓冲区内容，用于调试测试用例
void printbuffer( unsigned char buffer[], int len, char split )
{
    int i;
    for ( i = 0; i < len; i ++)
    {
        fprintf( stdout, "%02x", buffer[i] );
        if( i > 0 &&(( i % 16 )== 15 )) fprintf( stdout, "\n" );//16字节一行
        else if( i != len - 1 ) fprintf( stdout, "%c", split ); //非最后字符则打分隔符
        else fprintf( stdout, "\n" );                         //结束字符
    }
}

/* 清除测试环境 */
void finaltest()
{
    if ( _ErrorCount > 0 )
        printf( "%d failed in %d tests.\n", _ErrorCount, _TestCount );
    else
        printf( "%d tests passed.\n", _TestCount );
}

/* 测试断言，exp 为要测试的表达式结果，msg 为表达式结果为 0 时的错误消息 */
void assert( int exp, const char * msg )
{
    _TestCount ++;
    if (exp == 0)
    {
        _ErrorCount ++; //asdfasdf
        printf( msg );
        //if ( msg[strlen( msg ) - 1] != '\n' )
        printf( "\n" );
    }
}

void p_err(int exp)
{
	//_TestCount ++;
	if (exp != 0)
	{
	//	_ErrorCount ++;
//		printf(msg);
		switch(exp)
		{
			case -1:
				printf(" error: ERR_SYS\n");
				break;
			case -2:
				printf(" error: ERR_NODEV/ERR_NOFILE\n");
				break;
			case -3:
				printf(" error: ERR_TIMEOUT\n");
				break;
			case -4:
				printf(" error: ERR_INVAL\n");
				break;
			case -5:
				printf(" error: ERR_NOFUN\n");
				break;
			case -6:
				printf(" error: ERR_BUSY\n");
				break;
			case -7:
				printf(" error: ERR_NOINIT\n");
				break;
			case -8:
				printf(" error: ERR_NOMEM\n");
				break; 
			case -9:
				printf(" error: ERR_NODISK\n");
				break; 
			case -10:
				printf(" error: ERR_NORECORD\n");
				break; 
			case -11:
				printf(" error: ERR_CFG\n");
				break; 
			case -12:
				printf(" error: ERR_NOCFG\n");
				break; 
			case -13:
				printf(" error: ERR_DEVUNSTAB\n");
				break; 
			case -14:
				printf(" error: ERR_DISCONNECT\n");
				break;
			case -80:
				printf(" error: ERR_OTHER\n");
				break;
			default:
				printf("\n");
				break;
		}
				
	}
}

#endif                  /* XJ_ASSERT_DECLARE */

#endif                  /* XJ_ASSERT_H */
