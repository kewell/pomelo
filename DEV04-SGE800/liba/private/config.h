#ifndef _CONFIG_H
#define _CONFIG_H

#include "../include/pinio.h"

//调试配置
//#define CFG_DEBUG 1

#ifdef CFG_DEBUG
#define CFG_DEBUG_DIN
#define CFG_DEBUG_PULSE
#define CFG_DEBUG_POWERCHECK
#define CFG_DEBUG_ADC
#define CFG_DEBUG_RTC
#define CFG_DEBUG_TIMER
#define CFG_DEBUG_GPIO

#define CFG_DEBUG_COMPORT
#define CFG_DEBUG_NET
#define CFG_DEBUG_GSERIAL

#define CFG_DEBUG_DBS
#define CFG_DEBUG_FILE

#define CFG_DEBUG_THREAD
#define CFG_DEBUG_MSG
#endif

//AD转换模块配置
#define CFG_ADC_MODULE				1			//是否启用
#define CFG_ADC_NUM					8			//当前ADC通道数量
#define CFG_ADC_0					0			//0号通道对应的硬件通道
#define CFG_ADC_1					1			//1号通道对应的硬件通道
#define CFG_ADC_2					2			//2号通道对应的硬件通道
#define CFG_ADC_3					3			//3号通道对应的硬件通道
#define CFG_ADC_4					4			//4号通道对应的硬件通道
#define CFG_ADC_5					5			//5号通道对应的硬件通道
#define CFG_ADC_6					6			//6号通道对应的硬件通道
#define CFG_ADC_7					7			//7号通道对应的硬件通道

//定时器模块配置
#define CFG_TIMER_MODULE			1			//是否启用
#define CFG_TIMER_PWM_S				1			//脉宽调制时钟源，高精度
#define CFG_TIMER_MEASURE_S			2			//频率测量时钟源，低精度

//RTC模块配置
#define CFG_RTC_MODULE				1				//是否启用

//IO操作模块配置
#define CFG_GPIO_MODULE				1				//是否启用

//掉电上电检测模块配置
#define CFG_POWERCHECK_MODULE			1			//是否启用
#define CFG_POWERCHECK_IO				PIN_PB27	//掉电检测监视io口

//网络模块
#define CFG_NET_MODULE              1
#define CFG_NET_SERVPORT            3333                 //端口号
#define CFG_NET_BACKLOG             8                    //队列
#define CFG_NET_MAXSIZE             8                    //最大连入客户端数--最大为64

//串口配置	
#define CFG_COMPORT_MODULE		1			//是否启用

//USB从口虚拟串口配置	
#define CFG_GSERIAL_MODULE		1			//是否启用

//数据库配置
#define CFG_DBS_MODULE				1				//是否启用
#define CFG_DBS_ENV_FLAG     		DB_INIT_CDB |DB_CREATE |DB_PRIVATE|DB_INIT_MPOOL|DB_THREAD      //数据库环境变量
#define CFG_DBS_MPOOL_SIZE    		4*1024*1024          //共享内存池大小，默认256K
#define CFG_DBS_ENV_HOME      		"/mnt/local"         //设置打开环境的目录
#define CFG_DBS_UNM_MAX       		16                   //最大可支持的数据库个数
#define CFG_DBS_ARITH         		DB_HASH              //数据库所使用的算法
#define CFG_DBS_RECORD_MAX    		128                  //单条数据记录最大字节数
#define CFG_DBS_KEYSIZE_MAX   		8                    //关键字最大字节数

//文件操作模块配置
#define CFG_FILE_MODULE				1				//是否启用
#define CFG_FILE_NAME_MAX			32				//文件名的最大长度
#define CFG_FILE_DATA_MAX			0x1000			//文件数据读写的最大长度

//线程操作模块配置
#define CFG_THREAD_MODULE			1				//是否启用
#define CFG_THREAD_MAX				32				//最大支持线程数

//消息服务模块配置
#define CFG_MSG_MODULE				1				//是否启用
#define CFG_MSG_MAX					32				//最大支持消息队列数
#define CFG_MSG_SIZE				64				//消息队列所能容纳的消息数

//同步模块
#define CFG_SYN_MODULE              1
#define CFG_MUTEX_MAX               64                   //支持的互斥锁最大数
#define CFG_SEM_MAX                 64                   //支持的信号量最大数
#define CFG_RWLOCK_MAX              64                   //支持的读写锁最大数

