// uc864e_usb.h

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/spinlock.h>
#include <linux/usb.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>

#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/at91_pmc.h>
#else /*For 2.6.22.1 we used before*/
#include <asm/arch/gpio.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/hardware.h>
#endif

#include "L350IoctlPara.h"

#define	SLEEP(x)	{DECLARE_WAIT_QUEUE_HEAD (stSleep); if (10 > x) mdelay ((x * 1000)); else wait_event_interruptible_timeout (stSleep, 0, (x / 10));}

// USB device ID
#define TELIT_VENDOR_ID				0x1bc7
#define TELIT_UC864E_ID				0x1003

#define UC864E_USB_DRV_NAME			"uc864e_usb"
#define UC864E_USB_NAME				"usbUC864E%d"
#define UC864E_USB_MINORS			3
#define UC864E_USB_MINOR_BASE		0

#define UC864E_TTY_DRV_NAME			"uc864e_serial"
#define UC864E_TTY_NAME				"ttyUC864E"
#define UC864E_TTY_MAJOR			247
#define UC864E_TTY_MINORS			3
#define UC864E_TTY_MINOR_BASE		0

#define UC864E_MAX_BUF_SIZE			2048
	
#define to_uc864e_dev(d) container_of(d, struct uc864e_usb_cache, kref)

#define GPRS_VBUS_CTRL_PIN			AT91_PIN_PC9
#define GPRS_POWER_MON_PIN			AT91_PIN_PB13
	
#define GPRS_ON_PIN					AT91_PIN_PA10
#define GPRS_RESET_PIN				AT91_PIN_PA11
#define GPRS_RTS_PIN				AT91_PIN_PB28
#define GPRS_DTR_PIN				AT91_PIN_PA27
#define GPRS_DCD_PIN				AT91_PIN_PA28
#define GPRS_CTS_PIN				AT91_PIN_PB29
#define GPRS_DSR_PIN				AT91_PIN_PA25
#define GPRS_RI_PIN				AT91_PIN_PA26

#define RING_NONE                               0x00
#define RING_CALL                               0x01
#define RING_SMS                                0x02
#define RING_UNKNOWN                            0x03

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

static struct usb_device_id uc864e_table [] = {
	{ USB_DEVICE (TELIT_VENDOR_ID, TELIT_UC864E_ID) },
	{ }
};
MODULE_DEVICE_TABLE (usb, uc864e_table);

struct uc864e_usb_cache_t;
struct uc864e_tty_cache_t;
	
/* Structure to hold all of our device specific stuff */
struct uc864e_usb_cache_t
{
	int							index,
								probed;			/* this port has been probed */
	
	struct usb_device			*udev;			/* the usb device for this device */
	struct usb_interface		*interface;		/* the interface for this device */

	struct urb					*recv_urb;		/* the recv and send bulk urb */

	unsigned char				*recv_buf;

	size_t						recv_buf_size;
	
	__u8						bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
	__u8						bulk_out_endpointAddr;	/* the address of the bulk out endpoint */

	void						*tty_cache;
};

struct uc864e_tty_cache_t
{
	int							index;
	
	struct tty_struct			*tty;		/* pointer to the tty for this device */

	void						*usb_cache;
};

//static void uc864e_reset (void);
static void uc864e_hw_init (void);
static void uc864e_powerup (int sim);
static void uc864e_powerdown (void);
static void uc864e_reset (void);
static int uc864e_power_mon (void);
static int uc864e_chk_sim (int sim);
static int uc864e_get_sim (void);
static void uc864e_set_sim (int sim);

static void uc864e_usb_disconnect (struct usb_interface *interface);
static int uc864e_usb_probe (struct usb_interface *interface, const struct usb_device_id *id);
static void uc864e_usb_recv_callback (struct urb *urb);
static void uc864e_usb_send_callback (struct urb *urb);

static int uc864e_tty_open (struct tty_struct *tty, struct file *file);
static void uc864e_tty_close(struct tty_struct *tty, struct file * filp);
static int uc864e_tty_write (struct tty_struct * tty, const unsigned char *user_buf, int count);
static int uc864e_tty_write_room (struct tty_struct * tty);
static int uc864e_tty_chars_in_buffer (struct tty_struct *tty);
static int uc864e_tty_ioctl (struct tty_struct *tty, struct file * file, unsigned int cmd, unsigned long arg);

static int __init mod_uc864e_init (void);
static void __exit mod_uc864e_exit (void);

static unsigned char				g_ucFresh,
									g_ucDebug,
									g_ucHW_Init;

static struct tty_driver			*uc864e_tty_driver;

static struct mutex					uc864e_usb_mutex;

static struct uc864e_tty_cache_t	uc864e_tty_cache [UC864E_TTY_MINORS];
static struct uc864e_usb_cache_t	uc864e_usb_cache [UC864E_USB_MINORS];

static struct tty_operations uc864e_tty_ops = {
	.open				= uc864e_tty_open,
	.close				= uc864e_tty_close,
	.write				= uc864e_tty_write,
	.write_room			= uc864e_tty_write_room,
	.ioctl				= uc864e_tty_ioctl,
	.chars_in_buffer	= uc864e_tty_chars_in_buffer,
};

static struct usb_driver uc864e_usb_driver = {
	.name			= UC864E_USB_DRV_NAME,
	.id_table		= uc864e_table,
	.probe			= uc864e_usb_probe,
	.disconnect		= uc864e_usb_disconnect,
};
