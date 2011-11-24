/**
* plc_proto.h -- 载波应用协议接口
* 
* 
* 创建时间: 2010-5-21
* 最后修改时间: 2010-5-21
*/

#ifndef _PLC_PROTO_H
#define _PLC_PROTO_H

typedef struct {
	unsigned char protoid;
	int (*makeread)(const unsigned char *addr, unsigned short itemid, unsigned char *buf, int len);
	int (*makewrite)(const unsigned char *addr, const plwrite_config_t *pconfig, unsigned char *buf, int len);
	int (*makechktime)(const sysclock_t *pclock, unsigned char *buf, int len);
	int (*checkread)(const unsigned char *addr, unsigned short itemid, unsigned char *buf, int len);
	int (*checkwrite)(const unsigned char *addr, unsigned short itemid, const unsigned char *buf, int len);
} plcmet_prot_t;

const plcmet_prot_t *GetPlcMetProto(unsigned char proto);

#endif /*_PLC_PROTO_H*/

