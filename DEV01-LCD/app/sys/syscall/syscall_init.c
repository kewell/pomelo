/**
* syscall_init.c -- ϵͳ������ģ���ʼ��
* 
* 
* ����ʱ��: 2008-3-31
* ����޸�ʱ��: 2010-3-31
*/
#include <stdio.h>

#include "include/debug.h"

extern int SysTimerInit(void);
extern int SysTimeInit(void);

/**
* @brief ϵͳ������ģ���ʼ������
* @return ����0��ʾ�ɹ�, ����ʧ��
*/
DECLARE_INIT_FUNC(SysCallInit);
int SysCallInit(void)
{
	printf("  SysCall init...\n");

	if(SysTimeInit()) return 1;
	if(SysTimerInit()) return 1;

	SET_INIT_FLAG(SysCallInit);

	return 0;
}
