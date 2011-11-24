/**
* term.h -- 终端参数头文件
* 
* 
* 创建时间: 2010-5-6
* 最后修改时间: 2010-5-6
*/

#ifndef _PARAM_TERM_H
#define _PARAM_TERM_H

//F1, 终端通信参数设置
typedef struct {
	unsigned char rts;		//终端数传机延时时间
	unsigned char delay;	//终端作为启动站允许发送延时时间
	unsigned short rsnd;	//终端等待从动站相应的超时时间和重发次数
	unsigned char flagcon;	//需要主站确认的通信服务(CON=1)的标志
	unsigned char cycka;	//心跳周期, 分
} cfg_tcom_t;

//F2, 终端中继转发设置, 保留

//F3, 主站IP地址和端口号
typedef struct {
	unsigned char ipmain[4];
	unsigned short portmain;
	unsigned char ipbakup[4];
	unsigned short portbakup;
	char apn[20];
} cfg_svrip_t;

//F4, 主站电话号码和短信中心号码
typedef struct {
	unsigned char phone[8];
	unsigned char sms[8];
} cfg_smsc_t;

//F5, 终端密码设置
typedef struct {
	unsigned char art;    //密码算法编号
	unsigned char pwd[2];    //密钥
} cfg_pass_t;

//F6, 终端组地址设置
typedef struct {
	unsigned char addr[16];
} cfg_grpaddr_t;

//F7, 终端IP地址和端口
typedef struct {
	unsigned char ipterm[4];
	unsigned char maskterm[4];
	unsigned char ipgatew[4];
	unsigned char ipproxy[4];
	unsigned short portproxy;
	unsigned char proxy_type;
	unsigned char proxy_connect;
	char username[32];
	char pwd[32];
	unsigned short portlisten;
} cfg_termip_t;

//F8, 终端上行通信工作方式
typedef struct {
	unsigned char proto;  //0-TCP, 1-UDP
	unsigned char mode;   //0-mix, 1-client, 2-server
	unsigned char clientmode;   //0-永久在线, 1-被动激活, 2-时段在线
	unsigned char countdail;  //重拨次数
	unsigned short timedail;  //重拨间隔 second
	unsigned char timedown;  //无通信自动断线时间 minute
	unsigned char unsued;
	unsigned int onlineflag;  //在线时段标志
} cfg_uplink_t;

//F9, 终端事件记录配置
typedef struct {
	unsigned char valid[8];   //事件有效
	unsigned char rank[8];    //事件等级, 1-重要事件, 0-一般事件
} cfg_almflag_t;

//F16, 虚拟专网用户名, 密码
typedef struct {
	char user[32];
	char pwd[32];
} cfg_vpn_t;

typedef struct {
	cfg_tcom_t tcom;    //F1
	//F2
	cfg_svrip_t svrip;    //F3
	cfg_smsc_t smsc;    //F4
	cfg_pass_t pwd;    //F5
	cfg_grpaddr_t grpaddr;    //F6
	cfg_termip_t termip;    //F7
	cfg_uplink_t uplink;    //F8
	cfg_almflag_t almflag;    //F9
	cfg_vpn_t vpn;   //F16
} para_term_t;

#ifndef DEFINE_PARATERM
extern const para_term_t ParaTerm;
#endif

#endif /*_PARAM_TERM_H*/

