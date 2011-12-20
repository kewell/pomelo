/******************************************************************************
	项目名称	：  SGE800计量智能终端平台
	文件		：  swin.c
	描述		：  本文件定义了脉冲检测模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
//库配置头文件
#include "framework/config.h"
	
//模块启用开关
#ifdef CFG_SWIN_DEVICE
	
//C库头文件
#include <stdio.h>
#include <fcntl.h> 		//open 标志	
#include <sys/ioctl.h>		//ioctl
#include <string.h> 		//memset
#include <unistd.h>		//close
#include <sys/select.h>	//select

//基础平台头文件
#include "sge_core/device/pulselib.h"
#include "sge_core/device/timerlib.h"
#include "sge_core/device/gpiolib.h"
#include "sge_core/error.h"
#include "sge_core/pinio.h"
#include "sge_core/thread.h"
//#include "sge_core/rtc.h"

//业务平台头文件
#include "framework/device/swin.h"
#include "framework/message.h"
#include "framework/base.h"
#include "framework/systime.h"

/*************************************************
  静态全局变量及宏定义
*************************************************/
#define PINBASE 32						//传入到驱动的io口地址基址
#define MAX_SWINPIN (8 + 4)				//最大io口数量,8路普通，4路确定
#define SWIN_BUFSIZE 1024				//驱动缓冲区大小，单位字节
#define CUR_SWINPIN (CFG_SWIN_NUM + 4)		//当前io口数量,4路确定

#define CFG_CHECK_FILTER_NUM	3		//检查当前io口状态消抖次数
#define CFG_CHECK_FILTER_TIME	20		//检查当前io口状态消抖时间

#ifndef CFG_SWIN_READ_CYCLE
	#define CFG_SWIN_READ_CYCLE 1000
#endif

//#define CFG_SWIN_IOSTATE0		1			//IO初始状态电平
//#define CFG_SWIN_IOSTATE_DOOR	1			//IO初始状态电平
//#define CFG_SWIN_IOSTATE_TOP	1			//IO初始状态电平
//#define CFG_SWIN_IOSTATE_MID	1			//IO初始状态电平
//#define CFG_SWIN_IOSTATE_TAIL	1			//IO初始状态电平



static int fd;							//脉冲驱动打开文件描述符
static int t_fd;						//定时器驱动打开文件描述符
static int g_fd;						//gpio驱动文件描述符

static u8 swin_count = 0;				//模快打开计数
static pthread_mutex_t mutex;			//互斥锁

static u16			swin_buf[SWIN_BUFSIZE];		//保存采样数据
static swin_time_t	swin_result[CUR_SWINPIN];	//计算结果
static u8			swin_pin[CUR_SWINPIN];		//管脚io地址
static u32			swin_num[CUR_SWINPIN];		//管脚脉冲个数
static u16			high[CUR_SWINPIN], low[CUR_SWINPIN];	//数据中高电平，地电平连续个数
static u8			swin_iostate[CUR_SWINPIN];		//当前状态，脉冲高电平，低电平
static u8			swin_fun[CUR_SWINPIN]={0};		//当前功能，2-遥信remsig，1-脉冲pulse
static	u8			pulse_change[CUR_SWINPIN], remsig_change[CUR_SWINPIN];

//遥信消息环形缓冲区

#define SWIN_BUFF_SIZE 256			//循环队列大小
#define SWIN_BUFF_MSG_SIZE 8		//循环队列中每帧消息的大小
static struct swin_fifo_t {
	u8 buffer[SWIN_BUFF_SIZE];
	u8 in;
	u8 out;
}swin_fifo;
//环形缓冲区操作宏定义
#define BUF_HEAD 		(swin_fifo.buffer[swin_fifo.in])	//缓冲区头
#define BUF_TAIL 		(swin_fifo.buffer[swin_fifo.out])	//缓冲区尾
#define INCBUF(x,mod) 	((++(x)) & ((mod) - 1))				//mod 必须是2的幂

//日期保存到u32变量中，year为128年，秒精确到2s
//struct date_t{
//	u32 sec		:5;
//	u32 min		:6;
//	u32 hour	:5;
//	u32 day		:5;
//	u32 mon		:4;
//	u32 year	:7;
//};
//union swin_time_t{
//	u32 date;
//	struct date_t time;
//};

