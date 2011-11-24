/**
* adc.c -- A/D驱动接口
* 
* 
* 创建时间: 2010-5-16
* 最后修改时间: 2010-5-16
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "include/debug.h"
#include "include/sys/gpio.h"
#include "include/sys/schedule.h"
#include "include/sys/mutex.h"

static int FidAdc = -1;
static sys_mutex_t AdcMutex;

#define ADC_LOCK	SysLockMutex(&AdcMutex)
#define ADC_UNLOCK	SysUnlockMutex(&AdcMutex)

static unsigned short ChannelCfg = 0;
#define CHANNEL_SELMASK		0x0400
#define CHANNEL_SEL0		ChannelCfg &= ~CHANNEL_SELMASK
#define CHANNEL_SEL1		ChannelCfg |= CHANNEL_SELMASK

/**
* @brief A/D驱动接口初始化
* @return 0成功, 否则失败
*/
DECLARE_INIT_FUNC(AdcInit);
int AdcInit(void)
{
	unsigned short us;

	SysInitMutex(&AdcMutex);

	GpioSetDirect(GPIO_SPI_SEL1, 1);
	GpioSetValue(GPIO_SPI_SEL1, 0);
	GpioSetDirect(GPIO_SPI_SEL0, 1);
	GpioSetValue(GPIO_SPI_SEL0, 1);

	FidAdc = open("/dev/adc", O_RDWR);
	if(FidAdc < 0) {
		printf("can not open adc driver\n");
		return 1;
	}

	/*us = 0x8230;
	write(FidAdc, &us, 2);
	us = 0xa880;
	write(FidAdc, &us, 2);*/
	ChannelCfg = 0x8030;
	us = ChannelCfg;
	write(FidAdc, &us, 2);
	us = 0xb980;
	//us = 0xa880;
	write(FidAdc, &us, 2);

	SET_INIT_FLAG(AdcInit);

	return 0;
}

/**
* @brief 读取ADC数据
* @param channel 通道号
* @return 数值 (mV)
*/
int AdcRead(unsigned int channel)
{
	unsigned short us;
	int i;

	AssertLogReturn(FidAdc < 0, 0, "invalid fid\n");
	AssertLogReturn(channel>1, 0, "invalid channel(%d)\n", channel);

	ADC_LOCK;

	if(0 == channel) CHANNEL_SEL0;
	else CHANNEL_SEL1;

	us = ChannelCfg;
	write(FidAdc, &us, 2);
	Sleep(2);
	us = 0;
	write(FidAdc, &us, 2);
	//us &= 0x1fff;
	i = (int)us&0x1fff;
	i = (i*10000)/8191;

	ADC_UNLOCK;

	return i;
}

/***********
* 单元测试方法: 见shellcmd.c/shell_adc(), 配合硬件测试
*/

