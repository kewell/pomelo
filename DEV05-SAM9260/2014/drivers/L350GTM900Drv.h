#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>
#include <linux/delay.h> 

#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>

#include <asm/arch/gpio.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/hardware.h>

#include "L350IoctlPara.h"

#define	SLEEP(x)	{DECLARE_WAIT_QUEUE_HEAD (stSleep); if (10 > x) mdelay ((x * 1000)); else wait_event_interruptible_timeout (stSleep, 0, (x / 10));}

#define MODULE_NAME				"gprs"
#define MODULE_ID				248

#define GPRS_VBUS_CTRL_PIN			AT91_PIN_PC9
#define GPRS_POWER_MON_PIN			AT91_PIN_PB13
	
#define GPRS_ON_PIN			    	AT91_PIN_PA10
#define GPRS_RESET_PIN				AT91_PIN_PA11
#define GPRS_RTS_PIN				AT91_PIN_PB28
#define GPRS_DTR_PIN				AT91_PIN_PA27
#define GPRS_DCD_PIN				AT91_PIN_PA28
#define GPRS_CTS_PIN				AT91_PIN_PB29
#define GPRS_DSR_PIN				AT91_PIN_PA25
#define GPRS_RI_PIN			     	AT91_PIN_PA26
#define GPRS_TXD_PIN				AT91_PIN_PB6
#define GPRS_RXD_PIN				AT91_PIN_PB7

#define GPRS_SELECT_SIM_PIN			AT91_PIN_PC1
#define GPRS_CHK_SIM1_PIN			AT91_PIN_PC6	
#define GPRS_CHK_SIM2_PIN			AT91_PIN_PC7

#define GPRS_POWERON				1
#define GPRS_POWERDOWN				0
#define GPRS_POWERMON				22
#define GPRS_RESET					18
#define GET_SIM_SLOT				5
#define SET_SIM_SLOT				6
#define GPRS_RING                   7
#define SIM_WORK					13
#define GPRS_DTR					19
#define OPEN_DEBUG					17

#define RING_NONE                               0x00
#define RING_CALL                               0x01
#define RING_SMS                                0x02
#define RING_UNKNOWN                            0x03
