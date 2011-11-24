/**
* dl645_1997.h -- DL/T645-1997协议分析
* 
* 
* 创建时间: 2010-5-21
* 最后修改时间: 2010-5-21
*/

#ifndef _DL645_1997_H
#define _DL645_1997_H

int Dl1997MakeReadPkt(const unsigned char *addr, unsigned short itemid, unsigned char *buf, int len);
int Dl1997MakeWritePkt(const unsigned char *addr, const plwrite_config_t *pconfig, unsigned char *buf, int len);
int Dl1997MakeChkTimePkt(const sysclock_t *pclock, unsigned char *buf, int len);
int Dl1997CheckRead(const unsigned char *addr, unsigned short itemid, unsigned char *buf, int len);
int Dl1997CheckWrite(const unsigned char *addr, unsigned short itemid, const unsigned char *buf, int len);

#endif /*_DL645_1997_H*/

