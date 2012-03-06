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
 * 项目名称		:	rtc驱动应用层头文件
 * 文件名		:	rtclib.h
 * 描述			:	应用ds3231 rtc驱动程序所用到的数据结构及ioctl函数cmd命令
 * 版本			:	1.0.1
 * 作者			:	路冉冉
 *
 * 修改历史记录	:
 * --------------------
 * 01a, 18aug2009, Roy modified
 * --------------------
 *
 ******************************************************************************/

#ifndef _RTCLIB_H
#define _RTCLIB_H


/*
 * The struct used to pass data via the following ioctl. Similar to the
 * struct tm in <time.h>, but it needs to be here so that the kernel
 * source is self contained, allowing cross-compiles, etc. etc.
 */
 
struct rtc_time {
	int tm_sec;		 //秒：[0 - 59]
	int tm_min;		 //分钟：[0 - 59]
	int tm_hour;		 //时：[0 - 23]
	int tm_mday;		 //日：[1 - 31]
	int tm_mon;		 //月，自从一月以来的月份：[0 - 11]
	int tm_year;		 //年，自从1900年以来的年:
	int tm_wday;		 //周：距离周日的天数[0 - 6]
	int tm_yday;
	int tm_isdst;
};

/*	 Rtc_time 结构体使用注意：
	 Tm_mon 为距离1月的月份，比如tm_mon = 8 代表着现在是9月；
	 tm_year为距离1900年的年数，比如tm_year = 109代表现在是2009年。
*/



#define RTC_RD_TIME		_IOR('p', 0x09, struct rtc_time) /* Read RTC time   */
#define RTC_SET_TIME	_IOW('p', 0x0a, struct rtc_time) /* Set RTC time    */
#define RTC_GET_STAT	_IOR('p', 0x19, unsigned char) 			/* Read RTC status   */
#endif  /* _RTCLIB_H */

