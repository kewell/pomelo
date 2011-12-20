/******************************************************************************
	项目名称	：  SGE800计量智能终端业务平台
	文件		：  ether.c
	描述		：  本文件实现了网络设备的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
/*
 * wpara 低8字节为网络接收到数据大小，高8位最高位为1时说明这条消息是平台作为服务器接收到
 * 数据，为0为作为客户端接收到的数据，高8位其它位为客户端编号。lpara 为网络数据缓冲区指针
 */
//库配置头文件
#include "framework/config.h"
	
//模块启用开关
#ifdef CFG_ETHER_DEVICE

//C库头文件
#include <stdio.h>
#include <string.h>		//memcpy

//基础平台头文件
#include "sge_core/error.h"
#include "sge_core/thread.h"
#include "sge_core/net.h"
//业务平台头文件
#include "framework/base.h"
#include "framework/message.h"
#include "framework/device/ether.h"
	
/*************************************************
  结构类型定义
*************************************************/


/*************************************************
  静态全局变量及宏定义
*************************************************/

#define ETH0_MAX_CLIENT		8
#define ETH0_BUF_SIZE 256
#define ETH0_BUF_NUM 8
static struct eth0_client_t{
	u8 act;			//连接激活标志
	u8 nd;			//客户端连接号
	int fd;			//客户端连接描述符
	u8 server_nd;	//如果是连接服务器的客户端，表示服务器的nd
	pthread_t id;	//连接监听线程id
	u8 buf[ETH0_BUF_NUM][ETH0_BUF_SIZE];	//消息缓冲区
	u8 cur;	//当前消息写位置
}eth0_client[ETH0_MAX_CLIENT];

