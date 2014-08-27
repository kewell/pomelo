#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/arch/hardware.h>
#include <asm/arch/gpio.h>

#include "L350LedDrv.h"
#include "L350IoctlPara.h"

const int LED [LED_LENGHT] = {LED_D1_RUN, LED_D2_0, LED_D3_1, LED_D4_2, LED_D5_3, LED_D6_4, LED_D7_5, LED_D8_6};

/*****************************************************************************
 * 函数名称: led_open
 * 函数说明: 
 * 输入参数: struct inode *inode: 
 *           struct file *file: 
 * 输出参数: 
 * 返 回 值: int
 * 调用本函数的函数清单: 
 * 本函数调用的函数清单: 
 *           
 *****************************************************************************/
int led_open( struct inode *inode, struct file *file )
{
	return 0;
}

/*****************************************************************************
 * 函数名称: led_ioctl
 * 函数说明: 
 * 输入参数: struct inode *inode: 
 *           struct file *file: 
 *           unsigned int cmd: 
 *           unsigned long arg: 
 * 输出参数: 
 * 返 回 值: int
 * 调用本函数的函数清单: 
 * 本函数调用的函数清单: 
 *           at91_set_gpio_value 
 *           at91_get_gpio_value 
 *           
 *****************************************************************************/
int led_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
{
	//char			*pcVer = (char *) arg;
	//char			*sVer = MODULE_VER;
	//unsigned char	st = 0;
	
	if (LED_LENGHT <= arg)
		return -1;

	switch(cmd)
	{
		case LED_ON:
			at91_set_gpio_value( LED [arg], LOWLEVEL); 
			break;
		case LED_OFF:
			at91_set_gpio_value( LED [arg], HIGHLEVEL); 
			break;
		default:
			break;
	}
	
	return 0;
}

struct file_operations	led_fops = { .owner = THIS_MODULE,
						.open = led_open, .ioctl = led_ioctl };

/*****************************************************************************
 * 函数名称: led_driver_init
 * 函数说明: 
 * 输入参数: void: 
 * 输出参数: 
 * 返 回 值: int
 * 调用本函数的函数清单: 
 * 本函数调用的函数清单: 
 *           at91_set_gpio_direction 
 *           register_chrdev 
 *           printk 
 *           
 *****************************************************************************/
int led_driver_init( void )
{
	int i;
	
	for(i = 0; i < LED_LENGHT; i++)
	{
		//at91_set_gpio_direction( LED[i], OUTPUT, HIGHLEVEL );
		gpio_direction_output( LED [i], HIGHLEVEL);
	}
	
	/* 注册Linux设备 */
	if ( (register_chrdev(MODULE_LED_MAJOR, MODULE_LED_NAME, &led_fops)) < 0 )
	{
		//printk( "register a failed\n" );
		return -ENODEV;
	}

	return 0;
}

/*****************************************************************************
 * 函数名称: led_driver_exit
 * 函数说明: 
 * 输入参数: void: 
 * 输出参数: 
 * 返 回 值: void
 * 调用本函数的函数清单: 
 * 本函数调用的函数清单: 
 *           unregister_chrdev 
 *           printk 
 *           
 *****************************************************************************/
void led_driver_exit( void )
{
	unregister_chrdev(MODULE_LED_MAJOR, MODULE_LED_NAME);
}

module_init( led_driver_init );
module_exit( led_driver_exit );
MODULE_LICENSE( "GPL" );

