/**
* rs485bus.h -- RS485总线抄表接口头文件
* 
* 作者: dongqi
* 创建时间: 2010-1-19
* 最后修改时间: 2010-1-19
*/

#ifndef _RS485BUS_H
#define _RS485BUS_H

#define RS485BUS_BUF_LEN		272
int Rs485BusRead(const plc_dest_t *dest, unsigned short itemid, unsigned char *buf, int len);

#endif/*_RS485BUS_H*/

