/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：framework.c
	描述		：本文件实现了框架初始化接口
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.12
******************************************************************************/

//业务平台配置头文件
#include "framework/config.h"

//C库头文件

//基础平台头文件
#include "sge_core/msg.h"
#include "sge_core/thread.h"
#include "sge_core/error.h"

//业务平台头文件
#include "framework/debug.h"
#include "framework/framework.h"
#include "framework/systime.h"
#include "framework/message.h"
#include "framework/dbserver.h"

/*************************************************
  宏定义
*************************************************/


/*************************************************
  结构类型定义
*************************************************/



/*************************************************
  静态全局变量定义
*************************************************/
static struct {
	u8 state;
	u8 prio;
} thread_createinfo[CFG_FRAMEWORK_THREAD_MAX];



/*************************************************
  API函数实现
*************************************************/
//平台线程函数
static void* framework_thread (void *arg)
{
	u8 id;
	msg_t msg;
	struct BASE *obj;

	id = *(u8 *)arg;
	while(1) {
		if (msg_recv (id, &msg, 0)) {
			//错误处理
		}
		//dispatch的消息处理
		if (0 != msg.priv) {
			obj = (struct BASE *)msg.priv;
			obj->baseft->msghandle (obj, (message_t *)&msg);
		}
		//非dispatch的消息priv必须为0
		else {
			//直接发送的消息处理，目前还不支持，直接丢弃
		}
	}
}

/******************************************************************************
*	函数:	framework_init
*	功能:	平台框架初始化（时间管理、消息调度、数据访问；设备部分的初始化在业务模块调用）
*	参数:
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
 			-ERR_BUSY		- 	模块已初始化
			-ERR_NODEV		-	无此设备
*	说明:	此函数由main.c调用。
 ******************************************************************************/
int framework_init (void)
{
	int ret = 0;

	//时间管理模块
	ret = systime_init();
	if (ret) {
		goto error;
	}
	//消息调度模块
	ret = message_init();
	if (ret) {
		goto error;
	}
	//数据访问模块
	ret = dbserver_init();
	if (ret) {
		goto error;
	}

error:
	return ret;
}


/******************************************************************************
*	函数:	framework_start
*	功能:	平台框架启动（建立各业务模块对应的线程）
*	参数:
*	返回:	0				-	成功
 			-ERR_SYS		-	系统错误
*	说明:	此函数由main.c调用。
 ******************************************************************************/
int framework_start (void)
{
	int ret = 0;
	int i;
	u8 msgid;

	//根据业务模块的注册情况建立业务模块线程
	for (i=0; i<CFG_FRAMEWORK_THREAD_MAX; i++) {
		//状态为1表示需要建立线程
		if (1 == thread_createinfo[i].state) {
			msgid = i;
			//实时线程
			if (0 != thread_createinfo[i].prio){
				ret = thread_create(i, framework_thread, &msgid, THREAD_MODE_REALTIME, thread_createinfo[i].prio);
				if (ret) {
					goto error;
				}
			}
			//普通线程
			else {
				ret = thread_create(i, framework_thread, &msgid, THREAD_MODE_NORMAL, 0);
				if (ret) {
					goto error;
				}
			}
		}
	}

error:
	return ret;
}


/******************************************************************************
*	函数:	module_register
*	功能:	业务模块注册（为了获得业务模块请求的线程信息）
*	参数:	obj				-	业务模块对象指针
*	返回:	0				-	成功
 			-ERR_INVAL		-	参数错误
*	说明:
 ******************************************************************************/
int module_register (struct BASE *obj)
{
	int ret = 0;
	//参数有效性判断
	if ((obj->thread >= CFG_FRAMEWORK_THREAD_MAX) || (obj->prio > 99)) {
		ret = -ERR_INVAL;
		goto error;
	}
	//填入待建线程信息
	if (thread_createinfo[obj->thread].state == 0) {
		thread_createinfo[obj->thread].prio = obj->prio;
		thread_createinfo[obj->thread].state = 1;
	}
	else {
		if (obj->prio > thread_createinfo[obj->thread].prio) {
			thread_createinfo[obj->thread].prio = obj->prio;
		}
	}
error:
	return ret;
}

	
	
	
	
	
	
