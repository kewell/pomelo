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
    项目名称	 ：  SGE800计量智能终端平台
    文件名		 ：  net.c
    描述       		 ：  本文件定义了网络模块接口
    版本       		 ：  0.1
    作者       		 ：  孙锐
    创建日期   	 ：  2009.09
******************************************************************************/

//库配置头文件
#include "private/config.h"

//模块启用开关
#ifdef CFG_NET_MODULE

//调试头文件
#include "private/debug.h"

//驱动调用头文件

//C库头文件
#include <stdio.h>						//printf
#include <fcntl.h>						//open
#include <unistd.h>						//read,write
#include <string.h>						//bzero
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>  
#include <net/route.h>


//提供给用户的头文件
#include "include/error.h"
#include "include/net.h"

/*************************************************
  静态全局变量定义
*************************************************/
/*************************************************
  结构类型定义
*************************************************/
//服务器端结构体
static struct{
	struct sockaddr_in client_sockaddr;
	int sockfd;                          
	int client_fd[CFG_NET_BACKLOG];    //接入客户端套接字
	u64 client_state;                  //套接字使用状态
	socklen_t sin_size;
}server_net_info;

//客户端
static struct{
	int sockfd[CFG_NET_BACKLOG];
}client_net_info;

/******************************************************************************
*	函数:	net_ip_set
*	功能:	设置IP
*	参数:	ipaddr			-	ip地址
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数无效

*	说明:	修改ip后重新建立连接
******************************************************************************/
int net_ip_set(char *ipaddr)
{
	struct sockaddr_in sin;   
  	struct ifreq ifr;   
  	int fd = 0;
  	int ret = 0;
    //struct hostent *host;
	/*
	if ((host = gethostbyname(ipaddr)) == NULL){    //地址域解析
		ret = -ERR_INVAL;
		goto error;
	}
	*/
    const char *ifname = "eth0";                     //设备名
  	
  	memset (&ifr, 0 ,sizeof(struct ifreq));          //初始化结构
    if (ipaddr == NULL){
    	ret = -ERR_INVAL;   
    	goto error;
    }   
      
  	fd = socket (AF_INET, SOCK_DGRAM, 0);   
  	if (fd == -1)   
  	{   
  		ret = -ERR_SYS;
  		goto error;   
 	 }   
  	strncpy (ifr.ifr_name, ifname, IFNAMSIZ);        //配置接口属性
  	//ifr.ifr_name[IFNAMSIZ - 1] = 0;   
  	memset(&sin, 0, sizeof(sin));   
  	sin.sin_family = AF_INET;   
    //sin.sin_addr = *((struct in_addr *)host->h_addr);
  	sin.sin_addr.s_addr = inet_addr(ipaddr);
    memset(&(sin.sin_zero), 0, 8);                   //清空
    memcpy(&ifr.ifr_addr, &sin,sizeof(sin)); 
      
  	if(ioctl (fd, SIOCSIFADDR, &ifr) < 0)            //设置
  	{   
  		ret = -ERR_SYS;
  		goto error;
  	}   
    
 	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;   
  	if(ioctl (fd, SIOCSIFFLAGS,&ifr) < 0)     
  	{   
  		ret = -ERR_SYS;
  		goto error;   
  	}
 error:
 	close (fd);
 	return(ret);     
}

/******************************************************************************
*	函数:	net_gateway_set
*	功能:	设置网关
*	参数:	gateway			-	网关地址
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数无效

*	说明:	修改网关后重新建立连接
******************************************************************************/
int net_gateway_set(char *gateway)
{
    int ret;
    struct rtentry rt;
    struct sockaddr_in sin;
    int skfd = -1;
    struct hostent *host;
    
    if   (gateway == NULL){
    	ret = -ERR_INVAL;   
    	goto error;
    }   
    
    memset (&rt, 0, sizeof (struct rtentry));                     //结构初始化
    memset (&sin, 0, sizeof (struct sockaddr_in));
    
    if ((host = gethostbyname(gateway)) == NULL){                 //地址域解析
		ret = -ERR_INVAL;
		goto error;
	}
	sin.sin_family = AF_INET;   
    sin.sin_addr = *((struct in_addr *)host->h_addr);
    memset(&(sin.sin_zero),0,8);                                  //清空
    
    memcpy (&rt.rt_gateway, &sin, sizeof(struct sockaddr_in));    //配置接口属性
    ((struct sockaddr_in *) &rt.rt_dst)->sin_family = AF_INET;
    ((struct sockaddr_in *) &rt.rt_genmask)->sin_family = AF_INET;
    rt.rt_flags = RTF_GATEWAY;
    skfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
    	ret = -ERR_SYS;
    	goto error;
    }
    
    if (ioctl (skfd, SIOCADDRT, &rt) < 0)                         // 设置系统gaetway 
    {
    	ret = -ERR_SYS;
    	goto error;
    }
    ret = 0;
 error:
    close (skfd);
    return (ret);
   
}

