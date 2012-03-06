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
	文件		：  comport.c
	描述		：  本文件实现了串口模块中的API函数
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.01
******************************************************************************/
//库配置头文件
#include "private/config.h"

#ifdef CFG_COMPORT_MODULE
//驱动调用头文件

//C库头文件

#include <stdio.h>						//printf
#include <fcntl.h>						//open
#include <unistd.h>						//read,write
#include <termios.h>					//串口设置及宏定义
#include <string.h>						//bzero,memcyp
#include <sys/ioctl.h>					//ioctl
#include <errno.h>

//提供给用户的头文件
#include "include/comport.h"
#include "include/error.h"

/*************************************************
  静态全局变量及宏定义
*************************************************/
#ifdef  CFG_GSERIAL_MODULE
#define MAX_COMNUM 8		//如果定义usb转串口，则最大串口数为8
#else
#define MAX_COMNUM 7		//如果没有定义usb转串口，则最大串口数为7
#endif
#define RS485CMD 0x545f	//设置485模式的控制命令
static struct{
	comport_config_t com;	//配置选项
	int fd;
	struct termios opt;
	u8 count;				//打开计数
	u8 mode;
//	pthread_mutex_t mutex;	
}comport[MAX_COMNUM];

/*************************************************
  API函数实现
*************************************************/

/******************************************************************************
*	函数:	comport_init
*	功能:	打开并初始化串口
*	参数:	port			-	串口端口号
			mode			-	串口操作模式
*	返回:	0				-	成功
			-ERR_SYS			-	错误
			-ERR_INVAL		-	参数错误
			-ERR_BUSY		-	已经打开
*	说明:	无
 ******************************************************************************/
