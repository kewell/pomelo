
//C库头文件
#include <unistd.h>
#include <string.h>
#include <stdio.h>
//业务平台头文件
#include "framework/debug.h"
#include "framework/framework.h"
#include "framework/systime.h"
#include "framework/message.h"

#include "framework/device/key.h"
#include "framework/device/led.h"
#include "framework/device/relay.h"
#include "framework/device/powerd.h"
#include "framework/device/swin.h"
#include "framework/device/dstream.h"
#include "framework/device/ether.h"
//业务模块头文件
#include "device_test.h"

static u8 ascii2h(u8 * data);

int device_test_initmodel(struct BASE * this)
{
	int ret = 0;
	struct device_testModule *obj;

	comport_config_t dev_debug_cfg = {
			.baud	=	2400,
			.verify	= COMPORT_VERIFY_ODD,
			.ndata	= 8,
			.nstop	= 1,
			.timeout = 0,
	};
//	comport_config_t dev_zb_cfg = {
//			.baud	=	9600,
//			.verify	= COMPORT_VERIFY_ODD,
//			.ndata	= 8,
//			.nstop	= 1,
//			.timeout = 0,
//	};
//	comport_config_t dev_485_1_cfg = {
//			.baud	=	9600,
//			.verify	= COMPORT_VERIFY_NO,
//			.ndata	= 8,
//			.nstop	= 1,
//			.timeout = 0,
//	};
//	comport_config_t dev_485_2_cfg = {
//			.baud	=	1200,
//			.verify	= COMPORT_VERIFY_NO,
//			.ndata	= 8,
//			.nstop	= 1,
//			.timeout = 0,
//	};
//	comport_config_t dev_usbd_cfg = {
//			.baud	=	9600,
//			.verify	= COMPORT_VERIFY_NO,
//			.ndata	= 8,
//			.nstop	= 1,
//			.timeout = 0,
//	};

	obj = (struct device_testModule *)this;

//注册模块
	ret = module_register(this);
	if (ret) {
		goto error;
	}

//注册4秒周期定时
	ret = systime_register_period(this, 1, 4);
//	if (ret < 0) {
//		goto error;
//	}
//	else {
//		obj->tmhdr_4s = ret;
//	}
	ret = message_subscribe(this,MSG_KEY);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_SKEY);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_POWER);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_DIN);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_PULSE);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_DEVICE_DATA_DEBUG);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_DEVICE_DATA_ZB);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_DEVICE_DATA_485_1);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_DEVICE_DATA_485_2);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_DEVICE_DATA_USBD);
	if (ret) {
		goto error;
	}
	ret = message_subscribe(this,MSG_DEVICE_DATA_NET);
	if (ret) {
		goto error;
	}
	//初始化按键设备
	ret = key_init();
	if (ret < 0 && ret != -6) {
		goto error;
	}

//	//初始化状态灯设备
//	ret = led_init();
//	if (ret < 0 && ret != -6) {
//		goto error;
//	}

//	//初始化继电器设备
//	ret = relay_init();
//	if (ret) {
//		goto error;
//	}
//	//初始化掉电检测设备
//	ret = powerd_init();
//	if (ret) {
//		goto error;
//	}
//	//初始化遥信脉冲设备
//	ret = swin_init();
//	if (ret) {
//		goto error;
//	}
	//打开数据流设备debug设备

	ret = dstream_device[DSTREAM_DEBUG].open(&dev_debug_cfg);
	if (ret < 0 && ret != -6) {
		goto error;
	}
//	ret = dstream_device[DSTREAM_ZB].open(&dev_zb_cfg);
//	if (ret < 0 && ret != -6) {
//		goto error;
//	}
//	ret = dstream_device[DSTREAM_485_1].open(&dev_485_1_cfg);
//	if (ret < 0 && ret != -6) {
//		goto error;
//	}
//	ret = dstream_device[DSTREAM_485_2].open(&dev_485_2_cfg);
//	if (ret < 0 && ret != -6) {
//		goto error;
//	}
//	ret = dstream_device[DSTREAM_USBD].open(&dev_usbd_cfg);
//	if (ret < 0 && ret != -6) {
//		goto error;
//	}

	ret = 0;
