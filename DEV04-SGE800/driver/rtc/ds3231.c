/******************************************************************************
 * 许继电气股份有限公司                                    版权：2008-2015    *
 ******************************************************************************
 * 本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许     *
 * 可不得擅自修改或发布，否则将追究相关的法律责任。                           *
 *                                                                            *
 *                       河南许昌许继股份有限公司                             *
 *                       www.xjgc.com                                         *
 *                       (0374) 321 2924                                      *
 *                                                                            *
 ******************************************************************************
 * 
 * 项目名称		:	rtc驱动程序
 * 文件名		:	ds3231.c
 * 描述			:	based on linux/drivers/i2c/chips/ds1337                                                   
 * 版本			:	1.0.1
 * 作者			:	路冉冉
 *
 * 修改历史记录	:
 * --------------------
 * 01a, 18aug2009, Roy modified
 * --------------------
 *
 ******************************************************************************/

#include <linux/cdev.h>
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/types.h> 
#include <linux/mm.h> 
#include <linux/fs.h> 
#include <linux/ioport.h>
#include <linux/kernel.h>	//container of macro
#include <linux/errno.h>	//error code
#include <linux/ioctl.h>	//ioctl system call
#include <linux/device.h>	//class_create
#include <linux/interrupt.h> //irqaction
#include <asm/uaccess.h>	//copy_to_user

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/string.h>
//#include <linux/rtc.h>		/* get the user-level API */
#include <linux/bcd.h>
#include <linux/list.h>
#include <linux/miscdevice.h>

#include "rtclib.h"

#undef	DEBUG
//#define DEBUG
#ifdef DEBUG
#define	DPRINTK( x... )		printk("rtc: " x)
#else
#define DPRINTK( x... )
#endif

/* Device registers */
#define DS3231_REG_SECOND	0
#define DS3231_REG_MIN		1
#define DS3231_REG_HOUR		2
#define DS3231_REG_DAY		3
#define DS3231_REG_DATE		4
#define DS3231_REG_MONTH	5
#define DS3231_REG_YEAR		6

#define DS3231_REG_CONTROL	14
#define DS3231_REG_STATUS	15

static int ds3231_probe(struct i2c_client *client,const struct i2c_device_id *id);
static void ds3231_init_client(struct i2c_client *client);
static int __devexit ds3231_remove(struct i2c_client *client);
//static int ds3231_detach_client(struct i2c_client *client);
static int ds3231_command(struct i2c_client *client,unsigned int cmd,void *arg);

/*
 * Driver data (common to all clients)
 */
 struct i2c_device_id ds3231_idtable[] = {
       { "ds3231", 0 },
       { }
};
MODULE_DEVICE_TABLE(i2c, ds3231_idtable);

static struct i2c_driver ds3231_driver = {
	.driver = {
			.name	= "ds3231",
			.owner	= THIS_MODULE,
	},
	.id_table	= ds3231_idtable,
	.probe		= ds3231_probe,
	.remove		= __devexit_p(ds3231_remove),
	.command	= ds3231_command,
};

/*
 * Client data (each client gets its own)
 */
struct ds3231_data {
	struct list_head list;
	struct i2c_client *client;
//	struct rtc_device	*rtc;
	struct semaphore ds3231_sem;
};



/*
 * Internal variables
 */
static LIST_HEAD(ds3231_clients);

int ds3231_open(struct inode*inode,struct file *file)
{	
	struct list_head *walk;
	struct list_head *tmp;
	struct ds3231_data *data= NULL;
	//DPRINTK( "i2c_minor=%d\n",i2c_minor(inode));
	list_for_each_safe(walk, tmp, &ds3231_clients) {	// 遍历链表头ds3231_clients
			data = list_entry(walk, struct ds3231_data, list);
			file->private_data= data;
	}
	init_MUTEX(&(data->ds3231_sem));
	return 0;
}

int ds3231_release(struct inode*inode,struct file *file)
{	
	
	//DPRINTK( "ds3231: release!! \n");
	return 0;
}

/*
 * Chip access functions
 */
static int ds3231_get_datetime(struct i2c_client *client, struct rtc_time *dt)
{
	int result;
	u8 buf[7];
	u8 val;
	struct i2c_msg msg[2];
	u8 offs = 0;
	DPRINTK("%s: ENTER\n", __FUNCTION__);

	if (!dt) {
		DPRINTK("%s: EINVAL: dt=NULL\n", __FUNCTION__);
		return -EINVAL;
	}

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &offs;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = sizeof(buf);
	msg[1].buf = &buf[0];

	result = i2c_transfer(client->adapter, msg, 2);

	DPRINTK("%s: [%d] %02x %02x %02x %02x %02x %02x %02x\n",
		__FUNCTION__, result, buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5], buf[6]);

	if (result == 2) {
		dt->tm_sec = bcd2bin(buf[0]);
		dt->tm_min = bcd2bin(buf[1]);
		val = buf[2] & 0x3f;
		dt->tm_hour = bcd2bin(val);
		dt->tm_wday = bcd2bin(buf[3]) - 1;
		dt->tm_mday = bcd2bin(buf[4]);
		val = buf[5] & 0x7f;
		dt->tm_mon = bcd2bin(val) - 1;
		dt->tm_year = bcd2bin(buf[6]);
		if (buf[5] & 0x80)
			dt->tm_year += 100;

		DPRINTK("%s: secs=%d, mins=%d, "
			"hours=%d, mday=%d, mon=%d, year=%d, wday=%d\n",
			__FUNCTION__, dt->tm_sec, dt->tm_min,
			dt->tm_hour, dt->tm_mday,
			dt->tm_mon, dt->tm_year, dt->tm_wday);

		return 0;
	}

	dev_err(&client->dev, "error reading data! %d\n", result);
	return -EIO;
}