/*************************************************
  API
*************************************************/
static int swin_process(void);
static int swin_read(u8 id,swin_time_t *p);
static int swin_check(u8 id);
//static void swin_mkmsg(u32 msg_wpara, st_ymdhmsw_t time)
//{
//	;
//}
//操作循环缓冲区，并组遥信消息帧
//缓冲区数据帧格式，8位每帧。从低位到高位分别为 io端口号、极性、年、月、日、时、分、秒。
static void inline swin_msg(u8 chn,swin_time_t *result){
	int num ;
	message_t msg_swin ;
	st_ymdhmsw_t  *swin_time;

	if(swin_fun[chn] == REMSIG){
		if(chn >= CFG_SWIN_NUM)
			chn += SWIN_DOOR - 1;
	//逐个通道，组帧发送所有的遥信消息
		for(num = 0; num < result->num; num ++){
			BUF_HEAD = chn;
			swin_fifo.in = INCBUF(swin_fifo.in,SWIN_BUFF_SIZE);
			BUF_HEAD = result->jump[num].polar;
			swin_fifo.in = INCBUF(swin_fifo.in,SWIN_BUFF_SIZE);

			swin_time = &result->jump[num].time;
//							printf("%s:%d/%d/%d,%d:%d:%d\n",__FUNCTION__,swin_time->year + 2000,swin_time->mon,swin_time->day,swin_time->hour,swin_time->min,swin_time->sec);
			BUF_HEAD = swin_time->year;
			swin_fifo.in = INCBUF(swin_fifo.in,SWIN_BUFF_SIZE);
			BUF_HEAD = swin_time->mon;
			swin_fifo.in = INCBUF(swin_fifo.in,SWIN_BUFF_SIZE);
			BUF_HEAD = swin_time->day;
			swin_fifo.in = INCBUF(swin_fifo.in,SWIN_BUFF_SIZE);
			BUF_HEAD = swin_time->hour;
			swin_fifo.in = INCBUF(swin_fifo.in,SWIN_BUFF_SIZE);
			BUF_HEAD = swin_time->min;
			swin_fifo.in = INCBUF(swin_fifo.in,SWIN_BUFF_SIZE);
			BUF_HEAD = swin_time->sec;
			swin_fifo.in = INCBUF(swin_fifo.in,SWIN_BUFF_SIZE);

			msg_swin.type 	= MSG_DIN;
			msg_swin.wpara 	= chn;
			msg_swin.lpara 	= (u32)(&BUF_TAIL);
			message_dispatch(&msg_swin);

			swin_fifo.out += SWIN_BUFF_MSG_SIZE - 1;
			swin_fifo.out = INCBUF(swin_fifo.out , SWIN_BUFF_SIZE);
		}
	}else if(swin_fun[chn] == PULSE){
		msg_swin.type 	= MSG_PULSE;
		msg_swin.wpara 	= chn;
		msg_swin.lpara 	= result->num;
		message_dispatch(&msg_swin);
	}
}
/******************************************************************************
*	函数:	thread_swin_read
*	功能:	遥信脉冲读取线程
*	参数:
*	返回:
*	说明:
 ******************************************************************************/
static void * thread_swin_read(void * arg)
{
	int ret = -1;
	int chn = 0 ;
	u8 	pin;
	swin_time_t swin_result;

	fd_set rfds;
	struct timeval tv;
	int fd = 1;
	tv.tv_sec = 0;
	FD_ZERO (&rfds);
	FD_SET (fd, &rfds);

	while(1){
		pin = swin_process();
		if(pin > 0 ){
			for(chn = 0; chn < CUR_SWINPIN; chn ++){
				if((pin & (1 << chn)) > 0){
					ret = swin_read(chn, &swin_result);
					swin_msg(chn,&swin_result);
				}
			}
		}
		tv.tv_usec = CFG_SWIN_READ_CYCLE*1000;
		select (0, NULL, NULL, NULL, &tv);
	}
	pthread_exit(0);
}

