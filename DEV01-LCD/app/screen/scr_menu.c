/**
* scr_menu.c -- 菜单显示处理
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#include <string.h>
#include "scr_menu.h"
#include "scr_lcd.h"
#include "scr_input_api.h"
#include "include/debug.h"
#include "include/screen/scr_show.h"
#include "scr_lcd.h"

static MNU_ITEMP   m_menu_sys;  
scrpage_t scrpbuf_main;// 显示菜单缓冲区
unsigned char loopuseflag=0; 
static pscrpage_t pcurshow;   ///当前显示缓冲区指针
unsigned char loopshowfalg = 0 ;
static int curshowlineidx,curshowcolidx;  
static int loopfuncidx;
static MNU_LOOP_FUNC * mlpf;
static MNU_LOOP_FUNC *mfloop;


void PushKey(char key);
int  SelTemSn(void);
int input_date(void);
void InitControlPage(void);
int ScrPwdConfirm(void);
int SelTaskNum1(void);
int SelTaskNum2(void);
int SelMajorNum(void);
int SelPortNum(void);
char KeyKbHit(void);
void DisplayPageNumber(int number,unsigned char x);
char GetLoopState();
void SetLoopState(unsigned char LoopState);
/**
* @brief           得到菜单所处的位置
* @param item      菜单属性结构体变量
* @return          返回的按键值
*/
unsigned int GetMenuSelPosition(MNU_ITEMP item)
{
	unsigned char xkey,x=0,start,end,i=0;
	start = 0;  
	end   = 0; 
	
	if(item->item_num == 0) return (NULLKEY); 
	if(item->item_cur == NULLKEY)   
	{   
		item->item_cur = 0; 
		xkey = NULLKEY;
		goto mnushow;
	}
	if(item->item_action == DO_SHOW_CHG)
	{  
		xkey = NULLKEY;
		goto mnushow;
	}
	
	xkey = KeyKbHit();
	

	if(xkey==0)return (0);  
	if(m_menu_sys->item_action==DO_FUNC) return(xkey);
	if(xkey != DOWNKEY && xkey != UPKEY)return xkey;
	if(xkey == DOWNKEY&&loopshowfalg!=1) 
	{
		if(item->item_cur < item->item_num-1) 
		item->item_cur++; 
		else 
		item->item_cur = 0;
	}
	if(xkey == UPKEY&&loopshowfalg!=1)  
	{
		if(item->item_cur)
		item->item_cur--; 
		else 
		item->item_cur = item->item_num-1;
	}
	
mnushow:
	if(loopshowfalg!=1)
	{
		if(item->item_cur >= PG_MNU_MAX)  
		{
			start = item->item_cur+1-PG_MNU_MAX;  
			end   = item->item_cur+1;
			CleanPartOfScreen(16 * (i+2)-MENU_LINE_POS,7);
			for(i = 0; i < 7; i++)
		 	{
				DisplayNormal((unsigned char*)(item->sub_attr[start+i].title),strlen(item->sub_attr[start].title),16 * (i+2)-MENU_LINE_POS,0);
				SetLineInvs(8*16-MENU_LINE_POS);
		 	}
		}
		else 
		{
			start = 0;
			if(item->item_num<PG_MNU_MAX)
				end = item->item_num;
			else 
				end = PG_MNU_MAX;

		 	CleanPartOfScreen(16 * (x+2)-MENU_LINE_POS,7);
		 	for(x = start; x < end; x++)
		 	{
				DisplayNormal((unsigned char*)(item->sub_attr[x].title),strlen(item->sub_attr[x].title),16 * (x+2)-MENU_LINE_POS,0);
				if(x == item->item_cur) 
					SetLineInvs(16 * (x+2)-MENU_LINE_POS);
		 	}
		}
		CleanPageNum();
		CleanLoopMetNum();
		DisplayLcdBuffer();   
	}
	return (xkey);   
}

/**
* @brief           让本系统的各个界面菜单构成一个链表
* @param m_menux   要载入的菜单数组
*/
void LoadAllMenu(PMNU_ATTR m_menux)
{	
	unsigned char  i;
	
	for(;m_menux->mnu != MENU_NULL;m_menux++)
	{
		m_menux->mnu->father = m_menux->root_mnu;
		m_menux->mnu->sub_attr = m_menux->mnu_item;		
		m_menux->mnu->item_cur=NULLKEY; 

		i = 0;
		while(m_menux->mnu->sub_attr[i].title != MENU_NULL)
		{
			i++;  
		}
		m_menux->mnu->item_num = i;  
		m_menux->mnu->item_action = DO_SHOW; 
	}
	
}

