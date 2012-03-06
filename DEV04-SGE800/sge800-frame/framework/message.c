/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：message.c
	描述		：本文件实现了消息调度模块
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.11
******************************************************************************/

//业务平台配置头文件
#include "framework/config.h"

//C库头文件
#include <pthread.h>

//基础平台头文件
#include "sge_core/msg.h"
#include "sge_core/error.h"

//业务平台头文件
#include "framework/message.h"
#include "framework/list.h"

/*************************************************
  结构类型定义
*************************************************/
typedef struct {
	struct list_head list;
	struct BASE *obj;
} sub_node_t;

/*************************************************
  静态全局变量定义
*************************************************/
static struct {
	pthread_mutex_t mutex;
	struct list_head list_busy;	//有效订阅信息链表头
	struct list_head list_idle;	//空闲订阅信息链表头
	sub_node_t nodepool[CFG_MESSAGE_SUBSCRIBE_MAX];
} subinfo[CFG_MESSAGE_TYPE_MAX];



/*************************************************
  API函数实现
*************************************************/
/******************************************************************************
*	函数:	message_init
*	功能:	消息调度模块初始化
*	参数:
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
			-ERR_BUSY		- 	模块已初始化
*	说明:	此函数由framework_init调用。
 ******************************************************************************/
int message_init (void)
{
	int ret = 0;
	int i,j;
	//基础平台消息服务模块初始化
	ret = msg_init();
	if(ret) {
		goto error;
	}
	for (i=0; i<CFG_MESSAGE_TYPE_MAX; i++) {
		//互斥量初始化
		if (pthread_mutex_init(&subinfo[i].mutex, NULL)) {
			ret = -ERR_SYS;
			goto error;
		}
		//链表初始化，把所有订阅信息空间添加入空闲链表中
		INIT_LIST_HEAD(&subinfo[i].list_busy);
		INIT_LIST_HEAD(&subinfo[i].list_idle);
		for (j=0; j<CFG_MESSAGE_SUBSCRIBE_MAX; j++) {
			list_add(&subinfo[i].nodepool[j].list, &subinfo[i].list_idle);
		}
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	message_subscribe
*	功能:	订阅消息
*	参数:	obj				-	业务模块对象指针
			type			-	消息类型枚举值
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
			-ERR_INVAL		- 	无效参数
			-ERR_NOMEM		- 	无可用订阅空间
*	说明:
 ******************************************************************************/
int message_subscribe (struct BASE *obj, u16 type)
{
	int ret = 0;
	sub_node_t *pnode;	//链表管理的结构指针
	struct list_head *plist_busy;	//忙链表头
	struct list_head *plist_idle;	//闲链表头

	//参数有效性判断
	if ((NULL == obj) || (type >= CFG_MESSAGE_TYPE_MAX)) {
		ret = -ERR_INVAL;
		goto error;
	}
	//获得互斥锁
	if (pthread_mutex_lock (&subinfo[type].mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	//写入订阅信息
	plist_busy = &subinfo[type].list_busy;
	plist_idle = &subinfo[type].list_idle;
	if (list_empty(plist_idle)) {
		ret = -ERR_NOMEM;
		goto error1;
	}
	pnode = list_first_entry(plist_idle, sub_node_t, list);
	pnode->obj = obj;
	list_del (&pnode->list);
	list_add (&pnode->list, plist_busy);

error1:
	//解锁
	if (pthread_mutex_unlock (&subinfo[type].mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	message_unsubscribe
*	功能:	取消订阅消息
*	参数:	obj				-	业务模块对象指针
			type			-	消息类型枚举值
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
			-ERR_INVAL		- 	无效参数
			-ERR_NORECORD	- 	此消息未订阅
*	说明:
 ******************************************************************************/
int message_unsubscribe (struct BASE *obj, u16 type)
{
	int ret = 0;
	int flag = 0;
	sub_node_t *pnode;
	struct list_head *plist_busy;
	struct list_head *plist_idle;

	//参数有效性判断
	if ((NULL == obj) || (type >= CFG_MESSAGE_TYPE_MAX)) {
		ret = -ERR_INVAL;
		goto error;
	}
	//获得互斥锁
	if (pthread_mutex_lock (&subinfo[type].mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	//查找并删除订阅信息
	plist_busy = &subinfo[type].list_busy;
	plist_idle = &subinfo[type].list_idle;
	list_for_each_entry(pnode, plist_busy, list) {
		if (pnode->obj == obj) {
			flag = 1;
			break;
		}
	}
	if (flag) {
		list_del (&pnode->list);
		list_add (&pnode->list, plist_idle);
	}
	else {
		ret = -ERR_NORECORD;
		goto error1;
	}
error1:
	//解锁
	if (pthread_mutex_unlock (&subinfo[type].mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}

/******************************************************************************
*	函数:	message_dispatch
*	功能:	派发消息
*	参数:	msg				-	消息内容
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
			-ERR_INVAL		- 	无效参数
 			-ERR_NOMEM		-	消息队列满
*	说明:
 ******************************************************************************/
int message_dispatch (message_t *msg)
{
	int ret = 0;
	u16 type;
	u8 msgid;	//消息队列号
	msg_t tmpmsg;		//用于底层消息服务的消息内容
	sub_node_t *pnode;
	struct list_head *plist_busy;

	type = msg->type;
	//参数有效性判断
	if (type >= CFG_MESSAGE_TYPE_MAX) {
		ret = -ERR_INVAL;
		goto error;
	}
	//获得互斥锁
	if (pthread_mutex_lock (&subinfo[type].mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
	//根据订阅信息派发消息
	plist_busy = &subinfo[type].list_busy;
	tmpmsg.type = msg->type;
	tmpmsg.wpara = msg->wpara;
	tmpmsg.lpara = msg->lpara;
	list_for_each_entry(pnode, plist_busy, list) {
		msgid = pnode->obj->thread;
		tmpmsg.priv = (u32)(pnode->obj);
		ret = msg_send(msgid, &tmpmsg, 0);
		if (ret) {
			goto error1;
		}
	}
error1:
	//解锁
	if (pthread_mutex_unlock (&subinfo[type].mutex)) {
		ret = -ERR_SYS;
		goto error;
	}
error:
	return ret;
}
