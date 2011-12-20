/******************************************************************************
	项目名称	：SGE800计量智能终端业务平台
	文件		：base.h
	描述		：本文件定义了业务模块基类
	版本		：0.1
	作者		：万鸿俊
	创建日期	：2010.11
******************************************************************************/

#ifndef _BASE_H
#define _BASE_H

#include "sge_core/typedef.h"

/*************************************************
  结构类型定义
*************************************************/
//消息类型
typedef struct{
    u16  type;             //消息类型
    u16  wpara;            //消息参数
    u32  lpara;            //消息附加信息
} message_t;

struct BASE;

//基类方法
struct BASEFT
{
	int (*initmodel)(struct BASE * this);						//初始化模块模型
	int (*initdata)(struct BASE * this);						//初始化模块数据
	int (*msghandle)(struct BASE * this, message_t *msg);		//模块的消息处理函数
};

//基类定义
struct BASE
{
	struct BASEFT *baseft;				//基类方法
	u8 thread;							//模块对应线程号
	u8 prio;							//模块优先级
};

#endif
