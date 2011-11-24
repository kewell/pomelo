/**
* uplink_pkt.h -- 上行通信帧结构
* 
* 
* 创建时间: 2010-5-18
* 最后修改时间: 2010-5-18
*/

#ifndef _UPLINK_PKT_H
#define _UPLINK_PKT_H



#define LEN_UPLINKDL    8
#define LEN_UPLINKHEAD   8
#define MINLEN_UPLINKPKT    (LEN_UPLINKDL+LEN_UPLINKHEAD)

#define UPLINK_HEAD    0x68
#define UPLINK_TAIL    0x16

/*
长度在服务器处理程序里为减去控制域,
地址域, AFN, SEQ的长度, 即减去LEN_UPLINKHEAD
为head+len1[0]的短整型
*/
#define UPLINKAPP_LEN(pkt)    (*((unsigned short *)(pkt)))
typedef struct {
	unsigned char head;    //=68H
	unsigned char len1[2];
	unsigned char len2[2];
	unsigned char dep;    //=68H
	unsigned char ctrl;    //控制域C
	unsigned char area[2];
	unsigned char sn[2];
	unsigned char grp;
	unsigned char afn;    //应用层功能码
	unsigned char seq;
	unsigned char data[1022];
} uplink_pkt_t;
#define OFFSET_UPDATA		((int)(((uplink_pkt_t *)0)->data))

#define UPCTRL_DIR		0x80
#define UPCTRL_PRM		0x40
#define UPCTRL_CODE		0x0f

#define UPSEQ_TPV		0x80
#define UPSEQ_FIR		0x40
#define UPSEQ_FIN		0x20
#define UPSEQ_CON		0x10
#define UPSEQ_SEQ		0x0f
#define UPSEQ_SPKT		(UPSEQ_FIR|UPSEQ_FIN)

//PRM=1
#define UPCMD_RESET			1    //复位命令
#define UPCMD_USRDATA		4    //用户数据
#define UPCMD_LINKTEST		9    //链路测试
#define UPCMD_REQFIRD		10    //请求1级数据
#define UPCMD_REQSECD		11    //请求2级数据
#define UPCMD_SELFDEF		13    //自定义命令

//PRM=0
#define UPECHO_CONF			0    //认可
#define UPECHO_USRDATA		8    //用户数据
#define UPECHO_DENY			9    //否认: 无所召唤的数据
#define UPECHO_LINKSTAT		11    //链路状态

#define UPAFN_ECHO			0x00     //确认/否认
#define UPAFN_RESET			0x01    //复位
#define UPAFN_LINKTEST		0x02    //链路接口检测
#define UPAFN_RELAY			0x03    //中继站命令
#define UPAFN_SETPARA		0x04    //设置参数
#define UPAFN_CTRL			0x05    //控制命令
#define UPAFN_CASCADE		0x08    //请求级联主动上报
#define UPAFN_REQCFG		0x09    //请求终端配置
#define UPAFN_QRYPARA		0x0a    //查询参数
#define UPAFN_QRYTASK		0x0b    //任务数据查询
#define UPAFN_QRYCLS1		0x0c    //请求1 类数据
#define UPAFN_QRYCLS2		0x0d    //请求2 类数据
#define UPAFN_QRYCLS3		0x0e    //请求3 类数据
#define UPAFN_TRANFILE		0x0f    //文件传输
#define UPAFN_FORWARD		0x10    //数据转发
#define UPAFN_LOGO			0x20    //调试信息

#define UPLINK_MAXLEN_FILEDATA    512
#define UPLINK_WINNUM_FILEDATA    32

//时间标签
#define LEN_UPLINK_TIMETAG	6
typedef struct {
	unsigned char pfc;
	unsigned char time[4];
	unsigned char dly;
} uplink_timetag_t;

//数据单元标识
typedef struct {
	unsigned char da[2];    //信息点
	unsigned char dt[2];    //信息类
} uplink_duid_t;

//确认/否认报文
typedef struct {
	unsigned char da[2];    //信息点
	unsigned char dt[2];    //信息类
	unsigned char afn;
	unsigned char data[1];
} uplink_ack_t;

#define UPLINK_PID0(puid)    { \
	((uplink_duid_t *)(puid))->da[0] = ((uplink_duid_t *)(puid))->da[1] = 0; }

#define UPLINK_PID1(id, puid) { \
	((uplink_duid_t *)(puid))->da[0] = (unsigned char)(1<<(((id)-1)&0x07)); \
		((uplink_duid_t *)(puid))->da[1] = (unsigned char)(1<<(((id)-1)>>3)); }

#define UPLINK_FID(id, puid) { \
	((uplink_duid_t *)(puid))->dt[1] = (unsigned char)(((id)-1)>>3); \
	((uplink_duid_t *)(puid))->dt[0] = (unsigned char)(1<<(((id)-1)&0x07)); }

#endif /*_UPLINK_PKT_H*/

