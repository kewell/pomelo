
/**
* scr_task.c -- 菜单显示任务
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#include <unistd.h>
#include <string.h>
#include <stdio.h> 

#include "include/debug.h"
#include "include/sys/task.h"
#include "include/sys/schedule.h"
#include "include/screen/scr_show.h"
#include "scr_menupage.h"
#include "scr_menu.h"
#include "scr_lcd.h"
#include "include/plcmet/pltask.h"
#include "scr_lcd.h"
#include "include/plcmet/pltask.h"

///MNU_ITEM型的变量是传递给m_menu_sys的

static MNU_ITEM   m_menu_main;  
static MNU_ITEM   m_menu1;
static MNU_ITEM   m_menu2;
static MNU_ITEM   m_menu3;


extern scrpage_t scrpbuf_main;
extern unsigned short int scrhflag_alm;
extern unsigned char showlosflag;
static unsigned char lcd_back_light_conut;
//主菜单数组，每个成员为MNU_SUB_ATTR结构体
static MNU_SUB_ATTR menu_main[] = {
        ///标题     生成不带滑轮界面的函数    函数的参数      子菜单为全局结构体变量m_menu1，m_menu1在后面的make_menu函数中被赋值
	{"   测量点数据显示  ",   MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu1,SHOW_ATTR},  
	{"   参数设置与查看  ",   MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu2,SHOW_ATTR},  ///m_menu1的第一个成员的第一个元素为unsigned char类型
	{"   终端管理与维护  ",   MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu3,SHOW_ATTR},  ///m_menu1的第一个成员的第一个元素为unsigned char类型
	{MENU_NULL,MENU_NULL, {0,0,NULLKEY}, MENU_NULL,SHOW_ATTR}  ///结尾标志，方便链表判断
};


static MNU_ITEM   m_menu1_sub[2];   
///参数设置与查看界面
static MNU_SUB_ATTR menu_1[] = { 
		{" 全部测量点数据", MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu1_sub[0], SHOW_ATTR},
         //#if PARA_SET 
		//{" 台区总表参数", MENU_NULL, {0,0,OKKEY6}, (unsigned char *)&m_menu2_sub[1], SHOW_ATTR},
		{" 单个测量点数据", MENU_NULL, {0,0,OKKEY1}, (unsigned char *)&m_menu1_sub[1], SHOW_ATTR},
		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};



#if 0
static MNU_ITEM   m_menu1_1_sub[2];  

//static MNU_ITEM   m_menu1_sub[3];  
///测量点数据显示界面
static MNU_SUB_ATTR menu1_1[] = { 
		{" 当前抄表数据", ScrpPlmMetDay, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 日冻结数据", ScrpPlmMetDayData, {0,0,OKKEY7}, MENU_NULL, SHOW_ATTR},
		{" 重点用户数据", ScrpImportantUserData, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		//{" 载波抄表信息", ScrpPlmInfo, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},

		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};

static MNU_SUB_ATTR menu1_2[] = { 
		{" 当前抄表数据", ScrpPlmMetDayData, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 日冻结数据", ScrpPlmMetDayData, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 重点用户数据", ScrpImportantUserData, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		//{" 载波抄表信息", ScrpPlmInfo, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},

		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};
#endif


#if 1
//static MNU_ITEM   m_menu1_1_sub[2];  

//static MNU_ITEM   m_menu1_sub[3];  
///测量点数据显示界面
static MNU_SUB_ATTR menu1_1[] = { 
		{" 当前抄表数据", MENU_NULL, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 日冻结数据", MENU_NULL, {0,0,OKKEY7}, MENU_NULL, SHOW_ATTR},
		{" 重点用户数据", MENU_NULL, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		//{" 载波抄表信息", ScrpPlmInfo, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},

		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};

static MNU_SUB_ATTR menu1_2[] = { 
		{" 当前抄表数据", MENU_NULL, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 日冻结数据", MENU_NULL, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 重点用户数据", MENU_NULL, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		//{" 载波抄表信息", ScrpPlmInfo, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},

		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};
#endif


static MNU_ITEM   m_menu2_sub[2];   
///参数设置与查看界面
static MNU_SUB_ATTR menu_2[] = { 
		{" 终端参数    ", MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu2_sub[0], SHOW_ATTR},
         //#if PARA_SET 
		//{" 台区总表参数", MENU_NULL, {0,0,OKKEY6}, (unsigned char *)&m_menu2_sub[1], SHOW_ATTR},
		{" 台区总表参数", MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu2_sub[1], SHOW_ATTR},
         //#endif  
		 #if PARA_SET 
		{" 集抄电表参数    ",ScrpUserMetPara, {0,0,OKKEY2}, MENU_NULL, SHOW_ATTR},//没有子菜单
         #endif  
		 {" 通信参数设置", ScrShowTermPara, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		//{" 终端配置信息", MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu3_sub[2], SHOW_ATTR},
		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};

///终端参数的子菜单
static MNU_ITEM   m_menu2_1_sub[1];  
static MNU_SUB_ATTR menu2_1[] = {
		{" 通信参数", ScrpTermParaCom, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 端口抄表参数", MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu2_1_sub[0], SHOW_ATTR},
		///{" 抄表参数", ScrpReadMetPara, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		//{" 定时上报参数", MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu2_1_sub[1], SHOW_ATTR},
		//{" 事件记录配置", ScrpEventReadCfg, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		///{" 端口参数", MENU_NULL, {0,0,NULLKEY}, (unsigned char *)&m_menu3_1_sub[1], SHOW_ATTR},
		//{" 级联参数", ScrpCascadePara, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};

///端口参数子菜单
static MNU_SUB_ATTR menu2_1_2[] = {
		{" 载波端口参数", ScrpReadMetPara, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 485端口参数", ScrpReadMetPara, {0,1,NULLKEY}, MENU_NULL, SHOW_ATTR},
		//{" 端口3", ScrpReadMetPara, {0,2,NULLKEY}, MENU_NULL, SHOW_ATTR},
		//{" 端口4", ScrpReadMetPara, {0,3,NULLKEY}, MENU_NULL, SHOW_ATTR},

		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};


///总表参数
static MNU_SUB_ATTR menu2_2[] = {
		{" 基本参数", ScrpMajorMetBasePara, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{" 限值参数", ScrpMajorMetLmtPara, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},

		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};

static MNU_SUB_ATTR menu_3[] = { 
		{"      运行信息	   ", ScrpRunState, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{"      表计配置      ", ScrpMeterCfg, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{"      抄表状态      ", ScrpReadMeterStat, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{"      点抄电表      ", ScrpVesionInfo, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{"      重启终端      ", ScrpRestartTerm, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{"      终端配置      ", ScrpOtherCfg, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{"      版本信息      ", ScrpVesionInfo, {0,0,NULLKEY}, MENU_NULL, SHOW_ATTR},
		{MENU_NULL,MENU_NULL,{0,0,NULLKEY},MENU_NULL,SHOW_ATTR}
};

///将所有的菜单生成函数静态的存放在menu_all[]这个数组
///放在数组中的目的是为了方便实现对系统所有菜单实现链表式的连接
static MNU_ATTR menu_all[] = {
	{&m_menu_main,MENU_NULL,menu_main},  ///主菜单
	{&m_menu1,&m_menu_main,menu_1},      ///测量点数据显示
	{&m_menu1_sub[0],&m_menu1,menu1_1},  
	{&m_menu1_sub[1],&m_menu1,menu1_2},  
#if PARA_SET
	{&m_menu2,&m_menu_main,menu_2},      ///参数设置与查看
#endif
	{&m_menu3,&m_menu_main,menu_3},      ///终端管理与维护
	{&m_menu2_sub[0],&m_menu2,menu2_1},  
	{&m_menu2_sub[1],&m_menu2,menu2_2},  
	{&m_menu2_1_sub[0],&m_menu2_sub[0],menu2_1_2},

	{MENU_NULL,MENU_NULL,MENU_NULL}
};

//巡显菜单数组
static MNU_LOOP_FUNC loop_shows[] = {
		{ScrpHead,{0,0,NULLKEY}},   ///公司
		{ScrpMetDataEne,{0,0,NULLKEY}},  ///1号总表电能量
		{ScrpMetDataImm, {0,0,NULLKEY}}, ///1号总表瞬时量

		{ScrpMetDataEne,{1,0,NULLKEY}},  
		{ScrpMetDataImm, {1,0,NULLKEY}}, 

		{ScrpMetDataEne,{2,0,NULLKEY}},  
		{ScrpMetDataImm, {2,0,NULLKEY}}, 

		{ScrpMetDataEne,{3,0,NULLKEY}},  
		{ScrpMetDataImm, {3,0,NULLKEY}}, 

		{ScrpMetDataEne,{4,0,NULLKEY}},  
		{ScrpMetDataImm, {4,0,NULLKEY}}, 

		{ScrpMetDataEne,{5,0,NULLKEY}},  
		{ScrpMetDataImm, {5,0,NULLKEY}}, 

		{ScrpMetDataEne,{6,0,NULLKEY}},  
		{ScrpMetDataImm, {6,0,NULLKEY}},

		{ScrpMetDataEne,{7,0,NULLKEY}},  
		{ScrpMetDataImm, {7,0,NULLKEY}}, 

		{MENU_NULL, {0,0,NULLKEY}}  ///末尾标志
};

/**
* @brief			lcd显示任务
* @param arg		参数
*/
void ClearLcdBackLightConut(void)
{
	lcd_back_light_conut = 0;
}


