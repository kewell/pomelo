/**
* operation_inner.h -- 参数操作头文件(内部使用)
* 
* 
* 创建时间: 2010-5-7
* 最后修改时间: 2010-5-7
*/

#ifndef _PARAM_OPERATION_INNER_H
#define _PARAM_OPERATION_INNER_H

//特殊参数操作(有些参数查询需要带查询项)
typedef struct {
	unsigned short pn;   //测量点号
	const unsigned char *rbuf;  //读入缓存区指针
	int rlen;   //缓存区长度
	int ractlen;   //数据项内容长度

	//写数据时不使用
	unsigned char *wbuf;  //写出缓存区指针
	int wlen;   //缓存区长度 (最大)
	int wactlen;   //有效数据长度(由函数返回)
} param_specialop_t;

//**********

#define FILEINDEX_TERM		0
#define FILEINDEX_MIX		1
#define FILEINDEX_UNI		2
#define FILEINDEX_HARDW		3
#define FILEINDEX_METER		4
#define FILEINDEX_METP		5
#define FILEINDEX_TASK		6
#define FILEINDEX_CMETP		7
#define FILEINDEX_CHIEND	8
#define FILEINDEX_DATAUSE	9
#define FILEINDEX_ROUTE		10
#define FILEINDEX_MAX		11

#define SAVEFLAG_TERM		((unsigned int)1<<FILEINDEX_TERM)
#define SAVEFLAG_MIX		((unsigned int)1<<FILEINDEX_MIX)
#define SAVEFLAG_UNI		((unsigned int)1<<FILEINDEX_UNI)
#define SAVEFLAG_HARDW		((unsigned int)1<<FILEINDEX_HARDW)
#define SAVEFLAG_METER		((unsigned int)1<<FILEINDEX_METER)
#define SAVEFLAG_METP		((unsigned int)1<<FILEINDEX_METP)
#define SAVEFLAG_TASK		((unsigned int)1<<FILEINDEX_TASK)
#define SAVEFLAG_CMETP		((unsigned int)1<<FILEINDEX_CMETP)
#define SAVEFLAG_CHIEND		((unsigned int)1<<FILEINDEX_CHIEND)
#define SAVEFLAG_DATAUSE	((unsigned int)1<<FILEINDEX_DATAUSE)
#define SAVEFLAG_ROUTE		((unsigned int)1<<FILEINDEX_ROUTE)
#define SAVEFLAG_RUNSTATE	((unsigned int)0x80000000)

void SetSaveParamFlag(unsigned int flag);

int SaveParaTerm(void);
int LoadParaTerm(void);
void SetParaNetAddr(void);
int SaveParaMeter(void);
int LoadParaMeter(void);
int SaveParaCenMetp(void);
int LoadParaCenMetp(void);
int SaveParaPlcMetp(void);
int LoadParaPlcMetp(void);
int SaveParaChildEnd(void);
int LoadParaChildEnd(void);
int LoadParaMix(void);
int SaveParaMix(void);
int LoadParaDataUse(void);
int SaveParaDataUse(void);
int LoadParaTask(void);
int SaveParaTask(void);
int LoadParaHardw(void);
int SaveParaHardw(void);
int LoadParaRoute(void);
int SaveParaRoute(void);
int SaveParaManuf(void);
int LoadParaUni(void);

//function list

int ParaOperationF1(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF2(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF3(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF4(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF5(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF6(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF7(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF8(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF9(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF10(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF11(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF12(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF13(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF14(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF15(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF16(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF21(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF25(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF26(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF27(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF28(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF29(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF30(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF31(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF33(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF34(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF35(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF36(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF37(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF38(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF39(int flag, unsigned short metpid, param_specialop_t *option);
int ParaOperationF59(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF60(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF65(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF66(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF67(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF68(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF85(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);
int ParaOperationF161(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen);


#endif /*_PARAM_OPERATION_INNER_H*/