static int ds3231_set_datetime(struct i2c_client *client, struct rtc_time *dt)
{
	int result;
	u8 buf[8];
	u8 val;
	struct i2c_msg msg[1];

	if (!dt) {
		dev_dbg(&client->dev, "%s: EINVAL: dt=NULL\n", __FUNCTION__);
		return -EINVAL;
	}

	dev_dbg(&client->dev, "%s: secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n", __FUNCTION__,
		dt->tm_sec, dt->tm_min, dt->tm_hour,
		dt->tm_mday, dt->tm_mon, dt->tm_year, dt->tm_wday);

	buf[0] = 0;		/* reg offset */
	buf[1] = bin2bcd(dt->tm_sec);
	buf[2] = bin2bcd(dt->tm_min);
	buf[3] = bin2bcd(dt->tm_hour);
	buf[4] = bin2bcd(dt->tm_wday + 1);
	buf[5] = bin2bcd(dt->tm_mday);
	buf[6] = bin2bcd(dt->tm_mon + 1);
	val = dt->tm_year;
	if (val >= 100) {
		val -= 100;
		buf[6] |= (1 << 7);
	}
	buf[7] = bin2bcd(val);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = sizeof(buf);
	msg[0].buf = &buf[0];

	result = i2c_transfer(client->adapter, msg, 1);
	if (result == 1)
		return 0;

	dev_err(&client->dev, "error writing data! %d\n", result);
	return -EIO;
}

static int ds3231_command(struct i2c_client *client, unsigned int cmd,
			  void *arg)
{
	int	retval = 0;
	u8	status = 0;
	struct rtc_time tp={0,0,0,0,0,0,0};	
	
	DPRINTK( "%s: cmd=%d\n", __FUNCTION__, cmd);

	switch (cmd) {
	case RTC_RD_TIME:			
			retval = ds3231_get_datetime(client, &tp);	
			if( retval<0 )
				return retval;
			return copy_to_user((struct rtc_time *)arg, &tp, sizeof(tp));
			 
	case RTC_SET_TIME:				
			 return  ds3231_set_datetime(client, arg);

	case RTC_GET_STAT:
				status = i2c_smbus_read_byte_data(client, DS3231_REG_STATUS);
				status = status>>7;
				DPRINTK("status = %x,\n",status);
				return copy_to_user((u8 *)arg, &status, sizeof status);
	default:
		return -EINVAL;
	}
}

/*
 * Public API for access to specific device. Useful for low-level
 * RTC access from kernel code.
 */
 
int ds3231_do_command(int bus, int cmd, void *arg)
{
	struct list_head *walk;
	struct list_head *tmp;
	struct ds3231_data *data;

	list_for_each_safe(walk, tmp, &ds3231_clients) {
		data = list_entry(walk, struct ds3231_data, list);
		//DPRINTK( "%s:0...\n",__FUNCTION__);
		//if (data->client.adapter->nr == bus)
		//DPRINTK( "%s: adaper.nr=%d\n", __FUNCTION__, data->client.adapter->nr);	
		return ds3231_command(data->client, cmd, arg);
	}
	return -ENODEV;
}

int ds3231_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	//int ret = 0;	
	//DPRINTK( "%s: cmd=%d\n", __FUNCTION__, cmd);
	struct ds3231_data *data = (struct ds3231_data *)filp->private_data;
	int retval;

	//struct i2c_client *dev = (struct i2c_client *)filp->private_data;	
	
	retval = down_interruptible(&(data->ds3231_sem));
	if (retval) {
		retval = -EBUSY;
		return retval;	
	}	
	retval = ds3231_command(data->client, cmd, (void *)arg);
	up(&(data->ds3231_sem));
	return 	retval;	
}



static struct file_operations   ds3231_fops={
    .owner          =   THIS_MODULE,
    .open           =   &ds3231_open,
    .release        =   &ds3231_release,
    .ioctl          =   &ds3231_ioctl,
};

//static const struct rtc_class_ops ds3231_rtc_ops ={
//    .open           =   &ds3231_open,
//    .release        =   &ds3231_release,
//    .ioctl          =   &ds3231_ioctl,
//};

static struct miscdevice ds3231_miscdev = {
     .minor =   MISC_DYNAMIC_MINOR,
     .name  =   "rtc",
     .fops  =   &ds3231_fops,
};

