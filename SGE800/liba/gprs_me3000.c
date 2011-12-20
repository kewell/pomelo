/*****************************************************************************/
/*许继电气股份有限公司                                     版权：2008-2015   */
/*****************************************************************************/
/* 本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许    */
/* 可不得擅自修改或发布，否则将追究相关的法律责任。                          */
/*                                                                           */
/*                      河南许昌许继股份有限公司                             */
/*                      www.xjgc.com                                         */
/*                      (0374) 321 2924                                      */
/*                                                                           */
/*****************************************************************************/


/******************************************************************************
    项目名称    ：  SGE800计量终端业务平台
    文 件 名    ：  gprs_me3000.c
    描    述    ：  本文件用于业务平台库gprs功能的实现
    版    本    ：  0.1
    作    者    ：  孙锐
    创建日期    ：  2010.04
******************************************************************************/

//C库头文件
#include <stdio.h>			//printf
#include <stdlib.h>		    //exit
#include <unistd.h>		    //sleep
//#include <db.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

//平台库头文件
#include "include/sdev.h"
#include "include/gprs_me3000.h"
#include "include/comport.h"
#include "include/gpio.h"
#include "private/config.h"
#include "include/error.h"

/*************************************************
  结构类型定义
*************************************************/
//GPRS模块的配置
#define DEV_STREAM_SET_COMPORT      0           		//设置串口的属性
#define	DEV_STREAM_SET_DEV			1					//设置设备的属性
#define	check_echo(q,p) (memcmp(q, p, sizeof(p)-1))     //判断回显是否正确


#define DEV_GPRS_TCP				0					//TCP连接方式
#define DEV_GPRS_UDP				1					//UDP连接方式

//命令发送回复所有可能的交互状态
#define DEV_STREAM_DEMAND			0					//发送命令
#define DEV_STREAM_DEFINE			1					//收到发送允许确认：>
#define DEV_STREAM_RECV_OK	        2            		//gprs模块与串口交互提示符"ok"
#define DEV_STREAM_RECV_VERIFY      3             		//gprs模块与串口交互提示符确认具体信息

//建立连接状态
#define DEV_GPRS_ATE0				0					//gprs模块连接关闭回显状态
#define DEV_GPRS_CHECK_ATE0			1					//gprs模块连接查看回想是否关闭
#define DEV_GPRS_CNMI				2					//gprs模块连接设置短信格式
#define DEV_GPRS_CHECK_CNMI			3					//gprs模块连接查看短信格式
#define DEV_GPRS_CMGF				4					//gprs模块连接设置PDU格式
#define DEV_GPRS_CHECK_CMGF			5					//gprs模块连接查询是否为PDU格式
#define DEV_GPRS_CSMS				6					//gprs模块连接设置短信服务
#define DEV_GPRS_CHECK_CSMS			7					//gprs模块连接查询短信服务
#define DEV_GPRS_CSCS				8					//gprs模块连接设置编码形式
#define DEV_GPRS_CHECK_CSCS			9					//gprs模块连接查询编码形式
#define DEV_GPRS_CHECK_CREG			10					//gprs模块连接查询网络
#define DEV_GPRS_XISP				11					//gprs模块连接设置内部协议
#define DEV_GPRS_CHECK_XISP			12					//gprs模块连接查询协议
#define DEV_GPRS_CODE				13					//gprs模块连接设置用户名密码
#define DEV_GPRS_INITPPP			14					//gprs模块连接登录网络
#define DEV_GPRS_CHECK_PPPOK		15					//gprs模块连接查询是否登录网络
#define DEV_GPRS_CHECK_PPPNET		16					//gprs模块连接IP连接
#define DEV_GPRS_CHECK_PPPLINK		17					//gprs模块连接IP连接是否成功

/*******************************错误***********************************/
#define ERR_GPRS_MODEL      		34                  //gprs模块设置功能时回复错误
#define ERR_GPRS_CLOSE				35					//gprs模块被断开


//GPRS模块的状态
static struct{
	u8 init;              		//初始化标志
	u8 timeout;					//设置串口接收超时
	u8 busy;    				//0：空闲；1：忙碌（发送等待状态）；
	u8 connect_state;      		//0:非在线状态；1：在线状态
	u8 sms_flag;				//短信状态
	u8 ip[12];					//主站IP
	u8 connect_index;      		//0:TCP; 1:UDP
	pthread_mutex_t mutex;		//串口锁（防止同时发送接收）
	pthread_mutex_t mutex_data;	//数据锁，锁缓存数据
	u16 data_len;				//缓存有效数据长度
	u8 data_buf[CFG_DATA_MAX];	//最大缓存数据--可能发送数据的最大长度如果溢出可能丢帧
	pthread_mutex_t mutex_sms;  //短信锁，锁短信数据
	u8 sms_tel[11];				//发送方电话号码
	u16 sms_len;				//缓存有效短信长度
	u8 sms_buf[CFG_SMS_MAX];    //最大缓冲短信
}gprsstate;

/******************************************************************************
*	函数:	accesssmsnum
*	功能:	将字符型电话号码（11位）,转化成6个字节的数字(内部使用函数非外部接口)
*	参数:	num				-	六个字节的电话号码
			num_char		-	字符型电话号码的起始地址（必须为11个字符的电话字符串，否则后台补足）
*	返回:	0				-	成功(收到确认命令)

*	说明:	无
******************************************************************************/
int accesssmsnum(char *num_char, u8 *num)
{
	int i;
	u8 number[6];
	char *point;

	point = num_char;
	for(i=0; i<5; i++){
		number[i] = atoi(point);
		point ++;
		number[i] += atoi(point) << 4;
		point ++;
	}
	number[5] = atoi(point) | 0xF0;
	memcpy(num, number, sizeof(number));
	exit(0);
}

