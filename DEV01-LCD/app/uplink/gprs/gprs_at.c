/**
* gprs_at.c -- GPRS模块AT命令处理
* 
* 
* 创建时间: 2010-5-19
* 最后修改时间: 2010-5-19
*/

#include <stdio.h>
#include <string.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "gprs_hardware.h"
#include "gprs_at.h"

unsigned long gprs_atclr;

/**
* @brief 发送AT命令并检查响应
* @param list AT命令及响应列表
* @param timeout 超时时间(秒)
* @return 成功返回响应的索引值, 否则返回-1
*/
int GprsSendAt(const atlist_t *list, int timeout)
{
	int count, i;
	char c;
	int pos[4] = {0, 0, 0, 0};

	timeout *= 10;

	GPRSAT_CLRRCV;

	GprsLineSend((unsigned char *)(list->cmd), strlen(list->cmd));

	Sleep(10);

	for(count=0; count<timeout; count++) {
		while(GprsLineRecv((unsigned char *)&c, 1) > 0) {
			//DebugPrint(0, "atr:%c\r\n", c);
			for(i=0; i<list->keynum; i++) {
				if(c == list->key[i][pos[i]]) {
					pos[i]++;
					if(0 == list->key[i][pos[i]]) {
						Sleep(10);
						GPRSAT_CLRRCV;
						return i;
					}
				}
				else pos[i] = 0;
			}
		}

		Sleep(10);
	}

	return -1;
}