/*
 * The following function does more than just detection. If detection
 * succeeds, it also registers the new chip.
 */
static int ds3231_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
//	struct i2c_client *new_client;
	struct ds3231_data *data;
	int err = 0;
	struct device *dev = &client->dev;  /* to use for dev_ reports */

	DPRINTK("roy:enter ds3231 probe\n");

	if (!(data = kzalloc(sizeof(struct ds3231_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto exit;
	}
	data->client = client;
	//INIT_LIST_HEAD(&data->list);
	i2c_set_clientdata(client, data);

	/* Initialize the DS3231 chip */
	ds3231_init_client(client);

	/* Add client to local list */
	list_add(&data->list, &ds3231_clients);
	
//	data->rtc = rtc_device_register(client->name, &client->dev,
//				&ds3231_rtc_ops, THIS_MODULE);
//	if (IS_ERR(data->rtc)) {
//		err = PTR_ERR(data->rtc);
//		dev_err(&client->dev,
//			"unable to register the class device\n");
//		goto exit_free;
//	}
	
	 /* Register a misc device called "ds3231". */
     if ((err = misc_register( &ds3231_miscdev )))
		  	goto exit_free;
	dev_info(dev, "example client created\n");
	DPRINTK("%s: probe done!\n", __FUNCTION__);
	return 0;
exit_free:
	kfree(data);
exit:
	return err;
}

static void ds3231_init_client(struct i2c_client *client)
{
	static u8 status, control;
	static u8 val;
	//初始日历2000年1月1日0点0分0秒，周六
	static struct rtc_time dt_o = {0,0,0,1,0,100,5};
	
	/* On some boards, the RTC isn't configured by boot firmware.
	 * Handle that case by starting/configuring the RTC now.
	 */
	status = i2c_smbus_read_byte_data(client, DS3231_REG_STATUS);
	DPRINTK("status = %x,\n",status);
	control = i2c_smbus_read_byte_data(client, DS3231_REG_CONTROL);
	DPRINTK("control = %x \n",control);

	if ((status & 0x80) || (control & 0x80)) {	//rtc第一次运行，设置时间，并初始化
		/* RTC not running */
		u8 buf[1+16];	/* First byte is interpreted as address */		
		struct i2c_msg msg[1];
		/* Initialize all, including STATUS and CONTROL to zero */
		memset(buf, 0, sizeof(buf));
		/* Write valid values in the date/time registers */
		buf[0] = 0;		/* reg offset */
		buf[1] = bin2bcd(dt_o.tm_sec);
		buf[2] = bin2bcd(dt_o.tm_min);
		buf[3] = bin2bcd(dt_o.tm_hour);
		buf[4] = bin2bcd(dt_o.tm_wday + 1);
		buf[5] = bin2bcd(dt_o.tm_mday);
		buf[6] = bin2bcd(dt_o.tm_mon + 1);
		val = dt_o.tm_year;
		if (val >= 100) {
			val -= 100;
			buf[6] |= (1 << 7);
		}
		buf[7] = bin2bcd(val);
		
		buf[1+ DS3231_REG_CONTROL] = 1<<6;	//当电池供电时，使能秒脉冲及32k输出。
		buf[1+ DS3231_REG_STATUS] = 1<<3;	//使能32k输出。
		
		msg[0].addr = client->addr;
		msg[0].flags = 0;
		msg[0].len = sizeof(buf);
		msg[0].buf = buf;
		
		i2c_transfer(client->adapter, msg, 1);		
	} else {
		/* Running: ensure that device is set in 24-hour mode */
		s32 val;		
		val = i2c_smbus_read_byte_data(client, DS3231_REG_HOUR);		
		if ((val >= 0) && (val & (1 << 6)))
			i2c_smbus_write_byte_data(client, DS3231_REG_HOUR, val & 0x3f);		
	}

	status = i2c_smbus_read_byte_data(client, DS3231_REG_STATUS);
	DPRINTK("roy:status = %d,\n",status);
	control = i2c_smbus_read_byte_data(client, DS3231_REG_CONTROL);
	DPRINTK("roy: control = %d \n",control);
	 	
}


static int __devexit ds3231_remove(struct i2c_client *client)
{
	struct ds3231_data *data = i2c_get_clientdata(client);
	list_del(&ds3231_clients);
	kfree(data);
	misc_deregister( &ds3231_miscdev );
	return 0;
}
static int __init ds3231_init(void)
{
	DPRINTK("roy: enter init\n");
	return i2c_add_driver(&ds3231_driver);
}

static void __exit ds3231_exit(void)
{
	DPRINTK("%s: ENTER\n", __FUNCTION__);
	i2c_del_driver(&ds3231_driver);
}

MODULE_AUTHOR("Roy <luranran@gmail.com>");
MODULE_DESCRIPTION("DS3231 RTC driver");
module_exit(ds3231_exit);
MODULE_LICENSE("GPL");

EXPORT_SYMBOL_GPL(ds3231_do_command);

module_init(ds3231_init);
