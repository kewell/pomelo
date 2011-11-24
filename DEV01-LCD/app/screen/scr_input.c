/**
* scr_input.h -- 显示输入参数
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/
///#include <unistd.h>
#include <string.h>
#include "include/debug.h"
#include "include/sys/schedule.h"
#include "scr_menu.h"
#include "scr_input.h"
#include "scr_lcd.h"
#define CHAR_FLAG           1
extern  unsigned char myasc[4096];
static control_buffer_t control_buffer[MAXNUM_CONTROLS];
static int control_num = 0;
static int exit_return = -1;
static int first_goto = -1;
int asciiFlag;

char KeyKbHit(void);
void PushKey(char key);
/**
* @brief      初始化输入页面
*/
void InitControlPage(void)
{
	//CleanPartOfScreen(2*16-MENU_LINE_POS,7);
	control_num = 0;   
	exit_return = -1;
	first_goto = - 1;
}
/**
* @brief      显示所有的输入编辑选项
*/
static void ShowAllControls(void)
{
	int i;
	base_control_t *pctl;
	
	first_goto = 1;            
	pctl = (base_control_t *)&control_buffer[0]; 
	(*pctl->func_select)(pctl, ACTION_SELECT,0);

	for(i=1; i<control_num; i++) 
	{              
		pctl = (base_control_t *)&control_buffer[i];	
		(*pctl->func_select)(pctl, ACTION_UNSELECT,i);
	}
	if(first_goto)
	{
		DisplayLcdBuffer();
		first_goto = 0;
	}
	
}


/**
* @brief		输入处理
* @return		返回值为-1，退出函数
*/
int ShowControlPage(void)
{
	int set_select = 0;
	static unsigned char one_second_conut = 0;
	static int outkeytimes=0;
	unsigned char xkey;
	base_control_t *pctl= (base_control_t *)&control_buffer[0];
	
	if(control_num <= 0) return -1;	
	ShowAllControls();	
	while(1)
	{
		xkey = KeyKbHit();
		if(LOOPKEY == xkey)   
		{  
			///PushKey(LOOPKEY);       
			return -1;
		}
		else if(EXITKEY == xkey) 
		{
			xkey=0;
			return exit_return; 
		}
		else if(ESCKEY== xkey)
		{
			xkey=0;
			InitControlPage();  
			return -1;
		}
				
		else if(0 == control_num) continue;
		
		switch(xkey) 
	   {
		case UPKEY:
			xkey=0;
			LcdBakLight(1);
			outkeytimes=0;  
			if(NULL != pctl->func_change) 
				(*pctl->func_change)(pctl, ACTION_UP,set_select);			
			break;
		case DOWNKEY:
			xkey=0;
			LcdBakLight(1);	
			outkeytimes=0;
			if(NULL != pctl->func_change) 
				(*pctl->func_change)(pctl, ACTION_DOWN,set_select);
			break;
		case OKKEY:  
		    xkey=0;
			LcdBakLight(1);	
			outkeytimes=0;
			if(NULL != pctl->func_change) 
				(*pctl->func_change)(pctl, ACTION_OK,set_select);
			break;
		case LEFTKEY:
			xkey=0;
			LcdBakLight(1);
			outkeytimes=0;
			if(NULL != pctl->func_select) 
				(*pctl->func_select)(pctl, ACTION_UNSELECT,set_select);
			set_select--;   
			if(set_select < 0) 
				set_select = control_num - 1;  		
			pctl = (base_control_t *)&control_buffer[set_select];  
			if(NULL != pctl->func_select) 
				(*pctl->func_select)(pctl, ACTION_SELECT,set_select);
			break;
	  	case RIGHTKEY:
			xkey=0;
			LcdBakLight(1);
			outkeytimes=0;
			if(NULL != pctl->func_select) 
				(*pctl->func_select)(pctl, ACTION_UNSELECT,set_select);
			set_select++;
			if(set_select >= control_num) 
				set_select = 0;	
			pctl = (base_control_t *)&control_buffer[set_select];	
			if(NULL != pctl->func_select) 
				(*pctl->func_select)(pctl, ACTION_SELECT,set_select);
			break;
		default:
			break;
			}
		one_second_conut++;
		if(one_second_conut >= 9) 
		{	
			one_second_conut = 0;
            ShowTopFlag();
		}
		Sleep(10);
		}
}


/**
* @brief		申请输入的存储空间
* @return		申请到的存储空间的指针
*/
static control_buffer_t *AllocateControlBuffer(void)
{
	control_buffer_t *pctl;
	
	if(control_num >= MAXNUM_CONTROLS) return NULL;
	pctl = &control_buffer[control_num];
	control_num++;
	pctl->num++;   
	pctl->base.num=pctl->num;
	return pctl;  
}


/**
* @brief		响应ENTER的确认页面
*/
void QuitControlPage(int result)
{
	exit_return = result; 

	PushKey(EXITKEY);
	
}

