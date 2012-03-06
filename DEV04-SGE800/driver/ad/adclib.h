/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  adclib.h
	描述		：  本文件定义了ad转换模块的底层驱动程序接口
	版本		：  0.2
	作者		：  路冉冉
	创建日期	：  2010.04
******************************************************************************/

	
#ifndef _ADCLIB_H
#define _ADCLIB_H

#define SPI_IOC_MAGIC	0xE1

//#define SPI_IOC_SETUP		_IO(SPI_IOC_MAGIC, 0)	
//#define DS_ADC_ADD			_IO(SPI_IOC_MAGIC, 1)
#define DS_ADC_IRQ_QUERY	_IO(SPI_IOC_MAGIC, 2)
#define		DS_ADC_IRQ			1	//命令DS_ADC_IRQ_QUERY所对应的参数
#define		DS_ADC_QUERY		0


#ifdef DEBUG
#define TESTM		4		//测试
#define TESTMP		5		//测试
#define TESTP		6		//测试
#define READCFR		7		//读取tlv1504内部cfr寄存器内容
#define READFIFO	8		//读取tlv1504内部FIFO寄存器内容
#endif

#endif  /* _ADCLIB_H */