//继电器模块配置
#define CFG_RELAY_MODULE			1				//是否启用
#define CFG_RELAY_NUM				3				//当前RELAY通道数量

#define CFG_RELAY_0					PIN_PB22			//0号通道对应的硬件通道
//#undef  CFG_RELAY_D0
#define CFG_RELAY_D0				PIN_PB19		//0号通道双点对应的硬件通道
#define CFG_RELAY_TYPE0				0				//IO高低与继电器动作之间的联系，1高动作，0低动作

#define CFG_RELAY_1					PIN_PB23			//1号通道对应的硬件通道
//#undef  CFG_RELAY_D1
#define CFG_RELAY_D1				PIN_PB20		//1号通道双点对应的硬件通道
#define CFG_RELAY_TYPE1				0				//IO高低与继电器动作之间的联系，1高动作，0低动作

#define CFG_RELAY_2					PIN_PB24			//2号通道对应的硬件通道
//#undef  CFG_RELAY_D2
#define CFG_RELAY_D2				PIN_PB21		//2号通道双点对应的硬件通道
#define CFG_RELAY_TYPE2				0				//IO高低与继电器动作之间的联系，1高动作，0低动作

//#define CFG_RELAY_3					PIN_PB3			//3号通道对应的硬件通道
////#undef  CFG_RELAY_D3
//#define CFG_RELAY_D3				PIN_PB13		//3号通道对应的硬件通道
//#define CFG_RELAY_TYPE3				1				//IO高低与继电器动作之间的联系，1高动作，0低动作
//
//#define CFG_RELAY_4					PIN_PB4			//4号通道对应的硬件通道
////#undef  CFG_RELAY_D4
//#define CFG_RELAY_D4				PIN_PB14		//4号通道对应的硬件通道
//#define CFG_RELAY_TYPE4				1				//IO高低与继电器动作之间的联系，1高动作，0低动作
//
//#define CFG_RELAY_5					PIN_PB5			//5号通道对应的硬件通道
////#undef  CFG_RELAY_D5
//#define CFG_RELAY_D5				PIN_PB15		//5号通道对应的硬件通道
//#define CFG_RELAY_TYPE5				1				//IO高低与继电器动作之间的联系，1高动作，0低动作
//
//#define CFG_RELAY_6					PIN_PB6			//6号通道对应的硬件通道
////#undef  CFG_RELAY_D6
//#define CFG_RELAY_D6				PIN_PB16		//6号通道对应的硬件通道
//#define CFG_RELAY_TYPE6				1				//IO高低与继电器动作之间的联系，1高动作，0低动作
//
//#define CFG_RELAY_7					PIN_PB7			//7号通道对应的硬件通道
////#undef  CFG_RELAY_D7
//#define CFG_RELAY_D7				PIN_PB17		//7号通道对应的硬件通道
//#define CFG_RELAY_TYPE7				1				//IO高低与继电器动作之间的联系，1高动作，0低动作

//状态灯模块配置
#define CFG_LED_MODULE			1				//是否启用
#define CFG_LED_NUM				8				//当前LED通道数量

#define CFG_LED_0				PIN_PB0			//0号通道对应的硬件通道
#define CFG_LED_TYPE0			1				//IO高低与LED亮灭之间的联系，1高亮，0低亮
#define CFG_LED_1				PIN_PB1			//1号通道对应的硬件通道
#define CFG_LED_TYPE1			1				//IO高低与LED亮灭之间的联系，1高亮，0低亮

#define CFG_LED_2				PIN_PB2			//2号通道对应的硬件通道
#define CFG_LED_TYPE2			1				//IO高低与LED亮灭之间的联系，1高亮，0低亮

#define CFG_LED_3				PIN_PB3			//3号通道对应的硬件通道
#define CFG_LED_TYPE3			1				//IO高低与LED亮灭之间的联系，1高亮，0低亮

#define CFG_LED_4				PIN_PB4			//4号通道对应的硬件通道
#define CFG_LED_TYPE4			1				//IO高低与LED亮灭之间的联系，1高亮，0低亮

#define CFG_LED_5				PIN_PB5			//5号通道对应的硬件通道
#define CFG_LED_TYPE5			1				//IO高低与LED亮灭之间的联系，1高亮，0低亮

#define CFG_LED_6				PIN_PB6			//6号通道对应的硬件通道
#define CFG_LED_TYPE6			1				//IO高低与LED亮灭之间的联系，1高亮，低亮

