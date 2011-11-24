/**
* cyssave.h -- 周期储存
* 
* 
* 创建时间: 2010-6-11
* 最后修改时间: 2010-6-11
*/

#ifndef _CYCSAVE_H
#define _CYCSAVE_H

#define CYCSAVE_FLAG_RESET   0x00000001  //仅在复位前调用

struct cycle_save {
	void (*pfunc)(void);
	unsigned int flag;
};

/**
* @brief 声明一个周期储存
* @param func 执行函数
* @param flag 为0表示正常周期执行, 为CYCSAVE_FLAG_RESET表示仅在复位前调用
*/
#define DECLARE_CYCLE_SAVE(func, flag) \
	static const struct cycle_save _cycsave_##func \
	__attribute__((__used__)) \
	__attribute__((section("_cycle_save"), unused)) \
	= {func, flag}

/**
* @brief 循环储存
* @param flag 0-正常储存, 1-复位前储存
*/
void SysCycleSave(int flag);

#endif /*_CYCSAVE_H*/

