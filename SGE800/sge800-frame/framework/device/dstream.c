/******************************************************************************
	项目名称	：  SGE800计量智能终端业务平台
	文件		：  dstream.c
	描述		：  本文件实现了数据流设备模块的接口
	版本		：  0.1
	作者		：  路冉冉
	创建日期	：  2010.12
******************************************************************************/
//库配置头文件
#include "framework/config.h"
	
//模块启用开关
#ifdef CFG_DSTREAM_DEVICE

//C库头文件
#include <fcntl.h> 		//open 标志
#include <unistd.h>		//write
#include <string.h>		//memcpy
#include <stdio.h>
//基础平台头文件
#include "sge_core/error.h"
#include "sge_core/thread.h"
#include "sge_core/comport.h"

//业务平台头文件
#include "framework/base.h"
#include "framework/message.h"
#include "framework/device/dstream.h"
#include "framework/debug.h"

/*************************************************
  结构类型定义
*************************************************/
#define DS_BUF_SIZE 256
#define DS_BUF_NUM 8

typedef struct {
	u8 com;				//串口id，本地见comport.h,扩展为11,12,13
	u8 opend;			//是否启用
	u8 buf[DS_BUF_NUM][DS_BUF_SIZE];	//消息缓冲区
	u8 cur;	//当前消息写位置
	u8 msg_type;
} dstream_dev_drv;

typedef struct {
	u8 com;								//串口id，本地见comport.h,扩展为11,12,13
	u8 opend;							//是否启用
	u8 buf[DS_BUF_NUM][DS_BUF_SIZE];	//消息缓冲区
	u8 cur;								//当前消息写位置
//	u16 msg_type;
}mcu_device_com_t;

/*************************************************
  静态全局变量及宏定义
*************************************************/
extern int errno;

//mcu扩展功能设备
#define MCU_COM_485_ID		11
#define MCU_COM_IRDA_ID		12
#define MCU_COM_DEBUG_ID	13

static struct {
	u8	count;
	int fd;
	pthread_t id;
	int ret;			//写到mcu后等待的返回
	mcu_device_com_t debug;
	mcu_device_com_t irda;
	mcu_device_com_t rs485;
	pthread_cond_t ready;
	pthread_mutex_t lock;
}mcu = {
		.count		= 	0,
		.ret		=	0,
		.ready		=	PTHREAD_COND_INITIALIZER,
		.lock		=	PTHREAD_MUTEX_INITIALIZER,
		.debug		=	{
			.com 	=	MCU_COM_DEBUG_ID,
			.opend	=	0,
			.cur	=	0,

		},
		.irda		=	{
			.com 	=	MCU_COM_IRDA_ID,
			.opend	=	0,
			.cur	=	0,

		},
		.rs485		=	{
			.com 	=	MCU_COM_485_ID,
			.opend	=	0,
			.cur	=	0,

		},
	};
//mcu扩展宏定义
#define MCU_MAX_FRAME_SIZE 239

#define MCU_485_ID		1
#define MCU_IRDA_ID		2
#define MCU_DEBUG_ID	3
#define MCU_STATE_ID	4
#define MCU_38K_ID		5
#define MCU_BATVOLT_ID	6
#define MCU_BEEP_ID		7

//功能码
#define MCU_READ_COM	0x11	//读数据10001
#define MCU_READ_NEXT	0x12	//读后续数据10010
#define MCU_READ_SET	0x13	//读串口参数10011
#define MCU_WRITE_COM	0x14	//写串口数据10100
#define MCU_EN_FUN		0x15	//使能相应功能10101
#define MCU_SET_COM		0x16	//设置串口参数10110
#define MCU_DIS_FUN		0x17	//关闭相应功能10111

