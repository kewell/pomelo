/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：config.h
	描述		：本文件定义了业务平台框架的配置头文件
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.12
******************************************************************************/

#ifndef _CONFIG_H
#define _CONFIG_H

//调试配置
#define CFG_FRAMEWORK_DEBUG
#define CFG_MODULE_DEBUG

//消息调度模块配置项
#define CFG_MESSAGE_SUBSCRIBE_MAX 	8		//每个消息允许最大的订阅次数
#define CFG_MESSAGE_TYPE_MAX		64		//消息类型上限

//时间管理模块配置项
#define CFG_SYSTIME_TIMER_MAX		64		//允许注册的最大定时器数量

//框架初始化模块配置项
#define CFG_FRAMEWORK_THREAD_MAX	32		//框架初始化时支持的最大线程数

//平台设备配置项

//按键
#define CFG_KEY_DEVICE 			1				//是否启用
#define CFG_KEY_NUM				7				//当前KEY通道数量

#define CFG_KEY_LONG			3000			//长按开始响应间隔（以ms为基本单位）。注意不要大于按键获取api的超时。
#define CFG_KEY_PER				500				//长按返回间隔（以ms为基本单位）。注意不要小于按键扫描时间
#define CFG_KEY_SCAN_DELAY		100				//按键扫描延时，单位ms，推荐20~100ms间。
#define CFG_KEY_MODE			1				//按键响应模式，1-按下返回，0-释放返回

#define CFG_KEY_ENTER			PIN_PC8			//key0
#define CFG_KEY_ENTER_TYPE		0				//IO高低与按键是否按下之间的联系，1高按下，0低按下

#define CFG_KEY_CANCEL			PIN_PA25		//key1
#define CFG_KEY_CANCEL_TYPE		0				//IO高低与按键是否按下之间的联系，1高按下，0低按下

#define CFG_KEY_LEFT			PIN_PA22		//key2
#define CFG_KEY_LEFT_TYPE		0				//IO高低与按键是否按下之间的联系，1高按下，0低按下

#define CFG_KEY_UP				PIN_PA6			//key3
#define CFG_KEY_UP_TYPE			0				//IO高低与按键是否按下之间的联系，1高按下，0低按下

#define CFG_KEY_RIGHT			PIN_PB31		//key4
#define CFG_KEY_RIGHT_TYPE		0				//IO高低与按键是否按下之间的联系，1高按下，0低按下

#define CFG_KEY_DOWN			PIN_PA7			//key5
#define CFG_KEY_DOWN_TYPE		0				//IO高低与按键是否按下之间的联系，1高按下，0低按下

#define CFG_KEY_PROGREM			PIN_PB30		//PG key
#define CFG_KEY_PROGREM_TYPE 	0				//IO高低与按键是否按下之间的联系，1高按下，0低按下

//状态灯模块配置
#define CFG_LED_DEVICE			1				//是否启用
#define CFG_LED_NUM				4				//当前LED通道数量

#define CFG_LED_0				PIN_PB18		//D9 WARN LED
#define CFG_LED_TYPE0			0				//IO高低与LED亮灭之间的联系，1高亮，0低亮

#define CFG_LED_1				PIN_PA10		//D5 LED RXDJL 红光
#define CFG_LED_TYPE1			0				//IO高低与LED亮灭之间的联系，1高亮，0低亮

#define CFG_LED_2				PIN_PA9			//D5 LED TXDJL 绿光
#define CFG_LED_TYPE2			0				//IO高低与LED亮灭之间的联系，1高亮，低亮

#define CFG_LED_3				PIN_PA11		//D10 RUN LED
#define CFG_LED_TYPE3			0				//IO高低与LED亮灭之间的联系，1高亮，0低亮

//继电器设备配置
#define CFG_RELAY_DEVICE		1				//是否启用
#define CFG_RELAY_NUM			1				//当前RELAY通道数量

#define CFG_RELAY_0				PIN_PB17		//0号通道对应的硬件通道
//#undef  CFG_RELAY_D0
//#define CFG_RELAY_D0			PIN_PB19		//0号通道双点对应的硬件通道
#define CFG_RELAY_TYPE0			0				//IO高低与继电器动作之间的联系，1高动作，0低动作

//掉电检测设备配置
#define CFG_POWERD_DEVICE		1				//是否启用

//遥信与脉冲量配置
#define CFG_SWIN_DEVICE			1				//是否启用
#define CFG_SWIN_TIMER			5				//用于产生采样间隔的定时器号
#define CFG_SWIN_FILTER_TIME	5				//消抖采样间隔（单位ms）
#define CFG_SWIN_FILTER_NUM		5				//消抖采样次数
#define CFG_SWIN_READ_CYCLE		1000			//读取处理脉冲遥信周期，单位ms 默认1s

#define CFG_SWIN_DOOR			PIN_PB16			//门接点信号对应IO
#define CFG_SWIN_LID_TOP		PIN_PB16			//开表盖信号对应IO
#define CFG_SWIN_LID_MID		PIN_PB16			//开中盖信号对应IO
#define CFG_SWIN_LID_TAIL		PIN_PB16		//开尾盖信号对应IO

#define CFG_SWIN_NUM			1				//遥信与脉冲量数量
#define CFG_SWIN_0				PIN_PC8			//0号遥信或脉冲通道对应的io口

//#define CFG_SWIN_1				PIN_PA5		//1号遥信或脉冲通道对应的io口
//#define CFG_SWIN_2				PIN_PA6		//2号遥信或脉冲通道对应的io口
//#define CFG_SWIN_3				PIN_PA7		//3号遥信或脉冲通道对应的io口
//#define CFG_SWIN_4				PIN_PB8		//4号遥信或脉冲通道对应的io口
//#define CFG_SWIN_5				PIN_PB9		//1号遥信或脉冲通道对应的io口
//#define CFG_SWIN_6				PIN_PB10	//2号遥信或脉冲通道对应的io口
//#define CFG_SWIN_7				PIN_PB11	//3号遥信或脉冲通道对应的io口

//数据流设备配置
#define CFG_DSTREAM_DEVICE 	1
#define DSTREAM_ZB_COM		5		//载波对应的内部com口
#define DSTREAM_IRDA_COM	12		//红外
#define DSTREAM_DEBUG_COM	13		//调试
#define DSTREAM_USBD_COM	7		//USB从口
#define DSTREAM_JC_COM		6		//底板
#define DSTREAM_485_1_COM	3		//1#485口 采集485
#define DSTREAM_485_2_COM	2		//2#485口 级联485
#define DSTREAM_485_3_COM	11		//3#485口 下行485 接单片机

//GPRS设备配置
#define CFG_GPRS_DEVICE 	1

//网络设备配置
#define CFG_ETHER_DEVICE 	1


#endif
