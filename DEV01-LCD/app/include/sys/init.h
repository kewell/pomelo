/**
* init.h -- 系统抽象模块初始化函数接口头文件
* 
* 
* 创建时间: 2010-3-31
* 最后修改时间: 2010-3-31
*/

#ifndef _SYS_INIT_H
#define _SYS_INIT_H

int SysInit(void);
int DebugInit(void);
int ParamInit(void);
int CenMetInit(void);
int MonitorInit(void);
int PlcInit(void);
int SvrCommInit(void);
int ScreenInit(void);

#endif /*_SYS_INIT_H*/
