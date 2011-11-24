/**
* scr_control.c -- 显示输入数字
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#include "include/debug.h"
#include "scr_input_api.h"
#include "scr_menu.h"
#include "scr_lcd.h"
///输入密码的字符清单
static unsigned int    input_port_num;
static unsigned char   task_num;
static unsigned short  sn_num;
static unsigned int    major_met_num;
static unsigned char  qur_day[3];

const char typ_charlist_a[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
};
const char typ_charlist_a1[] = {
'1', '2', '3', '4'
};
const char typ_charlist_a2[] = {
'1', '2', '3', '4', '5', '6', '7', '8'
};
const char typ_charlist_a3[] = {
 '0','1'
};
const char typ_charlist_a4[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};
const char typ_charlist_a5[] = {
	'0', '1', '2'
};
const char typ_charlist_b[] = {
	'Y', 'N',
};

/**
* @brief           确认按键处理
*/
void DefOkClick(void)
{	
	InitControlPage();  ///清掉前面的屏
	QuitControlPage(CONTROL_OK_EXIT);  ///确实之后退出本界面
}

/**
* @brief           取消按键处理
*/
void DefEscClick(void)
{
	InitControlPage();
	QuitControlPage(-1);
}


/**
* @brief           字符转化为数字
* @param c         字符
* @return          0转化失败，非0中则转化成功
*/
static UCHAR CharToNum(char c)
{
	if((c >= '0') && (c <= '9')) return(c-'0');
	else if((c >= 'a') && (c <= 'f')) return(c-'a');
	else if((c >= 'A') && (c <= 'F')) return(c-'A');
	else return 0;
}
/**
* @brief           字符转化为BCD码
* @param bcds      BCD码首地址
* @param len       BCD码的长度
* @param chars     字符串首地址
* @return          返回的文件指针
*/
static void CharToBcd(UCHAR *bcds, int len, unsigned char *chars)
{
	int i;

	for(i=0; i<len; i++) {
		bcds[i] = CharToNum(*chars++);
		bcds[i] |= CharToNum(*chars++) << 4;
	}
}

/**
* @brief       密码输入显示及处理 
* @return      0输入成功，1输入失败
*/
int ScrPwdConfirm(void)
{
    ///char editbox_rtn[6];
	unsigned char editbox_rtn[6];
	UCHAR pwd[3];
	int i;	
	DisplayNormal("请输入密码:",11,2*16-MENU_LINE_POS,0);
	AddEditBox(4, 7, 10, (char *)&typ_charlist_a[0], 9, &editbox_rtn[0], CHAR_FLAG);
	AddEditBox(4, 8, 10, (char *)&typ_charlist_a[0], 9, &editbox_rtn[1], CHAR_FLAG);	
	AddEditBox(4, 9, 10, (char *)&typ_charlist_a[0], 9, &editbox_rtn[2], CHAR_FLAG);
	AddEditBox(4, 10, 10, (char *)&typ_charlist_a[0], 9, &editbox_rtn[3], CHAR_FLAG);
	AddEditBox(4, 11, 10, (char *)&typ_charlist_a[0], 9, &editbox_rtn[4], CHAR_FLAG);
	AddEditBox(4, 12, 10, (char *)&typ_charlist_a[0], 9, &editbox_rtn[5], CHAR_FLAG);
	AddButton(6, 5, "确定", DefOkClick);
	AddButton(6, 11, "取消", DefEscClick);
	if(CONTROL_OK_EXIT == ShowControlPage())   //调用菜单编辑函数，在里面等待按键的输入
	{	
		for(i=0; i<6; i++)   ///设置的密码存放在editbox_rtn[]中，typ_charlist_a[0]的值为0
		editbox_rtn[i] += typ_charlist_a[0];
		CharToBcd(pwd, 3, editbox_rtn);   ///将字符型转化为BCD码
		///if(pwd[0]!=para_term.com_pwd[0])return 1;
		///if(pwd[1]!=para_term.com_pwd[1])return 1;
		///if(pwd[2]!=para_term.com_pwd[2])return 1;

		return 0;  ///返回值为0输入的密码才正确
	}
		return 1;
}