/******************************************************************************
*	函数:	deal_recv
*	功能:	处理接收到得短消息和数据,判断其是否有效，有效将放入缓冲区中(内部使用函数非外部接口)
*	参数:	buf				-	已收到的内容的起始地址
			len				-	已收到内容的长度
			char			-	目的接收命令的起始地址
*	返回:	0				-	成功(收到确认命令)
			-ERR_NOFUN		-	没有收到确认命令
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
 		    -ERR_DISCONNECT	-	服务器未打开
 		    -ERR_GPRS_CLOSE	-	连接被动断开

*	说明:	无
******************************************************************************/
int deal_recv(u8 *buf, u32 len, char *cmd)
{
	int ret,ref;
	int i, j, k;
	u8 num;
	u16 multiflag;   //短信结束帧标志
	u8 *point;
	u32 point_len = 0;
	u32 data_len;
	struct timespec to;
	u8 tel_num[11]; //发送方电话号码
	u8 mid[512];   	//中间存放
	memset(mid, 0, sizeof(mid));

	memcpy(mid, buf, len);
	point_len += len;

	//继续接收串口数据
	ret = comport_recv(CFG_GPRS_ME3000_COMPORT, &mid[point_len-1], sizeof(mid)-len);
	if(ret > 0){
		point_len += ret;
	}else if (ret < 0){
		goto error;
	}
	//判断在读取确认过程中收到的是否是有效的接收到数据或者短信的消息
	if (check_echo((char *)mid, "\r\n+CMT") == 0){    		  //接收到短信
		//短信接收第一个数据为从第14位开始:\r\n+CMT : <oa>, <长度>,这个不是ASSIIC码
		u8 buff[512];
		memset(buff, 0, 512);
		memcpy(buff, &mid[13], strlen((char *)mid));  //strlen这个长度可能不准确,一般超长
		k = strlen((char *)mid)/2;
		//ASCII转为HEX
		for(j=0; j<k; j++){
			if((buff[2*j]>0x40) && (buff[2*j]<0x47)){
				buff[2*j] -= 0x37;
			}else{
				buff[2*j] -= 0x30;
			}
			if((buff[2*j+1]>0x40) && (buff[2*j+1]<0x47)){
				buff[2*j+1] -= 0x37;
			}else{
				buff[2*j+1] -= 0x30;
			}
			mid[j]=(buff[2*j] << 4) + buff[2*j+1];
		}
		//memcpy(RecSMSNum, &ReturnStr[1], ReturnStr[0]);  //短信中心号码
		num = 1 + mid[0] + 1 ;  //10
		if(mid[num] % 2){ //号码位数为奇数
			memset(tel_num, 0, sizeof(tel_num));
			memcpy(tel_num, &mid[num + 1], mid[num] / 2 + 1 + 1);  //发送源号码
			num += (mid[num] / 2 + 1) + 3 ;  //20
		}else{
			memset(tel_num, 0, 10);
			memcpy(tel_num, &mid[num + 1], mid[num] / 2 + 1);
			num += (mid[num] / 2 ) + 3;
		}
		//获得锁
		to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;      //秒
		to.tv_nsec = 0;                       			//纳秒
		ref = pthread_mutex_timedlock(&(gprsstate.mutex_sms), &to);
		if (ref == ETIMEDOUT){
			ret = -ERR_BUSY;               //设备忙（发送中）
			goto error;
		}else{
			ret = -ERR_SYS;
			goto error;
		}
		if ((gprsstate.sms_len > 0) && (memcmp(gprsstate.sms_tel, tel_num, sizeof(tel_num)-1) != 0) && (gprsstate.sms_flag == 0)){  //短信未收全，但是发送号码不一致
			//清空短信缓存？？？？？？
			memset(gprsstate.sms_buf, 0, sizeof(gprsstate.sms_buf));
			gprsstate.sms_len = 0;
			gprsstate.sms_flag = 0;
		}
		if(mid[num] != 0x04){ //如果不是HEX编码，返回
			ret = 0;
			goto error;
		}
		memcpy(&(gprsstate.sms_buf[gprsstate.sms_len]), &mid[num + 11], mid[num + 8] - 2);    //储存用户数据，前两个字节用于判断是否分帧
		gprsstate.sms_len += (mid[num + 8] - 2);
		multiflag = mid[num + 9] + mid[num + 10] * 256 ;  //低位在前，高位在后
		if(multiflag & 0x8000){     //smfin 为1 结束帧
			//收到完整的短信
			gprsstate.sms_flag = 1;
		}else{
			//继续接收短信
			gprsstate.sms_flag = 0;
		}
		//解除互斥锁
		ret = pthread_mutex_unlock(&(gprsstate.mutex_sms));
		if (ret){
			ret = -ERR_SYS;
			goto error;
		}
		
		//是否收到目标回复
		if (point_len >= mid[num+8]+num+9){
			//没有收到确认命令
			ret = -ERR_NOFUN;
			goto error;
		}else{
			//判断是否是确认命令
			num = mid[num+8]+num+9;
			if (check_echo((char *)&mid[num], cmd) == 0){
				ret = 0;
				return(ret);
			}else{
				ret = -ERR_NOFUN;
				goto error;
			}
		}
	}else if (check_echo((char *)mid, "\r\n+TCPCLOSE") == 0){ //GPRS模块被动断开
		gprsstate.connect_state = 0;   
		ret = -ERR_GPRS_CLOSE;     //模块异常断开
		goto error;
	
	}else if ((check_echo((char *)mid, "\r\n+TCPRECV") == 0) || (check_echo((char *)mid, "\r\n+UDPRECV") == 0)){  //接收到TCP数据
		point = &mid[13];       			//接收到数据长度字符的起始指针
		i =	atoi((char *)point);
		//判断接收数据长度正常
		if ((point_len > (i + 15)) && (i <= 1024)){
			if (i <	10){
				data_len = i + 16;      //point为整个有效长度  ???具体长度待考证    可能是17
			}else if(i < 100){
				data_len = i + 17;
			}else if(i < 1000){
				data_len = i + 18;
			}else{
				data_len = i + 19;
			}
			//将有效接收放入数据缓冲区中
			to.tv_sec = time(NULL) + 2;           //秒
			to.tv_nsec = 0;                       //纳秒
			ret = pthread_mutex_timedlock(&gprsstate.mutex_data, &to);    	//获得锁
			if(ret == 0){
				memset(gprsstate.data_buf, 0, sizeof(gprsstate.data_buf));  //清空：如果上一有效帧在2S内未处理将有可能被清空
				memcpy(gprsstate.data_buf, mid, data_len);
				gprsstate.data_len = data_len;
				//释放锁
				ret = pthread_mutex_unlock(&gprsstate.mutex_data);
				if(ret){
					goto error;
				}
			}
			//判断收到数据中是否包含确认帧
			if (point_len >= data_len + strlen(cmd)){    //有可能收到了确认
				for(i = data_len; i< point_len-strlen(cmd); i++){
					if (check_echo((char *)&mid[i], cmd) == 0){
						return(0);
					}else{
						ret = -ERR_NOFUN;
						goto error;
					}
				}
			}else{
				ret = -ERR_NOFUN;
				goto error;
			}

		}else{    //长度不正常
			data_len = strlen(cmd);
			if (point_len >= data_len){    //直接判断是否可能有确认回复
				for(i = 0; i< point_len-strlen(cmd); i++){
					if (check_echo((char *)&mid[i], cmd) == 0){
						return(0);
					}else{
						ret = -ERR_NOFUN;
						goto error;
					}
				}
			}else{
				ret = -ERR_NOFUN;
				goto error;
			}
		}
	//串口收到无效数据
	}else{
		//判断收到数据中是否包含确认帧
		if (point_len >= strlen(cmd)){    //有可能收到了确认
			for(i = 0; i< point_len-strlen(cmd); i++){
				if (check_echo((char *)&mid[i], cmd) == 0){
					return(0);
				}else{
					ret = -ERR_NOFUN;
					goto error;
				}
			}
		}else{
			ret = -ERR_NOFUN;
			goto error;
		}
	}
error:
	return(ret);
}
/******************************************************************************
*	函数:	gprs_me3000_init
*	功能:	初始化gprs(内部使用函数非外部接口)
*	参数:
*	返回:	0				-	成功(收到确认命令)
			-ERR_SYS		-	系统错误

*	说明:	无
******************************************************************************/
int gprs_me3000_init()
{
	int ret,ref;
	if (gprsstate.init == 1){
		ret = -ERR_BUSY;			//已初始化
		goto error;
	}
	gprsstate.busy = 0;    			//空闲
	gprsstate.connect_index = 0;    //未在线
	gprsstate.sms_flag = 0;         //非短信
	gprsstate.init = 0 ;
	memset(gprsstate.data_buf, 0, CFG_DATA_MAX);    //清空数据缓存
	gprsstate.data_len = 0;
	
	//初始化串口锁
	const pthread_mutex_t mutex_zero = PTHREAD_MUTEX_INITIALIZER;
	memcpy(&gprsstate.mutex, &mutex_zero, sizeof(pthread_mutex_t));
	ref = pthread_mutex_init(&gprsstate.mutex, NULL);
	if (ref){
		ret = -ERR_SYS;
		goto error;
	}
	//初始化数据锁
	memcpy(&gprsstate.mutex_data, &mutex_zero, sizeof(pthread_mutex_t));
	ref = pthread_mutex_init(&gprsstate.mutex_data, NULL);
	if (ref){
		ret = -ERR_SYS;
		goto error;
	}
	//初始化短信锁
	memcpy(&gprsstate.mutex_sms, &mutex_zero, sizeof(pthread_mutex_t));
	ref = pthread_mutex_init(&gprsstate.mutex_sms, NULL);
	if (ref){
		ret = -ERR_SYS;
		goto error;
	}
	
	//初始化gpio模块
	ret = gpio_init();  
	if ((ret != 0) && (ret != -ERR_BUSY)){
		goto error;
	}
	//设置gprs模块power的IO属性
	ret = gpio_set(CFG_GPRS_POWER_IO, GPIO_OUT, CFG_GPRS_POWER_IO_OD, CFG_GPRS_POWER_IO_PU);
	if (ret){
		goto error;
	}
	gprsstate.init = 1;
error:
	return(ret);
}


/******************************************************************************
*	函数:	gprs_me3000_setconfig
*	功能:	配置串口或gprs模块
*	参数:	cmd				-	0：配置串口；1：配置gprs模块
			cfg				-	配置接口
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误

*	说明:	无
******************************************************************************/
int gprs_me3000_setconfig(u8 cmd, void *cfg)
{
	int ret;
	comport_config_t cfg_set;
	if (cmd == DEV_STREAM_SET_COMPORT){      //设置与GPRS模块相连的串口的属性
		ret = comport_init(CFG_GPRS_ME3000_COMPORT, COMPORT_MODE_NORMAL);
		if ((ret != 0) && (ret != -ERR_BUSY)){
			goto error;
		}
		cfg_set.baud = ((comport_config_t *)cfg) -> baud;
		cfg_set.ndata = ((comport_config_t *)cfg) -> ndata;
		cfg_set.nstop = ((comport_config_t *)cfg) -> nstop;
		cfg_set.rtscts = ((comport_config_t *)cfg) -> rtscts;
		cfg_set.verify = ((comport_config_t *)cfg) -> verify;

		//设置超时！
		cfg_set.timeout = CFG_RECV_TIMEOUT;     //设置串口是超时方式
		((comport_config_t *)cfg) -> timeout = gprsstate.timeout;

		//设置串口属性
		ret = comport_setconfig (CFG_GPRS_ME3000_COMPORT, &cfg_set);
		if (ret){
			goto error;
		}
	}else if (cmd == DEV_STREAM_SET_DEV){	//设置ME3000属性

	}else{
		ret = -ERR_INVAL;
		goto error;
	}
	ret = 0;
error:
	return(ret);
}