int comport_init (u8 port, u8 mode)
{
	int ret = -1;	
	//ttygs0为usb虚拟串口设备
	char *dev[]={"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2","/dev/ttyS3",
				"/dev/ttyS4","/dev/ttyS5","/dev/ttyS6","/dev/ttyGS0"};
	if((port == (MAX_COMNUM - 1)) && (mode == COMPORT_MODE_485)){
		ret = -ERR_INVAL;
		goto err;
	}
	if(port < 0 || port >= MAX_COMNUM){
		ret = -ERR_INVAL;
		goto err;
	}
	if(comport[port].count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}
	if((mode == COMPORT_MODE_485) || (mode == COMPORT_MODE_NORMAL)){
		;
	}else{
		ret = -ERR_INVAL;
		goto err;
	}
	//com4 com5 com6 不支持485模式
	if((mode == COMPORT_MODE_485) && ((port > COMPORT4)||(port == 0))){
		ret = -ERR_NOFUN;
		goto err;
	}


	//打开串口
	comport[port].fd = open(dev[port], O_RDWR|O_NOCTTY|O_NDELAY);
	if (comport[port].fd < 0){
		//perror("Can't Open Serial Port");
		ret = -ERR_SYS;
		goto err;
	}
	if(mode == COMPORT_MODE_485){
		ret = ioctl(comport[port].fd, RS485CMD, 0);
		if(ret < 0){
			perror("com set error");
			ret = -ERR_SYS;
			goto err;
		}
	}	

	fcntl(comport[port].fd, F_SETFL,0);	//恢复串口阻塞模式

	//测试是否为终端设备
	if(isatty(STDIN_FILENO)==0){
		ret = -ERR_SYS;
		goto err;
	}

	//配置默认串口属性	
	comport[port].com.baud = 9600;	//设置波特率-9600
	comport[port].com.verify = COMPORT_VERIFY_NO;
	comport[port].com.ndata = 8;	//设置数据位-8位
	comport[port].com.nstop = 1;
	comport[port].com.timeout = 0xff;	//默认非阻塞
	comport[port].com.rtscts = COMPORT_RTSCTS_DISABLE;
	
	memset( &comport[port].opt, 0,sizeof( comport[port].opt ) );		
	comport[port].opt.c_cflag  |=  CLOCAL | CREAD; //设置字符大小	
	comport[port].opt.c_cflag &= ~CSIZE; 	
	
	comport[port].opt.c_cflag |= CS8;			//设置数据位-8位
	
	comport[port].opt.c_cflag &= ~PARENB;		//设置奇偶校验-无	
	
	cfsetispeed(&comport[port].opt, B9600);	//设置波特率-9600
	cfsetospeed(&comport[port].opt, B9600);	
	
	comport[port].opt.c_cflag &=  ~CSTOPB;		//设置停止位-1位
	
	comport[port].opt.c_cc[VTIME]  = 0;		//设定超时时间
	comport[port].opt.c_cc[VMIN] = 1;
	
	comport[port].opt.c_cflag &= ~CRTSCTS;		//设置硬件流控-无硬件软件流控
	comport[port].opt.c_iflag &= ~(IXON|IXOFF);
	
	if((tcsetattr(comport[port].fd, TCSANOW, &comport[port].opt)) != 0)//激活配置
	{
		ret = -ERR_SYS;
		goto err;
	}	
	tcflush(comport[port].fd,TCIFLUSH);//处理未接收字符

	comport[port].mode = mode;
	comport[port].count = 1;
	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	comport_setconfig
*	功能:	配置串口
*	参数:	port			-	串口端口号
			cfg				-	配置项（数据传入）
*	返回:	0				-	成功
			-ESYS			-	系统错误
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化
*	说明:	本串口支持的波特率为 50，110，150，300，600，1200，2400，4800，9600
 			115200，460800，4000000
 ******************************************************************************/
int comport_setconfig (u8 port, comport_config_t *cfg)
{
	int ret = -1;
		
	if(port < 0 || port >= MAX_COMNUM){
		ret = -ERR_INVAL;
		goto err;
	}
	if(comport[port].count == 0){
		ret = -ERR_NOINIT;		//没有初始化
		goto err;
	}
	memset( &comport[port].opt, 0,sizeof( comport[port].opt ) );		

	//设置数据位
	switch( cfg->ndata ){
		case 6:		//设置数据位6位
			comport[port].opt.c_cflag |= CS6;
			break;
		case 7:		//设置数据位
			comport[port].opt.c_cflag |= CS7;
			break;
		case 8:		//设置数据位
			comport[port].opt.c_cflag |= CS8;
			break;
		default:
			return -ERR_INVAL;
			break;
	}
	
	//设置奇偶校验位
	switch( cfg->verify){
	case COMPORT_VERIFY_EVEN://奇数
		comport[port].opt.c_cflag |= PARENB;
		comport[port].opt.c_cflag |= PARODD;
		comport[port].opt.c_iflag |= (INPCK | ISTRIP);
		break;
	case COMPORT_VERIFY_ODD: //偶数
		comport[port].opt.c_iflag |= (INPCK | ISTRIP);
		comport[port].opt.c_cflag |= PARENB;
		comport[port].opt.c_cflag &= ~PARODD;
		break;
	case COMPORT_VERIFY_NO:  //无奇偶校验位
		comport[port].opt.c_cflag &= ~PARENB;
		break;
	default:
		return -ERR_INVAL;
		break;
	}
	
	//设置波特率
	switch( cfg->baud){
		case 50:	//波特率
			cfsetispeed(&comport[port].opt, B50);
			cfsetospeed(&comport[port].opt, B50);
			break;
		case 110:	//波特率
			cfsetispeed(&comport[port].opt, B110);
			cfsetospeed(&comport[port].opt, B110);
			break;
		case 150:	//波特率
			cfsetispeed(&comport[port].opt, B150);
			cfsetospeed(&comport[port].opt, B150);
			break;
		case 300:	//波特率
			cfsetispeed(&comport[port].opt, B300);
			cfsetospeed(&comport[port].opt, B300);
			break;
		case 600:	//波特率
			cfsetispeed(&comport[port].opt, B600);
			cfsetospeed(&comport[port].opt, B600);
			break;
		case 1200:	//波特率
			cfsetispeed(&comport[port].opt, B1200);
			cfsetospeed(&comport[port].opt, B1200);
			break;
		case 2400:	//波特率
			cfsetispeed(&comport[port].opt, B2400);
			cfsetospeed(&comport[port].opt, B2400);
			break;
		case 4800:	//波特率
			cfsetispeed(&comport[port].opt, B4800);
			cfsetospeed(&comport[port].opt, B4800);
			break;
		case 9600:	//波特率
			cfsetispeed(&comport[port].opt, B9600);
			cfsetospeed(&comport[port].opt, B9600);
			break;
		case 19200:	//波特率
			cfsetispeed(&comport[port].opt, B19200);
			cfsetospeed(&comport[port].opt, B19200);
			break;
		case 38400:	//波特率
			cfsetispeed(&comport[port].opt, B38400);
			cfsetospeed(&comport[port].opt, B38400);
			break;
		case 57600:	//波特率
			cfsetispeed(&comport[port].opt, B57600);
			cfsetospeed(&comport[port].opt, B57600);
			break;
		case 115200:	//波特率
			cfsetispeed(&comport[port].opt, B115200);
			cfsetospeed(&comport[port].opt, B115200);
			break;
		case 230400:	//波特率
			cfsetispeed(&comport[port].opt, B230400);
			cfsetospeed(&comport[port].opt, B230400);
			break;
		case 460800:	//波特率
			cfsetispeed(&comport[port].opt, B460800);
			cfsetospeed(&comport[port].opt, B460800);
			break;
		case 500000:	//波特率
			cfsetispeed(&comport[port].opt, B500000);
			cfsetospeed(&comport[port].opt, B500000);
			break;
		case 576000:	//波特率
			cfsetispeed(&comport[port].opt, B576000);
			cfsetospeed(&comport[port].opt, B576000);
			break;
		case 921600:	//波特率
			cfsetispeed(&comport[port].opt, B921600);
			cfsetospeed(&comport[port].opt, B921600);
			break;
		case 1000000:	//波特率
			cfsetispeed(&comport[port].opt, B1000000);
			cfsetospeed(&comport[port].opt, B1000000);
			break;
		case 1152000:	//波特率
			cfsetispeed(&comport[port].opt, B1152000);
			cfsetospeed(&comport[port].opt, B1152000);
			break;	
		case 1500000:	//波特率
			cfsetispeed(&comport[port].opt, B1500000);
			cfsetospeed(&comport[port].opt, B1500000);
			break;
		case 2000000:	//波特率
			cfsetispeed(&comport[port].opt, B2000000);
			cfsetospeed(&comport[port].opt, B2000000);
			break;
		case 2500000:	//波特率
			cfsetispeed(&comport[port].opt, B2500000);
			cfsetospeed(&comport[port].opt, B2500000);
			break;
		case 3000000:	//波特率
			cfsetispeed(&comport[port].opt, B3000000);
			cfsetospeed(&comport[port].opt, B3000000);
			break;		
		case 3500000:	//波特率
			cfsetispeed(&comport[port].opt, B3500000);
			cfsetospeed(&comport[port].opt, B3500000);
			break;
		case 4000000:	//波特率
			cfsetispeed(&comport[port].opt, B4000000);
			cfsetospeed(&comport[port].opt, B4000000);
			break;
		default:
			ret = -ERR_INVAL;
			goto err;
			break;
	}
	
	//设置停止位
	if( cfg->nstop == 1 )
		comport[port].opt.c_cflag &=  ~CSTOPB;
	else if ( cfg->nstop == 2 )				//设置2位停止位
		comport[port].opt.c_cflag |=  CSTOPB;
	else{
		ret = -ERR_INVAL;
		goto err;
	}
	//设定超时时间
	if( (cfg->timeout < 0) || (cfg->timeout > 255) ){
		ret = -ERR_INVAL;
		goto err;
	}
	if(cfg->timeout == 0) {
		comport[port].opt.c_cc[VTIME] 	= 0;
		comport[port].opt.c_cc[VMIN] 	= 1;
	}else if( cfg->timeout == 0xff) {		//不阻塞
		comport[port].opt.c_cc[VTIME]	= 0;
		comport[port].opt.c_cc[VMIN]	= 0;
	}else{
		comport[port].opt.c_cc[VTIME]	= cfg->timeout;
		comport[port].opt.c_cc[VMIN] 	= 0;
	}
	
	//设置硬件流控
	if( cfg->rtscts == COMPORT_RTSCTS_ENABLE )
		comport[port].opt.c_cflag |= CRTSCTS;
	else if ( cfg->rtscts == COMPORT_RTSCTS_DISABLE )
		comport[port].opt.c_cflag &= ~CRTSCTS;
	else{
		ret = -ERR_INVAL;
		goto err;
	}
	//激活配置
	if((tcsetattr(comport[port].fd, TCSANOW, &comport[port].opt)) != 0)
	{
		ret = -ERR_SYS;
		goto err;
	}
	//处理未接收字符
	tcflush(comport[port].fd,TCIFLUSH);
	
	memcpy(&comport[port].com,cfg,sizeof(comport_config_t));
	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	comport_getconfig
*	功能:	获取串口配置
*	参数:	port			-	串口端口号
			cfg				-	配置项（数据传出）
*	返回:	0				-	成功
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化
*	说明:	无
 ******************************************************************************/
int comport_getconfig (u8 port, comport_config_t *cfg)
{
	int ret = -1;
	if(port < 0 || port >= MAX_COMNUM){
		ret = -ERR_INVAL;
		goto err;
	}
	if(comport[port].count == 0){
		ret = -ERR_NOINIT;		//没有打开
		goto err;
	}
	memcpy(cfg,&comport[port].com,sizeof(comport_config_t));
	ret = 0;
err:
	return ret;
}

/******************************************************************************
*	函数:	comport_send
*	功能:	串口发送
*	参数:	port			-	串口端口号
			buf				-	数据缓冲区（数据传入）
			count			-	请求字节数
*	返回:	>=0				-	发送成功的字节数
			-ESYS			-	系统错误
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化
*	说明:	无
 ******************************************************************************/
int comport_send (u8 port, u8 *buf, u32 count)
{
	int ret = -1;
	if(port < 0 || port >= MAX_COMNUM){
		ret = -ERR_INVAL;
		goto err;
	}
	if (count == 0){
		ret = -ERR_INVAL;
		goto err;
	}
	if(comport[port].count == 0){
		ret = -ERR_NOINIT;		//没有打开
		goto err;
	}
	ret = write(comport[port].fd, buf, count);
	if(ret < 0){
		ret = -ERR_SYS;
		goto err;
	}
err:
	return ret;
}

/******************************************************************************
*	函数:	comport_recv
*	功能:	串口接收
*	参数:	port			-	串口端口号
			buf				-	数据缓冲区（数据传出）
			count			-	请求字节数
*	返回:	>=0				-	接收成功的字节数
			-ERR_SYS		-	错误
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化
*	说明:	无
 ******************************************************************************/
int comport_recv (u8 port, u8 *buf, u32 count)
{	
	int ret = -1;
	if(port < 0 || port >= MAX_COMNUM){
		ret = -ERR_INVAL;
		goto err;
	}
	if(comport[port].count == 0){
		ret = -ERR_NOINIT;		//没有打开
		goto err;
	}
	ret = read(comport[port].fd, buf, count);
	if(ret == 0){		
		ret = -ERR_TIMEOUT;
		goto err;		
	}
	if(ret < 0){		
		ret = -ERR_SYS;		
		goto err;
	}
		
err:
	return ret;
}

/******************************************************************************
*	函数:	comport_flush
*	功能:	清串口缓冲区
*	参数:	port			-	串口端口号
			mode			-	清缓冲区模式
*	返回:	0				-	成功
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化
*	说明:	无
 ******************************************************************************/
int comport_flush (u8 port, u8 mode)
{
	int ret = -1;
	if(port < 0 || port >= MAX_COMNUM){
		ret = -ERR_INVAL;
		goto err;
	}
	if(comport[port].count == 0){
		ret = -ERR_NOINIT;		//没有打开
		goto err;
	}
	if(mode == COMPORT_FLUSH_ALL)
		ret = tcflush(comport[port].fd,TCIOFLUSH);
	else if(mode == COMPORT_FLUSH_RD)
		ret = tcflush(comport[port].fd,TCIFLUSH);
	else if(mode == COMPORT_FLUSH_WR)
		ret = tcflush(comport[port].fd,TCOFLUSH);
	else{
		ret = -ERR_INVAL;
		goto err;
	}
	if(ret < 0){		
		ret = -ERR_SYS;		
		goto err;
	}
	ret = 0;
err:
	return ret;
}
	

/******************************************************************************
*	函数:	comport_close
*	功能:	关闭串口
*	参数:	port			-	串口端口号
*	返回:	0				-	成功
			-ERR_SYS		-	错误
			-ERR_INVAL		-	参数无效
			-ERR_NOINIT		-	没有初始化
*	说明:	无
 ******************************************************************************/
int comport_close (u8 port)
{
	int ret = -1;
	if(port < 0 || port >= MAX_COMNUM){
		ret = -ERR_INVAL;
		goto err;
	}
	if(comport[port].count == 0){
		ret = -ERR_NOINIT;		//没有打开
		goto err;
	}
	ret = close(comport[port].fd);
	if(ret < 0){
		return -ERR_SYS;
	}
	comport[port].count = 0;	
	ret = 0;
err:
	return ret;
}
#endif		 //CFG_ADC_MODULE