#define MCU_DIRECT		(1<<7)	//传输方向位
#define MCU_DIRECT_MO	(0<<7)	//传输方向位,主站发出
#define MCU_DIRECT_SO	(1<<7)	//传输方向位，从站发出
#define MCU_ACT_FLAG	(1<<6)	//从站应答标志，0：正确应答，1：异常
#define MCU_ACT_OK		(0<<6)	//从站应答标志，0：正确应答
#define MCU_ACT_ERR		(1<<6)	//从站应答标志，1：异常
#define MCU_NEXT_FRAME	(1<<5)	//后续帧标志，0：无，1：有
#define MCU_NONNEXT_FRAME	(0<<5)	//后续帧标志，0：无
#define MCU_YESNEXT_FRAME	(1<<5)	//后续帧标志，1：有


//数据流设备调试口
#ifdef DSTREAM_DEBUG_COM
static dstream_dev_drv ds_debug = {
		.opend		=	0,
		.cur		=	0,
		.com		=	DSTREAM_DEBUG_COM,
		.msg_type	=	MSG_DEVICE_DATA_DEBUG,
};
#endif
#ifdef DSTREAM_IRDA_COM
//数据流设备红外
static dstream_dev_drv ds_irda = {
		.opend		=	0,
		.cur		=	0,
		.com		= 	DSTREAM_IRDA_COM,
		.msg_type	=	MSG_DEVICE_DATA_IRDA,
};
#endif
//载波流设备
#ifdef DSTREAM_ZB_COM
static dstream_dev_drv ds_zb = {
		.opend		=	0,
		.cur		=	0,
		.com		=	DSTREAM_ZB_COM,
		.msg_type	=	MSG_DEVICE_DATA_ZB,
};
#endif
//usb从口流设备
#ifdef DSTREAM_USBD_COM
static dstream_dev_drv ds_usbd = {
		.opend		=	0,
		.cur		=	0,
		.com		= 	DSTREAM_USBD_COM,
		.msg_type	=	MSG_DEVICE_DATA_USBD,
};
#endif
//底板流设备
#ifdef DSTREAM_JC_COM
static dstream_dev_drv ds_jc = {
		.opend		=	0,
		.cur		=	0,
		.com 		=	DSTREAM_JC_COM,
		.msg_type	=	MSG_DEVICE_DATA_JC,
};
#endif
#ifdef DSTREAM_485_1_COM
static dstream_dev_drv ds_485_1 = {
		.opend		=	0,
		.cur		=	0,
		.com 		=	DSTREAM_485_1_COM,
		.msg_type	=	MSG_DEVICE_DATA_485_1,
};
#endif
#ifdef DSTREAM_485_2_COM
static dstream_dev_drv ds_485_2 = {
		.opend		=	0,
		.cur		=	0,
		.com 		=	DSTREAM_485_2_COM,
		.msg_type	=	MSG_DEVICE_DATA_485_2,
};
#endif
#ifdef DSTREAM_485_3_COM
static dstream_dev_drv ds_485_3 = {
		.opend		=	0,
		.cur		=	0,
		.com		=	DSTREAM_485_3_COM,
		.msg_type	=	MSG_DEVICE_DATA_485_3,
};
#endif
/*************************************************
  API
*************************************************/

/*************************MCU扩展驱动程序***************************************/
static int mcu_read (u8 *data);
/******************************************************************************
*	函数:	thread_mcu_read
*	功能:	单片机扩展读单片机数据线程
*	参数:
*	返回:
*	说明:
 ******************************************************************************/