int gprs_me3000_getconfig(u8 cmd, void *cfg)
{
	int ret;
	comport_config_t cfg_get;


	if (cmd == DEV_STREAM_SET_COMPORT){      //获得与GPRS模块相连的串口的属性
		ret = comport_init(CFG_GPRS_ME3000_COMPORT, COMPORT_MODE_NORMAL);
		if ((ret != 0) && (ret != -ERR_BUSY)){
			goto error;
		}
		ret = comport_getconfig (CFG_GPRS_ME3000_COMPORT, &cfg_get);
		if (ret != 0){
			goto error;
		}
		((comport_config_t *)cfg) -> baud = cfg_get.baud;
		((comport_config_t *)cfg) -> ndata = cfg_get.ndata;
		((comport_config_t *)cfg) -> nstop = cfg_get.nstop;
		((comport_config_t *)cfg) -> rtscts = cfg_get.rtscts;
		((comport_config_t *)cfg) -> verify = cfg_get.verify;

		((comport_config_t *)cfg) -> timeout = gprsstate.timeout;

	}else if (cmd == DEV_STREAM_SET_DEV){	//获得M580属性

	}else{
		ret = -ERR_INVAL;
		goto error;
	}
	ret = 0;
error:
	return(ret);
}

/******************************************************************************
*	函数:	gprs_me3000_send
*	功能:	GPRS发送数据
*	参数:	buf				-	发送数据起始指针
			count			-	发送数据长度
*	返回:	>0				-	成功(返回实际发送成功的字节数)
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
 		    -ERR_DISCONNECT	-	未连接
 		    -ERR_BUSY		-	设备忙

*	说明:	超时时间通过配置发送超时定
******************************************************************************/
int gprs_me3000_send(u8 *buf, u32 count)
{
	int ret,ref;
	u8 recv_len,timeout,back;
	u8 step,flag,i;                 //交互状态
	char send_cmd[CFG_DATA_MAX];  	//发送数据命令
	u8 recv_cmd[CFG_RECV_BACKMAX];  				//接收命令
	struct timespec to;

	if (count <= 0){
		ret = -ERR_INVAL;
		return ret;
	}
	//判断是否为在线状态
	if(gprsstate.connect_state == 0){
		ret = -ERR_DISCONNECT;
		return ret;
	}
	//获得锁
	to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;      //秒
	to.tv_nsec = 0;                       			//纳秒
	ref = pthread_mutex_timedlock(&(gprsstate.mutex), &to);
	if (ref == ETIMEDOUT){
		ret = -ERR_BUSY;               //设备忙（发送中）
		return ret;
	}else if(ref){
		ret = -ERR_SYS;
		return ret;
	}
	//判断模块是否有足够的发送缓冲区（未写）
	//通知ME3000，将要发送数据
	memset(send_cmd, 0, sizeof(send_cmd));
	if (gprsstate.connect_index == 0){					//TCP模式
		sprintf(send_cmd, "%s%d\r", "AT+TCPSEND=0,",count);
	}else{												//UDP模式
		sprintf(send_cmd, "%s%d\r", "AT+UDPSEND=0,",count);
	}
	//清空发送缓冲
	comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_WR);
	//发送数据命令
	ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
	if(ret != strlen(send_cmd)){
		goto error;
	}
	timeout = 200;    //最大回复时间-----手册
	sleep(1);
	//200S内返回---最长时间？？？？？
	back = 1;     //确认标识
	step = DEV_STREAM_DEFINE;
	while(back){
		switch(step){
		case DEV_STREAM_DEFINE:     //接收允许发送提示
			memset(recv_cmd, 0, sizeof(recv_cmd));
			ret = comport_recv (CFG_GPRS_ME3000_COMPORT, recv_cmd, 10);
			flag = 0;
			if(ret > 0){     //接收到发送提示符
				for (i = 0; i < ret; i++){
					if (recv_cmd[i] == '>'){
						flag = 1;
						break;
					}
				}
				if (flag == 1){
					//清空发送缓冲
					comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_WR);
					//发送数据
					memset(send_cmd, 0, sizeof(send_cmd));
					memcpy(send_cmd, buf, count);
					send_cmd[count] = '\r';
					ret = comport_send (CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, (count+1));
					if (ret < 0){
						goto error;
					}
					timeout = 0;
					step = DEV_STREAM_RECV_OK;
				}else{
					ret = deal_recv(recv_cmd, ref, (char *)'>');    //读串口（将串口缓冲都读出来，把读到得数据放入数据缓冲之中并判断是否有需要的返回）
					if(ret == 0){
						timeout = 0;
						step = DEV_STREAM_RECV_OK;
					}else if(ret == -ERR_GPRS_CLOSE){  //服务器被动断开
						goto error;
					}else{
						step = DEV_STREAM_DEFINE;  //继续读“>”
					}
				}
			}else if(ret == 0){
				timeout = timeout - 1 - CFG_RECV_TIMEOUT;
				if(timeout > 0){
					step = DEV_STREAM_DEFINE;
					sleep(1);
				}else{
					back = 0;
					ret = -ERR_TIMEOUT;
					goto error;
				}
			}else if(ret < 0){
				goto error;
			}
			break;
		case DEV_STREAM_RECV_OK:   //接收发送确认“OK”
			//判断是否发送成功消息
			memset(recv_cmd, 0, sizeof(recv_cmd));
			recv_len = strlen("\r\nOK\r\n");
			ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, recv_len);
			if (ret < 0){
				goto error;      //接收错误
			}else if((ret > 0) && (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0)){
				timeout = 0;
				step = DEV_STREAM_RECV_VERIFY;
			}else if((ret > 0) && (check_echo((char *)recv_cmd, "\r\nOK\r\n") != 0)){
				ret = deal_recv(recv_cmd, ref, "\r\nOK\r\n");    //读串口（将串口缓冲都读出来，把读到得数据放入数据缓冲之中并判断是否有需要的返回）
				if(ret == 0){
					timeout = 0;
					step = DEV_STREAM_RECV_VERIFY;
				}else if(ret == -ERR_GPRS_CLOSE){  //服务器被动断开
					goto error;
				}else{
					step = DEV_STREAM_RECV_OK;  //继续读“\r\nOK\r\n”
				}
			}else if (ret==0){
				timeout = timeout - 1 - CFG_RECV_TIMEOUT;
				if(timeout > 0){
					step = DEV_STREAM_RECV_OK;
					sleep(1);
				}else{
					back = 0;
					ret = -ERR_TIMEOUT;
					goto error;
				}
			}
			break;
		case DEV_STREAM_RECV_VERIFY:
			ref = comport_recv (CFG_GPRS_ME3000_COMPORT, recv_cmd, 13);   //测试发个位数，十位数，三位数的情况
			if((ret > 0) && (check_echo((char *)recv_cmd, "\r\n+TCPSEND") == 0)){
				ret = atoi((char *)&recv_cmd[13]);
				back = 0;
				timeout = 0;
			}else if((ret > 0) && (check_echo((char *)recv_cmd, "\r\n+TCPSEND") != 0)){
				ret = deal_recv(recv_cmd, ref, "\r\n+TCPSEND");    //读串口（将串口缓冲都读出来，把读到得数据放入数据缓冲之中并判断是否有需要的返回）
				if(ret == 0){
					timeout = 0;
					back = 0;
				}else if(ret == -ERR_GPRS_CLOSE){  //服务器被动断开
					back = 0;
					goto error;
				}else{
					step = DEV_STREAM_RECV_VERIFY;  //继续读“+TCPSEND”
				}
			}else if(ret == 0){
				timeout = timeout - 1 - CFG_RECV_TIMEOUT;
				if(timeout > 0){
					step = DEV_STREAM_RECV_VERIFY;
					sleep(1);
				}else{
					back = 0;
					ret = -ERR_TIMEOUT;
					goto error;
				}
			}else if(ret < 0){
				goto error;
			}
			break;
		default:
			back = 0;
			ret = -ERR_SYS;
			goto error;
		break;
		}
	}
	
error:
	//解除互斥锁
	ret = pthread_mutex_unlock(&(gprsstate.mutex));
	if (ret){
		ret = -ERR_SYS;
		goto error;
	}
	return(ret);
}

