/**
* cyssave.c -- 周期储存
* 
* 
* 创建时间: 2010-6-11
* 最后修改时间: 2010-6-11
*/

#include <stdio.h>
#include "include/sys/cycsave.h"

static void DummyFunc(void)
{
	//printf("cycle save dummy func\n");
	return;
}
DECLARE_CYCLE_SAVE(DummyFunc, 0);

extern const struct cycle_save __start__cycle_save[];
extern const struct cycle_save __stop__cycle_save[];

/**
* @brief 循环储存
* @param flag 0-正常储存, 1-复位前储存
*/
void SysCycleSave(int flag)
{
	unsigned int i;

	for(i=0; __start__cycle_save+i<__stop__cycle_save; i++) {

		if(__start__cycle_save[i].flag & CYCSAVE_FLAG_RESET) {
			if(flag) (__start__cycle_save[i].pfunc)();
		}
		else (__start__cycle_save[i].pfunc)();
	}
}