/**
* @brief           初始化系统菜单
* @param xmenu     基本菜单
* @param xlpf      轮显菜单
*/
void InitSysMenu(MNU_ITEMP xmenu,MNU_LOOP_FUNC * lpf)
{
	m_menu_sys = xmenu;    
	mlpf = mfloop = lpf;       
	pcurshow = &scrpbuf_main;
	ClrShowBuf(&scrpbuf_main); 
	curshowcolidx = 0;  ///列
	curshowlineidx = 0; ///行
	loopshowfalg = 0;   ///巡显
	loopfuncidx = 0;    ///巡显的函数
}

/**
* @brief           清除显示缓冲区
* @param pshowp    缓冲区指针
*/
void ClrShowBuf(pscrpage_t pshowp)
{
	memset((UCHAR *)pshowp, 0, sizeof(scrpage_t));
	pshowp->line = 0;
}


#if 0
/**
* @brief           显示翻页菜单内容
* @param xkey      响应的按键
* @param loop_flag 轮显翻页标志
*/
void ShowScrPage(unsigned char xkey,unsigned char loop_flag)
{
	int start,end,i,j;
	int tint, tdot,num;
	int sum;

	ClearMenuBuffer();

	if(xkey == DOWNKEY)
		xkey = PAGE_DOWNKEY;
	if(xkey == UPKEY)
		xkey = PAGE_UPKEY;  
	
	if(xkey == PAGE_DOWNKEY)  
	{ 
		if(curshowlineidx+PG_MNU_MAX < pcurshow->line) 
			curshowlineidx += PG_MNU_MAX;  
		else 
			curshowlineidx = 0; 
	}
	else if(xkey == PAGE_UPKEY)
	{ 
		if(curshowlineidx >= PG_MNU_MAX) 
			curshowlineidx -= PG_MNU_MAX;
		else 
			curshowlineidx = ((pcurshow->line-1)/PG_MNU_MAX)*PG_MNU_MAX;
	}
	
	tint = curshowlineidx/PG_MNU_MAX;
	num = tint+1;                     
	tint = pcurshow->line/PG_MNU_MAX;  
	tdot = pcurshow->line%PG_MNU_MAX;
	if(tdot) sum = tint+1;   
	else sum = tint;
	if(num>sum) num = sum;
	
	start = curshowlineidx;        
	end = curshowlineidx+PG_MNU_MAX;		
	CleanPartOfScreen(2*16-MENU_LINE_POS,7);

    if(loop_flag==0)
    {
    		DisplayPageNumber(num,16);
		DisplayPageNumber(sum,18);
		DisplayPageNumber(10,17);
    }
	else
		CleanPageNum();

	if(num > 0)
	{
		for(j=0,i=(num-1)*PG_MNU_MAX; i<num*PG_MNU_MAX; i++)
		{
	    	DisplayNormal((unsigned char *)pcurshow->buf[i],strlen(pcurshow->buf[i]),16 * (++j+1)-MENU_LINE_POS ,0);
		}
	}
	DisplayLcdBuffer(); 
}
#endif