/******************************************************************************
*	函数:	gprs_me3000_recv
*	功能:	GPRS接收数据（N秒未收取可能被清空）
*	参数:	buf				-	接收数据起始指针
			count			-	接收缓冲区长度
*	返回:	>0				-	成功(返回实际发送成功的字节数)
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
 		    -ERR_DISCONNECT	-	未连接
 		    -ERR_BUSY		-	设备忙
 		    -ERR_NOMEM		-	接口存储空间小---丢帧

*	说明:	type为0收到短信，type = 1收到tcp数据，type=2收到udp数据，type=3，收到消息提示
			循环调用改函数接收数据，2s内未调用已接收的数据可能被新的数据替换掉；返回接收到数据后应该将数据拷贝出来处理
			并重新调用该函数继续接收数据直到返回为0,休眠一段时间（2S内）重新调用。
******************************************************************************/
int gprs_me3000_recv(u8 *buf, u32 count, u8 *type, u8 *tel)
{
	int ret;
	struct timespec to;
	
	int j,k;
	u8 buffer[CFG_DATA_MAX];
	u16 buffer_len = 0;
	u8 *data_point;    //数据区地址
	u16	num;
	u16 multiflag;
	u16 data_len;     //数据长度
	u8 remainder = gprsstate.timeout;     //超时

	memset(buffer, 0, CFG_DATA_MAX);
	//判断在缓冲数据区中是否有数据
	if(gprsstate.data_len){     	  //有数据
		to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;           	//秒
		to.tv_nsec = 0;                       					//纳秒
		ret = pthread_mutex_timedlock(&gprsstate.mutex_data, &to);    //获得锁
		if(ret == 0){
			//将数据拷出来
			memcpy(buffer, gprsstate.data_buf, gprsstate.data_len);
			buffer_len = gprsstate.data_len;
			//清空
			memset(gprsstate.data_buf, 0, CFG_DATA_MAX);
			gprsstate.data_len = 0;
			//解锁
			ret = pthread_mutex_unlock(&gprsstate.mutex_data);
			if(ret){
				goto error;
			}
		}else{
			goto error;
		}
	}else if (gprsstate.sms_flag){
		if(count >= gprsstate.sms_len ){
			to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;           	//秒
			to.tv_nsec = 0;                       					//纳秒
			ret = pthread_mutex_timedlock(&gprsstate.mutex_sms, &to);    //获得锁
			if(ret == 0){
				//将数据拷出来
				memcpy(buf, gprsstate.sms_buf, gprsstate.sms_len);
				buffer_len = gprsstate.sms_len;
				*type = 0;
				memcpy(tel, gprsstate.sms_tel,strlen((char *)gprsstate.sms_tel));
				//清空
				memset(gprsstate.sms_buf, 0, CFG_DATA_MAX);
				memset(gprsstate.sms_tel, 0, sizeof(gprsstate.sms_tel));
				gprsstate.sms_len = 0;
				//解锁
				ret = pthread_mutex_unlock(&gprsstate.mutex_sms);
				if(ret){
					goto error;
				}
				//返回短信长度
				return(buffer_len);
			}else{
				goto error;
			}
		}else{
			ret = -ERR_NOMEM;
			goto error;
		}
	}else{
		remainder = gprsstate.timeout;
		//直接从串口接收数据
		while(remainder > 0){
			//获得锁
			to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;      //秒
			to.tv_nsec = 0;                       			//纳秒
			ret = pthread_mutex_timedlock(&(gprsstate.mutex), &to);
			if ((ret != ETIMEDOUT) && (ret != 0)){
				ret = -ERR_SYS;
				goto error;
			}
			ret = comport_recv(CFG_GPRS_ME3000_COMPORT, buffer, CFG_DATA_MAX);
			//接锁
			if(pthread_mutex_unlock(&gprsstate.mutex) != 0){    
				ret = -ERR_SYS;
				goto error;
			}
			if (ret == 0){
				sleep(1);
				remainder = remainder - 1 - CFG_RECV_TIMEOUT;
			}else if (ret < 0){
				goto error;
			}else if (ret > 0){
				buffer_len = ret;
				remainder = 0;
			}
		}
	}

	if (buffer_len){
		//判断接收是否正确
		if (check_echo((char *)buffer, "\r\n+CMT") == 0){    		 //接收到短信
			*type = 0;  //收到短信
			//短信接收第一个数据为从第14位开始:\r\n+CMT : <oa>, <长度>,不用转换码制
			u8 buff[512];
			memset(buff, 0, 512);
			memcpy(buff, &buffer[13], strlen((char *)buffer));  //strlen这个长度可能不准确,一般超长
			k = strlen((char *)buffer)/2;
			//ASCII转为HEX
			for(j=0; j<k; j++){
				if((buff[2*j]>0x40) && (buff[2*j]<0x47)){
					buff[2*j] -= 0x37;
				}else{
					buff[2*j] -= 0x30;
				}
				if((buff[2*j+1]>0x40) && (buff[2*j+1]<0x47)){
					buff[2*j+1] -= 0x37;
				}else{
					buff[2*j+1] -= 0x30;
				}
				buffer[j]=(buff[2*j] << 4) + buff[2*j+1];
			}
			//memcpy(RecSMSNum, &ReturnStr[1], ReturnStr[0]);  //短信中心号码
			num = 1 + buffer[0] + 1 ;  			//10
			if(buffer[num] % 2){ 				//号码位数为奇数
				memset(tel, 0, 10);
				memcpy(tel, &buffer[num + 1], buffer[num] / 2 + 1 + 1);  //发送源号码
				num += (buffer[num] / 2 + 1) + 3 ;  //20
			}else{
				memset(tel, 0, 10);
				memcpy(tel, &buffer[num + 1], buffer[num] / 2 + 1);
				num += (buffer[num] / 2 ) + 3;
			}
			if(buffer[num] != 0x04){ //如果不是HEX编码，返回
				ret = 0;
				goto error;
			}
			memcpy(&(gprsstate.sms_buf[gprsstate.sms_len]), &buffer[num + 11], buffer[num + 8] - 2);    //储存用户数据，前两个字节用于判断是否分帧
			gprsstate.sms_len += (buffer[num + 8] - 2);
			multiflag = buffer[num + 9] + buffer[num + 10] * 256 ;  //低位在前，高位在后
			if(multiflag & 0x8000){     //smfin 为1 结束帧
				//收到完整的短信
				remainder = 0;
				if (count >= gprsstate.sms_len){
					memcpy(buf, gprsstate.sms_buf, gprsstate.sms_len);
					memcpy(tel, gprsstate.sms_tel, strlen((char *)gprsstate.sms_tel));
					//清空数据
					memset(gprsstate.sms_buf, 0, sizeof(gprsstate.sms_buf));
					memset(gprsstate.sms_tel, 0, sizeof(gprsstate.sms_tel));
					gprsstate.sms_flag = 0;
					ret = gprsstate.sms_len;
					return(ret);
				}else{
					gprsstate.sms_flag = 1;
					ret = -ERR_NOMEM;
					goto error;
				}
			}else{
				//继续接收短信
				remainder = gprsstate.timeout;
				gprsstate.sms_flag = 0;
			}
		}else if (check_echo((char *)buffer, "\r\n+TCPRECV") == 0){  //接收到TCP数据
			data_point = &buffer[13];       			//接收到数据长度字符的起始指针
			data_len =	atoi((char *)data_point);
			//判断接收数据长度正常
			if ((buffer_len > (data_len + 15)) && (buffer_len <= 1024)){
				if (data_len <	10){      //数据长度个位数
					data_point += 2;      //指向数据第一个字节的地址
				}else if(data_len < 100){ //数据长度两位数
					data_point += 3;
				}else if(data_len < 1000){ //数据长度三位数
					data_point += 4;
				}else{
					data_point += 5;
				}
				if (count >= data_len){
					memcpy(buf, data_point, data_len);
					ret = data_len;
					*type = 1;
				}else{
					ret = -ERR_NOMEM;
					goto error;
				}
			}
		}else if (check_echo((char *)buffer, "\r\n+UDPRECV") == 0){  	 //接收到UDP数据
			data_point = &buffer[13];       			//接收到数据长度字符的起始指针
			data_len =	atoi((char *)data_point);

			//判断接收数据长度正常
			if ((buffer_len > (data_len + 15)) && (data_len <= 1024)){
				if (data_len <	10){      //数据长度个位数
					data_point += 2;      //指向数据第一个字节的地址
				}else if(data_len < 100){ //数据长度两位数
					data_point += 3;
				}else if(data_len < 1000){ //数据长度三位数
					data_point += 4;
				}else{
					data_point += 5;
				}

				if (count >= data_len){      //接口接收空间小
					memcpy(buf, data_point, data_len);
					ret = data_len;
					*type = 2;
				}else{
					ret = -ERR_NOMEM;
					goto error;
				}
			}
		}else{
			ret = 0;
		}
	}else{
		ret = 0;
	}
error:
	return(ret);
}