static void *thread_mcu_read(void * arg)
{
	int ret,i;
	u8 buf_t[256];
	u8	id ;			//数据标识
	u8	ctrl;			//控制符
	u8	fun ;			//功能码
	u8	data_size;
	message_t msg_mcu;

	while(1){
		ret = mcu_read(buf_t);
		if(ret > 0){
			id = buf_t[2];
			ctrl = buf_t[0];
			fun = buf_t[0] & 0x1f;
			PRINTF("total read from mcu:\n");
			for(i = 0; i < ret; i ++){
				PRINTF("%x ", buf_t[i]);
			}
			PRINTF("%s:%d\n", __FILE__, __LINE__);
			switch(id){
			case MCU_485_ID:
				break;
			case MCU_IRDA_ID:
				break;
			case MCU_DEBUG_ID :
				if(mcu.debug.opend == 1){
					if((fun == MCU_SET_COM) || (fun == MCU_READ_SET) || (fun == MCU_WRITE_COM) ){
						if((ctrl & MCU_ACT_FLAG) == MCU_ACT_OK){
							mcu.ret = 0;
						}else{
							mcu.ret = -1;
						}
						pthread_cond_signal(&(mcu.ready));
					}else if((fun == MCU_READ_COM) || (fun == MCU_READ_NEXT)){
						data_size = ret - 3;
						memcpy(&(mcu.debug.buf[mcu.debug.cur][0]), &buf_t[3], data_size); //发送消息
						msg_mcu.type = MSG_DEVICE_DATA_DEBUG;
						msg_mcu.wpara = data_size;
						msg_mcu.lpara = (u32)&mcu.debug.buf[mcu.debug.cur][0];
						message_dispatch(&msg_mcu);
						if(++mcu.debug.cur >= DS_BUF_NUM){
							mcu.debug.cur = 0;
						}
					}
				}

				break;
			case MCU_38K_ID:
				break;
			case MCU_BATVOLT_ID:
				break;
			case MCU_BEEP_ID:
				break;
			default:
				break;
			}
		}
	}

	pthread_exit(0);
}
/******************************************************************************
*	函数:	mcu_open
*	功能:	单片机扩展设备打开
*	参数:
*	返回:	0				-	成功
			-ERR_NOFILE		-	没有此路径
			-ERR_BUSY		-	设备已经打开
*	说明:	无
 ******************************************************************************/
static int mcu_open (u8 id)
{
	int ret = -1;

	if(id == MCU_COM_485_ID){
		mcu.rs485.opend = 1;
	}else if(id == MCU_COM_IRDA_ID){
		mcu.irda.opend = 1;
	}else if(id == MCU_COM_DEBUG_ID){
		mcu.debug.opend = 1;
	}else{
		return -ERR_INVAL;
	}
	if(mcu.count == 1){
		ret = -ERR_BUSY;		//已经打开
		goto err;
	}
	//打开mcu驱动
	mcu.fd = open("/dev/mcu", O_RDWR);
   	if (mcu.fd < 0){
   		ret = -ERR_NOFILE;		//没有此路径
		goto err;
	}

	//创建掉电监测线程，实时线程，优先级80-90
	ret = thread_create_base(&mcu.id, thread_mcu_read, NULL, THREAD_MODE_REALTIME, 89);
	if((ret < 0) ){
		ret = -ERR_OTHER;
		goto err;
	}

	mcu.count = 1;
err:
	return ret;
}
/******************************************************************************
*	函数:	mcu_write
*	功能:	单片机扩展设备初始化
*	参数:	data			-	要写的数据指针
			size			-	要写的数据大小，此处限制最大242字节
*	返回:	0				-	成功
			<0				-	内核错误消息
*	说明:	无
 ******************************************************************************/
static int mcu_write (u8 *data, u16 size)
{
	int ret = -1;

	ret = write(mcu.fd, data, size);
	if (ret < 0){
		ret = errno;
	}else{
		pthread_mutex_lock(&(mcu.lock));
		pthread_cond_wait(&(mcu.ready), &(mcu.lock));

	//	ret = pthread_cond_timedwait(&(mcu.ready), &(mcu.lock), 0);
	//	if((ret < 0)){
	//		goto err;
	//	}
		pthread_mutex_unlock(&(mcu.lock));
		if(mcu.ret == 0){
			ret = size;
		}else{
			ret = mcu.ret;
		}
	}

	return ret;
}
/******************************************************************************
*	函数:	mcu_read
*	功能:	单片机扩展设备读函授
*	参数:	data			-	读数据缓冲区
*	返回:	0				-	成功
			<0				-	内核错误消息
*	说明:	无
 ******************************************************************************/
