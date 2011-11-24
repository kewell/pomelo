/**
* adc.h -- A/D驱动接口
* 
* 
* 创建时间: 2010-5-16
* 最后修改时间: 2010-5-16
*/

#ifndef _SYS_ADC_H
#define _SYS_ADC_H

#define ADC_CHN_BAT		0   //电池电压通道
#define ADC_CHN_TEMP	1   //温度通道
#define ADC_CHN_MAX		2

/**
* @brief 读取ADC数据
* @param channel 通道号
* @return 数值 (mV)
*/
int AdcRead(unsigned int channel);

#endif /*_SYS_ADC_H*/