/******************************************************************************
*	函数:	gprs_me3000_sendsmg
*	功能:	发送短息
*	参数:	buf				-	发送数据起始指针
			count			-	发送数据长度
			telnum			-	发送方电话号码起始指针（可以是11个字节的电话号码也可以是6个字节的udp号码格式）
			tellen			-	电话号码长度
*	返回:	>0				-	成功(返回实际发送成功的字节数)累计流量用
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
 		    -ERR_DISCONNECT	-	未连接
 		    -ERR_BUSY		-	设备忙
 		    -ERR_NOMEM		-	接口存储空间小---丢帧

*	说明:	
******************************************************************************/
int gprs_me3000_sendsmg(u8 *buf, u32 count, u8 *telnum, u8 tellen)
{
	int ret = 0;
	int ref;
	int i,j;
	u8 point;
	u8 recv_len,timeout,back;
	u8 step,flag;
	u8 lenbak = 0;            //交互状态
	char send_cmd[CFG_DATA_MAX];  	//发送数据命令
	u8 recv_cmd[CFG_RECV_BACKMAX];  //接收命令
	
	//短信中心号码
	u8 tel_center[6];
	//发送的手机号码
	u8 tel[6];   					
	u8 buff[512];
	u8 tel_x[11];

	struct timespec to;
	s16 smseq  = 0;  //分帧标识
	u8 sms_total;    //总帧数
	u8 smsdata[CFG_SMS_MAX][CFG_SMS_LEN_MAX];
	u8 smsdata_len[CFG_SMS_MAX];

	if (count <= 0){
		ret = -ERR_INVAL;
		return ret;
	}
	//判断是否为在线状态
	if(gprsstate.connect_state == 0){
		ret = -ERR_DISCONNECT;
		return ret;
	}

	//短信服务中心号码转化格式
	accesssmsnum(CFG_SMS_CENTER, tel_center);   //转化成数字
	
	//要发送的号码转化格式
	memset(tel_x, 0, sizeof(tel_x));
	if (tellen == 6){
		memcpy(tel_x, telnum, sizeof(tel));
	}else if(tellen == 11){
		memset(tel_x, 0, sizeof(tel_x));
		memcpy(tel_x, telnum, sizeof(tel_x));
		for(i=0; i<5; i++){
			tel[i] = tel_x[2*i] + (tel_x[2*i+1] << 4);
		}
		tel[5] = tel_x[10] + 0xF0;
	}else{
		ret = -ERR_INVAL;
		return ret;
	}

	//获得锁
	to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;      //秒
	to.tv_nsec = 0;                       			//纳秒
	ref = pthread_mutex_timedlock(&(gprsstate.mutex), &to);
	if (ref == ETIMEDOUT){
		ret = -ERR_BUSY;               //设备忙（发送中）
		return ret;
	}else if(ref){
		ret = -ERR_SYS;
		return ret;
	}
	//判断模块是否有足够的发送缓冲区（未写）

	//组发送短信命令
	if (count > 138){   //需分帧发送
		if(count%138){
			sms_total = count / 138 + 1;  //总帧数
		}else{
			sms_total = count / 138;
		}

		if(sms_total > CFG_SMS_MAX){      //数据太长越限
			ret = -ERR_INVAL;
			goto error;
		}
		for(i=0; i< sms_total; i++){
			point = 0;
			smsdata[i][point++] = 0x08;	    //固定8位
			smsdata[i][point++] = 0x91;
			smsdata[i][point++] = 0x68;

			//短信中心号码
			memcpy(&smsdata[i][point], tel_center, strlen((char *)tel_center));
			point += 6 ;
			smsdata[i][point++] = 0x11;            // 11	基本参数(TP-MTI/VFP)	发送，TP-VP用相对格式
			smsdata[i][point++] = 0x00;            // 00	消息基准值(TP-MR)	0

			//要发送的号码长度-----？？？？？？
			if((tel[strlen((char *)tel)-1] & 0xF0) == 0xF0){              	//要发送的号码最后一位的高4字节是否为F
				smsdata[i][point++] = (strlen((char *)tel) - 1) * 2 -1; 	//由于号码为奇数，总长度减去91或者A1（1字节），乘以2 再减去F
			}else{
				smsdata[i][point++] = (strlen((char *)tel) - 1) * 2;
			}
			memcpy(&smsdata[i][point], tel, strlen((char *)tel));         	//要发送的号码
			point += strlen((char *)tel) ;
			smsdata[i][point++] = 0x00; //协议标识，常用0x00或0x21
			smsdata[i][point++] = 0x04; //编码标志，为HEX
			smsdata[i][point++] = 0x00;                     //有效期 5分钟
			if(i == (sms_total - 1)){
				smsdata[i][point++] = count - 138 * i + 2;	//最后一帧发送长度
			}
			else{
				smsdata[i][point++] = 138 + 2;          //加上两个字节的分帧标志,多帧时前(SMS_MulFrameCnt-2)帧发140字节
			}

			if(i == 0){
				lenbak = point;  						//备份前面的固定长度 ，后面的为具体的报文内容
			}

			if(i == (sms_total - 1)){    //最后一帧？？？？？smseq
				smsdata[i][point++] = (smseq | 0x8000 | (((0x8000 >>12) | i)<<12));
				smsdata[i][point++] = (smseq | 0x8000 | (((0x8000 >>12) | i)<<12)); //先低字节再高字节
				memcpy(&smsdata[i][point], (buf + 138 * i), count - 138 * i);
				point += count - 138 * i;

				/* 数据转化为 ASCII */
				memset(buff, 0, 512);
				memcpy(buff, smsdata[i], point);
				for(j=0; j<point; j++){
					if(((buff[j] & 0xF0)>= 0xA0) && ((buff[j] & 0xF0) <= 0xF0)){
						smsdata[i][2*j]=(buff[j] >> 4) + 0x37;
					}else{
						smsdata[i][2*j]=(buff[j] >> 4) + 0x30;
					}
					if(((buff[j] & 0x0F)>=0x0A) && ((buff[j] & 0x0F)<=0x0F)){
						smsdata[i][2*j+1]=(buff[j] & 0x0F) + 0x37;
					}else{
						smsdata[i][2*j+1]=(buff[j] & 0x0F) + 0x30;
					}
				}

				smsdata[i][2*j+2] = 0x1A;   //ctrl+z
				smsdata_len[i] = lenbak + (count - 138 * i + 2) + 1;  //smsdata_len为要发送的数据的长度 （smsdata_len-9）为发AT+CMGS时后跟的数据长度
			}else{
				smsdata[i][point++] = (((smseq >> 12) | i) << 12);
				smsdata[i][point++] = (((smseq >> 12) | i) << 12); 	  //先低字节再高字节
				memcpy(&smsdata[i][point], (buf + 138 * i), 138);
				point += 138;

				/* 数据转化为 ASCII */
				memset(buff, 0, 512);
				memcpy(buff, smsdata[i], point);
				for(j=0; j<point; j++){
					if(((buff[j] & 0xF0) >= 0xA0) && ((buff[j] & 0xF0) <= 0xF0)){
						smsdata[i][2*j] = (buff[j] >> 4) + 0x37;
					}else{
						smsdata[i][2*j] = (buff[j] >> 4) + 0x30;
					}
					if(((buff[j] & 0x0F) >= 0x0A) && ((buff[j] & 0x0F) <= 0x0F)){
						smsdata[i][2*j+1] = (buff[j] & 0x0F) + 0x37;
					}else{
						smsdata[i][2*j+1] = (buff[j] & 0x0F) + 0x30;
					}
				}

				smsdata[i][2*j+2] = 0x1A;   //ctrl+z
				smsdata_len[i] = lenbak + 140 + 1;
			}
			point = 0 ;  //每一帧计数清零
			}
			smseq++;   	//每一整帧的值不变
			if(smseq > 0x0FFF){
				smseq = 0;
			}

	}else{
	    i = 0;
	    point = 0;
	    sms_total = 1;  //单帧
	    smsdata[i][point++] = 0x08;
	    smsdata[i][point++] = 0x91;
	    smsdata[i][point++] = 0x68;
	    //整理短信中心号码
	    memcpy(&smsdata[i][point], tel_center, 6);
	    point += 6;
	    smsdata[i][point++] = 0x11;             // 11	基本参数(TP-MTI/VFP)	发送，TP-VP用相对格式
        smsdata[i][point++] = 0x00;             // 00	消息基准值(TP-MR)	0

        if((tel[strlen((char *)tel)-1] & 0xF0) == 0xF0){  //号码最后一位的高4字节是否为F
	    	smsdata[i][point++] = (strlen((char *)tel) - 1) * 2 -1; //由于号码为奇数，总长度减去91或者A1（1字节），乘以2 再减去F
	    }else{
	    	smsdata[i][point++] = (strlen((char *)tel) - 1) * 2;
	    }
	    memcpy(&smsdata[i][point], tel, strlen((char *)tel)); //要发送的号码
	    point += strlen((char *)tel) ;
	    smsdata[i][point++] = 0x00; //协议标识，常用0x00或0x21
	    smsdata[i][point++] = 0x04; //编码标志，为HEX
	    smsdata[i][point++] = 0x00;                     //有效期 5分钟

	    smsdata[i][point++] = count + 2;	 //加上两个字节的分帧标志
	    smsdata[i][point++] = (smseq | 0x8000);
	    smsdata[i][point++] = (smseq | 0x8000) >> 8; //先低字节再高字节
	    
	    memcpy(&smsdata[i][point], buf, count);
	    point += count;

	    /* 数据转化为 ASCII */
    	memset(buff, 0, 512);
    	memcpy(buff, smsdata[i], point);
    	for(j=0; j<point; j++){
    		if(((buff[j] & 0xF0) >= 0xA0) && ((buff[j] & 0xF0) <= 0xF0)){
    			smsdata[i][2*j]=(buff[j] >> 4) + 0x37;
    		}else{
    			smsdata[i][2*j]=(buff[j] >> 4) + 0x30;
    		}
    		if(((buff[j] & 0x0F)>=0x0A) && ((buff[j] & 0x0F)<=0x0F)){
    			smsdata[i][2*j+1]=(buff[j] & 0x0F) + 0x37;
    		}else{
    			smsdata[i][2*j+1]=(buff[j] & 0x0F) + 0x30;
    		}
    		if(j > 138){
    			j--;
    		}
    	}
    	smsdata[i][2*j] = 0x1A;   //ctrl+z
    	smsdata_len[i] = point + 1;
	    point = 0 ;  //计数清零
   }

	//200S内返回---最长时间？？？？？
	timeout = 0;  //确认超时
	back = 1;     //确认标识
	step = DEV_STREAM_DEMAND;
	i = 0;
	while(back){
		switch(step){
		case DEV_STREAM_DEMAND:
			//清空发送缓冲
			memset(send_cmd, 0, sizeof(send_cmd));
			comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_WR);
			sprintf(send_cmd, "%s%d\r", "AT+CMGS=", (smsdata_len[i] - 10));	 //需要减去前面的9个字节(SMSC)和最后的0x1A
			//发送数据命令
			ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
			if(ret != strlen(send_cmd)){
				goto error;
			}
			step = DEV_STREAM_DEFINE;
			sleep(1);
			break;
		case DEV_STREAM_DEFINE:     //接收允许发送提示
			memset(recv_cmd, 0, sizeof(recv_cmd));
			ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, 10);
			flag = 0;
			if(ret > 0){     //接收到发送提示符
				for (j = 0; j < ret; j++)
				{
					if (recv_cmd[j] == '>'){
						flag = 1;
						break;
					}
				}
				if (flag == 1){
					//清空发送缓冲
					comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_WR);
					//发送短信
					ret = comport_send (CFG_GPRS_ME3000_COMPORT, smsdata[i], smsdata_len[i]);
					if (ret < 0){
						goto error;
					}
					timeout = 0;
					step = DEV_STREAM_RECV_VERIFY;
				}else{
					ret = deal_recv(recv_cmd, ref, (char *)'>');    //读串口（将串口缓冲都读出来，把读到得数据放入数据缓冲之中并判断是否有需要的返回）
					if(ret == 0){
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_WR);
						//发送短信
						ret = comport_send (CFG_GPRS_ME3000_COMPORT, smsdata[i], smsdata_len[i]);
						if (ret < 0){
							goto error;
						}
						timeout = 0;
						step = DEV_STREAM_RECV_VERIFY;
					}else if(ret == -ERR_GPRS_CLOSE){  //服务器被动断开
						goto error;
					}else{
						step = DEV_STREAM_DEFINE;  //继续读“>”
					}
				}
			}else if(ret == 0){
				timeout = timeout - 1 - CFG_RECV_TIMEOUT;
				if(timeout > 0){
					step = DEV_STREAM_DEFINE;
					sleep(1);
				}else{
					back = 0;
					ret = -ERR_TIMEOUT;
					goto error;
				}
			}else if(ret < 0){
				goto error;
			}
			break;
		case DEV_STREAM_RECV_VERIFY:
			ref = comport_recv (CFG_GPRS_ME3000_COMPORT, recv_cmd, 11);   //测试发个位数，十位数，三位数的情况
			if((ret > 0) && (check_echo((char *)recv_cmd, "\r\n+CMGS:") == 0)){
				ret = atoi((char *)&recv_cmd[8]);
				back = 0;
				timeout = 0;    //有可能把0K读出来，那样的话OK这个状态就不要了，直接认为发送成功
				i++;
			}else if((ret > 0) && (check_echo((char *)recv_cmd, "\r\nCMS ERROR:") == 0)){
				back = 0;
				timeout = 0;
				ret = -ERR_GPRS_MODEL;
				goto error;
			}else if(ret > 0){
				ret = deal_recv(recv_cmd, ref, "\r\n+CMGS:");    //读串口（将串口缓冲都读出来，把读到得数据放入数据缓冲之中并判断是否有需要的返回）
				if(ret == 0){
					timeout = 0;
					step = DEV_STREAM_RECV_OK;     //这个ok可能和+cmgs一块收到，那么这个OK这个状态就不要了??????待确定
					i++;
				}else if(ret == -ERR_GPRS_CLOSE){  //服务器被动断开
					goto error;
				}else{
					step = DEV_STREAM_RECV_VERIFY;  //继续读“+TCPSEND”
				}
			}else if(ret == 0){
				timeout = timeout - 1 - CFG_RECV_TIMEOUT;
				if(timeout > 0){
					step = DEV_STREAM_RECV_VERIFY;
					sleep(1);
				}else{
					back = 0;
					ret = -ERR_TIMEOUT;
					goto error;
				}
			}else if(ret < 0){
				goto error;
			}
			break;
		case DEV_STREAM_RECV_OK:   //接收发送确认“OK”????????????跟上一个一个状态
			//判断是否发送成功消息
			memset(recv_cmd, 0, sizeof(recv_cmd));
			recv_len = strlen("\r\nOK\r\n");
			ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, recv_len);
			if (ret < 0){
				goto error;      //接收错误
			}else if((ret > 0) && (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0)){
				timeout = 0;
				if(i == sms_total){
					back = 0;
					ret = count; //???????????流量
				}else{
					step = DEV_STREAM_DEMAND;   //继续发送
				}
			}else if((ret > 0) && (check_echo((char *)recv_cmd, "\r\nOK\r\n") != 0)){
				ret = deal_recv(recv_cmd, ref, "\r\nOK\r\n");    //读串口（将串口缓冲都读出来，把读到得数据放入数据缓冲之中并判断是否有需要的返回）
				if(ret == 0){
					timeout = 0;
					back = 0;
				}else if(ret == -ERR_GPRS_CLOSE){  //服务器被动断开
					back = 0;
					goto error;
				}else{
					step = DEV_STREAM_RECV_OK;  //继续读“\r\nOK\r\n”
				}
			}else if (ret==0){
				timeout = timeout - 1 - CFG_RECV_TIMEOUT;
				if(timeout > 0){
					step = DEV_STREAM_RECV_OK;
					sleep(1);
				}else{
					back = 0;
					ret = -ERR_TIMEOUT;
					goto error;
				}
			}
			break;
		default:
			back = 0;
			ret = -ERR_SYS;
			goto error;
		break;
		}
	}
	
