/**
* pldynamic.h -- 动态态路由载波通信接口
* 
* 
* 创建时间: 2010-8-25
* 最后修改时间: 2010-8-25
*/

#ifndef _PLDYNAMIC_H
#define _PLDYNAMIC_H

int PlDynamicRead(const plc_dest_t *dest, unsigned short itemid, unsigned char *buf, int len);
int PlDynamicWrite(const plc_dest_t *dest, const plwrite_config_t *pconfig);
int PlDynamicCheckTime(const plc_dest_t *dest, int flag);

#endif /*_PLDYNAMIC_H*/