/**
* @brief       输入选择测量点号
* @return      0输入成功，1输入失败
*/
int SelTemSn(void)
{
	unsigned char editbox_rtn[6];

	//DisplayNormal("请输入载波表序号:",17,2*16-MENU_LINE_POS,0);
	CleanPartOfScreen(2*16-MENU_LINE_POS,7);
	DisplayNormal("请输入电表序号:",17,2*16-MENU_LINE_POS,0);
	AddEditBox(4, 8, 3, (char *)&typ_charlist_a5[0], 0, &editbox_rtn[0], CHAR_FLAG);
	AddEditBox(4, 9, 10, (char *)&typ_charlist_a[0], 0, &editbox_rtn[1], CHAR_FLAG);	
	AddEditBox(4, 10, 10, (char *)&typ_charlist_a[0], 0, &editbox_rtn[2], CHAR_FLAG);
	AddEditBox(4, 11, 10, (char *)&typ_charlist_a[0], 0, &editbox_rtn[3], CHAR_FLAG);
	DisplayNormal("      (3-2040)",15,5*16-MENU_LINE_POS,0);

	AddButton(7, 5, "确定", DefOkClick);
	AddButton(7, 11, "取消", DefEscClick);

	if(CONTROL_OK_EXIT == ShowControlPage())   //调用菜单编辑函数，在里面等待按键的输入
	{	
		sn_num = 1000*editbox_rtn[0]+100*editbox_rtn[1]+10*editbox_rtn[2]+editbox_rtn[3];
		PrintLog(0, "sn_num= %d.\r\n", sn_num);
		if(sn_num<=2 || sn_num>2039)
		{
			//ClearLcdScreen();
			//scrpbuf_main.line = 1;
			//strcpy(scrpbuf_main.buf[0], " 输入测量点号错误!");
			//strcpy(scrpbuf_main.buf[1], " 系统5秒后复位...");
			//DisplayNormal((unsigned char*)&scrpbuf_main.buf[0],strlen(&scrpbuf_main.buf[0]),16 * 3,0);
			//DisplayNormal((unsigned char*)&scrpbuf_main.buf[1],strlen(&scrpbuf_main.buf[1]),16 * 4,0);
			//DisplayLcdBuffer();
			return 1;
		}
		else
			return 0; 
	}
	return 1;
}

int input_date(void)
{
	unsigned char editbox_rtn[8];

	//DisplayNormal("请输入载波表序号:",17,2*16-MENU_LINE_POS,0);
	CleanPartOfScreen(2*16-MENU_LINE_POS,7);
	DisplayNormal("请输入查询日期:",15,2*16-MENU_LINE_POS,0);
	AddEditBox(4, 5, 3, (char *)&typ_charlist_a[0], 2, &editbox_rtn[0], CHAR_FLAG);
	AddEditBox(4, 6, 10, (char *)&typ_charlist_a[0], 0, &editbox_rtn[1], CHAR_FLAG);	
	AddEditBox(4, 7, 10, (char *)&typ_charlist_a[0], 1, &editbox_rtn[2], CHAR_FLAG);
	AddEditBox(4, 8, 10, (char *)&typ_charlist_a[0], 1, &editbox_rtn[3], CHAR_FLAG);
	//DisplayNormal(".",1,4*16-MENU_LINE_POS,80+24);
	DisplayNormal("-",1,4*16-MENU_LINE_POS,8 * 8 + 8);
	AddEditBox(4, 10, 10, (char *)&typ_charlist_a[0], 0, &editbox_rtn[4], CHAR_FLAG);
	AddEditBox(4, 11, 10, (char *)&typ_charlist_a[0], 1, &editbox_rtn[5], CHAR_FLAG);
	DisplayNormal("-",1,4*16-MENU_LINE_POS,11 * 8 + 8);
	AddEditBox(4, 13, 10, (char *)&typ_charlist_a[0], 0, &editbox_rtn[6], CHAR_FLAG);
	AddEditBox(4, 14, 10, (char *)&typ_charlist_a[0], 1, &editbox_rtn[7], CHAR_FLAG);
	

	AddButton(7, 5, "确定", DefOkClick);
	AddButton(7, 11, "取消", DefEscClick);

	if(CONTROL_OK_EXIT == ShowControlPage())   //调用菜单编辑函数，在里面等待按键的输入
	{	
		qur_day[0] = 10*editbox_rtn[2]+editbox_rtn[3];
		qur_day[1] = 10*editbox_rtn[4]+editbox_rtn[5];
		qur_day[2] = 10*editbox_rtn[6]+editbox_rtn[7];
		//sn_num = 1000*editbox_rtn[0]+100*editbox_rtn[1]+10*editbox_rtn[2]+editbox_rtn[3];
		PrintLog(0, "sn_num= %d.\r\n", sn_num);
		if(qur_day[1]<1 || qur_day[1]>12 || qur_day[2]<1 || qur_day[2] >31)
		{
			//ClearLcdScreen();
			//scrpbuf_main.line = 1;
			//strcpy(scrpbuf_main.buf[0], " 输入测量点号错误!");
			//strcpy(scrpbuf_main.buf[1], " 系统5秒后复位...");
			//DisplayNormal((unsigned char*)&scrpbuf_main.buf[0],strlen(&scrpbuf_main.buf[0]),16 * 3,0);
			//DisplayNormal((unsigned char*)&scrpbuf_main.buf[1],strlen(&scrpbuf_main.buf[1]),16 * 4,0);
			//DisplayLcdBuffer();
			return 1;
		}
		else
			return 0; 
	}
	return 1;
}




