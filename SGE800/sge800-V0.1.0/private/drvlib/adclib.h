/******************************************************************************
 * 许继电气股份有限公司                                    版权：2008-2015    *
 ******************************************************************************
 * 本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许     *
 * 可不得擅自修改或发布，否则将追究相关的法律责任。                           *
 *                                                                            *
 *                       河南许昌许继股份有限公司                             *
 *                       www.xjgc.com                                         *
 *                       (0374) 321 2924                                      *
 *                                                                            *
 ******************************************************************************
 * 
 * 项目名称		:	adc驱动程序在应用层头文件
 * 文件名		:	adclib.h
 * 描述			:	应用程序所用到的ioctl函数的命令及arg参数,read函数参数.
 * 版本			:	1.0.1
 * 作者			:	路冉冉
 *
 * 修改历史记录	:
 * --------------------
 * 01a, 18aug2009, Roy modified
 * --------------------
 *
 ******************************************************************************/
	
#ifndef _ADCLIB_H
#define _ADCLIB_H

#define SPI_IOC_MAGIC	108		//ioctl cmd幻数

#define 	DS_ADC_CH			_IO(SPI_IOC_MAGIC, 1) 	//设置id通道
#define 	DS_ADC_IRQ_QUERY	_IO(SPI_IOC_MAGIC, 2) 	//ioctl cmd
#define		DS_ADC_IRQ			1	//命令DS_ADC_IRQ_QUERY所对应的参数
#define		DS_ADC_QUERY		0	//命令DS_ADC_IRQ_QUERY所对应的参数



#define TESTM		4		//测试
#define TESTMP		5		//测试
#define TESTP		6		//测试
#define READCFR		7		//读取tlv1504内部cfr寄存器内容
#define READFIFO	8		//读取tlv1504内部FIFO寄存器内容


#endif  /* _ADCLIB_H */