error:
//printf("%s,%d:ret = %d!\n",__FILE__,__LINE__,ret);
	return ret;
}

int device_test_initdata(struct BASE * this)
{
	//...;
	return 0;
}

int device_test_msghandle(struct BASE * this, message_t *msg)
{
	u8 i;
	int ret;
	struct device_testModule *obj;
	obj = (struct device_testModule *)this;

//	swin_time_t *result_din;
	u8 * din_data;
	st_ymdhmsw_t  din_time;

//	u8 tmp[256] = {" ",};
	u8 send_buf[256] ;
	u8 recv_buf[256] = {0};
	u8 buf_size;
	u8 data_size;
	switch (msg->type) {
	case MSG_KEY:

		if(msg->wpara == KEY_UP){
//			led_on(LED_WARN, 0, 0, 0);
			buf_size = ascii2h(send_buf);
//			fputs(send_buf, stdout);
			ret = dstream_device[DSTREAM_DEBUG].send(send_buf,buf_size);
			printf("%s,%d:send ok ret = %d!\n",__FILE__,__LINE__,ret);
//			relay_on(0,0,0,0);
//			led_on(LED_TXDJL, 0, 0, 0);
//			sleep(1);
//			led_off(LED_WARN);
//			led_off(LED_TXDJL);
		}
//		if(msg->wpara == KEY_DOWN)
//			led_on(LED_TXDJL, 0, 1000, 2000);
		if(msg->wpara == KEY_LEFT){
//			buf_size = ascii2h(send_buf);
//			ret = dstream_device[DSTREAM_USBD].send(send_buf,buf_size);
//			printf("%s,%d:usbd ret = %d!\n",__FILE__,__LINE__,ret);
			ret = ether_device[ETHER_0].disconnect(0);
			if(ret < 0){
				printf("%s,%d:eth0 disc err!\n",__FILE__,__LINE__);
			}
			printf("%s,%d:eth0 server disc !\n",__FILE__,__LINE__);
		}
		if(msg->wpara == KEY_ENTER){
//			buf_size = ascii2h(send_buf);
////			fputs(send_buf, stdout);
//			ret = dstream_device[DSTREAM_485_1].send(send_buf,buf_size);
//			printf("%s,%d:485_1 ret = %d!\n",__FILE__,__LINE__,ret);
			ret = ether_device[ETHER_0].connect(ETHER_MODE_TCP_CLIENT,"192.168.2.100",8080);
			if(ret < 0){
				printf("%s,%d:eth0 config err!\n",__FILE__,__LINE__);
			}

		}
		if(msg->wpara == KEY_CANCEL){
//			buf_size = ascii2h(send_buf);
////			fputs(send_buf, stdout);
//			ret = dstream_device[DSTREAM_485_2].send(send_buf,buf_size);
//			printf("%s,%d:485_2 ret = %d!\n",__FILE__,__LINE__,ret);

//			//网络客户端连接测试
//			ret = ether_device[ETHER_0].connect(ETHER_MODE_TCP_CLIENT,"192.168.2.103",3333);
//			if(ret < 0){
//				printf("%s,%d:eth0 config err!\n",__FILE__,__LINE__);
//			}
			//网络服务器端连接测试
			ret = ether_device[ETHER_0].connect(ETHER_MODE_TCP_SERVER,NULL,3333);
			if(ret < 0){
				printf("%s,%d:eth0 config err!\n",__FILE__,__LINE__);
			}
		}
//
//		if(msg->wpara == KEY_UP + 100){
//			led_off(LED_WARN);
//			relay_off(0);
//		}
		if(msg->wpara == KEY_DOWN ){
			PRINTF("check backup bat voltage:\n");
			ret = mcu_batvolt_check();
//			buf_size = ascii2h(send_buf);
//			ret = dstream_device[DSTREAM_DEBUG].send(send_buf,buf_size);
//			printf("%s,%d:ret = %d!\n",__FILE__,__LINE__,ret);
//			ret = ether_device[ETHER_0].config(ETHER_CONFIG_TYPE_GATEWAY,"192.168.2.8");
//			ret = ether_device[ETHER_0].config(ETHER_CONFIG_TYPE_MASK,"255.255.0.0");
//			ret = ether_device[ETHER_0].connect(ETHER_MODE_TCP_CLIENT,"192.168.2.100",3333);
//			if(ret < 0){
//				printf("%s,%d:eth0 config err ret %d!\n",__FILE__,__LINE__,ret);
//			}
//			ret = ether_device[ETHER_0].connect(ETHER_MODE_TCP_CLIENT,"192.168.2.100",8080);
//			if(ret < 0){
//				printf("%s,%d:eth0 config err!\n",__FILE__,__LINE__);
//			}
//			ret = ether_device[ETHER_0].connect(ETHER_MODE_TCP_CLIENT,"192.168.2.103",3333);
//			if(ret < 0){
//				printf("%s,%d:eth0 config err!\n",__FILE__,__LINE__);
//			}
//			printf("%s,%d:eth0 connect ok!\n",__FILE__,__LINE__);

		}
//		if(msg->wpara == KEY_LEFT + 100)
//			led_off(LED_RXDJL);
//		if(msg->wpara == KEY_RIGHT + 100)
//			led_off(LED_RUN);

		break;
	case MSG_SKEY:
		printf("%s,%d:get key %d!\n",__FILE__,__LINE__,msg->wpara);
		for(i=0; i < 4; i++){
			ret = led_off(i);
			if(ret < 0)
				printf("device_test:led off error\n");
		}
		break;
	case MSG_POWER:
		printf("%s,%d:power shift %d!\n",__FILE__,__LINE__,msg->wpara);
		break;
	case MSG_DIN:
		//result_din = (swin_time_t *)msg->lpara;
		din_data =(u8 *)msg->lpara;
		printf("%s,%d:DIN :Channel %d,polar %d  ",__FUNCTION__,__LINE__, *din_data,*(din_data + 1));

		din_data += 2;
		din_time.year = *(din_data++);
		din_time.mon = *(din_data++);
		din_time.day = *(din_data++);
		din_time.hour = *(din_data++);
		din_time.min = *(din_data++);
		din_time.sec = *(din_data);

		printf("%d/%d/%d,%d:%d:%d \n",
				din_time.year + 2000,
				din_time.mon,
				din_time.day,
				din_time.hour,
				din_time.min,
				din_time.sec);
		break;
	case MSG_PULSE:
		printf("%s,%d:PUSLE %d!\n",__FILE__,__LINE__,msg->lpara);
		break;
	case MSG_DEVICE_DATA_DEBUG:
		memcpy(recv_buf,(u8 *)msg->lpara, msg->wpara);
		recv_buf[ msg->wpara] = '\0';
		printf("%s,%d:aha!DSSTREAM debug count = %d!\n",__FILE__,__LINE__,msg->wpara);
		for(i = 0; i<msg->wpara;i++){
			printf("%x ",recv_buf[i]);
		}
		printf("\n");
		//printf("%s\n",recv_buf);
		break;
	case MSG_DEVICE_DATA_ZB:
		memcpy(recv_buf,(u8 *)msg->lpara, msg->wpara);
		recv_buf[ msg->wpara] = '\0';
		printf("%s,%d:aha!DSSTREAM zb count = %d!\n",__FILE__,__LINE__,msg->wpara);
		for(i = 0; i<msg->wpara;i++){
			printf("%x ",recv_buf[i]);
		}
		printf("\n");
		//printf("%s\n",recv_buf);
		break;
	case MSG_DEVICE_DATA_485_1:
		memcpy(recv_buf,(u8 *)msg->lpara, msg->wpara);
		recv_buf[ msg->wpara] = '\0';
		printf("%s,%d:aha!DSSTREAM 485_1 count = %d!\n",__FILE__,__LINE__,msg->wpara);
		for(i = 0; i<msg->wpara;i++){
			printf("%x ",recv_buf[i]);
		}
		printf("\n");
		printf("%s\n",recv_buf);
		break;
	case MSG_DEVICE_DATA_485_2:
		memcpy(recv_buf,(u8 *)msg->lpara, msg->wpara);
		recv_buf[ msg->wpara] = '\0';
		printf("%s,%d:aha!DSSTREAM 485_2 count = %d!\n",__FILE__,__LINE__,msg->wpara);
		for(i = 0; i<msg->wpara;i++){
			printf("%x ",recv_buf[i]);
		}
		printf("\n");
		printf("%s\n",recv_buf);
		break;
	case MSG_DEVICE_DATA_USBD:
		memcpy(recv_buf,(u8 *)msg->lpara, msg->wpara);
		recv_buf[ msg->wpara] = '\0';
		printf("%s,%d:aha!DSSTREAM usbd count = %d!\n",__FILE__,__LINE__,msg->wpara);
		for(i = 0; i<msg->wpara;i++){
			printf("%x ",recv_buf[i]);
		}
		printf("\n");
		printf("%s\n",recv_buf);
		break;
	case MSG_DEVICE_DATA_NET:
		data_size = msg->wpara&0xff;
		memcpy(recv_buf,(u8 *)msg->lpara, data_size);
		recv_buf[ data_size] = '\0';
		printf("%s,%d:net recv %dByte,nd =%d\n",__FILE__,__LINE__,data_size,msg->wpara >> 8);
//		for(i = 0; i<data_size;i++){
//			printf("%x ",recv_buf[i]);
//		}
//		printf("\n");
		printf("%s\n",recv_buf);
		ret = ether_device[ETHER_0].senddata(msg->wpara >> 8,recv_buf,data_size);
		if(ret < 0){
			printf("%s,%d:eth0 config err!\n",__FILE__,__LINE__);
		}
		break;

	default:
		break;
	}
	return 0;
}

