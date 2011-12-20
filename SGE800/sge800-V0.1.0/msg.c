/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/******************************************************************************
	项目名称	：SGE800计量智能终端平台
	文件		：msg.c
	描述		：本文件实现了消息服务模块中的API函数
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2009.12
******************************************************************************/
//库配置头文件
#include "private/config.h"

//模块启用开关
#ifdef CFG_MSG_MODULE

//调试头文件
#include "private/debug.h"

//驱动调用头文件

//C库头文件
#include <stdio.h>                      //printf
#include <semaphore.h>                  //sem_t
#include <string.h>
#include <pthread.h>                    //pthread_mutex


//提供给用户的头文件
#include "include/msg.h"
#include "include/error.h"


/*************************************************
  静态全局变量定义
*************************************************/
#define CFG_MSG_REALSIZE    (CFG_MSG_SIZE + 1)

typedef struct {
    msg_t msg;
    u8 prio;
} msg_prio_t;

static int msg_inited;

static struct {
    sem_t sem;
    pthread_mutex_t mutex;
    u32 head;
    u32 tail;
    msg_prio_t message[CFG_MSG_REALSIZE];
} msg_info[CFG_MSG_MAX];

/*************************************************
  API函数实现
*************************************************/
/******************************************************************************
*	函数:	msg_init
*	功能:	消息服务模块初始化
*	参数:	无
*	返回:	0				-	成功
 			-ERR_SYS		- 	系统异常
 			-ERR_BUSY		- 	模块已初始化
*	说明:
 ******************************************************************************/
int msg_init(void)
{
    int ret = 0;
    int i;

    if (0 != msg_inited) {
        ret = -ERR_BUSY;
        goto error;
    }
//初始化信号量和互斥锁
    for (i=0; i<CFG_MSG_MAX; i++) {
        if (sem_init(&msg_info[i].sem, 0, 0)) {
            ret = -ERR_SYS;
            goto error;
        }
        if (pthread_mutex_init(&msg_info[i].mutex, NULL)) {
            ret = -ERR_SYS;
            goto error;
        }
    }
    msg_inited = 1;
error:
    return ret;
}

/******************************************************************************
*	函数:	msg_send
*	功能:	发送消息
*	参数:	id				-	消息队列号
 			msg				-	消息（数据传入）
 			prio			-	优先级（0为最低，99为最高）
*	返回:	0				-	成功
 			-ERR_SYS		-	系统异常
 			-ERR_NOINIT		-	模块未初始化
 			-ERR_NODEV		-	无此消息队列
 			-ERR_INVAL		-	参数错误
 			-ERR_NOMEM		-	消息队列满
*	说明:
 ******************************************************************************/