/**
* @brief           显示翻页菜单内容
* @param xkey      响应的按键
* @param loop_flag 轮显翻页标志
*/
void ShowScrPage(unsigned char xkey,unsigned char loop_flag)
{
	int start,end,i,j;
	int tint, tdot,num;
	int sum;

	ClearMenuBuffer();

	if(xkey == DOWNKEY)
		xkey = PAGE_DOWNKEY;
	if(xkey == UPKEY)
		xkey = PAGE_UPKEY;  
	
	if(xkey == PAGE_DOWNKEY)  
	{ 
		if(curshowlineidx+PG_MNU_MAX < pcurshow->line) 
			curshowlineidx += PG_MNU_MAX;  
		else 
			curshowlineidx = 0; 
	}
	else if(xkey == PAGE_UPKEY)
	{ 
		if(curshowlineidx >= PG_MNU_MAX) 
			curshowlineidx -= PG_MNU_MAX;
		else 
			curshowlineidx = ((pcurshow->line-1)/PG_MNU_MAX)*PG_MNU_MAX;
	}
	
	tint = curshowlineidx/PG_MNU_MAX;
	num = tint+1;                     
	tint = pcurshow->line/PG_MNU_MAX;  
	tdot = pcurshow->line%PG_MNU_MAX;
	if(tdot) sum = tint+1;   
	else sum = tint;
	if(num>sum) num = sum;
	
	start = curshowlineidx;        
	end = curshowlineidx+PG_MNU_MAX;		
	CleanPartOfScreen(2*16-MENU_LINE_POS,7);

	if(loop_flag==0)
	{
		
		if(sum >= 1 && sum  <= 9)
		{
			DisplayPageNumber(num,16);
			DisplayPageNumber(sum,18);
			DisplayPageNumber(10,17);
		}
		
		else if(sum >= 10 && sum <= 99)
		{
			if(num >= 10 && num <= 99)
			{
				DisplayPageNumber(num / 10,15);
				DisplayPageNumber(num % 10,16);
				DisplayPageNumber(10,17);
				DisplayPageNumber(sum / 10,18);
				DisplayPageNumber(sum % 10,19);
			}
			else if(num >= 1 && num <= 9)
			{
				DisplayPageNumber(num,16);
				DisplayPageNumber(10,17);
				DisplayPageNumber(sum / 10,18);
				DisplayPageNumber(sum % 10,19);
			}
		}
		
		else if(sum >= 100 && sum <= 999)
		{
			if(num >= 100 && num <= 999)
			{
				DisplayPageNumber(num /100,13);
				DisplayPageNumber((num %100)/10,14);
				DisplayPageNumber((num %100)%10,15);
				DisplayPageNumber(10,16);
				DisplayPageNumber(sum /100,17);
				DisplayPageNumber((sum %100)/10,18);
				DisplayPageNumber((sum %100)%10,19);
			}
			else if(num >= 10 && num <= 99)
			{
				DisplayPageNumber(num / 10,14);
				DisplayPageNumber(num % 10,15);
				DisplayPageNumber(10,16);
				DisplayPageNumber(sum /100,17);
				DisplayPageNumber((sum %100)/10,18);
				DisplayPageNumber((sum %100)%10,19);
			}
			else if(num >= 1 && num <= 9)
			{
				DisplayPageNumber(num,15);
				DisplayPageNumber(10,16);
				DisplayPageNumber(sum /100,17);
				DisplayPageNumber((sum %100)/10,18);
				DisplayPageNumber((sum %100)%10,19);
			}
		}
		
	}
	else
		CleanPageNum();

	if(num > 0)
	{
		for(j=0,i=(num-1)*PG_MNU_MAX; i<num*PG_MNU_MAX; i++)
		{
	    	DisplayNormal((unsigned char *)pcurshow->buf[i],strlen(pcurshow->buf[i]),16 * (++j+1)-MENU_LINE_POS ,0);
		}
	}
	DisplayLcdBuffer(); 
}




/**
* @brief		处理轮显显示
*/
void ShowDspBufLoop(void)
{	
	
	Dsp_loop:		
    
	if(mlpf[loopfuncidx].mpmenu_do_func != MENU_NULL)   ///轮显界面
	{
		if(curshowlineidx == 0 && loopuseflag==0)
		{
		    pcurshow = &scrpbuf_main;
			ClrShowBuf(&scrpbuf_main);  ///调入新的显示函数
        	scrpbuf_main.line = 0;
			mlpf[loopfuncidx].mpmenu_do_func(mlpf[loopfuncidx].para.id,mlpf[loopfuncidx].para.para,mlpf[loopfuncidx].para.key);
			if(0 == scrpbuf_main.line) 
			{
				loopfuncidx++;   ///换一个显示函数
				loopuseflag=0; 
				goto Dsp_loop;
			}

			if(scrpbuf_main.line >7)  ///说明同一个显示函数需要翻页
			{
					ShowScrPage(OKKEY,1); 
					loopuseflag=1;   ///同一个显示函数需要翻页标志
					return;			
			}
						
		}
	}
	else 
	{//NULL function
		if(loopfuncidx==0)
			DefMenuFuc(0,0,NULLKEY);
		else 
		{
			loopfuncidx = 0;  ///显示回到数组0，重新开始巡显
			loopuseflag=0;    
			return;
		}
	}

	ShowScrPage(PAGE_DOWNKEY,1);  ///显示载入的scrpbuf_main

	if(curshowlineidx != 0)
	{
		if(curshowlineidx+PG_MNU_MAX >=scrpbuf_main.line)
		{	
			curshowlineidx=0; ///当前行被置0
			loopfuncidx++; ///转到下一个显示函数
			loopuseflag=0; ///清0
			return;
		}
	}

	
	if(curshowlineidx == 0)   ///如果当前行为0
	{
		loopfuncidx++;  ///转到下一个显示函数
		loopuseflag=0;
	}
}