static int mcu_read (u8 *data)
{
	int ret = -1;
	ret = read(mcu.fd, data, 0);
	if (ret < 0){
		ret = errno;
	}
	return ret;
}

/*************************MCU设备，串口功能***************************************/


/******************************************************************************
*	函数:	mcu_com_frame
*	功能:	单片机扩展调试串口设置组帧并发送
*	参数:	cfg				-	串口参数
*	返回:	0				-	成功
			-ERR_INVAL		-	奇偶校验位错误
*	说明:	无
 ******************************************************************************/
static int mcu_com_frame(u8 com_id, comport_config_t *cfg)
{
	int ret;
	u8 i = 0;
	u8 write_buf[6];
	u8 frame_ctrl, frame_size, frame_id;
	com_id -= 10;
	frame_ctrl	=	MCU_SET_COM + MCU_NONNEXT_FRAME;
	frame_size	=	0x04;
	frame_id 	=	com_id;
	//构造设置串口参数帧
	write_buf[i++] = frame_ctrl;
	write_buf[i++] = frame_size;
	write_buf[i++] = frame_id;
	write_buf[i++] = cfg->baud & 0xff;			//波特率低字节
	write_buf[i++] = (cfg->baud >> 8) & 0xff;	//波特率高字节
	switch(cfg->verify){
		case COMPORT_VERIFY_NO:
			write_buf[i] = 0;
			break;
		case COMPORT_VERIFY_ODD:
			write_buf[i] = 2;
			break;
		case COMPORT_VERIFY_EVEN:
			write_buf[i] = 1;
			break;
		default:
			return -ERR_INVAL;
	}
	ret = mcu_write(write_buf,6);
	if((ret < 0)){
		goto err;
	}
	ret = 0;
err:
	return ret;

}
/******************************************************************************
*	函数:	mcu_com_frame
*	功能:	单片机扩展调试串口组帧并发送
*	参数:	cfg				-	串口参数
*	返回:	0				-	成功
			-ERR_INVAL		-	奇偶校验位错误
*	说明:	无
 ******************************************************************************/
static int mcu_com_send_frame(u8 com_id, u8 *pdata, u16 data_size, u8 next)
{
	int ret = -1;
	u8 i = 0;
	u8 write_buf[256];
	u8 frame_ctrl, frame_size, frame_id;
	com_id -= 10;
	frame_ctrl	=	MCU_WRITE_COM + (next?MCU_YESNEXT_FRAME:MCU_NONNEXT_FRAME);
	frame_size	=	data_size + 1;  //数据+数据标识 最大240字节
	frame_id 	=	com_id;
	//构造设置串口参数帧
	write_buf[i++] = frame_ctrl;
//	printf("%s,%d: %x,%x\n",__FILE__,__LINE__,frame_ctrl,frame_size);
	write_buf[i++] = frame_size;
	write_buf[i++] = frame_id;
	memcpy(&write_buf[i], pdata,data_size);

	ret = mcu_write(write_buf,data_size + 3);
	if((ret < 0)){
		goto err;
	}
//	printf("%s,%d:ret = %d!\n",__FILE__,__LINE__,ret);
	ret = 0;
err:
	return ret;

}

static int mcu_com_open (u8 id)
{
	int ret = -1;

	ret = mcu_open(id);
	if((ret < 0) && (ret != -ERR_BUSY)){
		goto err;
	}

err:
	return ret;
}