struct BASEFT device_test_ft = {device_test_initmodel, device_test_initdata, device_test_msghandle};

static u8 ascii2h(u8 * data)
{
	int j,i=0;
	u8 size;
again:
	data[0] = ' ';
	printf("input h val:xx xx xx\n");
	fgets(&data[1],255,stdin);
	size = strlen(data) - 1;
	for(j = 0; j < size - 1; j ++){
		if(data[j] == ' '){
			j++;
			if((data[j] >= '0') && (data[j] <= '9') ){
				data[j] -=  '0';
			}else if((data[j] >= 'a') && (data[j] <= 'f')){
				data[j] = data[j] - 'a' + 10;
			}else if((data[j] >= 'A') && (data[j] <= 'F')){
				data[j] = data[j] - 'A' + 10;
			}else{
				goto again;
			}
//			printf("%x ",data[j]);
			j++;
			if((data[j] >= '0') && (data[j] <= '9') ){
				data[j] -=  '0';
			}else if((data[j] >= 'a') && (data[j] <= 'f')){
				data[j] = data[j] - 'a' + 10;
			}else if((data[j] >= 'A') && (data[j] <= 'F')){
				data[j] = data[j] - 'A' + 10;
			}else{
				goto again;
			}
//			printf("%x\n",data[j]);
			data[i] = (data[j - 1] << 4 )  +  data[j] ;
			i++;
//			printf("%x ",data[(j-2) / 3]);
		}

	}
	size = i;
	return size;
}
