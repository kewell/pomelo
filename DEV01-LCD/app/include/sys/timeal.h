/**
* timeal.h -- 时钟操作接口头文件
* 
* 
* 创建时间: 2008-5-16
* 最后修改时间: 2010-4-23
*/


#ifndef _SYS_TIMEAL_H
#define _SYS_TIMEAL_H

///从2000.1.1 0:0 开始的秒数
#define utime_t   int 

///系统时间结构
typedef struct {
	unsigned char year;    // year - 2000
	unsigned char month;    // 1~12
	unsigned char day;     // 1~31
	unsigned char hour;  // 0~23
	unsigned char minute;  // 0~59
	unsigned char second;  // 0~59
	unsigned char week;  //from sunday, 0~6
	unsigned char unuse;
} sysclock_t;

/**
* @brief 读取系统时间
* @param pclock 读取的时间值变量指针
* @return 成功时返回0, 否则返回非零值
*/
int SysClockRead(sysclock_t *pclock);
/**
* @brief 设置系统时间
* @param pclock 设置的时间值变量指针
* @return 成功时返回0, 否则返回非零值
*/
int SysClockSet(const sysclock_t *pclock);

/*
* @brief 将系统时间转换为utime_t格式
* @param ptime 系统时间值变量指针
* @return 成功时返回对应的utime_t时间, 否则返回-1
*/
utime_t SysClockToUTime(const sysclock_t *ptime);
/**
* @brief 将utime_t格式转换为系统时间
* @param utime 需转换utime_t时间变量
* @param ptime 转换后的系统时间值变量指针
*/
void UTimeToSysClock(utime_t utime, sysclock_t *ptime);

/**
* @brief 比较2个系统时间的差异
* @param ptime1 第一个系统时间变量指针
* @param ptime2 第二个系统时间变量指针
* @return 
*   2个时间之间的差异, 以秒为单位
*   <0 表示ptime1早于ptime2
*   =0 表示2个时间相同
*   >0 表示ptime1晚于ptime2
*/
int SysClockDifference(const sysclock_t *ptime1, const sysclock_t *ptime2);

///时间间隔格式
#define UTIMEDEV_MINUTE		0  //分
#define UTIMEDEV_HOUR		1  //小时
#define UTIMEDEV_DAY		2  //天
#define UTIMEDEV_MONTH		3  //月
/**
* @brief 在原有时间上增加一段时间
* @param time 原始时间变量
* @param mod 增加的时间数值
* @param dev 增加的时间单位
*     UTIMEDEV_MINUTE -- 分
*     UTIMEDEV_HOUR -- 小时
*     UTIMEDEV_DAY -- 天
*     UTIMEDEV_MONTH -- 月
* @return 增加后的时间值
*/
utime_t UTimeAdd(utime_t time, int mod, int dev);

/**
* @brief 得到当前系统时间
*   这个函数返回定时器任务定时更新的当前时间缓存值, 因此要远快于SysClockRead()
* @param pclock 储存时间的变量指针
*/
void SysClockReadCurrent(sysclock_t *pclock);

/**
* @brief 得到当前系统时间(utime_t格式)
*   这个函数返回定时器任务定时更新的当前时间缓存值, 因此要远快于SysClockRead()
* @return 当前系统时间(utime_t格式)
*/
utime_t UTimeReadCurrent(void);

/**
* 注意: XXFormat函数使用同一个缓存区,因此不要同时调用
*/

/**
* @brief 将utime_t时间转换为ascii字符串
* @param time 输入的时间
* @return 时间字符串指针
*/
const char *UTimeFormat(utime_t time);
/**
* @brief 将sysclock_t时间转换为ascii字符串
* @param clock 输入的时间
* @return 时间字符串指针
*/
const char *SysClockFormat(const sysclock_t *pclock);

/**
* @brief 开始测量时间
*/
void StartTimeMeasure(void);
/**
* @brief 停止测量时间
* @return 从开始测量到停止经过的毫秒数
*/
int StopTimeMeasure(void);
/**
* @brief 停止测量时间(格式化返回)
* @return 从开始测量到停止经过的毫秒数(字符串形式)
*/
const char *StopTimeMeasureFormat(void);

/**
* @brief 获得系统启动时间
* @param clock 时钟变量指针
*/
void GetClockSysStart(sysclock_t *clock);

/**
* @brief 读取外部时钟
* @param clock 返回时钟变量指针
* @return 返回0表示成功, 否则失败
*/
int ReadExternSysClock(sysclock_t *clock);

#endif /*_SYS_TIMEAL_H*/