error:
	//解除互斥锁
	ref = pthread_mutex_unlock(&(gprsstate.mutex));
	if (ref){
		ret = -ERR_SYS;
		goto error;
	}
	return(ret);
}


int gprs_me3000_recvsmg(u8 *buf, u32 count)
{
	return(0);
}

/******************************************************************************
*	函数:	gprs_me3000_connect
*	功能:	gprs模块建立连接
*	参数:	mode			-	连接方式：0创建新连接；1激活重连
*	返回:	0				-	成功(收到确认命令)
			-ERR_TIMEOUT	-	连接超时

*	说明:	无
******************************************************************************/
int gprs_me3000_connect(u8 mode)
{
	int ret,ref;
	u8 step, connect_flag;
	u8 timeout;
	char send_cmd[CFG_DATA_MAX];  	//发送数据命令
	u8 recv_cmd[CFG_RECV_BACKMAX];  //接收命令
	
	u8 ip_port[6];
	memcpy(ip_port, gprsstate.ip, sizeof(ip_port));
	struct timespec to;
	//获得串口锁
	to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;      //秒
	to.tv_nsec = 0;                       			//纳秒
	ref = pthread_mutex_timedlock(&(gprsstate.mutex), &to);
	if (ref == ETIMEDOUT){
		ret = -ERR_BUSY;               //设备忙（发送中）
		return ret;
	}else{
		ret = -ERR_SYS;
		return ret;
	}
	if (mode == 0){
		step = DEV_GPRS_ATE0;
	}else if(mode == 1){
		step = DEV_GPRS_CHECK_PPPNET;
	}
	connect_flag = 1;
	while(connect_flag){     //connect_flag = 0 连接已经建立
		switch(step){
			case DEV_GPRS_ATE0:    //关闭回显
				memset(send_cmd, 0, sizeof(send_cmd));
				memcpy(send_cmd, "ATE0\r\n", sizeof("ATE0\r\n")-1);   
				//清空发送缓冲
				comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
				//发送数据命令
				ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
				if(ret != strlen(send_cmd)){
					goto error;
				}
				timeout = 10;   //回显回复最长允许时间 ----- 手册
				step = DEV_GPRS_CHECK_ATE0;
				sleep(1);
				break;
			case DEV_GPRS_CHECK_ATE0:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if ((check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0) || (check_echo((char *)recv_cmd, "ATE0") == 0)){    //回显被关断
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CNMI=2,2,0,0,0\r\n", sizeof("AT+CNMI=2,2,0,0,0\r\n")-1);   
						//清空缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 5;   //CNMI设置回复最长允许时间 ----- 手册
						step = DEV_GPRS_CNMI;
						sleep(1);
					}else{
						step = DEV_GPRS_CHECK_ATE0;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						//重发
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CHECK_ATE0;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CNMI:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0){
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CNMI?\r\n", sizeof("AT+CNMI?\r\n")-1);   
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 10;   //CNMI查询回复最长允许时间 ----- 手册
						step = DEV_GPRS_CHECK_CNMI;
						sleep(1);	
					}else if (check_echo((char *)recv_cmd, "\r\nCME ERROR:") == 0){
						//可以打印一下错误标识
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
					}else{
						step = DEV_GPRS_CNMI;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CNMI;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CHECK_CNMI:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\n+CNMI: 2,2,0,0,0\r\nOK\r\n") == 0){       //？？？？待验证
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CMGF=0\r\n", sizeof("AT+CMGF=0\r\n")-1);  //设置为PDU模式 
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 10;   //CMGF回复最长允许时间 ----- 手册
						step = DEV_GPRS_CMGF;
						sleep(1);	
					}else{
						step = DEV_GPRS_CHECK_CNMI;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CHECK_CNMI;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CMGF:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0){
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CMGF?\r\n", sizeof("AT+CMGF?\r\n")-1);   //查询是否为PDU格式
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 10;   //CMGF查询回复最长允许时间 ----- 手册
						step = DEV_GPRS_CHECK_CMGF;
						sleep(1);	
					}else if (check_echo((char *)recv_cmd, "\r\nCME ERROR:") == 0){
						//可以打印一下错误标识
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
					}else{
						step = DEV_GPRS_CMGF;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CMGF;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CHECK_CMGF:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\n+CMGF: 0\r\nOK\r\n") == 0){    //????待检测
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CSMS=0\r\n", sizeof("AT+CSMS=0\r\n")-1);  //设置短信服务，SMS相关AT指令支持 GSM07.05 Phase 2；
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 10;   //CNMI查询回复最长允许时间 ----- 手册
						step = DEV_GPRS_CSMS;
						sleep(1);	
					}else{
						step = DEV_GPRS_CHECK_CMGF;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CHECK_CMGF;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CSMS:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0){     //回复待验证？？？？
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CSMS?\r\n", sizeof("AT+CSMS?\r\n")-1);   //查询CSMS
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 10;   //CNMI查询回复最长允许时间 ----- 手册
						step = DEV_GPRS_CHECK_CSMS;
						sleep(1);	
					}else if (check_echo((char *)recv_cmd, "\r\nCME ERROR:") == 0){
						//可以打印一下错误标识
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
					}else{
						step = DEV_GPRS_CSMS;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CSMS;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CHECK_CSMS:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\n+CSMS: 0") == 0){
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CSCS=\"HEX\"\r\n", sizeof("AT+CSCS=\"HEX\"\r\n")-1);  //设置编码形式
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 10;   //CSCS查询回复最长允许时间 ----- 手册
						step = DEV_GPRS_CSCS;
						sleep(1);	
					}else{
						step = DEV_GPRS_CHECK_CSMS;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CHECK_CSMS;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CSCS:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0){
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CSCS?\r\n", sizeof("AT+CSCS?\r\n")-1);   //查询CSMS
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 10;   //CSCS查询回复最长允许时间 ----- 手册
						step = DEV_GPRS_CHECK_CSCS;
						sleep(1);	
					}else if (check_echo((char *)recv_cmd, "\r\nCME ERROR:") == 0){
						//可以打印一下错误标识
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
					}else{
						step = DEV_GPRS_CSCS;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CSCS;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CHECK_CSCS:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\n+CSCS: \"HEX\"") == 0){
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CREG?\r\n", sizeof("AT+CREG?\r\n")-1);  //查询网络	
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 200;   //查询网络回复最长允许时间 ----- 手册
						step = DEV_GPRS_CHECK_CREG;
						sleep(1);	
					}else{
						step = DEV_GPRS_CHECK_CSCS;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CHECK_CSCS;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CHECK_CREG:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\n+CREG:") == 0){     //？？？？？OK？？？
						//搜网成功
						if ((recv_cmd[11] == '1') || (recv_cmd[11] == '5')){   //1表示已注册本地网络，5表示已注册漫游状态
		            		memset(send_cmd, 0, sizeof(send_cmd));
							memcpy(send_cmd, "AT+XISP=0\r\n", sizeof("AT+XISP=0\r\n")-1);  //设置内部协议栈
							//清空发送缓冲
							comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
							//发送数据命令
							ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
							if(ret != strlen(send_cmd)){
								goto error;
							}
							//设置时间不超过80s
							step = DEV_GPRS_XISP;
							timeout = 80;    //设置内部协议时间----手册
							sleep(1);	
						}else{
							timeout = 0;
						}
					}else{
						step = DEV_GPRS_CHECK_CREG;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CHECK_CREG;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_XISP:     //设置内部协议
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0){
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+XISP?\r\n", sizeof("AT+XISP?\r\n")-1);  //设置协议类型0内部协议
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 30;   //查询XISP回复最长允许时间 ----- 手册
						step = DEV_GPRS_CHECK_XISP;
						sleep(1);	
					}else{
						step = DEV_GPRS_XISP;
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_XISP;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CHECK_XISP:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\n+XISP:0") == 0){
						//协议设置正确
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+CGDCONT=1,\"IP\",\"CMNET\"\r\n", sizeof("AT+CGDCONT=1,\"IP\",\"CMNET\"\r\n")-1);   //设置PDP格式
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_INITPPP;
						timeout = 80;        //设置PDP格式回复时间 -----手册                      
						sleep(1);	
					}else{
						step = DEV_GPRS_CHECK_XISP;
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_CHECK_XISP;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CODE:
				/************设置登录用户名和密码、登录网络的顺序？？？******************/
				//获得APN，USER,PWD？？？？？？？？？
				memset(send_cmd, 0, sizeof(send_cmd));
				snprintf(send_cmd, 100, "AT+ZPNUM=\"%s\",\"%s\",\"%s\"\r\n", "APN", "USER", "PWD");
				//清空发送缓冲
				comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
				//发送数据命令
				ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
				if(ret != strlen(send_cmd)){
					goto error;
				}
				timeout = 80;   //密码回复最长允许时间 ----- 手册
				step = DEV_GPRS_INITPPP;
				sleep(1);	
				break;
			case DEV_GPRS_INITPPP:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0){
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+XIIC=1\r\n", sizeof("AT+XIIC=1\r\n")-1);            //登录网络
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 200;   //登录网络回复最长允许时间 ----- 手册
						step = DEV_GPRS_CHECK_PPPOK;
						sleep(1);	
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						//重新发送命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						step = DEV_GPRS_INITPPP;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
					
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CHECK_PPPOK:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0){
						memset(send_cmd, 0, sizeof(send_cmd));
						memcpy(send_cmd, "AT+XIIC?\r\n", sizeof("AT+XIIC?\r\n")-1);   //查询是否登录网络
						//清空发送缓冲
						comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
						//发送数据命令
						ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
						if(ret != strlen(send_cmd)){
							goto error;
						}
						timeout = 30;   //登录网络回复最长允许时间 ----- 手册
						step = DEV_GPRS_CHECK_PPPNET;
						sleep(1);	
					}else{
						step = DEV_GPRS_CODE;   //没有登录成功去设置用户名和密码
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						step = DEV_GPRS_CHECK_PPPOK;
						sleep(1);
					}else{
						step = DEV_GPRS_CODE;   //没有登录成功去设置用户名和密码
					}
				}else{
					goto error;
				}
				break;
			case DEV_GPRS_CHECK_PPPNET:
				if (mode == 0){
					memset(recv_cmd, 0, sizeof(recv_cmd));
					ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
					if (ret > 0){
						if (check_echo((char *)recv_cmd, "\r\n+XIIC:1") == 0){
							//PPP已连接
							//???????????????????设置状态
							//IP连接
	    					memset(send_cmd, 0, sizeof(send_cmd));
							if(gprsstate.connect_index == DEV_GPRS_TCP){
	        					sprintf(send_cmd, "AT+TCPSETUP=0,%d.%d.%d.%d,%d\r\n",ip_port[0],ip_port[1],ip_port[2],ip_port[3],ip_port[4]|(ip_port[5]<<8));    //TCP连接,bit7为0
	    
	    					}else{
	    						sprintf(send_cmd, "AT+UDPSETUP=0,%d.%d.%d.%d,%d\r\n",ip_port[0],ip_port[1],ip_port[2],ip_port[3],ip_port[4]|(ip_port[5]<<8));    //UDP连接,bit7为1
	    					}
							//清空发送缓冲
							comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
							//发送数据命令
							ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
							if(ret != strlen(send_cmd)){
								goto error;
							}
							timeout = 60;   //IP连接回复最长允许时间 ----- 手册
							step = DEV_GPRS_CHECK_PPPLINK;
							sleep(1);	
							
						}else{
							step = DEV_GPRS_CHECK_CREG;
						}
					}else if (ret == 0){
						timeout = timeout - 1 - CFG_RECV_TIMEOUT;
						if(timeout > 0){
							step = DEV_GPRS_CHECK_CREG;
							sleep(1);
						}else{
							ret = -ERR_TIMEOUT;
							goto error;
						}
					}else{
						goto error;
					}
				}else if (mode == 1){      //重新激活连接
					memset(send_cmd, 0, sizeof(send_cmd));
					if(gprsstate.connect_index == DEV_GPRS_TCP){
    					sprintf(send_cmd, "AT+TCPSETUP=0,%d.%d.%d.%d,%d\r\n",ip_port[0],ip_port[1],ip_port[2],ip_port[3],ip_port[4]|(ip_port[5]<<8));    //TCP连接,bit7为0

					}else{
						sprintf(send_cmd, "AT+UDPSETUP=0,%d.%d.%d.%d,%d\r\n",ip_port[0],ip_port[1],ip_port[2],ip_port[3],ip_port[4]|(ip_port[5]<<8));    //UDP连接,bit7为1
					}
					//清空发送缓冲
					comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
					//发送数据命令
					ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
					if(ret != strlen(send_cmd)){
						goto error;
					}
					timeout = 60;   //IP连接回复最长允许时间 ----- 手册
					step = DEV_GPRS_CHECK_PPPLINK;
					sleep(1);	
				}
				break;
			case DEV_GPRS_CHECK_PPPLINK:
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret > 0){
					if (check_echo((char *)recv_cmd, "\r\nOK\r\n\r\n+TCPSETUP:0,OK") == 0){    //连接成功
						//状态位？？？？
					}
				}else if (ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						step = DEV_GPRS_CHECK_CREG;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}else{
					goto error;
				}
				break;
			default:
				break;
		}
	}
	
