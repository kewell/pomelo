/**
* scr_menu.h -- 菜单显示处理头文件
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#ifndef _SCR_MENU_H_
#define _SCR_MENU_H_
//------------------------------------------------------
#define SCHAR     char
#define UCHAR     unsigned char
#define VCHAR     volatile unsigned char
#define SSHORT    short
#define USHORT    unsigned short
#define VSHORT    volatile unsigned short
#define SINT      int
#define UINT      unsigned int
#define VINT      volatile unsigned int
#define SLONG     long
#define ULONG     unsigned long
#define VLONG     volatile unsigned long
#define INT64     __int64
//------------------------------------------------------
#define DO_FUNC			0x100
#define DO_SHOW		    0x200
#define DO_SHOW_CHG	    0x300

#define SHOW_ATTR		0x00
#define DO_ATTR			0x01

#define MENU_NULL	    0x00
//--------------------------------------------------------
#define START_LINE_POS	0
#define START_COL_POS	0
#define ARR_COL_POS	    104

#define PG_MNU_MAX	    7
#define MENU_LINE_POS	9
//---------------------------------------------------------


//-------------------------define key value-----------------------------
///在进入最底层菜单之前只有UPKEY，DOWNKEY，OKKEY，ESCKEY有效
#define UPKEY		0x08
#define DOWNKEY	0x02
#define LEFTKEY		0x05
#define RIGHTKEY	0x07
#define OKKEY		0x01
#define ESCKEY		0x04

/*
#define OKKEY1		0x18
#define OKKEY2		0x19
#define OKKEY3		0x20
#define OKKEY4		0x21
#define OKKEY5		0x22
#define OKKEY6		0x22
*/

#define OKKEY1		0x18
#define OKKEY2		0x19
#define OKKEY3		0x2A
#define OKKEY4		0x2B
#define OKKEY5		0x2C
#define OKKEY6		0x2D
#define OKKEY7		0x2E

/*
#define CHGFKEY		0x07
#define EXITKEY		0x08
#define ADJKEY		0x09
#define FUNCKEY		0x0a
#define OUTKEY		0x0b
#define LIGHTKEY		0x0c
#define CANCELKEY	0x0d


#define LOOPKEY		0x0e
#define PAGE_UPKEY	0x0f
#define PAGE_DOWNKEY	0x10
#define OUTKEY2		0x11
#define NULLKEY	0xff
*/

#define CHGFKEY		0x09
#define EXITKEY		0x0a
#define ADJKEY		0x0b
#define FUNCKEY		0x0c
#define OUTKEY		0x0d
#define LIGHTKEY	0x0e
#define CANCELKEY	0x0f


#define LOOPKEY		0x10
#define PAGE_UPKEY	0x11
#define PAGE_DOWNKEY	0x12
#define OUTKEY2		0x13
#define NULLKEY	0xff
//---------------------------------------------------------
#define LCD_ROW_SIZE	7
#define LCD_COL_SIZE	20  

#define PARA_SET 1
///#define PARA_SET 0
#define MET_NUM_SEL 2

///显示界面生成函数
typedef  void (*pmenu_do_func)(unsigned short id,unsigned long para,unsigned char key);


///#define MAX_SHOW_LINE	32   //最大的行
///#define MAX_SHOW_COL	20   //最大的列

#define MAX_SHOW_LINE	63   //最大的行
#define MAX_SHOW_COL	21   //最大的列


typedef struct{
	int line;      //行
	char buf[MAX_SHOW_LINE][MAX_SHOW_COL];  //显示缓冲区
	}scrpage_t,*pscrpage_t;

typedef struct{
	unsigned short  id; //测量点号
	unsigned long int para; //参数
	unsigned char key;    
	}MNU_FUC_PARA,*PMNU_FUC_PARA;


typedef struct{
	char *  title;   //标题字符串
	///unsigned char *  title;   //标题字符串
	pmenu_do_func mpmenu_do_func;  //生成下级菜单的函数，有mpmenu_do_func就没有son
	MNU_FUC_PARA para;   //显示函数的参数
	///son在后面使用时进行了一个指针类型转换，指向了MNU_ITEMP
	unsigned char * son;/*MNU_ITEMP*/   ///有son就没有mpmenu_do_func
	unsigned char attr;
	}MNU_SUB_ATTR,*PMNU_SUB_ATTR;


typedef struct{
	pmenu_do_func mpmenu_do_func;
	MNU_FUC_PARA para;
	}MNU_LOOP_FUNC,*PMNU_LOOP_FUNC;

///可以定义一个全局的结构体，以形参输入给函数进行处理
typedef struct XMNU_ITEM{
    ///PMNU_SUB_ATTR为指针类型，sub_attr作为数组使用
	PMNU_SUB_ATTR sub_attr;  ///拥有上一级滑轮菜单的子菜单，在初始化的时候被赋予一个界面的数组
	struct XMNU_ITEM * father;  ///父菜单为上一级滑轮菜单，形成一个链表
	char item_num;    ///此界面条目的个数,即sub_attr数组成员的个数
    ///确定选择菜单在界面中的行，可以作为sub_attr的数组下标
	unsigned char item_cur;    ///当前滑轮所选择的行，通过递增或递减来改变sub_attr[]的下标，从而选择不同的菜单
	unsigned int  item_action;    ///是滑轮界面还是翻页界面
	}MNU_ITEM,* MNU_ITEMP;  ///此结构体用来存放一个界面菜单所有的信息


///实现滑轮菜单的链表式连接
typedef struct{
	MNU_ITEMP mnu;          
	MNU_ITEMP root_mnu;   
	MNU_SUB_ATTR *  mnu_item;  ///为一数组类型
	}MNU_ATTR,*PMNU_ATTR;


void ReturnRoot(void);

///unsigned int GetMenu(MNU_ITEMP item);
///void Make_menu(PMNU_ATTR m_menux);
void SetSysMenu(MNU_ITEMP xmenu,MNU_LOOP_FUNC * lpf, MNU_LOOP_FUNC * almf);
///unsigned char Run_Menu(void);
void DefMenuFuc(unsigned char id,unsigned long int x,unsigned char key);

void ClrShowBuf(pscrpage_t pshowp);
void ShowPageBuf(unsigned char key);
void ShowDspBufLoop(void);
void scr_alarm(UCHAR flag, pmenu_do_func pfunc);
void scr_note(UCHAR flag, char *info);
void LoadAllMenu(PMNU_ATTR m_menux);
void InitSysMenu(MNU_ITEMP xmenu,MNU_LOOP_FUNC * lpf);
unsigned char MenuProcess(void);
//------------------------------------------------------------------------------
void DrawMenuLine(void);
#endif