int msg_send(u8 id, msg_t *msg, u8 prio)
{
    int ret = 0;

    if (0 == msg_inited) {
        ret = -ERR_NOINIT;
        goto error;
    }
    if (id > CFG_MSG_MAX - 1) {
        ret = -ERR_NODEV;
        goto error;
    }
    if (prio > MSG_PRIO_MAX) {
        ret = -ERR_INVAL;
        goto error;
    }
//获得互斥锁
    if (pthread_mutex_lock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
//判断消息队列是否满
    if (msg_info[id].head == (msg_info[id].tail + 1) % CFG_MSG_REALSIZE) {
        ret = -ERR_NOMEM;
        goto error1;
    }
//写入消息
    memcpy(&msg_info[id].message[msg_info[id].tail].msg, msg, sizeof(msg_t));
    msg_info[id].message[msg_info[id].tail].prio = prio;
//队列尾+1
    msg_info[id].tail = (msg_info[id].tail + 1) % CFG_MSG_REALSIZE;
error1:
//解锁
    if (pthread_mutex_unlock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
//触发信号量
    if (sem_post (&msg_info[id].sem)) {
        ret = -ERR_SYS;
        goto error;
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	msg_recv
*	功能:	接收消息
*	参数:	id				-	消息队列号
 			msg				-	消息（数据传出）
 			timeout			-	超时时间（0为永久等待，0xffff为不等待，其余为等待秒数）
*	返回:	0				-	成功
 			-ERR_SYS		-	系统异常
 			-ERR_NODEV		-	无此消息队列
 			-ERR_NOINIT		-	模块未初始化
 			-ERR_TIMEOUT	-	超时未接收到消息
*	说明:
 ******************************************************************************/
int msg_recv(u8 id, msg_t *msg, u16 timeout)
{
    int ret = 0;
//等待时间
    struct timespec to;

    if (0 == msg_inited) {
        ret = -ERR_NOINIT;
        goto error;
    }
    if (id > CFG_MSG_MAX - 1) {
        ret = -ERR_NODEV;
        goto error;
    }
//等待信号量
    if (MSG_RECV_BLOCK == timeout) {
        if (sem_wait (&msg_info[id].sem)) {
            ret = -ERR_SYS;
            goto error;
        }
    }
    else if (MSG_RECV_NONBLOCK == timeout) {
        if (sem_trywait (&msg_info[id].sem)) {
            ret = -ERR_TIMEOUT;             //无消息
            goto error;
        }
    }
    else {
        to.tv_sec = time(NULL) + timeout;
        to.tv_nsec = 0;
        if (sem_timedwait (&msg_info[id].sem, &to)) {
            ret = -ERR_TIMEOUT;
            goto error;
        }
    }

//获得互斥锁
    if (pthread_mutex_lock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
//判断队列是否为空
    if (msg_info[id].head == msg_info[id].tail) {
        ret = -ERR_SYS;
        goto error1;
    }
//读取消息
    memcpy(msg, &msg_info[id].message[msg_info[id].head].msg, sizeof(msg_t));
//队列头+1
    msg_info[id].head = (msg_info[id].head + 1) % CFG_MSG_REALSIZE;

error1:
//解锁
    if (pthread_mutex_unlock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	msg_recv_prio
*	功能:	接收优先消息
*	参数:	id				-	消息队列号
 			msg				-	消息（数据传出）
 			timeout			-	超时时间（0为永久等待，0xffff为不等待，其余为等待秒数）
*	返回:	0				-	成功
 			-ERR_SYS		-	系统异常
 			-ERR_NODEV		-	无此消息队列
 			-ERR_NOINIT		-	模块未初始化
 			-ERR_TIMEOUT	-	超时未接收到消息
*	说明:
 ******************************************************************************/
int msg_recv_prio(u8 id, msg_t *msg, u16 timeout)
{
    int ret = 0;
    int i;
//寻找优先级最高的消息
    u32 index, head, tail;
    s8 prio = -1;
//等待时间
    struct timespec to;

    if (0 == msg_inited) {
        ret = -ERR_NOINIT;
        goto error;
    }
    if (id > CFG_MSG_MAX - 1) {
        ret = -ERR_NODEV;
        goto error;
    }
//等待信号量
    if (MSG_RECV_BLOCK == timeout) {
        if (sem_wait (&msg_info[id].sem)) {
            ret = -ERR_SYS;
            goto error;
        }
    }
    else if (MSG_RECV_NONBLOCK == timeout) {
        if (sem_trywait (&msg_info[id].sem)) {
            ret = -ERR_TIMEOUT;             //无消息
            goto error;
        }
    }
    else {
        to.tv_sec = time(NULL) + timeout;
        to.tv_nsec = 0;
        if (sem_timedwait (&msg_info[id].sem, &to)) {
            ret = -ERR_TIMEOUT;
            goto error;
        }
    }

//获得互斥锁
    if (pthread_mutex_lock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
    head = msg_info[id].head;
    tail = msg_info[id].tail;
    index = head;
//判断队列是否为空
    if (head == tail) {
        ret = -ERR_SYS;
        goto error1;
    }
//寻找优先级最高的消息，index为找到的消息
    i = head;
    do {
    	if ((s8)(msg_info[id].message[i].prio) > prio) {
            prio = (s8)(msg_info[id].message[i].prio);
            index = i;
        }
        i = (i + 1) % CFG_MSG_REALSIZE;
    }
    while(i != tail);
//读出消息并排列好剩余的消息
    memcpy(msg, &msg_info[id].message[index].msg, sizeof(msg_t));
    if (index == head) {
        msg_info[id].head = (head + 1) % CFG_MSG_REALSIZE;
    }
    else if (index > head) {
        memmove(&msg_info[id].message[head] + 1, &msg_info[id].message[head], (index - head) * sizeof(msg_prio_t));
    	msg_info[id].head = (head + 1) % CFG_MSG_REALSIZE;
    }
    else {
        memmove(&msg_info[id].message[index], &msg_info[id].message[index] + 1, (tail - index - 1) * sizeof(msg_prio_t));
        if(0 == tail) {
            msg_info[id].tail = CFG_MSG_REALSIZE - 1;
        }
        else {
            msg_info[id].tail = tail - 1;
        }
    }
error1:
//解锁
    if (pthread_mutex_unlock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	msg_clear
*	功能:	清空消息
*	参数:	id				-	消息队列号
*	返回:	0				-	成功
 			-ERR_SYS		-	系统异常
 			-ERR_NODEV		-	无此消息队列
 			-ERR_NOINIT		-	模块未初始化
*	说明:
 ******************************************************************************/
int msg_clear(u8 id)
{
    int ret = 0;

    if (0 == msg_inited) {
        ret = -ERR_NOINIT;
        goto error;
    }
    if (id > CFG_MSG_MAX - 1) {
        ret = -ERR_NODEV;
        goto error;
    }
//复位信号量
    if (sem_init(&msg_info[id].sem, 0, 0)) {
		ret = -ERR_SYS;
		goto error;
	}
//获得互斥锁
    if (pthread_mutex_lock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
//清空消息队列
    msg_info[id].head = msg_info[id].tail;
//解锁
    if (pthread_mutex_unlock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
error:
    return ret;
}

/******************************************************************************
*	函数:	msg_getsize
*	功能:	获得消息数量
*	参数:	id				-	消息队列号
*	返回:	>0				-	消息数量
 			-ERR_SYS		-	系统异常
 			-ERR_NODEV		-	无此消息队列
 			-ERR_NOINIT		-	模块未初始化
*	说明:
 ******************************************************************************/
int msg_getsize(u8 id)
{
    int ret = 0;
    int size;

    if (0 == msg_inited) {
        ret = -ERR_NOINIT;
        goto error;
    }
    if (id > CFG_MSG_MAX - 1) {
        ret = -ERR_NODEV;
        goto error;
    }
//获得互斥锁
    if (pthread_mutex_lock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
//获得消息队列长度
    size = (int)msg_info[id].tail - (int)msg_info[id].head;
    if(size < 0) {
        size = size + CFG_MSG_REALSIZE;
    }
//解锁
    if (pthread_mutex_unlock (&msg_info[id].mutex)) {
        ret = -ERR_SYS;
        goto error;
    }
    ret = size;
error:
    return ret;
}

#endif /* CFG_MSG_MODULE */
