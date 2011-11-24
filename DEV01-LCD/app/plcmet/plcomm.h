/**
* plccomm.h -- 载波通信接口头文件
* 
* 
* 创建时间: 2010-4-24
* 最后修改时间: 2010-4-24
*/

#ifndef _PLCOMM_H
#define _PLCOMM_H

#define PLC_UART_PORT		5
   
//载波路由表（最多5级中继）
#define PLC_ROUTENUM    5
typedef struct {
	unsigned char level;  //路由级数
	unsigned char phase;   //0-未指定, 1-a, 2-b,3-c
	unsigned char addr[PLC_ROUTENUM*6];
} plc_route_t;

//目的配置
typedef struct {
	unsigned short metid;  // 从1开始, 0无效
	unsigned char portcfg;
	unsigned char proto;
	unsigned char src[6];
	unsigned char dest[6];
	plc_route_t route;
} plc_dest_t;

typedef struct {
	unsigned short itemid;
	unsigned char *pwd;
	int pwdlen;
	const unsigned char *command;
	int cmdlen;
} plwrite_config_t; 

#define PLCOMM_BUF_LEN		272
unsigned char *GetPlCommBuffer(void);

void MakePlcDest(unsigned short metid, plc_dest_t *dest);

#define PLCHKTIME_POLL		1   //轮询方式较表
#define PLCHKTIME_BROCAST	2   //广播方式较表


//返回错误码
#define PLCERR_INVALID		-1
#define PLCERR_TIMEOUT		-2

int PlcRead(const plc_dest_t *dest, unsigned short itemid, unsigned char *buf, int len);
int PlcWrite(const plc_dest_t *dest, const plwrite_config_t *pconfig);
int PlcCheckTime(void);

#ifndef DEFINE_PLCOMM
extern const int PlcTimeChecking;
#endif

#endif /*_PLCOMM_H*/

