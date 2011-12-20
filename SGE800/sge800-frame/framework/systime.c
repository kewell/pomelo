/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：systime.c
	描述		：本文件实现了系统时间模块
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.11
******************************************************************************/

//业务平台配置头文件
#include "framework/config.h"

//C库头文件
#include <pthread.h>
#include <string.h>

//基础平台头文件
#include "sge_core/msg.h"
#include "sge_core/rtc.h"
#include "sge_core/timer.h"
#include "sge_core/thread.h"
#include "sge_core/error.h"

//业务平台头文件
#include "framework/debug.h"
#include "framework/systime.h"
#include "framework/message.h"
#include "framework/list.h"

/*************************************************
  宏定义
*************************************************/
//是否变位标识
#define FLAG_SEC	(1 << 0)
#define FLAG_MIN	(1 << 1)
#define FLAG_HOUR	(1 << 2)
#define FLAG_DAY    (1 << 3)
#define FLAG_MON    (1 << 4)
#define FLAG_YEAR   (1 << 5)

/*************************************************
  结构类型定义
*************************************************/
typedef struct {
	struct list_head list;
	struct BASE *obj;
	int handle;
	union {
		u8 period;
		st_ymdhms_t clock;
		struct {
			u32 sec;
			u32 cnt;
			u8 num;
		} timer;
	} value;
} timer_node_t;


/*************************************************
  静态全局变量定义
*************************************************/
//系统时间
static struct {
	pthread_mutex_t mutex;
	st_ymdhmsw_t time;
} systime;

//时间注册信息
static pthread_mutex_t timer_mutex;
static timer_node_t nodepool[CFG_SYSTIME_TIMER_MAX];
static struct list_head list_period_sec;		//秒周期类链表头
static struct list_head list_period_min;		//分周期类链表头
static struct list_head list_period_hour;		//时周期类链表头
static struct list_head list_period_day;		//日周期类链表头
static struct list_head list_period_mon;		//月周期类链表头
static struct list_head list_period_year;		//年周期类链表头
static struct list_head list_clock;				//绝对时刻到达类链表头
static struct list_head list_timer;				//倒计时类链表头
static struct list_head list_idle;				//空闲链表头