/**
* @brief				修改输入项
* @param pctl			输入操作的缓存
* @param action			输入操作的动作
* @param num			字符的数目
*/
static void EditBoxChange(base_control_t *pctl, int action,int num)
{
	editbox_t *peditbox = (editbox_t *)pctl;	
	char str[2];
	
	if((ACTION_UP != action) && (ACTION_DOWN != action)) return;
    
	if(ACTION_UP == action)
	{ 
		peditbox->cursel--;  
		if(peditbox->cursel < 0) 
			peditbox->cursel = peditbox->numchars-1;  
		*peditbox->prtn = peditbox->cursel; 
	}
	else if(ACTION_DOWN == action)
	{
		peditbox->cursel++;
		if(peditbox->cursel >= peditbox->numchars) 
			peditbox->cursel = 0;
		*peditbox->prtn = peditbox->cursel;
	}

	if(CHAR_FLAG == peditbox->flag) 
	{   
	    SetInvs(1);
		str[0] = peditbox->pchars[peditbox->cursel];	
		asciiFlag=0xfffff;
		DisplayNormal(&str[0],1,pctl->x*16-MENU_LINE_POS,pctl->y*8);
        if(first_goto == 0)
			DisplayLcdBuffer();
	}
	else   
	{	
        SetInvs(1);
		int len=strlen(&peditbox->pchars[(peditbox->cursel)*MAX_LINECHAR]);
		DisplayNormal(&peditbox->pchars[(peditbox->cursel)*MAX_LINECHAR],len,pctl->x*16-MENU_LINE_POS,pctl->y*8);
        if(first_goto == 0)
			DisplayLcdBuffer();
	}
}


/**
* @brief				选择输入项
* @param pctl			输入操作的缓存
* @param action			输入操作的动作
* @param num			字符的数目
*/
static void EditBoxSelect(base_control_t *pctl, int action,int num)
{	
	editbox_t *peditbox = (editbox_t *)pctl;
	char str[2];
	
	if((ACTION_SELECT != action) && (ACTION_UNSELECT != action)) return;

	if((peditbox->cursel < 0) || (peditbox->cursel >= peditbox->numchars)) peditbox->cursel = 0;
	if(ACTION_SELECT == action) 
		SetInvs(1);
	else  
		SetInvs(0);
	
	if(CHAR_FLAG == peditbox->flag)
	{
		str[0] = peditbox->pchars[peditbox->cursel];
		asciiFlag=0xfffff;
        DisplayNormal(&str[0],1,pctl->x*16-MENU_LINE_POS,pctl->y*8);
		if(first_goto == 0)
			DisplayLcdBuffer();
	}
	else
	{		
		int len=strlen(&peditbox->pchars[(peditbox->cursel)*MAX_LINECHAR]);
		DisplayNormal(&peditbox->pchars[(peditbox->cursel)*MAX_LINECHAR],len,pctl->x*16-MENU_LINE_POS,pctl->y*8);
        if(first_goto == 0)
			DisplayLcdBuffer();
	}
}

/**
* @brief			增加编辑选项
* @param x			在屏幕上的横坐标
* @param y			在屏幕上的纵坐标
* @param numchars	字符的个数
* @param charlist	字符串的首地址
* @param defsel		默认的字符
* @param prtn		返回的字符换首地址
* @param flag		输入为字符或字符串标志
* @return   		返回值为0，则指向成功，为1则执行失败
*/
int AddEditBox(unsigned char x, unsigned char y, int numchars, char *charlist, int defsel, unsigned char *prtn,char flag)
{
	editbox_t *peditbox;

	if((x > 8) || (y > 127) || (numchars <= 1) || (NULL == charlist)) return 1;
		peditbox = (editbox_t *)AllocateControlBuffer();
	if(NULL == peditbox) return 1;
	peditbox->base.x = x;  
	peditbox->base.y = y;   
	peditbox->numchars = numchars;  
	peditbox->pchars = (unsigned char*)charlist; 
	peditbox->flag = flag; 
	if((defsel < 0) || (defsel >= numchars)) 
		peditbox->cursel = 0;
	else peditbox->cursel = defsel; 
		peditbox->prtn = prtn;  
    *prtn = peditbox->cursel;
	peditbox->base.func_select = EditBoxSelect;  ///指向左右键的处理函数
	peditbox->base.func_change = EditBoxChange;  ///指向上下键的处理函数
	return 0;
}
/**
* @brief				确认和取消选项的选择
* @param pctl			输入操作的缓存
* @param action			输入操作的动作
* @param num			字符的数目
*/
static void ButtonSelect(base_control_t *pctl, int action,int num)
{
	button_t *pbutton = (button_t *)pctl;

	if(ACTION_OK!= action) return;
	if(NULL != pbutton->func_click) (*pbutton->func_click)();
}
/**
* @brief				确认和取消选项的执行
* @param pctl			输入操作的缓存
* @param action			输入操作的动作
* @param num			字符的数目
*/
static void ButtonChange(base_control_t *pctl, int action,int num)
{
	button_t *pbutton = (button_t *)pctl;

	if((ACTION_SELECT != action) && (ACTION_UNSELECT != action)) return;

	if(ACTION_SELECT == action)    
		SetInvs(1);  
	else  
		SetInvs(0);
	DisplayNormal(pbutton->text,4,pctl->x*16-MENU_LINE_POS,pctl->y*8);
    if(first_goto == 0)
		DisplayLcdBuffer();
}

/**
* @brief			增加确认取消按钮
* @param x			在屏幕上的横坐标
* @param y			在屏幕上的纵坐标
* @param text		输入的文本
* @param pf_click	执行的函数
* @return   		返回值为0，则指向成功，为1则执行失败
*/
int AddButton(unsigned char x, unsigned char y, char *text, pf_button_click_t pf_click)
{                
	button_t *pbutton;

	if((x > 8) || (y > 127) || (NULL == text)) return 1;
	pbutton = (button_t *)AllocateControlBuffer();
	if(NULL == pbutton) return 1;
                                                      
 	pbutton->base.x = x;  
 	pbutton->base.y = y;  
	pbutton->func_click = pf_click;
	strcpy((char *)pbutton->text, text);
	pbutton->base.func_select = ButtonChange;  
	pbutton->base.func_change = ButtonSelect; 
	return 0;
}



