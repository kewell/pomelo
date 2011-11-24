/**
* scr_input.h -- 显示输入头文件
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#ifndef _SCR_INPUT_H_
#define _SCR_INPUT_H_

#define ACTION_SELECT	0
#define ACTION_UNSELECT	1
#define ACTION_UP	2
#define ACTION_DOWN	3
#define ACTION_OK 4


typedef struct st_base_control{
	short name;
	unsigned char x;
	unsigned char y;
	unsigned char num;
	///用于移动
	void (*func_select)(struct st_base_control *pctl, int action,int num);   //left , right key
    ///用于修改
	void (*func_change)(struct st_base_control *pctl, int action,int num);  //up, down key  
} base_control_t;

typedef struct {
	base_control_t base;   //函数的属性
	short numchars;        //字符清单成员的个数
	short cursel;          ///当前选择的成员的数组下标
	///char *pchars;
	unsigned char *pchars; ///字符清单
	///char *prtn;
	unsigned char *prtn;   ///返回值
	char flag;
} editbox_t;
typedef struct {
	base_control_t base;
	short numchars;
	short cursel;
	char **pchars;
	char *prtn;
	char flag;
} editbox_t2;
//editbox_t editbox_Num[20];
typedef struct {
	base_control_t base;
	void (*func_click)(void);
	unsigned char text[21];
	///char text[21];
} button_t;

typedef struct {
	base_control_t base;   ///基本控制
	unsigned char pad[36]; ///块的个数
	unsigned char num;     ///此块的号码
} control_buffer_t;
#define MAXNUM_CONTROLS	 50
 /*_SCR_INPUT_H*/

typedef void (*pf_button_click_t)(void);

#define _SCR_INPUT_API_H

#define STRING_FLAG		0
///#define CHAR_FLAG           1

#define MAX_LINECHAR	21

#define CONTROL_OK_EXIT	1

///#define NULL 0
#endif