//返回到根菜单
///在进入轮显之前，让m_menu_sys返回到链表的根菜单节点
/**
* @brief           返回到根菜单
*/
void ReturnRoot(void)
{
	while(m_menu_sys->father != MENU_NULL)
	{
		m_menu_sys = m_menu_sys->father;  ///一直返回到最上层菜单
	}
	m_menu_sys->item_action = DO_SHOW; 
	m_menu_sys->item_cur = NULLKEY;
}


/**
* @brief           返回到轮显界面
*/
void ReturnToLoop(void)
{
	while(m_menu_sys->father != MENU_NULL)
	{
		m_menu_sys = m_menu_sys->father;  ///一直返回到最上层菜单
	}
}

/**
* @brief           菜单显示处理
* @return          按键值
*/
unsigned char MenuProcess(void)
{
	unsigned char ch ;
	unsigned char id = 0;
	unsigned long int para = 0;
	
	ch = GetMenuSelPosition(m_menu_sys);	
		///轮显处理
    if(ch == LOOPKEY&&GetLoopState()==1)   ///进入轮显后翻页
	{
		loopshowfalg = 1;
		ShowDspBufLoop();    ///显示轮显
     }
	 else if(ch == LOOPKEY&&GetLoopState()==2)  ///触发进入轮显
	{
		LcdBakLight(0);
		CloseLcdBakLightState();
		loopshowfalg = 1;
		ReturnRoot();
		CleanPartOfScreen(2*16-MENU_LINE_POS,7);
		DisplayLcdBuffer(); 
		curshowcolidx = 0;  ///列
		curshowlineidx = 0; ///行
		loopfuncidx = 0;    ///回到巡显的第一个函数
		pcurshow = &scrpbuf_main;
		ClrShowBuf(&scrpbuf_main); 
		mlpf[loopfuncidx].mpmenu_do_func(mlpf[loopfuncidx].para.id,mlpf[loopfuncidx].para.para,mlpf[loopfuncidx].para.key);
		ShowDspBufLoop();    ///显示轮显
     }
	 ///触发退出轮显
	 else if((ch == ESCKEY||ch == OKKEY||ch == UPKEY||ch == DOWNKEY||ch == LEFTKEY||ch == RIGHTKEY)&&loopshowfalg == 1)
	 {
	  	ch = 0;
		loopshowfalg = 0;
	 	pcurshow = &scrpbuf_main;
		ClrShowBuf(&scrpbuf_main); 
		CleanPartOfScreen(2*16-MENU_LINE_POS,7);
		CleanLoopMetNum();
		curshowcolidx = 0;  ///列
		curshowlineidx = 0; ///行
		loopshowfalg = 0;   ///巡显
		loopfuncidx = 0;    ///回到巡显的第一个函数
	 	ReturnRoot();
	 }

	if(m_menu_sys->item_action == DO_SHOW_CHG)
		m_menu_sys->item_action = DO_SHOW; 
    ///最底层翻页菜单显示
	if(m_menu_sys->item_action == DO_FUNC)  
	{ 
		ShowScrPage(ch,0);  	
		switch(ch)  
		{
		case OKKEY: 
			break;
		case ESCKEY: 
			m_menu_sys->item_action = DO_SHOW_CHG;   ///离开DO_FUNC处理
			break;
		case EXITKEY://退出菜单，在点击确认时产生此按键
			m_menu_sys->item_action = DO_SHOW_CHG;
          	break;
		default:	break;
		}
	}
	///非翻页带滑轮菜单显示
	else 
	{
		switch(ch)
		{
		case OKKEY:    ///进入到下一层菜单
			///如果没有子菜单
			if(m_menu_sys->sub_attr[m_menu_sys->item_cur].mpmenu_do_func != MENU_NULL)
			{
				curshowlineidx = 0;
			  	#if PARA_SET    
			    if(m_menu_sys->sub_attr[m_menu_sys->item_cur].para.key == OKKEY2) 
				{
					ClrShowBuf(&scrpbuf_main);
					InitControlPage();
					if(SelTemSn())  
					{
						m_menu_sys->item_action = DO_SHOW_CHG; 
						break;
					}
				}
				
				///else if(m_menu_sys->sub_attr[m_menu_sys->item_cur].para.key == OKKEY5) 
				///{
				///	ClrShowBuf(&scrpbuf_main);
				///	InitControlPage();
				///	if(SelPortNum())  
				///	{
				///		m_menu_sys->item_action = DO_SHOW_CHG; 
				///		break;
				///	}
				///}

				else if(m_menu_sys->sub_attr[m_menu_sys->item_cur].para.key == OKKEY3) 
				{
					ClrShowBuf(&scrpbuf_main);
					InitControlPage();
					if(SelTaskNum1())  
					{
						m_menu_sys->item_action = DO_SHOW_CHG;  
						break;
					}
				}
				
				else if(m_menu_sys->sub_attr[m_menu_sys->item_cur].para.key == OKKEY4) 
				{
					ClrShowBuf(&scrpbuf_main);
					InitControlPage();
					if(SelTaskNum2())  
					{
						m_menu_sys->item_action = DO_SHOW_CHG;  
						break;
					}
				}
				else if(m_menu_sys->sub_attr[m_menu_sys->item_cur].para.key == OKKEY7) 
				{
					ClrShowBuf(&scrpbuf_main);
					InitControlPage();
					if(input_date()) 
					{
						m_menu_sys->item_action = DO_SHOW_CHG; 
						break;
					}
				}

				#endif
			   	id = m_menu_sys->sub_attr[m_menu_sys->item_cur].para.id;
			   	para =  m_menu_sys->sub_attr[m_menu_sys->item_cur].para.para;
			   	ClrShowBuf(&scrpbuf_main);
			   	m_menu_sys->sub_attr[m_menu_sys->item_cur].mpmenu_do_func(id,para,NULLKEY);				
               	PushKey(OKKEY);
			   	m_menu_sys->item_action = DO_FUNC; ///跳到DO_FUNC判断去执行
			}
			///如果有子菜单
			else if(m_menu_sys->sub_attr[m_menu_sys->item_cur].son != MENU_NULL)
			{
                curshowlineidx = 0;
				if(m_menu_sys->sub_attr[m_menu_sys->item_cur].para.key == OKKEY) 
				{
					ClrShowBuf(&scrpbuf_main);
					InitControlPage();
					if(ScrPwdConfirm())  
					{
						m_menu_sys->item_action = DO_SHOW_CHG;  
						break;
					}
				}
    			else if(m_menu_sys->sub_attr[m_menu_sys->item_cur].para.key == OKKEY1) 
    			{
    				ClrShowBuf(&scrpbuf_main);
    				InitControlPage();
    				if(SelTemSn())  
    				{
    					m_menu_sys->item_action = DO_SHOW_CHG;  
    					break;
    				}
    			}
				
				
	            else if(m_menu_sys->sub_attr[m_menu_sys->item_cur].para.key == OKKEY6) 
				{
					ClrShowBuf(&scrpbuf_main);
					InitControlPage();
					if(SelMajorNum()) 
					{
						m_menu_sys->item_action = DO_SHOW_CHG; 
						break;
					}
				}


				
	     		m_menu_sys = (MNU_ITEMP)m_menu_sys->sub_attr[m_menu_sys->item_cur].son;
	            m_menu_sys->item_action = DO_SHOW_CHG; 
				m_menu_sys->item_cur = NULLKEY;
			}
            ///未定义菜单处理
			else  
			{
				curshowlineidx = 0;
				DefMenuFuc(0,0,NULLKEY);  
				m_menu_sys->item_action = DO_FUNC;
			}
			break;
		case ESCKEY:   ///退到上一层菜单
			if(m_menu_sys->father!=MENU_NULL)
			{   
				m_menu_sys = m_menu_sys->father;  
				m_menu_sys->item_action = DO_SHOW_CHG; 
			}
			break;
		default:	break;
		}
	}

     
	return ch;   
}
/**
* @brief			默认菜单处理
* @param mid		表号
* @param arg		显示参数
* @param key		按键值
*/
void DefMenuFuc(unsigned char id,unsigned long int x,unsigned char key)
{
	//strcpy(scrpbuf_main.buf[0],"未定义菜单项" );
	//strcpy(scrpbuf_main.buf[1],"未定义菜单项" );
	//strcpy(scrpbuf_main.buf[2],"未定义菜单项" );              
	//scrpbuf_main.line = 3;              
	strcpy(scrpbuf_main.buf[0],"         " );           
	scrpbuf_main.line = 1;       
}                    




