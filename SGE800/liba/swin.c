/*****************************************************************************
	许继电气股份有限公司			版权：2008-2015

	本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许
	可不得擅自修改或发布，否则将追究相关的法律责任。

						河南许昌许继股份有限公司
						www.xjgc.com
						(0374) 321 2924
*****************************************************************************/


/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  swin.c
	描述		：  本文件定义了脉冲检测模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2009.12
******************************************************************************/
//库配置头文件
#include "private/config.h"
	
//模块启用开关
#ifdef CFG_SWIN_MODULE
	
//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//open 标志	
#include <sys/ioctl.h>		//ioctl
#include <string.h> 		//memset
#include <unistd.h>		//close
	
//提供给用户的头文件
#include "include/swin.h"
#include "include/timer.h"
#include "include/error.h"
	
//驱动调用头文件
#include "private/drvlib/pulselib.h"
#include "private/drvlib/timerlib.h"
#include "private/drvlib/gpiolib.h"
#include "private/debug.h"
	
/*************************************************
  静态全局变量及宏定义
*************************************************/
#define PINBASE 32						//传入到驱动的io口地址基址
#define MAX_SWINPIN (8 + 4)				//最大io口数量,8路普通，4路确定
#define SWIN_BUFSIZE 1024				//驱动缓冲区大小，单位字节
#define CUR_SWINPIN (CFG_SWIN_NUM + 4)	//当前io口数量,4路确定

#define CFG_CHECK_FILTER_NUM	3		//检查当前io口状态消抖次数
#define CFG_CHECK_FILTER_TIME	20		//检查当前io口状态消抖时间

static int fd;							//脉冲驱动打开文件描述符
static int t_fd;						//定时器驱动打开文件描述符
static int g_fd;						//gpio驱动文件描述符

static u8 swin_count = 0;				//模快打开计数
static u16 swin_buf[SWIN_BUFSIZE];		//保存采样数据
static pthread_mutex_t mutex;			//互斥锁

static swin_time_t	swin_result[CUR_SWINPIN];		//计算结果
static u8	swin_pin[CUR_SWINPIN];		//管脚io地址
static u32	swin_num[CUR_SWINPIN];		//管脚脉冲个数
static u16	high[CUR_SWINPIN], low[CUR_SWINPIN];	//数据中高电平，地电平连续个数
static u8	swin_flag[CUR_SWINPIN];		//当前状态，脉冲高电平，低电平
static u8	swin_fun[CUR_SWINPIN];		//当前功能，1-遥信remsig，0-脉冲pulse
static	u8	pulse_change[CUR_SWINPIN] = {0}, remsig_change[CUR_SWINPIN] = {0};

/*************************************************
  API
*************************************************/
/******************************************************************************
*	函数:	swin_init
*	功能:	遥信及脉冲检测模块初始化
*	参数:	无
*	返回:	0				-	成功
			-ERR_CFG		-	配置超限
			-ERR_BUSY		-	已经打开
			-ERR_SYS		-	系统错误
			-ERR_NOFILE		-	没有此路径
 * 说明:	无
 ******************************************************************************/
