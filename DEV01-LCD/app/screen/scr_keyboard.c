/**
* scr_keyboard.c -- 按键输入
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#include "scr_menu.h"
#include "scr_lcd.h"

///#define LOOP_TIMEOUT	300
#define LOOP_CYCLE		30 //3S
#define LOOP_TIMEOUT	300 //30s


static char key_pushed = 0;
static char loop_state = 0;

/**
* @brief           读取按键值
* @return          返回值为读取到的按键值
*/
char KeyKbHit(void)
{
	///static int count = LOOP_TIMEOUT;   ///初始值为LOOP_TIMEOUT，默认开机后进入巡显
	static int count = 0;             ///开机等待一会后再进入轮显
	static int state = 1;   ///状态默认值为1
    ///读到的按键值
	char ch = LcdGetKey();  ///0.1秒才读一次
	
	int count_max ;

	if(key_pushed) 
	{
		ch = key_pushed;   ///赋予虚拟按键
		key_pushed = 0;
	}
    
	if(0 == ch)   ///没有长时间按键按下时
	{
		if(0 == state) 
		{
			count_max = LOOP_CYCLE;   ///巡显翻页时间间隔
			loop_state = 1;
		}
		else 
		{
			count_max = LOOP_TIMEOUT; ///进入巡显时间间隔
			loop_state = 2;
		}

		count++;
		if(count >= count_max)  ///超过最大的计数
		{
			state = 0;
			count = 0;      ///计数清0，又开始计数巡显
			return LOOPKEY;   ///返回值为翻页的时间间隔，可以作为翻页的时间计数器
		}
	}
	
	
	else  ///如果有键按下
	{
		state = 1;   ///状态设为1
		count = 0;  ///计数清0
		loop_state = 0;
	}

	return ch;     ///返回读到的按键值
}

void PushKey(char key)
{
	key_pushed = key;
}

char GetLoopState()
{
	return loop_state;
}

void SetLoopState(unsigned char LoopState)
{
	loop_state = LoopState ;
}