/******************************************************************************
*	函数:	net_mask_set
*	功能:	设置mask
*	参数:	mask			-	mask子网掩码地址
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	参数无效

*	说明:	修改后重新建立连接
******************************************************************************/
int net_mask_set(char *mask)
{
	struct sockaddr_in sin;    
  	struct ifreq ifr;   
  	int fd = 0;
  	int ret = 0;
    struct hostent *host;
	
	if ((host = gethostbyname(mask)) == NULL){               //地址域解析
		ret = -ERR_INVAL;
		goto error;
	}
    const char *ifname = "eth0";                             //设备名
  	
  	memset(&ifr, 0 ,sizeof(struct ifreq));   
    if (ifname == NULL){
    	ret = -ERR_SYS;   
    	goto error;
    }
    if (mask == NULL){
    	ret = -ERR_INVAL;   
    	goto error;
    }   
      
  	fd = socket(AF_INET, SOCK_DGRAM, 0);   
  	if (fd == -1)   
  	{   
  		ret = -ERR_SYS;
  		goto error;   
 	 }   
  	strncpy (ifr.ifr_name, ifname, IFNAMSIZ);                  //配置接口属性
  	//ifr.ifr_name[IFNAMSIZ - 1] = 0;   
  	memset (&sin, 0, sizeof(sin));   
  	sin.sin_family = AF_INET;   
    sin.sin_addr = *((struct in_addr *)host->h_addr);
    memset (&(sin.sin_zero),0,8);                              //赋值
    memcpy (&ifr.ifr_addr,&sin,sizeof(sin)); 
      
  	if (ioctl (fd, SIOCSIFNETMASK, &ifr) < 0)                 //设置mark
  	{   
  		ret = -ERR_SYS;
  		goto error;
  	}   
    
 	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;   
  	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0)
  	{   
  		ret = -ERR_SYS;
  		goto error;   
  	}
 error:
 	close (fd);
 	return(ret);     
}
/*******************************客户端IAP函数实现*********************************************/

/******************************************************************************
*	函数:	net_client_init
*	功能:	初始化客户端
*	参数:	timout			-	阻塞模式下接收发送超时时间
			num				-	客户端号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误

*	说明:	无
******************************************************************************/
int net_client_init(u8 num, u16 timeout)
{
	int ret;
	struct timeval tout;

	//创建socket
	if ((client_net_info.sockfd[num] = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		ret = -ERR_SYS;
		goto error;
	}
	if (timeout != 0xffff){
		//设置网络阻塞接收、发送超时时间
		tout.tv_sec = timeout;
		tout.tv_usec = 0;
		//设置发送超时
		ret = setsockopt(client_net_info.sockfd[num], SOL_SOCKET, SO_SNDTIMEO, &tout, sizeof(tout));
		if (ret)
		{
			ret = -ERR_SYS;
			goto error;
		}
		//设置接收超时
		ret = setsockopt(client_net_info.sockfd[num], SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout));
		if (ret)
		{
			ret = -ERR_SYS;
			goto error;
		}
	}

	ret = 0;
error:
	return (ret);
}