//#define ETH0_MAX_SERVER		8			//最大服务器个数
#define ETH0_MAX_SERVER_CLIENT	8		//每个服务器最大可连接客户端个数
static struct eth0_server_t{
	u8 act;			//连接激活标志
	u8 nd;			//服务器连接号
	pthread_t id;	//连接监听线程id
	struct eth0_client_t client[ETH0_MAX_SERVER_CLIENT];
}eth0_server;
/*************************************************
  API
*************************************************/
static void *thread_eth0_client(void * arg)
{
	int ret;
	u8 buf_client[256];
	message_t msg_client;
	u16 recv_size;
	struct eth0_client_t *client = (struct eth0_client_t *)arg;
	//设置线程属性，收到cancel后立即退出
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
//	printf("%s,%d:eth0 client start nd = %d!\n",__FILE__,__LINE__,client->nd);
	while(1){
		ret = net_client_receive(client->nd, buf_client, ETH0_BUF_SIZE, &recv_size, NET_BLOCK);
//		printf("%s,%d:eth0 client %d, recv data size = %d, ret = %d\n",__FILE__,__LINE__,client->nd,recv_size,ret);
		if((ret >= 0) && (recv_size > 0)){
			memcpy(client->buf[client->cur],buf_client,recv_size);
			msg_client.type = MSG_DEVICE_DATA_NET;
			msg_client.wpara = recv_size + (client->nd << 8);
			msg_client.lpara = (u32)&client->buf[client->cur][0];
			message_dispatch(&msg_client);
			if(++client->cur  >= ETH0_BUF_NUM){
				client->cur = 0;
			}
			recv_size = 0;
		}else if((ret == ERR_DISCONNECT) || (ret == -1) || (ret == 0)){
			client->act = 0;
			ret  = net_client_disconnect(client->nd);
//			printf("%s,%d:eth0 client %d, disconnect, ret = %d\n",__FILE__,__LINE__,client->nd,ret);
			//发送连接断开消息
			msg_client.type = MSG_DEVICE_STATE_NET;
			msg_client.wpara =  (client->nd << 8);
			message_dispatch(&msg_client);
			pthread_exit(0);
		}
	}

	pthread_exit(0);
}
static void *thread_eth0_server_client(void * arg)
{
	int ret;
	u8 buf_client[256];
	message_t msg_client;
	u16 recv_size;
	struct eth0_client_t *client = (struct eth0_client_t *)arg;

	//设置线程属性，收到cancel后立即退出
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

//	printf("%s,%d:eth0 server client start nd = %d, fd = %d\n",__FILE__,__LINE__,client->nd,client->fd);
	while(1){
		ret = net_server_receive(client->fd,buf_client, ETH0_BUF_SIZE, &recv_size, NET_BLOCK);
//		printf("%s,%d:eth0 server client recv ret = %d,size = %d,fd = %d,nd = %d\n",__FILE__,__LINE__,ret,recv_size,client->fd,client->nd);
		if(ret >= 0 && (recv_size > 0)){
			memcpy(client->buf[client->cur],buf_client,recv_size);
			msg_client.type = MSG_DEVICE_DATA_NET;
			msg_client.wpara = recv_size + (client->nd << 8)  + (1<<15);
			msg_client.lpara = (u32)&client->buf[client->cur][0];
			message_dispatch(&msg_client);
			if(++client->cur  >= ETH0_BUF_NUM){
				client->cur = 0;
			}
			recv_size = 0;
		}else if((ret == ERR_DISCONNECT) || (ret == 0)){
			client->act = 0;
			ret = net_server_disconnect(client->fd);
			msg_client.type = MSG_DEVICE_STATE_NET;
			msg_client.wpara =  (client->nd << 8)  + (1<<15);
			message_dispatch(&msg_client);
//			printf("%s,%d:eth0 server client disc ret = %d,\n",__FILE__,__LINE__,ret);
			pthread_exit(0);
		}
	}

	pthread_exit(0);
}
static void *thread_eth0_server(void * arg)
{
	int ret;
	int i;
	u8 fd;
	struct eth0_server_t *server = (struct eth0_server_t *)arg;
	printf("%s,%d:eth0 server start nd = %d!\n",__FILE__,__LINE__,server->nd);
	while(1){
		ret = net_server_listen(NULL,NET_LISTEN);
		if(ret >= 0){
			ret = net_server_connect(&fd);
			if(ret >= 0){
				for(i = 0; i < ETH0_MAX_SERVER_CLIENT; i++){
					if(server->client[i].act == 0){
						server->client[i].act = 1;
						server->client[i].nd = i;
						server->client[i].fd = fd;
						server->client[i].server_nd = server->nd;
						//创建客户端网络接收线程，实时线程，优先级80-90
						ret = thread_create_base(&server->client[i].id, thread_eth0_server_client, (void *)&server->client[i], THREAD_MODE_REALTIME, 89);
						if((ret < 0) ){
							server->client[i].act = 0;
							ret = net_server_disconnect(fd);
						}
						break;
					}
				}
				//没有可用连接，客户端连接数已经用完
			}else{
				if(ret == -ERR_BUSY){
					//没有可用连接，客户端连接数已经用完
				}
				printf("%s,%d:eth0 server disc \n",__FILE__,__LINE__);
			}
		}else{
			printf("%s,%d:eth0 server listen err ret = %d\n",__FILE__,__LINE__,ret);
		}
	}
	pthread_exit(0);
}
static int eth0_config(u8 type, char *cfg)
{
	int ret;

	if(type == ETHER_CONFIG_TYPE_IP){
		ret = net_ip_set(cfg);
	}else if(type == ETHER_CONFIG_TYPE_GATEWAY){
		ret = net_gateway_set(cfg);
	}else if(type == ETHER_CONFIG_TYPE_MASK){
		ret = net_mask_set(cfg);
	}else{
		return -ERR_INVAL;
	}

	return ret;
}