/******************************************************************************
*	函数:	swin_set
*	功能:	设置为遥信或脉冲模式
*	参数:	id				-	开入通道号
			mode			-	具体见头文件配置
*	返回:	0				-	成功
			-ERR_INVAL		-	参数错误
			-ERR_NODEV		-	无此设备或者不需要配置

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
*	函数:	swin_init
*	功能:	遥信及脉冲检测模块初始化
*	参数:	无
*	返回:	0				-	成功
			-ERR_CFG		-	配置超限,或者没有配置遥信或脉冲模式
			-ERR_BUSY		-	已经打开
			-ERR_SYS		-	系统错误
			-ERR_NOFILE		-	没有此路径
 * 说明:	无
 ******************************************************************************/
int swin_init(void)
{
	int ret = -1;
	int i = 0;
	pthread_t swin_id;
	char *dev[]={"/dev/atmel_tc0","/dev/atmel_tc1","/dev/atmel_tc2",
				"/dev/atmel_tc3","/dev/atmel_tc4","/dev/atmel_tc5"};
	
	if(CFG_SWIN_NUM > MAX_SWINPIN){
		ret = -ERR_CFG; 	//配置超限
		goto err;
	}

	for(i = 0; i < CFG_SWIN_NUM; i++){
		if((swin_fun[i] == REMSIG) || (swin_fun[i] == PULSE)){				//范围检查
			goto start;
		}
	}
	return -ERR_CFG;

start:
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
	memset(swin_iostate, 0, CUR_SWINPIN*sizeof(u8));
//	memset(swin_fun, 0xff, CUR_SWINPIN*sizeof(u8));
	memset(pulse_change, 0, CUR_SWINPIN*sizeof(u8));
	memset(remsig_change, 0, CUR_SWINPIN*sizeof(u8));

	swin_fifo.in = swin_fifo.out = 0;
	//设置gpio物理地址
#ifdef CFG_SWIN_0
	swin_pin[0] = CFG_SWIN_0 + PINBASE;
//	swin_iostate[0] = CFG_SWIN_IOSTATE0;
#endif
#ifdef CFG_SWIN_1
	swin_pin[1] = CFG_SWIN_1 + PINBASE;
#endif
#ifdef CFG_SWIN_2
	swin_pin[2] = CFG_SWIN_2 + PINBASE;
#endif
#ifdef CFG_SWIN_3
	swin_pin[3] = CFG_SWIN_3 + PINBASE;
#endif
#ifdef CFG_SWIN_4
	swin_pin[4] = CFG_SWIN_4 + PINBASE;
#endif
#ifdef CFG_SWIN_5
	swin_pin[5] = CFG_SWIN_5 + PINBASE;
#endif
#ifdef CFG_SWIN_6
	swin_pin[6] = CFG_SWIN_6 + PINBASE;
#endif
#ifdef CFG_SWIN_7
	swin_pin[7] = CFG_SWIN_7 + PINBASE;
#endif

	swin_pin[CFG_SWIN_NUM] 		= CFG_SWIN_DOOR + PINBASE;       //门接点信号对应IO
	swin_fun[CFG_SWIN_NUM] 		= REMSIG;
//	swin_iostate[CFG_SWIN_NUM] = CFG_SWIN_IOSTATE_DOOR;
	swin_pin[CFG_SWIN_NUM + 1]	= CFG_SWIN_LID_TOP + PINBASE;    //开表盖信号对应IO
	swin_fun[CFG_SWIN_NUM + 1] 	= REMSIG;
//	swin_iostate[CFG_SWIN_NUM + 1] = CFG_SWIN_IOSTATE_TOP;
	swin_pin[CFG_SWIN_NUM + 2]	= CFG_SWIN_LID_MID + PINBASE;    //开中盖信号对应IO
	swin_fun[CFG_SWIN_NUM + 2] 	= REMSIG;
//	swin_iostate[CFG_SWIN_NUM + 2] = CFG_SWIN_IOSTATE_MID;
	swin_pin[CFG_SWIN_NUM + 3]	= CFG_SWIN_LID_TAIL + PINBASE;   //开尾盖信号对应IO
	swin_fun[CFG_SWIN_NUM + 3]	 = REMSIG;
//	swin_iostate[CFG_SWIN_NUM + 3] = CFG_SWIN_IOSTATE_TAIL;

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
		if(swin_pin[i] > 0){
			ret = ioctl(fd, ADD_IO, swin_pin[i]);
			if (ret < 0){
				ret = -ERR_SYS;		//没有此路径
				goto err;
			}
		}
	}

	//打开gpio模块
	g_fd = open("/dev/atmel_gpio", O_RDWR | O_NOCTTY);
	if (g_fd < 0){
		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}

	//设置遥信和脉冲端口初始状态
	for(i = 0; i < CUR_SWINPIN; i ++){
		if((swin_fun[i] == REMSIG) || (swin_fun[i] == PULSE)){
			ret = swin_check(i);
			if (ret < 0){
				goto err;
			}else{
				swin_iostate[i] = ret;
			}
		}
	}

	 //启动
	ret = ioctl(fd, PSTART, 0);
	if (ret < 0){
   		ret = -ERR_SYS;	
		goto err;
	}	
	swin_count = 1;
	//创建按键扫描线程，实时线程，优先级80-90
	ret = thread_create_base(&swin_id, thread_swin_read, NULL, THREAD_MODE_REALTIME, 80);
	if((ret < 0) ){
		swin_count = 0;
		goto err;
	}
	ret = 0;