/******************************************************************************
*	函数:	net_client_connect
*	功能:	客户端与服务器建立连接
*	参数:	ip				-	服务器IP地址
			port			-	服务器端口号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
 		    -ERR_DISCONNECT	-	服务器未打开

*	说明:	无
******************************************************************************/
int net_client_connect(u8 num, char *ip,u16 port)
{
	int ret = 0;
	struct sockaddr_in serv_addr;
	struct hostent *host;
	
	if (num >= CFG_NET_BACKLOG){
		ret = -ERR_INVAL;
		return ret;
	}

	if ((host = gethostbyname(ip)) == NULL){    //地址域解析
		ret = -ERR_INVAL;
		goto error;
	}
	//设置参数
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&(serv_addr.sin_zero),0,8);          //将server_sockaddr.sin_zero所在的内存设置为0
	
	//发起对服务器端的连接
	if (connect(client_net_info.sockfd[num], (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1){
		NETPRINTF("connect fail!\n");
		ret = -ERR_DISCONNECT;
		goto error;
	}
	else{
		NETPRINTF("connect!\n");
	}
	
error:
	return (ret);
}

/******************************************************************************
*	函数:	net_client_receive
*	功能:	客户端接收数据
*	参数:	buf				-	接收缓冲区地址
            len				-	接收缓冲区大小
            length			-	实际接收大小（输出）
            flag			-	阻塞非阻塞属性：0表示阻塞；0xffff表示非阻塞
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口参数设置错误
 		    -ERR_TIMEOUT	-	连接超时
 		    -ERR_DISCONNECT	-	网络断开

*	说明:	无
******************************************************************************/
int net_client_receive(u8 num, u8 *buf, u16 max_length, u16 *length, u8 flag)
{
	int recvbytes,ret;
	int errnum;
	
	if (num >= CFG_NET_BACKLOG){
		ret = -ERR_INVAL;
		return ret;
	}

	if (flag == NET_NONBLOCK){
		recvbytes = recv(client_net_info.sockfd[num], buf, max_length, MSG_DONTWAIT);    //非阻塞接收
	}else if (flag == NET_BLOCK){
 		recvbytes = recv(client_net_info.sockfd[num], buf, max_length, 0);               //阻塞接收
	}else{
		ret = -ERR_INVAL;
		goto error;
	}

 	if (recvbytes != -1){
 		memcpy(length, &recvbytes, 2);						//数据长度2个字节
 		ret = 0;
    }else{
    	errnum = errno;
		NETPRINTF("error=%d\n",errnum);
    	if (errnum == EAGAIN){
    		ret = -ERR_TIMEOUT;
    		goto error;
    	}else{
    		ret = -ERR_SYS;
    		goto error;
    	}
    }
    ret = 0;
error:
	return(ret);
}

/******************************************************************************
*	函数:	net_client_send
*	功能:	客户端发送数据
*	参数:	buf				-	发送缓冲区地址
            length			-	发送字节数
*	返回:	>0				-	成功发送的字节数
			-ERR_INVAL		-	接口参数设置错误
 		    -ERR_TIMEOUT	-	连接超时
 		    -ERR_DISCONNECT	-	网络断开

*	说明:	无
******************************************************************************/
int net_client_send(u8 num, u8 *buf,u16 length)
{
	int ret;
	int errnum;
	if (length == 0){
		ret = -ERR_INVAL;
		goto error;
	}
	if (num >= CFG_NET_BACKLOG){
			ret = -ERR_INVAL;
			return ret;
	}

	ret = send(client_net_info.sockfd[num], buf, length, 0);
	if (ret == -1){
		errnum = errno;
		if ((errnum == EPIPE) || (errnum == EDESTADDRREQ)){
			ret = -ERR_DISCONNECT;
		}else{
			NETPRINTF("error=%d\n",errnum);
			ret = -ERR_TIMEOUT;
		}
		goto error;
	}
error:
	return(ret);
}


/******************************************************************************
*	函数:	net_client_disconnect
*	功能:	客户端断开连接
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
*	说明:	无
******************************************************************************/
int net_client_disconnect(u8 num)
{
	int ref,ret;
	int errnum;
	ref = close(client_net_info.sockfd[num]);
	if (ref == 0){
		NETPRINTF("disconnect net success!\n");
	}else{
		errnum = errno;
		if(errnum == EBADF){
			NETPRINTF("net disconnect already!\n");	
			ret = -ERR_BUSY;
			goto error;
		}else{
			ret = -ERR_SYS;
			goto error;
		}
	}
	ret = 0;
error:
	return(ret);
}

/*******************************服务器端API函数实现***********************************/
/******************************************************************************
*	函数:	net_server_init
*	功能:	初始化服务器端
*	参数:	port			-	服务器端口号
			timout			-	阻塞模式下发送/接收/建立连接超时
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误

*	说明:	无
******************************************************************************/
int net_server_init(u16 port, u16 timeout)
{
	int ret,i;
	int optval = 1; 
	struct timeval tout;
	
	struct sockaddr_in server_sockaddr;
	server_net_info.client_state = 0;                                         //初始化套接字使用状态1表示改套接字没用
	for (i = 0;i < CFG_NET_MAXSIZE; i++){
		server_net_info.client_fd[i] = 0;
	}
	
	if ((server_net_info.sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){       //AF_INET--IPv4协议
		NETPRINTF("opening socket failed!");
		ret = -ERR_SYS;
		goto error;
	}else{ 
		NETPRINTF("socket success!");
		server_sockaddr.sin_family = AF_INET;
		server_sockaddr.sin_port = htons(port);
		server_sockaddr.sin_addr.s_addr = INADDR_ANY;                         //INADDR_ANY 表示所有计算机都能连过来
		memset(&(server_sockaddr.sin_zero), 0, 8);
		
		if (timeout != 0xffff){
			//设置网络阻塞接收、发送超时时间
			tout.tv_sec = timeout;
			tout.tv_usec = 0;
			//设置发送超时
			ret = setsockopt(server_net_info.sockfd, SOL_SOCKET, SO_SNDTIMEO, &tout, sizeof(tout));
			if (ret)
			{
				ret = -ERR_SYS;
				goto error;
			}
			//设置接收超时
			ret = setsockopt(server_net_info.sockfd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout));
			if (ret)
			{
				ret = -ERR_SYS;
				goto error;
			}
		}
		//设置端口复用
  		if (setsockopt(server_net_info.sockfd, SOL_SOCKET, SO_REUSEADDR,(char *)&optval,sizeof(optval)) !=  0){   
        	NETPRINTF("error!setsockopt failed!\n");
        	ret = -ERR_INVAL;
        	goto error;   
          }   

		if (bind(server_net_info.sockfd, (struct sockaddr *)&server_sockaddr, sizeof(struct sockaddr)) == -1){
			NETPRINTF("blind failed!\n");
			ret = -ERR_INVAL;
			goto error;
		}else{ 
			NETPRINTF("blind success!\n");
		}
		if(listen(server_net_info.sockfd, CFG_NET_BACKLOG) == -1){         //创建等待队列
    		NETPRINTF("listen failed!\n");
    		ret = -ERR_SYS;
    		goto error;
		}
	}
	ret = 0;
	return (ret);
 error:
  	close(server_net_info.sockfd);
  	return (ret);
}

/******************************************************************************
*	函数:	net_server_listen
*	功能:	服务器监听
*	参数:	id 				-	返回监听到的套接字号
			mode			-	（NET_LISTEN_SELECT服务器端单线程实现方式）同时监听服务器套接字和已连接客户端的多个套接字
							-	NET_LISTEN:监听服务器套接字，这种模式下id接口无效
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口错误

*	说明:	服务器端单线程实现方式id返回Oxff表示有新的连接应调用建立连接函数，
			id返回其他为套接字号为id的连接有数据，应调用服务器接收函数
******************************************************************************/
int net_server_listen(u8 *id, u8 mode)
{
	int ref,ret;
	int i,max_fd;
	fd_set testfds;                                    //select 套接字队列   
	
	if (mode == NET_LISTEN_SELECT)
	{
    	FD_ZERO (&testfds);
    	FD_SET (server_net_info.sockfd, &testfds);         //加入监听状态字集
    	max_fd = server_net_info.sockfd;                   //8位状态位
    	for (i = 0; i < CFG_NET_MAXSIZE; i ++){
    		if (server_net_info.client_state & (1 << i)){   //判断状态位
    			FD_SET(server_net_info.client_fd[i], &testfds);
    		}
    		if (max_fd < server_net_info.client_fd[i]){
    			max_fd = server_net_info.client_fd[i];
    		}
    	}
    	
    	ref = select (max_fd+1, &testfds, NULL, NULL, NULL);
    	if(ref < 0){
    		ret = -ERR_SYS;
    		goto error;
    	}else{
    		if (FD_ISSET (server_net_info.sockfd, &testfds)){
    			*id = 0xff;        							//id返回Oxff表示有新的连接应调用建立连接函数
    			NETPRINTF("new connect\n");
    		}else{
    			for (i = 0;i < CFG_NET_MAXSIZE;i++){
    				if (FD_ISSET(server_net_info.client_fd[i], &testfds)){
    					*id = i;
    				}
    			}
    		}
    	}
	}else if (mode == NET_LISTEN){
		ref = listen(server_net_info.sockfd, CFG_NET_BACKLOG);
		if (ref < 0){
			ret = -ERR_SYS;
    		goto error;
		}
	}else{
		ret = -ERR_INVAL;
		goto error;
	}
	
	ret = 0;
error:
	return (ret);
}

/******************************************************************************
*	函数:	net_server_connect
*	功能:	与客户端建立新连接
*	参数:	id 				-	连接分配的套接字号（返回）
*	返回:	0				-	成功
*	返回:	0				-	成功
			-ERR_TIMEOUT	-	连接超时
			-ERR_SYS		-	系统错误
*	说明:	无
******************************************************************************/
int net_server_connect(u8 *id)
{
	int ret,i;
	u8 ud = 0;
	int errnum;
	int socket_fd_temp;
	
	if (server_net_info.client_state == (1<<CFG_NET_MAXSIZE)-1)
	{
		ret = -ERR_BUSY;
		return(ret);
	}
	NETPRINTF("server waiting**********\n");
	//客户端和服务器断开之后才重新accept
	if((socket_fd_temp = accept(server_net_info.sockfd, \
	(struct sockaddr *)&server_net_info.client_sockaddr, &server_net_info.sin_size)) == -1){
		errnum = errno;
		if (errnum == EAGAIN){
			NETPRINTF("timeout!\n");
			ret = -ERR_TIMEOUT;
			goto error;
		}else{
			NETPRINTF("accept failed!\n");
			ret = -ERR_SYS;
			goto error;
		}
	}
	i = 0;
	while(i < CFG_NET_MAXSIZE){                        //自动分配套接字
		if((server_net_info.client_state & (1 << i)) == 0){        //移位状态位
			*id = i;
			ud = i;
			server_net_info.client_state |= (1 << i);	//移位状态位
			i = CFG_NET_MAXSIZE;
		}else{
			if (i == CFG_NET_MAXSIZE - 1){
				ret = -ERR_BUSY;
				return(ret);
			}
		}
		i ++;
	}

	server_net_info.client_fd[ud]  = socket_fd_temp;
	ret = 0;
	return (ret);
error:
	server_net_info.client_state &= ~(1<< ud);   //移位状态位
	return(ret);
}

/******************************************************************************
*	函数:	net_server_receive
*	功能:	服务器接受函数
*	参数:	id 				-	输入监听到的接收到数据的套接字号
			buf				- 	返回数据缓冲区
			max_length		-	返回数据缓冲区的最大
            length			-	实际返回数据的大小
            flag	      	-	flag=0为阻塞方式（超时时间在初始化中设），0xffff为非阻塞
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口错误
			-ERR_DISCONNECT	-	连接断开
 			-ERR_TIMEOUT	-	超时

*	说明:	无
******************************************************************************/
int net_server_receive(u8 id, u8 *buf, u16 max_length,u16 *length, u8 flag)
{
	int ret,recvbytes;
	int errnum;
	
	if (id >=CFG_NET_MAXSIZE){
		ret = -ERR_INVAL;
		goto error;
	}
	if (max_length == 0){
		ret = -ERR_INVAL;
		goto error;
	}
	if (flag == NET_BLOCK){     						//阻塞方式
		recvbytes = recv(server_net_info.client_fd[id], buf, max_length, 0);
		if(recvbytes != -1){
			memcpy(length, &recvbytes, 2);     			//返回数据长度
			ret = 0;
		}else{
			errnum = errno;
			if ((errnum == EBADF) || (errnum == ENOTCONN)){
				ret = -ERR_DISCONNECT;
				goto error;
			}else if (errnum == EAGAIN){
				ret = -ERR_TIMEOUT;
				goto error;
			}else{
				ret = -ERR_SYS;
				goto error;
			}
		}
	}
	else if(flag == NET_NONBLOCK){					//非阻塞方式
		recvbytes = recv(server_net_info.client_fd[id], buf, max_length, MSG_DONTWAIT);
		if(recvbytes != -1){
			memcpy(length, &recvbytes, 2);     		//返回数据长度
			ret = 0;
		}else{
			errnum = errno;
			if ((errnum == EBADF) || (errnum ==  ENOTCONN)){
				ret = -ERR_DISCONNECT;
				goto error;
			}else if (errnum == EAGAIN){
				ret = -ERR_TIMEOUT;
				goto error;
			}else{
				ret = -ERR_SYS;
				goto error;
			}
		}
	}
	else{
		ret = -ERR_INVAL;
		goto error;
	}

    ret = 0;
error:
	return(ret);
}

/******************************************************************************
*	函数:	net_server_send
*	功能:	服务器发送函数
*	参数:	id 				-	输入要返回数据连接的套接字号
			buf				- 	发送数据缓冲区
            length			-	发送数据的大小
*	返回:	>0				-	成功发送的字节数
			-ERR_INVAL		-	接口错误
			-ERR_SYS		-	系统错误
			-ERR_DISCONNECT	-	连接断开
 			-ERR_TIMEOUT	-	超时

*	说明:	无
******************************************************************************/
int net_server_send(u8 id, u8 *buf, u16 length)
{
	int ret;
	int errnum;
	if (id >= CFG_NET_MAXSIZE){
		ret = -ERR_INVAL;
		goto error;
	}
	if (length == 0){
		ret = -ERR_INVAL;
		goto error;
	}
	ret = send(server_net_info.client_fd[id], buf, length, MSG_NOSIGNAL);
	if (ret == -1){
		NETPRINTF("send data fail!\n");
		errnum = errno;
		if ((errnum == EPIPE) || (errnum == EDESTADDRREQ)){
			ret = -ERR_DISCONNECT;
		}else{
			ret = -ERR_SYS;
		}
		goto error;
	}
error:
	return(ret);
}

/******************************************************************************
*	函数:	net_server_disconnect
*	功能:	服务器断开连接
*	参数:	id 				-	输入要返回数据连接的套接字号
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			-ERR_INVAL		-	接口错误
			-ERR_BUSY		-	已断开

*	说明:	无
******************************************************************************/
int net_server_disconnect(u8 id)
{
	int ref,ret;
	int errnum;
	
	if (id >= CFG_NET_MAXSIZE){
		ret = -ERR_INVAL;
		goto error;
	}
	ref = close(server_net_info.client_fd[id]);
	if (ref == 0){
		NETPRINTF("disconnect net success!\n");
		server_net_info.client_state &= ~(1 << id);      //更新状态位
	}else{
		errnum = errno;
		if (errnum == EBADF){
			NETPRINTF("net disconnect already!\n");	
			ret = -ERR_BUSY;
			goto error;
		}else{
			ret = -ERR_SYS;
			goto error;
		}
	}
	
	ret = 0;
error:
	return(ret);
}

/******************************************************************************
*	函数:	net_server_close
*	功能:	服务器关闭
*	参数:	无
*	返回:	0				-	成功
			-ERR_SYS		-	系统错误
			—ERR_BUSY		-	已关闭

*	说明:	无
******************************************************************************/
int net_server_close()
{
	int ref,ret;
	int errnum;
	
	ref = close(server_net_info.sockfd);
	if (ref == 0){
		NETPRINTF("disconnect net success!\n");
	}else{
		errnum = errno;
		if (errnum == EBADF){
			NETPRINTF("net disconnect already!\n");	
			ret = -ERR_BUSY;
			goto error;
		}else{
			ret = -ERR_SYS;
			goto error;
		}
	}
	ret = 0;
error:
	return ret;
}

#endif      /* CFG_NET_MODULE */