//每个月的天数与月份的对应关系表
static u8 monthday[2][13] = {
	[0] = {0xff, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	[1] = {0xff, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

/*************************************************
  API函数实现
*************************************************/
static void systime_add_sec (u8 *flag)
{
	*flag = 0;
	systime.time.sec++;
	*flag = *flag | FLAG_SEC;
	if (systime.time.sec > 59) {
		systime.time.sec = 0;
		systime.time.min++;
		*flag = *flag | FLAG_MIN;
		if (systime.time.min > 59) {
			systime.time.min = 0;
			systime.time.hour++;
			*flag = *flag | FLAG_HOUR;
			if (systime.time.hour > 23) {
				systime.time.hour = 0;
				systime.time.day++;
				systime.time.wday++;
				*flag = *flag | FLAG_DAY;
				if (systime.time.wday > 7) {
					systime.time.wday = 1;
				}
				if (systime.time.day > monthday[(!(systime.time.year % 4)) && (systime.time.year % 100)][systime.time.mon]) {
					systime.time.day = 1;
					systime.time.mon++;
					*flag = *flag | FLAG_MON;
					if (systime.time.mon > 12) {
						systime.time.mon = 1;
						systime.time.year++;
						*flag = *flag | FLAG_YEAR;
					}
				}
			}
		}
	}
}

//时间管理线程函数
static void* systime_thread (void *arg)
{
	int ret;
	u8 flag;		//时间变位标志
	timer_node_t *pnode;
	timer_node_t *pnode_del;
	msg_t tmpmsg;

	tmpmsg.type = MSG_TIME;

	//初始化定时器
	ret = timer_init (TIMER0, TIMER_MODE_HEART);
	if (ret) {
		goto error;
	}
	ret = timer_heart_setconfig (TIMER0, 1000);	//1000ms定时
	if (ret) {
		goto error;
	}
	ret = timer_heart_start(TIMER0);
	if (ret) {
		goto error;
	}
	//线程循环开始
	while(1) {
		//等待定时器
		ret = timer_heart_wait(TIMER0);
		if (ret) {
			goto error;
		}
		//获得系统时间锁
		if (pthread_mutex_lock (&systime.mutex)) {
			ret = -ERR_SYS;
			goto error;
		}
		//更新系统时钟并获得时间变位标志
		systime_add_sec(&flag);
		//获得时间消息注册锁
		if (pthread_mutex_lock (&timer_mutex)) {
			ret = -ERR_SYS;
			goto error;
		}
		//发送时间消息
		if (flag & FLAG_SEC) {
			//秒周期时间消息
			list_for_each_entry(pnode, &list_period_sec, list) {
				if (0 == (systime.time.sec % pnode->value.period)) {
					tmpmsg.lpara = pnode->handle;
					tmpmsg.priv = (u32)(pnode->obj);
					ret = msg_send(pnode->obj->thread, &tmpmsg, 0);
					if (ret) {
						goto error;
					}
				}
			}
			//绝对时间到达消息
			pnode_del = NULL;
			list_for_each_entry(pnode, &list_clock, list) {
				if (pnode_del) {
					list_del (&pnode_del->list);
					list_add (&pnode_del->list, &list_idle);
				}
				if (0 == memcmp((char *)&systime.time, (char *)&pnode->value.clock, 6)) {//st_ymdhms_t类型为8字节含有2个对齐空缺
					tmpmsg.lpara = pnode->handle;
					tmpmsg.priv = (u32)(pnode->obj);
					ret = msg_send(pnode->obj->thread, &tmpmsg, 0);
					if (ret) {
						goto error;
					}
					pnode_del = pnode;
				}
			}
			if (pnode_del) {
				list_del (&pnode_del->list);
				list_add (&pnode_del->list, &list_idle);
			}
			//倒计时到达消息
			pnode_del = NULL;
			list_for_each_entry(pnode, &list_timer, list) {
				if (pnode_del) {
					list_del (&pnode_del->list);
					list_add (&pnode_del->list, &list_idle);
				}
				pnode->value.timer.cnt--;
				if (0 == pnode->value.timer.cnt) {
					tmpmsg.lpara = pnode->handle;
					tmpmsg.priv = (u32)(pnode->obj);
					ret = msg_send(pnode->obj->thread, &tmpmsg, 0);
					if (ret) {
						goto error;
					}
					//num为0xFF表示无限制次数倒计时
					if (0xFF != pnode->value.timer.num) {
						pnode->value.timer.num--;
						if (0 == pnode->value.timer.num) {
							pnode_del = pnode;
						}
						else {
							pnode->value.timer.cnt = pnode->value.timer.sec;
						}
					}
				}
			}
			if (pnode_del) {
				list_del (&pnode_del->list);
				list_add (&pnode_del->list, &list_idle);
			}
			//分周期时间消息
			if (flag & FLAG_MIN) {
				list_for_each_entry(pnode, &list_period_min, list) {
					if (0 == (systime.time.min % pnode->value.period)) {
						tmpmsg.lpara = pnode->handle;
						tmpmsg.priv = (u32)(pnode->obj);
						ret = msg_send(pnode->obj->thread, &tmpmsg, 0);
						if (ret) {
							goto error;
						}
					}
				}
				//时周期时间消息
				if (flag & FLAG_HOUR) {
					list_for_each_entry(pnode, &list_period_hour, list) {
						if (0 == (systime.time.hour % pnode->value.period)) {
							tmpmsg.lpara = pnode->handle;
							tmpmsg.priv = (u32)(pnode->obj);
							ret = msg_send(pnode->obj->thread, &tmpmsg, 0);
							if (ret) {
								goto error;
							}
						}
					}
					//日周期时间消息
					if (flag & FLAG_DAY) {
						list_for_each_entry(pnode, &list_period_day, list) {
							tmpmsg.lpara = pnode->handle;
							tmpmsg.priv = (u32)(pnode->obj);
							ret = msg_send(pnode->obj->thread, &tmpmsg, 0);
							if (ret) {
								goto error;
							}
						}
						//月周期时间消息
						if (flag & FLAG_MON) {
							list_for_each_entry(pnode, &list_period_mon, list) {
								tmpmsg.lpara = pnode->handle;
								tmpmsg.priv = (u32)(pnode->obj);
								ret = msg_send(pnode->obj->thread, &tmpmsg, 0);
								if (ret) {
									goto error;
								}
							}
							//年周期时间消息
							if (flag & FLAG_YEAR) {
								list_for_each_entry(pnode, &list_period_year, list) {
									tmpmsg.lpara = pnode->handle;
									tmpmsg.priv = (u32)(pnode->obj);
									ret = msg_send(pnode->obj->thread, &tmpmsg, 0);
									if (ret) {
										goto error;
									}
								}
							}
						}
					}
				}
			}
		}
		//解时间消息注册锁
		if (pthread_mutex_unlock (&timer_mutex)) {
			ret = -ERR_SYS;
			goto error;
		}
		//解系统时间锁
		if (pthread_mutex_unlock (&systime.mutex)) {
			ret = -ERR_SYS;
			goto error;
		}
	}
error:
	while(1);
}

/******************************************************************************
*	函数:	systime_init
*	功能:	系统时间模块初始化
*	参数:
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
			-ERR_BUSY		- 	模块已初始化
			-ERR_INVAL		-	参数错误
			-ERR_NODEV		-	无此设备
*	说明:	此函数由framework_init调用。
 ******************************************************************************/
int systime_init (void)
{
	int ret = 0;
	int i;
	pthread_t tid;
	st_ymdhmsw_t tmptime;
//第一步：系统时间初始化
	if (pthread_mutex_init(&systime.mutex, NULL)) {
		ret = -ERR_SYS;
		goto error;
	}
	//读取RTC时间并初始化系统时间
	ret = rtc_init();
	if (ret) {
		goto error;
	}
	ret = rtc_gettime (&tmptime);
	if (ret) {
		goto error;
	}
	ret = systime_set (&tmptime);
	if (ret) {
		goto error;
	}
//第二步：时间消息注册机制初始化
	if (pthread_mutex_init(&timer_mutex, NULL)) {
		ret = -ERR_SYS;
		goto error;
	}
	INIT_LIST_HEAD(&list_period_sec);
	INIT_LIST_HEAD(&list_period_min);
	INIT_LIST_HEAD(&list_period_hour);
	INIT_LIST_HEAD(&list_period_day);
	INIT_LIST_HEAD(&list_period_mon);
	INIT_LIST_HEAD(&list_period_year);
	INIT_LIST_HEAD(&list_clock);
	INIT_LIST_HEAD(&list_timer);
	INIT_LIST_HEAD(&list_idle);
	for (i=0; i<CFG_SYSTIME_TIMER_MAX; i++) {
		nodepool[i].handle = i;
		list_add(&nodepool[i].list, &list_idle);
	}
//第三步：建立时间管理线程
	ret = thread_create_base (&tid, systime_thread, NULL, THREAD_MODE_REALTIME, 90);
	if (ret) {
		goto error;
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	systime_get
*	功能:	读取系统时间
*	参数:	time			-	时间（数据传出）
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
*	说明:
 ******************************************************************************/
int systime_get (st_ymdhmsw_t *time)
{
	int ret = 0;
	//获得互斥锁
	if (pthread_mutex_lock (&systime.mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	memcpy ((char *)time, (char *)&systime.time, sizeof(st_ymdhmsw_t));
	//解锁
	if (pthread_mutex_unlock (&systime.mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	systime_set
*	功能:	设置系统时间
*	参数:	time			-	时间（数据传入）
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数错误
*	说明:
 ******************************************************************************/
int systime_set (st_ymdhmsw_t *time)
{
	int ret = 0;

	//参数有效性判断
	if ((time->mon < 1)||(time->mon > 12)||(time->day < 1)||(time->day > 31)||(time->hour > 23)||(time->min > 59)||(time->sec > 59)||(time->wday < 1)||(time->wday >7)) {
		ret = -ERR_INVAL;
		goto error;
	}
	//获得互斥锁
	if (pthread_mutex_lock (&systime.mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	//复制时间
	memcpy ((char *)&systime.time, (char *)time, sizeof(st_ymdhmsw_t));
	//解锁
	if (pthread_mutex_unlock (&systime.mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	systime_register_period
*	功能:	注册周期性定时消息
*	参数:	obj				-	业务模块对象指针
			type			-	周期类型（秒、分、日...）
			period			-	周期
*	返回:	>=0				-	时间消息句柄
 			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数错误
			-ERR_NOMEM		-	无可用注册空间
*	说明:
 ******************************************************************************/
int systime_register_period (struct BASE *obj, u8 type, u8 period)
{
	int ret = 0;
	timer_node_t *pnode;
	//获得互斥锁
	if (pthread_mutex_lock (&timer_mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	//获取空闲队列里一个节点
	if (list_empty(&list_idle)) {
		ret = -ERR_NOMEM;
		goto error1;
	}
	pnode = list_first_entry(&list_idle, timer_node_t, list);
	//根据不同类型添加时间消息注册信息
	switch (type) {
	case SYSTIME_REGISTER_PERIOD_SEC:
		if ((period < 1) || (period > 59) || (0 != 60%period)) {
			ret = -ERR_INVAL;
			goto error1;
		}
		pnode->obj = obj;
		pnode->value.period = period;
		list_del (&pnode->list);
		list_add (&pnode->list, &list_period_sec);
		break;
	case SYSTIME_REGISTER_PERIOD_MIN:
		if ((period < 1) || (period > 59) || (0 != 60%period)) {
			ret = -ERR_INVAL;
			goto error1;
		}
		pnode->obj = obj;
		pnode->value.period = period;
		list_del (&pnode->list);
		list_add (&pnode->list, &list_period_min);
		break;
	case SYSTIME_REGISTER_PERIOD_HOUR:
		if ((period < 1) || (period > 23) || (0 != 24%period)) {
			ret = -ERR_INVAL;
			goto error1;
		}
		pnode->obj = obj;
		pnode->value.period = period;
		list_del (&pnode->list);
		list_add (&pnode->list, &list_period_hour);
		break;
	case SYSTIME_REGISTER_PERIOD_DAY:
		if (period != 1) {
			ret = -ERR_INVAL;
			goto error1;
		}
		pnode->obj = obj;
		pnode->value.period = period;
		list_del (&pnode->list);
		list_add (&pnode->list, &list_period_day);
		break;
	case SYSTIME_REGISTER_PERIOD_MON:
		if (period != 1) {
			ret = -ERR_INVAL;
			goto error1;
		}
		pnode->obj = obj;
		pnode->value.period = period;
		list_del (&pnode->list);
		list_add (&pnode->list, &list_period_mon);
		break;
	case SYSTIME_REGISTER_PERIOD_YEAR:
		if (period != 1) {
			ret = -ERR_INVAL;
			goto error1;
		}
		pnode->obj = obj;
		pnode->value.period = period;
		list_del (&pnode->list);
		list_add (&pnode->list, &list_period_year);
		break;
	default:
		ret = -ERR_INVAL;
		goto error1;
	}
	ret = pnode->handle;
error1:
	//解锁
	if (pthread_mutex_unlock (&timer_mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	systime_register_clock
*	功能:	注册时间到达消息
*	参数:	obj				-	业务模块对象指针
			time			-	到达时间（数据传入）
*	返回:	>=0				-	时间消息句柄
 			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数错误
			-ERR_NOMEM		-	无可用注册空间
*	说明:
 ******************************************************************************/
int systime_register_clock (struct BASE *obj, st_ymdhms_t *time)
{
	int ret = 0;
	timer_node_t *pnode;

	//参数有效性判断
	if ((time->mon < 1)||(time->mon > 12)||(time->day < 1)||(time->day > 31)||(time->hour > 23)||(time->min > 59)||(time->sec > 59)) {
		ret = -ERR_INVAL;
		goto error;
	}
	//获得互斥锁
	if (pthread_mutex_lock (&timer_mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	//获取空闲队列里一个节点
	if (list_empty(&list_idle)) {
		ret = -ERR_NOMEM;
		goto error1;
	}
	pnode = list_first_entry(&list_idle, timer_node_t, list);
	//添加时间消息注册信息
	memcpy ((char *)&pnode->value, (char *)time, sizeof(st_ymdhms_t));
	pnode->obj = obj;
	list_del (&pnode->list);
	list_add (&pnode->list, &list_clock);
	ret = pnode->handle;
error1:
	//解锁
	if (pthread_mutex_unlock (&timer_mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	systime_register_timer
*	功能:	注册倒计时到达消息
*	参数:	obj				-	业务模块对象指针
			sec				-	倒计时到达秒数
			num				-	倒计时次数（0xff表示无限次）
*	返回:	>=0				-	时间消息句柄
 			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数错误
			-ERR_NOMEM		-	无可用注册空间
*	说明:
 ******************************************************************************/
int systime_register_timer (struct BASE *obj, u32 sec, u8 num)
{
	int ret = 0;
	timer_node_t *pnode;

	//参数有效性判断
	if ((0 == sec) || (0 == num)) {
		ret = -ERR_INVAL;
		goto error;
	}
	//获得互斥锁
	if (pthread_mutex_lock (&timer_mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	//获取空闲队列里一个节点
	if (list_empty(&list_idle)) {
		ret = -ERR_NOMEM;
		goto error1;
	}
	pnode = list_first_entry(&list_idle, timer_node_t, list);
	//添加时间消息注册信息
	pnode->value.timer.sec = sec;
	pnode->value.timer.cnt = sec;
	pnode->value.timer.num = num;
	pnode->obj = obj;
	list_del (&pnode->list);
	list_add (&pnode->list, &list_timer);
	ret = pnode->handle;
error1:
	//解锁
	if (pthread_mutex_unlock (&timer_mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	systime_unregister
*	功能:	取消时间消息注册
*	参数:	handle			-	时间消息句柄
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
*	说明:
 ******************************************************************************/
int systime_unregister (int handle)
{
	int ret = 0;
	timer_node_t *pnode;

	//获得互斥锁
	if (pthread_mutex_lock (&timer_mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	//得到待取消的节点
	pnode = &nodepool[handle];
	//添加时间消息注册信息
	list_del (&pnode->list);
	list_add (&pnode->list, &list_idle);
	//解锁
	if (pthread_mutex_unlock (&timer_mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}