err:
	return ret;
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
static int swin_process(void)
{
	int ret;
	int i,j,read_byte;
	u16 * p;

	p = swin_buf;


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
	memset(p,0,SWIN_BUFSIZE*sizeof(u16));
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
					if(swin_iostate[j] == 0){
						high[j] ++;
						low[j] = 0;
						if(high[j] >= CFG_SWIN_FILTER_NUM){
							high[j] = 0;
							swin_iostate[j] = 1;

							swin_result[j].num += 1;
							if(swin_result[j].num >= 8){	//遥信信号变位最多8个
								swin_result[j].num = 8;		//遥信信号变位最多8个
							}

							if(remsig_change[j] >= 7){	//遥信信号变位时刻buf满时替代最后一个
								remsig_change[j] = 7;	//最后一个时间
							}
							swin_result[j].jump[remsig_change[j]].polar = 1;
							ret =  systime_get(&swin_result[j].jump[remsig_change[j]].time);
							if(ret < 0 ){
								goto err;
							}
							remsig_change[j] += 1;
						}
					}
				}else{
					if(swin_iostate[j] == 1){
						low[j] ++;
						high[j] = 0;
						if(low[j] >= CFG_SWIN_FILTER_NUM){
							low[j] = 0;
							swin_iostate[j] = 0;
							swin_result[j].num += 1;
							if(swin_result[j].num >= 8){	//遥信信号变位最多8个
								swin_result[j].num = 8;		//遥信信号变位最多8个
							}

							if(remsig_change[j] >= 7){		//遥信信号变位时刻buf满时替代最后一个
								remsig_change[j] = 7;		//最后一个时间及极性
							}
							swin_result[j].jump[remsig_change[j]].polar = 0;
							ret =  systime_get (&swin_result[j].jump[remsig_change[j]].time);
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
					if(swin_iostate[j] == 0){
						high[j] ++;
						low[j] = 0;
						if(high[j] >= CFG_SWIN_FILTER_NUM){
							swin_iostate[j] = 1;
							swin_num[j] += 1;
							pulse_change[j] = 1;
						}
					}
				}else{
					low[j] ++;
					high[j] = 0;
					if(low[j] >= CFG_SWIN_FILTER_NUM)
						swin_iostate[j] = 0;
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
static int swin_read(u8 id,swin_time_t *p)
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
static int swin_check(u8 id)
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
//static int swin_close(void)
//{
//	int ret = -1;
//
//	if(swin_count == 0)
//		return -ERR_NOINIT;
//
//	ret = close(fd);
//	if(ret < 0)
//		return -ERR_SYS;
//	ret = close(t_fd);
//	if(ret < 0)
//		return -ERR_SYS;
//	ret = close(g_fd);
//	if(ret < 0)
//		return -ERR_SYS;
//
//	swin_count = 0;
//	//销毁互斥锁
//	if (pthread_mutex_destroy(&mutex)) {
//		ret = -ERR_OTHER;
//	}
//	ret = 0;
//	return ret;
//}

#endif