error:
	//解除互斥锁
	ret = pthread_mutex_unlock(&(gprsstate.mutex));
	if (ret){
		ret = -ERR_SYS;
		goto error;
	}
	return(ret);
}

/******************************************************************************
*	函数:	gprs_me3000_disconnect
*	功能:	gprs模块断开连接
*	参数:	
*	返回:	0				-	成功(收到确认命令)
			-ERR_TIMEOUT	-	连接超时

*	说明:	无
******************************************************************************/
int gprs_me3000_disconnect(void)
{
	int ret,ref;
	u8 timeout,back;
	u8 step;            			//交互状态
	char send_cmd[CFG_DATA_MAX];  	//发送数据命令
	u8 recv_cmd[CFG_RECV_BACKMAX];  				//接收命令
	
	struct timespec to;
	
	//判断是否为在线状态
	if(gprsstate.connect_state == 0){
		ret = -ERR_DISCONNECT;
		return ret;
	}

	//获得锁
	to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;      //秒
	to.tv_nsec = 0;                       			//纳秒
	ref = pthread_mutex_timedlock(&(gprsstate.mutex), &to);
	if (ref == ETIMEDOUT){
		ret = -ERR_BUSY;               //设备忙（发送中）
		return ret;
	}else if(ref){
		ret = -ERR_SYS;
		return ret;
	}
	
	timeout = 0;  //确认超时
	back = 1;     //确认标识
	step = DEV_STREAM_DEMAND;
	while(back){
		switch(step){
			case DEV_STREAM_DEMAND:
				memset(send_cmd, 0, sizeof(send_cmd));
				if(gprsstate.connect_index == 0){    //tcp方式
					memset(send_cmd, 0, sizeof(send_cmd));
					memcpy(send_cmd, "AT+TCPCLOSE=0\r\n", sizeof("AT+TCPCLOSE=0\r\n")-1);

				}else{                               //UDP方式
					memset(send_cmd, 0, sizeof(send_cmd));
					memcpy(send_cmd, "AT+UDPCLOSE=0\r\n", sizeof("AT+UDPCLOSE=0\r\n")-1);
				}
				//清空发送缓冲
				comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
				//发送数据命令
				ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
				if(ret != strlen(send_cmd)){
					goto error;
				}
				timeout = 60;   //IP连接回复最长允许时间 ----- 手册
				step = DEV_GPRS_CHECK_PPPLINK;
				sleep(1);
				break;
			case DEV_STREAM_RECV_VERIFY:
				//判断是否收到成功断开消息
				memset(recv_cmd, 0, sizeof(recv_cmd));
				ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
				if (ret < 0){
					goto error;      //接收错误
				}else if(ret > 0){
					if ((check_echo((char *)recv_cmd, "\r\n+TCPCLOSE") == 0) && (check_echo((char *)recv_cmd, "\r\n+UDPCLOSE") == 0)){
					//??状态量
						back = 0;
						ret = 0;
					}
				}else if(ret == 0){
					timeout = timeout - 1 - CFG_RECV_TIMEOUT;
					if(timeout > 0){
						step = DEV_GPRS_CHECK_CREG;
						sleep(1);
					}else{
						ret = -ERR_TIMEOUT;
						goto error;
					}
				}
				break;
			default:
				break;
		}
	}
	
error:
	//解除互斥锁
	ret = pthread_mutex_unlock(&(gprsstate.mutex));
	if (ret){
		ret = -ERR_SYS;
		goto error;
	}
	return(ret);
}

