/**
* cenmet_proto.h -- 表计协议
* 
* 
* 创建时间: 2010-5-17
* 最后修改时间: 2010-5-17
*/

#ifndef _CENMET_PROTO_H
#define _CENMET_PROTO_H

int LoadCenMetProto(void);

#define OPMOD_NORMAL    0
#define OPMOD_BYIDX    1

typedef struct {
	int baudrate;
	unsigned char port;
	unsigned char metid;
	unsigned short itemid;

	unsigned char addr[6];
	char databits;
	char stopbits;
	char parity;

	unsigned char *rbuf;
	int rlen;

	const unsigned char *password;
	const unsigned char *usrname;
} metinfo_t;

#define FUNCTYPE_READ    0
#define FUNCTYPE_SET    1

#define PROTOATTR_SINGLE    0
#define PROTOATTR_BURST    1
typedef struct {
	unsigned char id;
	unsigned char attr;
	unsigned char read_num;
	unsigned char reserv;

	int speed;
	char databits;
	char stopbits;
	char parity;

	int (*read)(metinfo_t *pmet);
	int (*set)(metinfo_t *pmet, unsigned char *sbuf, int slen);
	unsigned short (*get_itemid)(int idx);
} metfunc_t;

#ifndef DEFINE_CENMETPROTO
extern const unsigned int CenMetProtoValid;
#endif

#define PROTO_VALID		CenMetProtoValid

typedef struct {
	unsigned char mkyear;
	unsigned char mkmonth;
	unsigned char mkday;
	unsigned char mkhour;
	unsigned char mkminute;
	unsigned char ver_major;
	unsigned char ver_minor;
	unsigned char ver_area[2];
	unsigned char num;
	unsigned char id[2];
} protoinfo_t;

int PrintCMetProtoInfo(unsigned char *buf);
metfunc_t *GetCMetFunc(unsigned char metid, unsigned char functype);
int GetCMetStdLen(unsigned short itemid);

#define METATTR_DL645LIKE    0
#define METATTR_ABBLIKE    1
#define METATTR_EMPTY    0xff
int GetCMetAttr(unsigned char metid, unsigned char *pattr);

#endif /*_CENMET_PROTO_H*/
