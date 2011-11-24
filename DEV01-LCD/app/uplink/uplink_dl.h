/**
* uplink_dl.h -- 上行通信数据链路层
* 
* 
* 创建时间: 2010-5-18
* 最后修改时间: 2010-5-18
*/

#ifndef _UPLINK_DL_H
#define _UPLINK_DL_H

//上行通信信道编号
#define UPLINKITF_SERIAL		0   //串口
#define UPLINKITF_ETHMTN		1   //以太网服务器
#define UPLINKITF_GPRS			2   //GPRS
#define UPLINKITF_ETHER			3  //以太网
#define UPLINKITF_SMS			4   //短信
#define UPLINKITF_IR			5  //红外
#define UPLINKITF_CASCADE		6  //级联

#define UPLINKITF_NUM			7

#define UPLINKATTR_NOECHO    0x01    //主动发送不需响应(如短信信道)

typedef struct {
	unsigned char *rcvbuf;  //接收缓存
	unsigned char *sndbuf;    //发送缓存
	int (*rawsend)(const unsigned char *buf, int len);    //发送函数
	int (*getchar)(unsigned char *buf);    //接收函数
	int (*linestat)(void);
	int timeout;   //接收超时

	int rcvmax;    //接收数据区最大长度
	int sndmax;    //发送数据区最大长度
	int sndnor;     //发送数据一般长度上限

	unsigned int attr;   //通道属性
} uplinkitf_t;

extern const uplinkitf_t UplinkInterface[UPLINKITF_NUM];
#define UPLINK_RCVBUF(itf)		(UplinkInterface[itf].rcvbuf)
#define UPLINK_SNDBUF(itf)		(UplinkInterface[itf].sndbuf)
#define UPLINK_RCVMAX(itf)		(UplinkInterface[itf].rcvmax)
#define UPLINK_TIMEOUT(itf)		(UplinkInterface[itf].timeout)
#define UPLINK_SNDMAX(itf)		(UplinkInterface[itf].sndmax)
#define UPLINK_SNDNOR(itf)		(UplinkInterface[itf].sndnor)
#define UPLINK_ATTR(itf)		(UplinkInterface[itf].attr)

void UplinkClearState(unsigned char itf);
int UplinkRecvPkt(unsigned char itf);
int UplinkSendPkt(unsigned char itf, uplink_pkt_t *pkt);
int CascadeForwardPkt(unsigned char itf, uplink_pkt_t *pkt);
int CascadeSvrSendPkt(unsigned char itf, uplink_pkt_t *pkt);

#define UPRTN_OK    0      //发送成功
#define UPRTN_FAIL    1    //发送失败
#define UPRTN_OKRCV    2    //发送成功, 收到命令包
#define UPRTN_FAILRCV    3    //发送之前收到命令包
#define UPRTN_TIMEOUT    4   //发送超时

//flag = 0, not wait echo, 1-wait echo
int UplinkActiveSend(unsigned char itf, unsigned char flag, uplink_pkt_t *psnd);
int UplinkDelay(unsigned char itf, const uplink_pkt_t *pkt);

int UplinkLogon(unsigned char itf);
int UplinkLinkTest(unsigned char itf);

//unsigned char faal_datomid(unsigned char *da);
//int UplinkCheckEchoPkt(uplink_pkt_t *pkt);

#endif /*_UPLINK_DL_H*/