#define CFG_LED_7				PIN_PB7			//7号通道对应的硬件通道
#define CFG_LED_TYPE7			1				//IO高低与LED亮灭之间的联系，1高亮，0低亮

//遥信与脉冲量模块配置
#define CFG_SWIN_MODULE				1			//是否启用

#define CFG_SWIN_DOOR				PIN_PB5		//门接点信号对应IO
#define CFG_SWIN_LID_TOP			PIN_PB6		//开表盖信号对应IO
#define CFG_SWIN_LID_MID			PIN_PB7		//开中盖信号对应IO
#define CFG_SWIN_LID_TAIL			PIN_PB5		//开尾盖信号对应IO

#define CFG_SWIN_NUM				1			//遥信与脉冲量数量
#define CFG_SWIN_0					PIN_PB11		//0号遥信或脉冲通道对应的io口
//#define CFG_SWIN_1					PIN_PA5		//1号遥信或脉冲通道对应的io口
//#define CFG_SWIN_2					PIN_PA6		//2号遥信或脉冲通道对应的io口
//#define CFG_SWIN_3					PIN_PA7		//3号遥信或脉冲通道对应的io口
//#define CFG_SWIN_4					PIN_PB8		//4号遥信或脉冲通道对应的io口
//#define CFG_SWIN_5					PIN_PB9		//1号遥信或脉冲通道对应的io口
//#define CFG_SWIN_6					PIN_PB10	//2号遥信或脉冲通道对应的io口
//#define CFG_SWIN_7					PIN_PB11	//3号遥信或脉冲通道对应的io口

#define CFG_SWIN_TIMER				5			//用于产生采样间隔的定时器号
#define CFG_SWIN_FILTER_TIME		5			//消抖采样间隔（单位ms）
#define CFG_SWIN_FILTER_NUM			5			//消抖采样次数

//键盘模块配置
#define CFG_KEY_MODULE			1				//是否启用
#define CFG_KEY_NUM				2				//当前KEY通道数量

#define CFG_KEY_LONG			3000			//长按开始响应间隔（以ms为基本单位）。注意不要大于按键获取api的超时。
#define CFG_KEY_PER				500				//长按返回间隔（以ms为基本单位）。注意不要小于按键扫描时间
#define CFG_KEY_SCAN_DELAY		50				//按键扫描延时，单位ms，推荐20~100ms间。
#define CFG_KEY_MODE			1				//按键响应模式，1-按下返回，0-释放返回

#define CFG_KEY_1				PIN_PB11		//1号通道对应的硬件通道
#define CFG_KEY_TYPE1			0				//IO高低与按键是否按下之间的联系，1高按下，0低按下

#define CFG_KEY_2				PIN_PB27		//2号通道对应的硬件通道
#define CFG_KEY_TYPE2			1				//IO高低与按键是否按下之间的联系，1高按下，0低按下

//#define CFG_KEY_3				PIN_PB3			//3号通道对应的硬件通道
//#define CFG_KEY_TYPE3			1				//IO高低与按键是否按下之间的联系，1高按下，0低按下
//
//#define CFG_KEY_4				PIN_PB4			//4号通道对应的硬件通道
//#define CFG_KEY_TYPE4			1				//IO高低与按键是否按下之间的联系，1高按下，0低按下
//
//#define CFG_KEY_5				PIN_PB5			//5号通道对应的硬件通道
//#define CFG_KEY_TYPE5			1				//IO高低与按键是否按下之间的联系，1高按下，0低按下
//
//#define CFG_KEY_6				PIN_PB6			//6号通道对应的硬件通道
//#define CFG_KEY_TYPE6			1				//IO高低与按键是否按下之间的联系，1高按下，0低按下
//
//#define CFG_KEY_7				PIN_PB7			//7号通道对应的硬件通道
//#define CFG_KEY_TYPE7			1				//IO高低与按键是否按下之间的联系，1高按下，0低按下

//直流模拟量模块配置
#define CFG_DCSIG_MODULE				1			//是否启用
#define CFG_DCSIG_NUM					4			//当前通道数量
#define CFG_DCSIG_BIT					10			//ad转换位数
#define CFG_DCSIG_REFL					0			//参考电压低
#define CFG_DCSIG_REFH					2			//参考电压高

#endif /* _CONFIG_H */
