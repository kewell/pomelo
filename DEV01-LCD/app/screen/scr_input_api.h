/**
* scr_input_api.h -- 显示输入接口头文件
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#ifndef _SCR_INPUT_API_H
#define _SCR_INPUT_API_H

#define STRING_FLAG		0
#define CHAR_FLAG           1

#define MAX_LINECHAR	 21

#define CONTROL_OK_EXIT	 1
void InitControlPage(void);
int ShowControlPage(void);
void QuitControlPage(int result);

void show_string(unsigned char x, unsigned char y, char *str,int col);
///int AddEditBox(unsigned char x, unsigned char y, int numchars, char *charlist, int defsel, char *prtn, char flag);
int AddEditBox(unsigned char x, unsigned char y, int numchars, char *charlist, int defsel, unsigned char *prtn,char flag);

typedef void (*pf_button_click_t)(void);
int AddButton(unsigned char x, unsigned char y, char *text, pf_button_click_t pf_click);

void DefOkClick(void);
void DefEscClick(void);
int ScrPwdConfirm(void);
#endif /*_SCR_INPUT_API_H*/