/**
* @brief       输入选择1类任务号
* @return      0输入成功，1输入失败
*/
int SelTaskNum1(void)
{

	unsigned char editbox_rtn[6];
	DisplayNormal("请输入任务号:",15,2*16-MENU_LINE_POS,0);
	DisplayNormal("        (1-8)",14,5*16-MENU_LINE_POS,0);
	AddEditBox(4, 10, 8, (char *)&typ_charlist_a2[0], 0, &editbox_rtn[0], CHAR_FLAG);
	AddButton(6, 5, "确定", DefOkClick);
	AddButton(6, 11, "取消", DefEscClick);
	///只有按下确定或取消才能退出
	if(CONTROL_OK_EXIT == ShowControlPage())   //调用菜单编辑函数，在里面等待按键的输入
	{
		task_num = editbox_rtn[0] + 1;
		return 0; 
	}
		return 1;
}
/**
* @brief       输入选择2类任务号
* @return      0输入成功，1输入失败
*/
int SelTaskNum2(void)
{
	unsigned char editbox_rtn[6];	
	DisplayNormal("请输入任务号:",15,2*16-MENU_LINE_POS,0);
	DisplayNormal("       (1-16)",13,5*16-MENU_LINE_POS,0);
	AddEditBox(4, 9, 2, (char *)&typ_charlist_a3[0], 0, &editbox_rtn[0], CHAR_FLAG);	
	AddEditBox(4, 10, 10, (char *)&typ_charlist_a4[0], 1, &editbox_rtn[1], CHAR_FLAG);
	AddButton(7, 5, "确定", DefOkClick);
	AddButton(7, 11, "取消", DefEscClick);
	///只有按下确定或取消才能退出
	if(CONTROL_OK_EXIT == ShowControlPage())   //调用菜单编辑函数，在里面等待按键的输入
	{	
		task_num = editbox_rtn[0]*10+editbox_rtn[1];
		return 0; 
	}
		return 1;
}
/**
* @brief       输入选择端口号
* @return      0输入成功，1输入失败
*/
int SelPortNum(void)
{
	unsigned char editbox_rtn[6];

	DisplayNormal("请输入端口号:",13,2*16-MENU_LINE_POS,0);	
	AddEditBox(4, 10, 4, (char *)&typ_charlist_a1[0], 0, &editbox_rtn[0], CHAR_FLAG);
    DisplayNormal("        (1-4)",13,5*16-MENU_LINE_POS,0);
	AddButton(7, 5, "确定", DefOkClick);
	AddButton(7, 11, "取消", DefEscClick);
	///只有按下确定或取消才能退出
	if(CONTROL_OK_EXIT == ShowControlPage())   //调用菜单编辑函数，在里面等待按键的输入
	{	
		input_port_num = editbox_rtn[0];
		return 0; 
	}
	return 1;
}
/**
* @brief       输入选择总表表计号
* @return      0输入成功，1输入失败
*/
int SelMajorNum(void)
{
	unsigned char editbox_rtn[6];
	///unsigned char rtn[3];
	DisplayNormal("请输入总表号:",13,2*16-MENU_LINE_POS,0);
	DisplayNormal("        (1-8)",13,5*16-MENU_LINE_POS,0);	
	AddEditBox(4, 10, 8, (char *)&typ_charlist_a2[0], 0, &editbox_rtn[0], CHAR_FLAG);
	AddButton(6, 5, "确定", DefOkClick);
	AddButton(6, 11, "取消", DefEscClick);
	if(CONTROL_OK_EXIT == ShowControlPage())   //调用菜单编辑函数，在里面等待按键的输入
	{	
		major_met_num = editbox_rtn[0] + 1;
		return 0; 
	}
	return 1;
}

/**
* @brief           得到输入的端口号
* @return          总表号
*/
unsigned int GetPortNum()
{
	return input_port_num;
}
/**
* @brief           得到输入的任务号
* @return          总表号
*/
unsigned char GetTaskNum()
{
	return task_num;
}
/**
* @brief           得到输入的电表号
* @return          总表号
*/
unsigned short GetSnNum()
{
	return sn_num;
}
/**
* @brief           得到输入的总表号
* @return          总表号
*/
unsigned int GetMajorMetNum()
{
	return major_met_num;
}


unsigned char *GetQurDay()
{
	return qur_day;
}