static int eth0_connect(u8 mode, char *ip, u16 port)
{
	int ret = -1;
	int i;
	if(mode == ETHER_MODE_TCP_CLIENT){
		for(i = 0; i < ETH0_MAX_CLIENT; i++){
			if(eth0_client[i].act == 0){
				ret = net_client_init(i,0xffff);
				if(ret < 0){
					goto err;
				}
				ret = net_client_connect(i,ip, port);
				if(ret < 0){
					goto err;
				}
				eth0_client[i].act = 1;
				eth0_client[i].nd =  i;

				//创建客户端网络监测线程，实时线程，优先级80-90
				ret = thread_create_base(&eth0_client[i].id, thread_eth0_client, (void *)&eth0_client[i], THREAD_MODE_REALTIME, 89);
				if((ret < 0) ){
					ret = ERR_OTHER;
					goto err;
				}
				return ret;
			}
		}
		//无可用连接
		ret = -ERR_BUSY;
		return ret;

	}else if(mode == ETHER_MODE_TCP_SERVER){

//		for(i = 0; i < ETH0_MAX_SERVER; i++){
//			if(eth0_server[i].act == 0){
//				eth0_server[i].act = 1;
//				eth0_server[i].nd = i;
//				//创建客户端网络监测线程，实时线程，优先级80-90
//				ret = thread_create_base(&eth0_server[i].id, thread_eth0_server, (void *)&eth0_server[i], THREAD_MODE_REALTIME, 89);
//				if((ret < 0) ){
//					goto err;
//				}
//				return ret;
//			}
//		}
		if(eth0_server.act == 0){
			for(i = 0; i < ETH0_MAX_SERVER_CLIENT; i++){
				eth0_server.client[i].act = 0;
			}
			ret = net_server_init(port, 0xffff);
			if(ret < 0){
				goto err;
			}
			eth0_server.act = 1;
			//创建服务器端网络监测线程，实时线程，优先级80-90
			ret = thread_create_base(&eth0_server.id, thread_eth0_server, (void *)&eth0_server, THREAD_MODE_REALTIME, 89);
			if((ret < 0) ){
				ret = ERR_OTHER;
				goto err;
			}
			return ret;
		}else{
			//无可用连接
			ret = -ERR_BUSY;
			return ret;
		}
	}else if(mode == ETHER_MODE_UDP_CLIENT){

	}else if(mode == ETHER_MODE_UDP_SERVER){

	}else{
		return -ERR_INVAL;
	}
err:
	return ret;
}

static int eth0_disconnect(u8 nd)
{
	int ret;
	//最高位为1表示服务器端
	if(nd >= (1<<7)){
		nd &= ~(1 << 7);
//		printf("%s,%d:eth0 server disc nd = %d\n",__FILE__,__LINE__,nd);
		ret = net_server_disconnect(eth0_server.client[nd].fd);
		eth0_server.client[nd].act = 0;
		//结束接收线程
		ret = pthread_cancel(eth0_server.client[nd].id);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		ret = pthread_join(eth0_server.client[nd].id,NULL);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}else{
		ret  = net_client_disconnect(nd);
		eth0_client[nd].act = 0;
		//结束接收线程
		ret = pthread_cancel(eth0_client[nd].id);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
		ret = pthread_join(eth0_client[nd].id,NULL);
		if(ret < 0){
			ret = -ERR_SYS;
			goto err;
		}
	}
err:
	return ret;
}

static int eth0_senddata(u8 nd, u8 *buf, u32 count)
{
	int ret;
	//最高位为1表示服务器端
	if(nd >= (1<<7)){
		nd &= ~(1 << 7);
		if(eth0_server.act ){
			ret = net_server_send(nd, buf, count);
		}else{
			ret = -ERR_DISCONNECT;
		}
	}else{
		if(eth0_client[nd].act ){
			ret  = net_client_send(nd,buf, count);
		}else{
			ret = -ERR_DISCONNECT;
		}
	}
	return ret;
}


ether_device_t ether_device[] = {
	{
		eth0_config,
		eth0_connect,
		eth0_disconnect,
		eth0_senddata,
	},
};

#endif
