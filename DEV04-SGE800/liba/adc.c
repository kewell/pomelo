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
	文件		：  adc.c
	描述		：  本文件定义了ad转换模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.01
******************************************************************************/
//库配置头文件
#include "private/config.h"
	
//模块启用开关
#ifdef CFG_ADC_MODULE	
//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//open 标志	
#include <sys/ioctl.h>	//ioctl
#include <string.h> 	//memset
#include <unistd.h>		//close
#include <pthread.h>	//pthread库函数
	
//提供给用户的头文件
#include "include/adc.h"
#include "include/error.h"
	
//驱动调用头文件
#include "private/drvlib/adclib.h"
#include "private/debug.h"
	
/*************************************************
  静态全局变量及宏定义
*************************************************/
#define MAX_ADCCHN 8			//最大io口数量
static pthread_mutex_t mutex;			//互斥锁
static u8 	adc_chn[CFG_ADC_NUM];		//ADC通道对应的物理通道
static u8 	adc_count = 0;				//模快打开计数
static int 	fd;							//外部ad文件描述符
//static  u8 	adc_type = DS_ADC_IRQ;		//默认读取方式为中断
static  u16 adc_timeout = 1;			//默认100ms超时

/*************************************************
  API
*************************************************/
/******************************************************************************
*	函数:	adc_init
*	功能:	AD转换模块初始化
*	参数:	无
*	返回:	0				-	成功
			-ERR_CFG		-	配置超限
			-ERR_BUSY		-	已经打开
			-ERR_SYS		-	系统错误
			-ERR_NOFILE		-	没有此路径
 ******************************************************************************/
int adc_init(void)
{
	int ret = -1;
	
	if(CFG_ADC_NUM > MAX_ADCCHN){
		ret = -ERR_CFG; 	//配置超限
		goto err;
	}
	if(adc_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}	
	if (pthread_mutex_init(&mutex, NULL)) {	//初始化互斥锁
		ret = -ERR_SYS;
		goto err;
	}
	//设置adc通道
#ifdef CFG_ADC_0
	adc_chn[CFG_ADC_0] = CFG_ADC_0 ;
#endif
#ifdef CFG_ADC_1
	adc_chn[CFG_ADC_1] = CFG_ADC_1 ;
#endif
#ifdef CFG_ADC_2
	adc_chn[CFG_ADC_2] = CFG_ADC_2;
#endif
#ifdef CFG_ADC_3
	adc_chn[CFG_ADC_3] = CFG_ADC_3 ;
#endif
#ifdef CFG_ADC_4
	adc_chn[CFG_ADC_4] = CFG_ADC_4 ;
#endif
#ifdef CFG_ADC_5
	adc_chn[CFG_ADC_5] = CFG_ADC_5 ;
#endif
#ifdef CFG_ADC_6
	adc_chn[CFG_ADC_6] = CFG_ADC_6 ;
#endif
#ifdef CFG_ADC_7
	adc_chn[CFG_ADC_7] = CFG_ADC_7 ;
#endif

	
	fd = open("/dev/tlv1504", O_RDONLY);	//打开外部ADC驱动
	if (fd < 0){
		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}	
	//打开内部ADC驱动
	
//	//设置数据读取方式
//#if CFG_ADC_TYPE
//	adc_type = DS_ADC_IRQ;
//#else
//	adc_type = DS_ADC_QUERY;
//#endif
//	ret = ioctl(fd, DS_ADC_IRQ_QUERY, adc_type);
//	if (ret < 0){
//		ret = -ERR_SYS;		//没有此路径
//		goto err;
//	}

	adc_count = 1;
	adc_timeout = 1;	//超时时间默认为100ms
	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	adc_setwaittime
*	功能:	设置AD转换超时时间
*	参数:	timeout			-	超时时间
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
*	说明:	timeout单位是100ms
 ******************************************************************************/
int adc_setwaittime(u16 timeout)
{
	if(timeout <= 0 ){
		return -ERR_INVAL;
	}
	adc_timeout = timeout;
	return 0;	
}

/******************************************************************************
*	函数:	adc_getwaittime
*	功能:	获取AD转换超时时间
*	参数:	timeout			-	超时时间（数据传出指针）
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
*	说明:	timeout单位是100ms
 ******************************************************************************/
int adc_getwaittime(u16 *timeout)
{
	*timeout = adc_timeout;
	return 0;
}	

/******************************************************************************
*	函数:	adc_read
*	功能:	读取转换结果
*	参数:	id				-	AD通道号
			result			-	AD转换结果（数据传出指针）
*	返回:	0				-	成功
			-ERR_TIMEOUT	-	超时
			-ERR_NODEV 		-	无此设备
			-ERR_NOINIT		-	锁没有初始化化
			-ERR_OTHER:		-	其他关于线程互斥锁的错误
*	说明:	result是AD转换的原始值。
 ******************************************************************************/
int adc_read(u8 id, u16 *result)
{
	int ret = -1;
	int adc_result;
	int timeout;
	timeout = adc_timeout;
	if((id < 0) || (id >= MAX_ADCCHN)){		//din范围检查
		return -ERR_NODEV;		
	}	
	//读取数据
	do{
		if(id <= CHANNEL7){	//外部AD读取
			
			ret = read(fd, &adc_result, adc_chn[id]);
			if (ret < 0){
				timeout --;
			} 
		}else{			//内部AD读取			
			adc_result = 0;//模拟
			ret=0;
		}
		usleep(100000);	//延时100ms
	}while((ret < 0) && (timeout > 0));
	if (ret < 0){
		ret = -ERR_TIMEOUT;
		goto err;
	} 
	*result = adc_result;
	ret = 0;
err:	
	if (pthread_mutex_unlock (&mutex)) {
		ret = -ERR_OTHER;	
	}	
	return ret;
}

/******************************************************************************
*	函数:	adc_close
*	功能:	adc模块关闭函数
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没有初始化
			-ERR_OTHER		-	其他关于线程互斥锁的错误
*	说明:	无
 ******************************************************************************/
int adc_close(void)
{
	int ret = -1;
		
	if(adc_count == 0){
		return -ERR_NOINIT;
	}
	ret = close(fd);
	if(ret < 0){
		return -ERR_SYS;	
	}
	adc_count = 0;
	if (pthread_mutex_destroy(&mutex)) {
		ret = -ERR_OTHER;
	}
	ret = 0;
	return ret;
}
#endif