static int mcu_com_cfg (u8 id, comport_config_t *cfg)
{
	int ret = -1;
	if(mcu.count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = mcu_com_frame(id, cfg);
	if((ret < 0)){
		goto err;
	}

err:
	return ret;
}

static int mcu_com_send (u8 id, u8 *buf, u32 count)
{
	int ret = -1;
	u16 j;
	u8 times;
	u8 next_frame = 1;
	u16 write_count;

	if(mcu.count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}

	times = count / MCU_MAX_FRAME_SIZE;
	for(j = 0; j <= times ; j ++){

		if(j == times){
			next_frame = 0;
			write_count = count - j * MCU_MAX_FRAME_SIZE;
		}else{
			write_count = MCU_MAX_FRAME_SIZE;
		}
//		printf("%s,%d:j = %d!\n",__FILE__,__LINE__,j);
		ret = mcu_com_send_frame(id, &buf[j * MCU_MAX_FRAME_SIZE],	write_count, next_frame);
		if((ret < 0)){
			goto err;
		}
	}
	ret = count;
err:
	return ret;
}

/*************************MCU设备，备用电池电压***************************************/

/******************************************************************************
*	函数:	mcu_batv_frame
*	功能:	单片机扩展查询备用电池电压组帧并发送
*	参数:
*	返回:	0				-	成功
*	说明:	无
 ******************************************************************************/
static int mcu_batvolt_frame(void)
{
	int ret;
	u8 i = 0;
	u8 write_buf[3];
	u8 frame_ctrl, frame_size, frame_id;
#define MCU_BATVOLT_CTRL 0x11
#define MCU_BATVOLT_SIZE 0x01
#define MCU_BATVOLT_DI 0x06
	frame_ctrl	=	MCU_BATVOLT_CTRL;
	frame_size	=	MCU_BATVOLT_SIZE;
	frame_id 	=	MCU_BATVOLT_ID;
	//构造设置串口参数帧
	write_buf[i++] = frame_ctrl;
	write_buf[i++] = frame_size;
	write_buf[i++] = frame_id;

	ret = mcu_write(write_buf,3);
	if((ret < 0)){
		goto err;
	}
	ret = 0;
err:
	return ret;

}

int mcu_batvolt_open (void)
{
	int ret = -1;

	ret = mcu_open(MCU_COM_DEBUG_ID);
	if((ret < 0) && (ret != -ERR_BUSY)){
		goto err;
	}

err:
	return ret;
}

int mcu_batvolt_check (void)
{
	int ret = -1;
	if(mcu.count == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = mcu_batvolt_frame();
	if((ret < 0)){
		goto err;
	}

err:
	return ret;
}




static void *thread_com_read(void * arg)
{
	int ret;
	u8 buf_com[256];
	message_t msg_com;
	dstream_dev_drv *ds_dev = (dstream_dev_drv *)arg;
	printf("%s,%d:aha!thread com %d creat \n",__FILE__,__LINE__,ds_dev->com);
	while(1){
		ret = comport_recv(ds_dev->com,buf_com,256);
		if(ret > 0){
			memcpy(&(ds_dev->buf[ds_dev->cur][0]), buf_com, ret); //发送消息
			msg_com.type = ds_dev->msg_type;
			msg_com.wpara = ret;
			msg_com.lpara = (u32)&ds_dev->buf[ds_dev->cur][0];
			message_dispatch(&msg_com);
			if(++ds_dev->cur >= DS_BUF_NUM){
				ds_dev->cur = 0;
			}
		}
	}
	pthread_exit(0);
}

//串口操作基础函数
static int ds_serial_open(dstream_dev_drv *ds_dev, comport_config_t *cfg, u8 mode)
{
	int ret = -1;
	pthread_t com_id;
	if(ds_dev->com > 10){
		ret = mcu_com_open(ds_dev->com);
		if((ret < 0) && (ret != -ERR_BUSY)){
			goto err;
		}
		ret = mcu_com_cfg(ds_dev->com, cfg);
		if(ret < 0){
			goto err;
		}
	}else{
		ret = comport_init(ds_dev->com, mode);
		if(ret < 0){
			goto err;
		}

		//创建串口读取线程，实时线程，优先级80-90
		ret = thread_create_base(&com_id, thread_com_read, (void *)ds_dev, THREAD_MODE_REALTIME, 88);
		if((ret < 0) ){
			goto err;
		}

		ret = comport_setconfig(ds_dev->com, cfg);
		if(ret < 0){
			goto err;
		}
	}
err:
	return ret;
}

static int ds_serial_cfg(u8 com_port, comport_config_t *cfg)
{
	int ret = -1;
	if(com_port > 10){
		ret = mcu_com_cfg(com_port, cfg);
		if(ret < 0){
			goto err;
		}
	}else {
		ret = comport_setconfig(com_port, cfg);
		if(ret < 0){
			goto err;
		}
	}
err:
	return ret;
}
static int ds_serial_send(u8 com_port, u8 * data, u32 size)
{
	int ret = -1;
	if(com_port > 10){
		ret = mcu_com_send(com_port, data, size);
		if(ret < 0){
			goto err;
		}
	}else{
		ret = comport_send(com_port, data, size);
		if(ret < 0){
			goto err;
		}
	}
err:
	return ret;
}



/*************************数据流设备-红外***************************************/
static int ds_irda_open (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_irda.opend == 1){
		ret = -ERR_BUSY;
		goto err;
	}
	ret = ds_serial_open(&ds_irda, cfg,COMPORT_MODE_NORMAL);
	if(ret < 0){
		goto err;
	}
	ds_irda.opend = 1;
err:
	return ret;
}
static int ds_irda_cfg (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_irda.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_cfg(ds_irda.com,cfg);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

static int ds_irda_send (u8 *buf, u32 count)
{
	int ret = -1;
	if(ds_irda.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_send(ds_irda.com, buf, count);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

/*************************数据流设备-载波***************************************/
static int ds_zb_open (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_zb.opend == 1){
		ret = -ERR_BUSY;
		goto err;
	}
	ret = ds_serial_open(&ds_zb,cfg,COMPORT_MODE_NORMAL);
	if(ret < 0){
		goto err;
	}
	ds_zb.opend = 1;
err:
	return ret;
}
static int ds_zb_cfg (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_zb.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_cfg(ds_zb.com,cfg);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

static int ds_zb_send (u8 *buf, u32 count)
{
	int ret = -1;
	if(ds_zb.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_send(ds_zb.com, buf, count);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

/*************************数据流设备-调试***************************************/
static int ds_debug_open (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_debug.opend == 1){
		ret = -ERR_BUSY;
		goto err;
	}
	ret = ds_serial_open(&ds_debug,cfg,COMPORT_MODE_NORMAL);
	if(ret < 0){
		goto err;
	}
	ds_debug.opend = 1;
err:
	return ret;
}
static int ds_debug_cfg (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_debug.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_cfg(ds_debug.com,cfg);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

static int ds_debug_send (u8 *buf, u32 count)
{
	int ret = -1;
	if(ds_debug.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_send(ds_debug.com, buf, count);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

/*************************数据流设备-USBD***************************************/
static int ds_usbd_open (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_usbd.opend == 1){
		ret = -ERR_BUSY;
		goto err;
	}
	ret = ds_serial_open(&ds_usbd,cfg,COMPORT_MODE_NORMAL);
	if(ret < 0){
		goto err;
	}
	ds_usbd.opend = 1;
err:
	return ret;
}
static int ds_usbd_cfg (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_usbd.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_cfg(ds_usbd.com,cfg);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

static int ds_usbd_send (u8 *buf, u32 count)
{
	int ret = -1;
	if(ds_usbd.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_send(ds_usbd.com, buf, count);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

/*************************数据流设备-底板***************************************/
static int ds_jc_open (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_jc.opend == 1){
		ret = -ERR_BUSY;
		goto err;
	}
	ret = ds_serial_open(&ds_jc,cfg,COMPORT_MODE_NORMAL);
	if(ret < 0){
		goto err;
	}
	ds_jc.opend = 1;
err:
	return ret;
}
static int ds_jc_cfg (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_jc.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_cfg(ds_jc.com,cfg);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

static int ds_jc_send (u8 *buf, u32 count)
{
	int ret = -1;
	if(ds_jc.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_send(ds_jc.com, buf, count);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

/*************************数据流设备-485a***************************************/
static int ds_485_1_open (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_485_1.opend == 1){
		ret = -ERR_BUSY;
		goto err;
	}
	ret = ds_serial_open(&ds_485_1,cfg,COMPORT_MODE_485);
	if(ret < 0){
		goto err;
	}
	ds_485_1.opend = 1;
err:
	return ret;
}
static int ds_485_1_cfg (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_485_1.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_cfg(ds_485_1.com,cfg);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

static int ds_485_1_send (u8 *buf, u32 count)
{
	int ret = -1;
	if(ds_485_1.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_send(ds_485_1.com, buf, count);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}



/*************************数据流设备-485b***************************************//*************************数据流设备-调试***************************************/
static int ds_485_2_open (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_485_2.opend == 1){
		ret = -ERR_BUSY;
		goto err;
	}
	ret = ds_serial_open(&ds_485_2,cfg,COMPORT_MODE_485);
	if(ret < 0){
		goto err;
	}
	ds_485_2.opend = 1;
err:
	return ret;
}
static int ds_485_2_cfg (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_485_2.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_cfg(ds_485_2.com,cfg);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

static int ds_485_2_send (u8 *buf, u32 count)
{
	int ret = -1;
	if(ds_485_2.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_send(ds_485_2.com, buf, count);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

/*************************数据流设备-485c试***************************************/
static int ds_485_3_open (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_485_3.opend == 1){
		ret = -ERR_BUSY;
		goto err;
	}
	ret = ds_serial_open(&ds_485_3,cfg,COMPORT_MODE_485);
	if(ret < 0){
		goto err;
	}
	ds_485_3.opend = 1;
err:
	return ret;
}
static int ds_485_3_cfg (comport_config_t *cfg)
{
	int ret = -1;
	if(ds_485_3.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_cfg(ds_485_3.com,cfg);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}

static int ds_485_3_send (u8 *buf, u32 count)
{
	int ret = -1;
	if(ds_485_3.opend == 0){
		ret = -ERR_NOINIT;
		goto err;
	}
	ret = ds_serial_send(ds_485_3.com, buf, count);
	if(ret < 0){
		goto err;
	}
err:
	return ret;
}


dstream_device_t dstream_device[8] = {

	[DSTREAM_ZB] = {
			.open 	=	ds_zb_open,
			.config =	ds_zb_cfg,
			.send 	=	ds_zb_send,
	},
	[DSTREAM_IRDA] = {
			.open 	=	ds_irda_open,
			.config =	ds_irda_cfg,
			.send 	=	ds_irda_send,
	},
	[DSTREAM_DEBUG] = {
			.open 	=	ds_debug_open,
			.config =	ds_debug_cfg,
			.send 	=	ds_debug_send,
	},
	[DSTREAM_USBD] = {
			.open 	=	ds_usbd_open,
			.config =	ds_usbd_cfg,
			.send 	=	ds_usbd_send,
	},
	[DSTREAM_JC] = {
			.open 	=	ds_jc_open,
			.config =	ds_jc_cfg,
			.send 	=	ds_jc_send,
	},
	[DSTREAM_485_1] = {
			.open 	=	ds_485_1_open,
			.config =	ds_485_1_cfg,
			.send 	=	ds_485_1_send,
	},
	[DSTREAM_485_2] = {
			.open 	=	ds_485_2_open,
			.config =	ds_485_2_cfg,
			.send 	=	ds_485_2_send,
	},
	[DSTREAM_485_3] = {
			.open 	=	ds_485_3_open,
			.config =	ds_485_3_cfg,
			.send 	=	ds_485_3_send,
	},
};


#endif