int swin_init(void)
{
	int ret = -1;
	int i = 0;
	char *dev[]={"/dev/atmel_tc0","/dev/atmel_tc1","/dev/atmel_tc2",
				"/dev/atmel_tc3","/dev/atmel_tc4","/dev/atmel_tc5"};
	
	if(CFG_SWIN_NUM > MAX_SWINPIN){
		ret = -ERR_CFG; 	//配置超限
		goto err;
	}
	if(swin_count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}
	//初始化互斥锁
	if (pthread_mutex_init(&mutex, NULL)) {
		ret = -ERR_SYS;
		goto err;
	}

	//初始化数据
	memset(swin_num, 0, CUR_SWINPIN*sizeof(u32));
	memset(swin_buf, 0, SWIN_BUFSIZE*sizeof(u16));
	memset(high, 0, CUR_SWINPIN*sizeof(u16));
	memset(low, 0, CUR_SWINPIN*sizeof(u16));
	memset(swin_flag, 0, CUR_SWINPIN*sizeof(u8));
	memset(swin_fun, 0, CUR_SWINPIN*sizeof(u8));
	memset(pulse_change, 0, CUR_SWINPIN*sizeof(u8));
	memset(remsig_change, 0, CUR_SWINPIN*sizeof(u8));

	//设置gpio物理地址
#ifdef CFG_SWIN_0
	swin_pin[SWIN0] = CFG_SWIN_0 + PINBASE;
#endif
#ifdef CFG_SWIN_1
	swin_pin[SWIN1] = CFG_SWIN_1 + PINBASE;
#endif
#ifdef CFG_SWIN_2
	swin_pin[SWIN2] = CFG_SWIN_2 + PINBASE;
#endif
#ifdef CFG_SWIN_3
	swin_pin[SWIN3] = CFG_SWIN_3 + PINBASE;
#endif
#ifdef CFG_SWIN_4
	swin_pin[SWIN4] = CFG_SWIN_4 + PINBASE;
#endif
#ifdef CFG_SWIN_5
	swin_pin[SWIN5] = CFG_SWIN_5 + PINBASE;
#endif
#ifdef CFG_SWIN_6
	swin_pin[SWIN6] = CFG_SWIN_6 + PINBASE;
#endif
#ifdef CFG_SWIN_7
	swin_pin[SWIN7] = CFG_SWIN_7 + PINBASE;
#endif

	swin_pin[CFG_SWIN_NUM] = CFG_SWIN_DOOR + PINBASE;       //门接点信号对应IO
	swin_fun[CFG_SWIN_NUM] = REMSIG;
	swin_pin[CFG_SWIN_NUM+1] = CFG_SWIN_LID_TOP + PINBASE;    //开表盖信号对应IO
	swin_fun[CFG_SWIN_NUM+1] = REMSIG;
	swin_pin[CFG_SWIN_NUM+2] = CFG_SWIN_LID_MID + PINBASE;    //开中盖信号对应IO
	swin_fun[CFG_SWIN_NUM+2] = REMSIG;
	swin_pin[CFG_SWIN_NUM+3] = CFG_SWIN_LID_TAIL + PINBASE;   //开尾盖信号对应IO
	swin_fun[CFG_SWIN_NUM+3] = REMSIG;

	//打开定时器驱动
	t_fd = open(dev[CFG_SWIN_TIMER], O_RDWR | O_NOCTTY );
   	if (t_fd < 0){
   		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}
	//设置定时器脉冲采集功能
	ret = ioctl(t_fd, SET_PULSE, 0);
	if (ret < 0){
   		ret = -ERR_SYS;
		goto err;
	}
	//设置定时器时钟
	ret = ioctl(t_fd, SET_CLOCK, SCK32KIHZ); 
	if (ret < 0){
   		ret = -ERR_SYS;
		goto err;
	}
	//设置定时器延时时间
	ret = ioctl(t_fd, SET_DELAY, CFG_SWIN_FILTER_TIME);
	if (ret < 0){
   		ret = -ERR_SYS;
		goto err;
	}
	//打开脉冲开入模块驱动
	fd = open("/dev/atmel_ps", O_RDONLY );//| O_NONBLOCK
	if (fd < 0){
   		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}  
	
	ret = ioctl(fd, SET_TC, CFG_SWIN_TIMER);
	if (ret < 0){
   		ret = -ERR_SYS;
		goto err;
	}
	//加入要检测的io口
	for(i = 0; i < CUR_SWINPIN; i ++){
		ret = ioctl(fd, ADD_IO, swin_pin[i]);
		if (ret < 0){
	   		ret = -ERR_SYS;		//没有此路径
			goto err;
		} 		
	}
	//初始化rtc
	ret = rtc_init();
	if((ret < 0) && (ret != -ERR_BUSY)){
		goto err;
	}

	//打开gpio模块
	g_fd = open("/dev/atmel_gpio", O_RDWR | O_NOCTTY);
	if (g_fd < 0){
		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}

	 //启动
	ret = ioctl(fd, PSTART, 0);
	if (ret < 0){
   		ret = -ERR_SYS;	
		goto err;
	}	

	swin_count = 1;
	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	swin_set
*	功能:	设置开入模式
*	参数:	id				-	开入通道号
			mode			-	1-遥信,0-脉冲
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_NODEV		-	无此设备
*	说明:
 ******************************************************************************/
int swin_set(u8 id, u8 mode)
{
	if((id < 0) || (id >= CFG_SWIN_NUM)){		//范围检查
		return -ERR_NODEV;
	}
	if((mode != REMSIG) && (mode != PULSE)){				//范围检查
			return -ERR_INVAL;
	}
	swin_fun[id] = mode;
	return 0;
}

/******************************************************************************
*	函数:	swin_process
*	功能:	处理出所有开入的结果（包括脉冲数，遥信变位时刻）
*	参数:
*	返回:	>=0				-	标志位，低16位表示各通道是否有变化
			-ERR_INVAL		-	参数错误
			-ERR_SYS		-	系统错误，ioctl调用失败或实时时钟读取时间错误
			-ERR_NOINIT		-	锁或实时时钟模块没有初始化化
			-ERR_OTHER:		-	其他关于线程互斥锁的错误
*	说明:
 ******************************************************************************/
int swin_process(void)
{
	int ret;
	int i,j,read_byte;
	u16 * p;

	p = swin_buf;
	memset(p,0,SWIN_BUFSIZE*sizeof(u16));

	if(swin_count == 0)
		return -ERR_NOINIT;
	for(j = 0; j < CUR_SWINPIN; j ++){
		if((swin_fun[j] != REMSIG) && (swin_fun[j] != PULSE)){	//范围检查
			return -ERR_NOCFG;
		}
	}
	//获得互斥锁
	if (pthread_mutex_lock (&mutex)) {
		return	-ERR_NOINIT;
	}
	//读取数据
	ret = read(fd, (void*)p, 0);		//ret为返回字节数
	if (ret < 0){
   		ret = -ERR_SYS;
		goto err;
	}
	read_byte = ret;
	for(i = 0; i < read_byte/2 ; i++){				//i为第i次采样，j为第j路io口
		for(j = 0; j < CUR_SWINPIN; j ++){
			if(swin_fun[j] == REMSIG){

				//解析存储采样数据-遥信
				if((swin_buf[i] >> j & 0x01) == 1 ){
					if(swin_flag[j] == 0){
						high[j] ++;
						low[j] = 0;
						if(high[j] >= CFG_SWIN_FILTER_NUM){
							high[j] = 0;
							swin_flag[j] = 1;

							swin_result[j].num += 1;
							if(swin_result[j].num >= 8){	//遥信信号变位最多8个
								swin_result[j].num = 8;		//遥信信号变位最多8个
							}

							if(remsig_change[j] >= 7){	//遥信信号变位时刻buf满时替代最后一个
								remsig_change[j] = 7;	//最后一个时间
							}
							swin_result[j].jump[remsig_change[j]].polar = 1;
							ret =  rtc_gettime (&swin_result[j].jump[remsig_change[j]].time);
							if(ret < 0 ){
								goto err;
							}
							remsig_change[j] += 1;
						}
					}
				}else{
					if(swin_flag[j] == 1){
						low[j] ++;
						high[j] = 0;
						if(low[j] >= CFG_SWIN_FILTER_NUM){
							low[j] = 0;
							swin_flag[j] = 0;
							swin_result[j].num += 1;
							if(swin_result[j].num >= 8){	//遥信信号变位最多8个
								swin_result[j].num = 8;		//遥信信号变位最多8个
							}

							if(remsig_change[j] >= 7){		//遥信信号变位时刻buf满时替代最后一个
								remsig_change[j] = 7;		//最后一个时间及极性
							}
							swin_result[j].jump[remsig_change[j]].polar = 0;
							ret =  rtc_gettime (&swin_result[j].jump[remsig_change[j]].time);
							if(ret < 0 ){
								goto err;
							}
							remsig_change[j] += 1;
						}
					}
				}

			//解析存储采样数据-正脉冲
			}else if(swin_fun[j] == PULSE){
				if((swin_buf[i] >> j & 0x01) == 1 ){
					if(swin_flag[j] == 0){
						high[j] ++;
						low[j] = 0;
						if(high[j] >= CFG_SWIN_FILTER_NUM){
							swin_flag[j] = 1;
							swin_num[j] += 1;
							pulse_change[j] = 1;
						}
					}
				}else{
					low[j] ++;
					high[j] = 0;
					if(low[j] >= CFG_SWIN_FILTER_NUM)
						swin_flag[j] = 0;
				}

			}
		}
	}

	ret = 0;
	for(j = 0; j < CUR_SWINPIN ; j++){
		if((swin_fun[j] == PULSE) && (pulse_change[j] == 1)){	//如果有尚未读走的脉冲个数，对应路置1
			ret |= 1 << j;
		}
		if((swin_fun[j] == REMSIG) && (remsig_change[j] >= 1)){//如果有尚未读走的遥信，对应路置1
			ret |= 1 << j;
		}
	}

err:	//解锁
	if (pthread_mutex_unlock (&mutex)) {
		ret = -ERR_OTHER;
	}
	return ret;
}

/******************************************************************************
*	函数:	swin_read
*	功能:	读出指定通道号的脉冲数或遥信变位时刻
*	参数:	id				-	脉冲通道号
			p				-	脉冲数或遥信变位时刻（数据传出）
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_NODEV		-	无此设备
			-ERR_NOINIT		-	模块没有初始化化
*	说明:
 ******************************************************************************/
int swin_read(u8 id,swin_time_t *p)
{
	int ret;

	if(swin_count == 0)
		return -ERR_NOINIT;

	if((id < 0) || (id >= CUR_SWINPIN)){		//范围检查
		return -ERR_NODEV;
	}
	if(swin_fun[id] == PULSE){
		memset(p, 0,sizeof(swin_time_t));
		p->num = swin_num[id];
		swin_num[id] = 0;
		pulse_change[id] = 0;
	}else if(swin_fun[id] == REMSIG){
		memcpy(p, &swin_result[id],sizeof(swin_time_t));
		memset(&swin_result[id], 0,sizeof(swin_time_t));
		memset(remsig_change, 0, CUR_SWINPIN*sizeof(u8));
	}else{
		ret = -ERR_INVAL;
		goto err;
	}

	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	swin_check
*	功能:	实时查询当前状态
*	参数:	id				-	脉冲通道号
*	返回:	0				-	当前状态为低电平
*			1				-	当前状态为高电平
			-ERR_NODEV		-	无此设备
			-ERR_SYS		-	系统错误，gpio驱动ioctl调用失败
			-ERR_DEVUNSTAB	-	设备不稳定
*	说明:
 ******************************************************************************/
int swin_check(u8 id)
{
	int ret;
	u32 tmp[CFG_CHECK_FILTER_NUM];
	int i;

	fd_set rfds;
	struct timeval tv;
	int fd_t = 1;

	tv.tv_sec = 0;
	tv.tv_usec = CFG_CHECK_FILTER_TIME*1000;
	FD_ZERO (&rfds);
	FD_SET (fd_t, &rfds);

	if(swin_count == 0)
		return -ERR_NOINIT;

	if((id < 0) || (id >= CUR_SWINPIN)){		//din范围检查
		return -ERR_NODEV;
	}

	//滤波读取io口状态
	for(i = 0; i < CFG_CHECK_FILTER_NUM; i ++){
		tmp[i] = swin_pin[id];
		ret = ioctl(g_fd, IOGETI, &tmp[i]);

		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}


		if((tmp[i] < 0) || (tmp[i] > 1)){
			ret = -ERR_SYS;
			goto err;
		}
		select (0, NULL, NULL, NULL, &tv);
	}
	for(i = 0; i < CFG_CHECK_FILTER_NUM - 1; i ++){
		if(tmp[i]  != tmp[i+1]){
			ret = -ERR_DEVUNSTAB;
			goto err;
		}
	}
	ret = tmp[0];
err:
	return ret;
}

/******************************************************************************
*	函数:	swin_close
*	功能:	关闭遥信与脉冲模块
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_NOINIT		-	没有初始化
			-ERR_OTHER		-	其他关于线程互斥锁的错误
*	说明:	无
 ******************************************************************************/
int swin_close(void)
{
	int ret = -1;
	
	if(swin_count == 0)
		return -ERR_NOINIT;

	ret = close(fd);
	if(ret < 0)
		return -ERR_SYS;
	ret = close(t_fd);
	if(ret < 0)
		return -ERR_SYS;
	ret = close(g_fd);
	if(ret < 0)
		return -ERR_SYS;

	swin_count = 0;
	//销毁互斥锁	
	if (pthread_mutex_destroy(&mutex)) {
		ret = -ERR_OTHER;
	}
	ret = 0;
	return ret;
}

#endif