void *screen_task(void *arg)
{
	Sleep(10);
	InitLcd();
	ShowTopFlag();
	LoadAllMenu(menu_all);  ///构成各界面菜单的链表
	InitSysMenu(&m_menu_main,loop_shows); 
   
	while(1)
	{
		MenuProcess();        //按键显示处理 
		Sleep(10);
	}

	return 0;
}

int CheckCSQ1();
extern unsigned char GprsDevSigState;
void *screen_task1(void *arg)
{
	unsigned char one_second_conut = 0;
	unsigned char term_stat_check = 0;
	unsigned char botm_displ_buf[30];

	while(1)
	{
	    	one_second_conut++;
		//if(one_second_conut >= 80)
		if(one_second_conut >= 80 * 3)	
		{	
			one_second_conut = 0;
			//GprsDevSigState = CheckCSQ1();
            		ShowTopFlag();
		}
		if(GetLcdBakLightState())
		{
			lcd_back_light_conut++;
			if(lcd_back_light_conut >= 120)
			{
				lcd_back_light_conut = 0;
				LcdBakLight(0);
				CloseLcdBakLightState();
			}
		}

		#if 1
		if((meter_total_cnt - cen_meter_cnt))	
		{
			term_stat_check++;
			//if(term_stat_check >= 10 * 30)
			if(term_stat_check >= 100)	
			{	
				term_stat_check = 0;
				CleanPartOfScreen(16 * 9,1);

				memset(botm_displ_buf,'\0',30);
				if(!read_meter_finish)
				{
					sprintf(botm_displ_buf, "终端正在抄表... %d",meter_read_succ_cnt*100/(meter_total_cnt - cen_meter_cnt));
					strcat(botm_displ_buf,"%");	
				}
				else
				{
					sprintf(botm_displ_buf, "终端抄表完成! %d",meter_read_succ_cnt*100/(meter_total_cnt - cen_meter_cnt));
					strcat(botm_displ_buf,"%");
				}
				
				DisplayNormal(botm_displ_buf,strlen((char *)botm_displ_buf),BTM_LINE_ROW+1,0);
				DisplayLcdBuffer();
			}
		}
		#endif
		Sleep(10);
	}

	return 0;
}



DECLARE_INIT_FUNC(ScreenInit);


/**
* @brief			lcd显示任务初始化
* @return			返回值为0则初始化成功，否则失败
*/
int ScreenInit(void)
{
	SysCreateTask(screen_task, NULL);
	SysCreateTask(screen_task1, NULL);
	SET_INIT_FLAG(ScreenInit);

	return 0;
}





