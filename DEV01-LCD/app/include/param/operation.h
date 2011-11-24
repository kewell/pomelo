/**
* operation.h -- 参数操作头文件
* 
* 
* 创建时间: 2010-5-7
* 最后修改时间: 2010-5-7
*/

#ifndef _PARAM_OPERATION_H
#define _PARAM_OPERATION_H

#define POERR_OK			0
#define POERR_INVALID		1
#define POERR_FATAL		2

int WriteParam(unsigned char *buf, int len, int *pactlen);

typedef struct {
	unsigned char *buf;   //输入缓存区指针
	int len;   //缓存区长度
	int actlen;   //有效数据长度(由函数返回)
} para_readinfo_t;
int ReadParam(unsigned char *buf, int len, int *pactlen, para_readinfo_t *readinfo);

void ClearSaveParamFlag(void);
void SaveParam(void);

void ClearAllData(void);
void ClearAllParam(void);
int get_meter_param_change_flag(void);
void clear_meter_param_change_flag(void);
void set_meter_param_change_flag(void);
#endif /*_PARAM_OPERATION_H*/