int gprs_me3000_getstat(u8 cmd, void *stat)
{
	return(0);
}

/******************************************************************************
*	函数:	gprs_me3000_turnon
*	功能:	开机
*	参数:	
*	返回:	0				-	成功
			

*	说明:	无
******************************************************************************/
int gprs_me3000_turnon(void)
{
	return(0);
}

/******************************************************************************
*	函数:	gprs_me3000_turnoff
*	功能:	关机
*	参数:	
*	返回:	0				-	成功
			-ERR_TIMEOUT	-	超时

*	说明:	硬件断电前应先软关机，保护GPRS模块
******************************************************************************/
int gprs_me3000_turnoff(void)
{
	int ret,ref;
	u8 timeout;
	char send_cmd[CFG_DATA_MAX];  	//发送数据命令
	u8 recv_cmd[CFG_RECV_BACKMAX];  //接收命令
	struct timespec to;
	
	//获得锁
	to.tv_sec = time(NULL) + CFG_SEND_TIMEOUT;      //秒
	to.tv_nsec = 0;                       			//纳秒
	ref = pthread_mutex_timedlock(&(gprsstate.mutex), &to);
	if (ref == ETIMEDOUT){
		ret = -ERR_BUSY;               //设备忙（发送中）
		return ret;
	}else if(ref){
		ret = -ERR_SYS;
		return ret;
	}
	
	timeout = CFG_SEND_TIMEOUT;
	memset(send_cmd, 0, sizeof(send_cmd));
	//清空发送缓冲
	comport_flush(CFG_GPRS_ME3000_COMPORT, COMPORT_FLUSH_ALL);
	//发送关机命令
	memcpy(send_cmd, "AT+CPWROFF\r\n", sizeof("AT+CPWROFF\r\n")-1);
	ret = comport_send(CFG_GPRS_ME3000_COMPORT, (u8 *)send_cmd, strlen(send_cmd));
	if(ret != strlen(send_cmd)){
		goto error;
	}
	timeout = 10;   //关机回复最长允许时间 ----- 手册
	while(timeout){
		//判断是否收到成功断开消息
		memset(recv_cmd, 0, sizeof(recv_cmd));
		ret = comport_recv(CFG_GPRS_ME3000_COMPORT, recv_cmd, sizeof(recv_cmd));
		if (ret < 0){
			goto error;      //接收错误
		}else if(ret > 0){
			if (check_echo((char *)recv_cmd, "\r\nOK\r\n") == 0){
				ret = 0;
				timeout = 0;
			}
		}else if(ret == 0){
			timeout = timeout - 1 - CFG_RECV_TIMEOUT;
			ret = -ERR_TIMEOUT;
		}
	}
	
error:
	//解除互斥锁
	ret = pthread_mutex_unlock(&(gprsstate.mutex));
	if (ret){
		ret = -ERR_SYS;
		goto error;
	}
	return(ret);
}

/******************************************************************************
*	函数:	gprs_me3000_poweron
*	功能:	上电
*	参数:	
*	返回:	0				-	成功
			

*	说明:	无
******************************************************************************/
int gprs_me3000_poweron(void)
{
	return(0);
}

/******************************************************************************
*	函数:	gprs_me3000_poweroff
*	功能:	掉电
*	参数:	
*	返回:	0				-	成功
			

*	说明:	无
******************************************************************************/
int gprs_me3000_poweroff(void)
{
	return(0);
}

/******************************************************************************
*	函数:	gprs_me3000_reset
*	功能:	重新启动
*	参数:	
*	返回:	0				-	成功
			

*	说明:	无
******************************************************************************/
int gprs_me3000_reset(void)
{
	return(0);
}





