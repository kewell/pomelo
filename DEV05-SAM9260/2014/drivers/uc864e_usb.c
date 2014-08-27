#include <linux/irq.h>
#include "uc864e_usb.h"

struct mutex                    mutex; /* Used to lock g_ucRing */
static unsigned char            g_ucRing = RING_NONE;   /* 0x01 Means SMS, 0X02 Means Incoming call */

static void uc864e_hw_init (void)
{
//	unsigned int *US_CR;
	
	at91_set_A_periph (GPRS_CTS_PIN, DISPULLUP);
	at91_set_A_periph (GPRS_RTS_PIN, DISPULLUP);
	//at91_set_A_periph (GPRS_RI_PIN, DISPULLUP);

	at91_set_gpio_output (GPRS_DTR_PIN, LOWLEVEL);
	
	at91_set_gpio_input (GPRS_DSR_PIN, DISPULLUP);
	//at91_set_gpio_input (GPRS_RI_PIN, DISPULLUP);
	at91_set_gpio_input (GPRS_DCD_PIN, DISPULLUP);

	at91_set_gpio_input (GPRS_CHK_SIM1_PIN, DISPULLUP);
	at91_set_gpio_input (GPRS_CHK_SIM2_PIN, DISPULLUP);
	
	at91_set_A_periph (GPRS_RXD_PIN, DISPULLUP);
	at91_set_A_periph (GPRS_TXD_PIN, ENPULLUP);
/*
	at91_sys_write (AT91_PMC_PCER, 1 << AT91C_ID_US1);
	US_CR = ioremap (AT91SAM9260_BASE_US1 + 0x00, 0x04);
	*US_CR = AT91C_US_RXEN;
	*US_CR = AT91C_US_TXEN;
	*US_CR = AT91C_US_DTREN;
*/	
	at91_set_gpio_input (GPRS_POWER_MON_PIN, DISPULLUP);
//	at91_set_gpio_input (GPRS_POWER_MON_PIN, ENPULLUP);
//	at91_set_gpio_value (GPRS_POWER_MON_PIN, LOWLEVEL);

	at91_set_gpio_output (GPRS_SELECT_SIM_PIN, HIGHLEVEL);	// Default set to SIM1
	at91_set_gpio_output (GPRS_RESET_PIN, LOWLEVEL);
	
	at91_set_gpio_output (GPRS_ON_PIN, HIGHLEVEL);
	at91_set_gpio_output (GPRS_VBUS_CTRL_PIN, LOWLEVEL);
}

static void uc864e_powerup (int sim)
{
	// 0 = use back current sim slot
	if (0 != sim)
	{
		// Check current selected sim slot with wanted sim slot
		if (sim != uc864e_get_sim ())
		{
			uc864e_set_sim (sim);
			uc864e_reset ();
			return;
		}
	}
	
	// Currently it is power off
	if (0 == uc864e_power_mon ())
	{
		g_ucFresh = 0x00;
		
		at91_set_gpio_value (GPRS_RESET_PIN, HIGHLEVEL);
		at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, HIGHLEVEL);	// Turn on USB V-BUS after power up
	
		SLEEP (100);
		at91_set_gpio_value (GPRS_ON_PIN, LOWLEVEL);		
		SLEEP (1100);
		at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);
	}
}

static void uc864e_powerdown (void)
{
	// Currently it is power on
	if (0 != uc864e_power_mon ())
	{
		g_ucFresh = 0x00;

		at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, LOWLEVEL);
		SLEEP (200);
			
		at91_set_gpio_value (GPRS_ON_PIN, LOWLEVEL);
		SLEEP (2200);		
		at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);
		SLEEP (20);
	}
}

static void uc864e_reset (void)
{
	// Currently it is power off
	if (0 == uc864e_power_mon ())
	{
		uc864e_powerup (0);
	}
	else
	{
		g_ucFresh = 0x00;

		at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, LOWLEVEL);
		SLEEP (200);
		
		at91_set_gpio_value (GPRS_RESET_PIN, LOWLEVEL);
		SLEEP (220);
		at91_set_gpio_value (GPRS_RESET_PIN, HIGHLEVEL);

		SLEEP (200);
		at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, HIGHLEVEL);
	}
}

// return 0 = power off, else means power on
static int uc864e_power_mon (void)
{
	// Use the Fresh flag to indicate because the power monitor pin is go HIGH when module is not power up/down before
	if (0x01 == g_ucFresh || LOWLEVEL == at91_get_gpio_value (GPRS_POWER_MON_PIN))
	{
		return 0;
	}

	return 1;
}

// return 0 = have sim, else means dont have sim
static int uc864e_chk_sim (int sim)
{
	if (1 == sim)
			return at91_get_gpio_value (GPRS_CHK_SIM1_PIN);
	else
			return at91_get_gpio_value (GPRS_CHK_SIM2_PIN);
}

// return 0 = sim 2, else means sim 1
static int uc864e_get_sim (void)
{
	if (LOWLEVEL == at91_get_gpio_value (GPRS_SELECT_SIM_PIN))
		return 2;
	else
		return 1;
}

static void uc864e_set_sim (int sim)
{
	if (2 == sim)
		at91_set_gpio_value (GPRS_SELECT_SIM_PIN, LOWLEVEL);
	else
		at91_set_gpio_value (GPRS_SELECT_SIM_PIN, HIGHLEVEL);

	SLEEP (10);
}

static void uc864e_usb_disconnect (struct usb_interface *interface)
{
	struct uc864e_usb_cache_t *usb_cache;

	usb_cache = usb_get_intfdata (interface);

	if (usb_cache)
	{
		usb_cache->probed = 0;

		printk ("uc864e usb #%d device now detached\n", usb_cache->index);
		
		if (usb_cache->recv_urb)
		{
			usb_kill_urb (usb_cache->recv_urb);
			usb_free_urb (usb_cache->recv_urb);
		}

		if (usb_cache->recv_buf)
			kfree (usb_cache->recv_buf);
	}
	
	usb_set_intfdata (interface, 0);
}

static int uc864e_usb_probe (struct usb_interface *interface, const struct usb_device_id *id)
{
	int i, retval = -ENOMEM;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	struct uc864e_usb_cache_t *usb_cache = 0;

	if (g_ucDebug)
		printk ("uc864e is probing\n");

	mutex_lock (&uc864e_usb_mutex);

	// search for empty slot on usb cache
	for (i = 0; i < UC864E_USB_MINORS; ++i)
	{
		if (0 == uc864e_usb_cache [i].probed)
		{
			usb_cache = &uc864e_usb_cache [i];
			break;
		}
	}
	
	mutex_unlock (&uc864e_usb_mutex);

	if (!usb_cache)
		return -ENODEV;
	
	// set up the endpoint information
	// use only the first bulk-in and bulk-out endpoints
	iface_desc = interface->cur_altsetting;

	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i)
	{
		endpoint = &iface_desc->endpoint [i].desc;

		if (!usb_cache->bulk_in_endpointAddr && usb_endpoint_is_bulk_in (endpoint))
		{
			// we found a bulk in endpoint
			usb_cache->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			// set the recv buffer size
			usb_cache->recv_buf_size = (UC864E_MAX_BUF_SIZE > le16_to_cpu (endpoint->wMaxPacketSize)) ? UC864E_MAX_BUF_SIZE : le16_to_cpu (endpoint->wMaxPacketSize);
		}
		
		if (!usb_cache->bulk_out_endpointAddr && usb_endpoint_is_bulk_out (endpoint))
		{
			// we found a bulk out endpoint
			usb_cache->bulk_out_endpointAddr = endpoint->bEndpointAddress;
		}
	}
	
	if (!(usb_cache->bulk_in_endpointAddr && usb_cache->bulk_out_endpointAddr))
	{
		printk ("uc864e is not able to get endpoint address\n");
		return -ENODEV;
	}
	
	usb_cache->udev = usb_get_dev (interface_to_usbdev (interface));
	usb_cache->interface = interface;

	// save our data pointer in this interface device
	usb_set_intfdata (interface, usb_cache);

	usb_cache->recv_urb = usb_alloc_urb (0, GFP_KERNEL);
	usb_cache->recv_buf = kmalloc (usb_cache->recv_buf_size, GFP_KERNEL);
	
	if (!usb_cache->recv_urb || !usb_cache->recv_buf)
		return -ENOMEM;
	
	usb_cache->probed = 1;

	// submit the recv urb to start the recv looping
	usb_fill_bulk_urb (usb_cache->recv_urb,
		usb_cache->udev,
		usb_rcvbulkpipe (usb_cache->udev, usb_cache->bulk_in_endpointAddr),
		usb_cache->recv_buf,
		usb_cache->recv_buf_size,
		uc864e_usb_recv_callback, usb_cache);

	retval = usb_submit_urb (usb_cache->recv_urb, GFP_KERNEL);

	if (retval)
	{
		printk ("uc864e usb #%d failed to submit urb %d\n", usb_cache->index, retval);
		goto error;
	}

	// let the user know what node this device is now attached to
	printk ("uc864e usb #%d device now attached\n", usb_cache->index);
	return 0;

error:
	
	return retval;
}

static void uc864e_usb_recv_callback (struct urb *urb)
{
	struct uc864e_usb_cache_t *usb_cache = urb->context;
	struct uc864e_tty_cache_t *tty_cache;
	int retval,
		length,
		room;
	unsigned char *data;
	
	if (!usb_cache)
	{
		printk ("uc864e usb cache is empty\n");
		return;
	}
	
	tty_cache = usb_cache->tty_cache;

	if (!tty_cache)
	{
		printk ("uc864e usb #%d tty is empty\n", usb_cache->index);
		return;
	}
	
	switch (urb->status)
	{
	case 0:
		if (urb->actual_length && tty_cache->tty)
		{
			if (g_ucDebug)
				printk ("uc864e usb #%d device now recv %d bytes\n", usb_cache->index, urb->actual_length);

			length = urb->actual_length;
			data = urb->transfer_buffer;
			
			while (0 < length)
			{
				room = tty_buffer_request_room (tty_cache->tty, length);
			
				if (0 >= room)
				{
					printk ("uc864e usb #%d device recv data dropped %d bytes\n", usb_cache->index, length);
					length = 0;
					data += length;
				}
				else
				{
					// submit received data to tty
					retval = tty_insert_flip_string (tty_cache->tty, data, room);
					
					if (retval < room)
					{
						printk ("uc864e usb #%d device recv data partially lost %d bytes\n", usb_cache->index, room - retval);
						length -= retval;
						data += retval;
					}
					else
					{
						length -= room;
						data += room;
					}
					
					tty_flip_buffer_push (tty_cache->tty);
				}
			}
		}

		break;
		
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	
	case -ETIME:	// ignore this error
		break;
	
	default:
		printk ("uc864e usb #%d recv urb error status %d\n", usb_cache->index, urb->status);
	}

	if (1 == usb_cache->probed)
	{
		usb_fill_bulk_urb (usb_cache->recv_urb,
			usb_cache->udev,
			usb_rcvbulkpipe (usb_cache->udev, usb_cache->bulk_in_endpointAddr),
			usb_cache->recv_urb->transfer_buffer,
			usb_cache->recv_urb->transfer_buffer_length,
			uc864e_usb_recv_callback, usb_cache);

		retval = usb_submit_urb (usb_cache->recv_urb, GFP_ATOMIC);

		if (retval)
		{
			printk ("uc864e usb #%d failed to submit recv urb. error %d\n", usb_cache->index, retval);
		}
	}
}

static void uc864e_usb_send_callback (struct urb *urb)
{
	if (urb->transfer_buffer)
		usb_buffer_free (urb->dev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);
	
	usb_free_urb (urb);
}

// =================================================================================

static int uc864e_tty_open (struct tty_struct *tty, struct file *file)
{
	int index;
	struct uc864e_tty_cache_t *tty_cache;
	struct uc864e_usb_cache_t *usb_cache;

	tty->driver_data = 0;

	index = tty->index;
	
	// check the permitted range of index
	if (UC864E_TTY_MINORS <= index || UC864E_TTY_MINOR_BASE > index)
		return -ENODEV;

	tty_cache = &uc864e_tty_cache [index];
	usb_cache = tty_cache->usb_cache;
	
	if (!usb_cache)
		return -ENODEV;

	tty_cache->tty = tty;
	tty->driver_data = tty_cache;

	/* force low_latency on so that our tty_push actually forces the data through, 
	   otherwise it is scheduled, and with high data rates (like with OHCI) data
	   can get lost. */
	tty->low_latency = 1;

	// Init the module if not yet perform hardware init
	if (0x00 == g_ucHW_Init)
	{
		g_ucHW_Init = 0x01;
		uc864e_hw_init ();
	}
	
	if (g_ucDebug)
		printk ("uc864e tty #%d device now attached\n", tty_cache->index);

	return 0;
}

static void uc864e_tty_close(struct tty_struct *tty, struct file * filp)
{
	struct uc864e_tty_cache_t *tty_cache = tty->driver_data;
	
	if (!tty_cache)
		return;

	if (g_ucDebug)
		printk ("uc864e tty #%d device now detached\n", tty_cache->index);

	tty_cache->tty = 0;
	tty->driver_data = 0;
}

static int uc864e_tty_write (struct tty_struct * tty, const unsigned char *user_buf, int count)
{
	struct uc864e_tty_cache_t *tty_cache = tty->driver_data;
	struct uc864e_usb_cache_t *usb_cache;
	struct urb *urb = 0;
	int retval = 0;
	unsigned char *buf = 0;
	
	if (0 == count)
		return 0;
	
	if (!tty_cache)
		return -ENODEV;
	
	usb_cache = tty_cache->usb_cache;
	
	if (!usb_cache)
		return -ENODEV;
	
	// check the usb is probed or not
	if (!usb_cache->probed || !usb_cache->udev)
		return -ENODEV;

	if (g_ucDebug)
		printk ("uc864e tty #%d device now write %d bytes\n", tty_cache->index, count);

	urb = usb_alloc_urb (0, GFP_ATOMIC);
	if (!urb)
		goto error;

	buf = usb_buffer_alloc (usb_cache->udev, count, GFP_ATOMIC, &urb->transfer_dma);
	if (!buf)
		goto error;
	
	memcpy (buf, user_buf, count);

	/* initialize the urb properly */
	usb_fill_bulk_urb (urb,
		usb_cache->udev,
		usb_sndbulkpipe (usb_cache->udev, usb_cache->bulk_out_endpointAddr),
		buf,
		count,
		uc864e_usb_send_callback,
		usb_cache);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	
	/* send the data out the bulk port */
	retval = usb_submit_urb (urb, GFP_ATOMIC);

	if (!retval)
	{
		retval = count;
		urb = 0;
		buf = 0;
	}

error:
	if (urb)
	{
		if (urb->transfer_buffer)
			usb_buffer_free (usb_cache->udev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);
		
		usb_free_urb (urb);
	}
	
	return retval;
}

static int uc864e_tty_write_room (struct tty_struct * tty)
{
	return UC864E_MAX_BUF_SIZE;
}

static int uc864e_tty_chars_in_buffer (struct tty_struct *tty)
{
	return 0;
}

static int uc864e_tty_ioctl (struct tty_struct *tty, struct file * file, unsigned int cmd, unsigned long arg)
{
	int flag, retVal;
	
	switch (cmd)
	{
	case GPRS_POWERON:
		if (!(1 == arg || 2 == arg))
			return -1;

		// power up with selected sim
		uc864e_powerup (arg);

		if (g_ucDebug)
			printk ("uc864e powered up module with sim %ld\n", arg);
		return 0;
			
	case GPRS_POWERDOWN:
		uc864e_powerdown ();

		if (g_ucDebug)
			printk ("uc864e powered down module\n");
		return 0;
		
	case GPRS_RESET:
		if (!(0 == arg || 1 == arg || 2 == arg))
			return -1;

		if (0 != arg)
			uc864e_set_sim (arg);
		
		uc864e_reset ();
		
		if (g_ucDebug)
			printk ("uc864e reset module\n");
		return 0;
		
	case GPRS_POWERMON:
		flag = uc864e_power_mon ();
		return flag;
	
	case GET_SIM_SLOT:
		if (!arg)
			return -EINVAL;
	
		flag = uc864e_get_sim ();		
		return flag;

	case SET_SIM_SLOT:
		// 1, 11 = Select SIM 1
		// 2, 12 = Select SIM 2
		if (!(1 == arg || 2 == arg))
			return -1;

		// power up with selected sim
		uc864e_powerup (arg);

		if (g_ucDebug)
			printk ("uc864e selected sim %ld\n", arg);
		return 0;
		
    case GPRS_RING:
        retVal = g_ucRing;     /* Save the Return value */
        mutex_lock(&mutex);
        g_ucRing = RING_NONE;  /* Clear the incoming RING */
        mutex_unlock(&mutex);
        if(1 == g_ucDebug)
        {
                printk("Get Ring Call: %d\n", g_ucRing);
        }
        return retVal;
        
	case SIM_WORK:
		// Check the sim slot have sim or not
		if (!(1 == arg || 2 == arg))
			return -1;

		flag = uc864e_chk_sim (arg);
		
		if (0 == flag)
			return 0;
		else
			return 1;

	case GPRS_DTR:
		if (0 == arg)
			at91_set_gpio_value (GPRS_DTR_PIN, LOWLEVEL);
		else
			at91_set_gpio_value (GPRS_DTR_PIN, HIGHLEVEL);
		
		return 0;
		
	case OPEN_DEBUG:
		if (0 == arg)
			g_ucDebug = 0x00;
		else
			g_ucDebug = 0x01;
		
		return 0;
	}
	
	return -ENOIOCTLCMD;
}

static irqreturn_t catchRing_irq (int irq, void * id)
{
  int                     value;
  static unsigned long    msec = 0;
  static unsigned long    cycle_start = 0;

  value = at91_get_gpio_value(GPRS_RI_PIN);
  if(LOWLEVEL == value)  /* It's a FALLING EDGE */
    {
      msec = jiffies;     /* FALLING to lowlevel time start */
    }
  else if(HIGHLEVEL==value) /* It's a RISING EDGE */
    {
      if(time_before(jiffies, cycle_start))
	{
	  cycle_start = jiffies+660;
	  return IRQ_HANDLED; /* It's the incoming call continue RING tone, skip it */
	}

      msec = jiffies - msec;   /* How long time the low level hold */
      if(11<msec &&  msec< 17 )  /* 14ms means an incoming SMS */
	{
	  if (g_ucDebug)
	    {
	      printk("RING for %lums, It's an Incoming SMS.\n", msec);
	    }
	  mutex_lock(&mutex);
	  g_ucRing = RING_SMS;
	  mutex_unlock(&mutex);
	}
      else if(95<msec && msec<105) /* 100ms means an incoming call */
	{
	  if (g_ucDebug)
	    {
	      printk("RING for %lums, It's an Incoming Call.\n", msec);
	    }
	  mutex_lock(&mutex);
	  g_ucRing = RING_CALL;
	  mutex_unlock(&mutex);
	}
      cycle_start = jiffies+660;
    }

  return IRQ_HANDLED;
}

static int __init mod_uc864e_init (void)
{
	int i, result;
	
	g_ucFresh = 0x01;
	g_ucDebug = 0x00;
	g_ucHW_Init = 0x00;
	
	memset (&uc864e_tty_cache, 0, sizeof (uc864e_tty_cache));
	memset (&uc864e_usb_cache, 0, sizeof (uc864e_usb_cache));

	mutex_init (&uc864e_usb_mutex);
	
	for (i = 0; i < UC864E_USB_MINORS; i++)
	{
		uc864e_usb_cache [i].index = i;
		uc864e_usb_cache [i].tty_cache = &uc864e_tty_cache [i];
	}
	
	for (i = 0; i < UC864E_TTY_MINORS; i++)
	{
		uc864e_tty_cache [i].index = i;
		uc864e_tty_cache [i].usb_cache = &uc864e_usb_cache [i];
	}
	
	// allocate the tty driver
	uc864e_tty_driver = alloc_tty_driver (UC864E_TTY_MINORS);
	
	if (!uc864e_tty_driver)
		return -ENOMEM;

	// initialize the tty driver
	uc864e_tty_driver->owner = THIS_MODULE;
	uc864e_tty_driver->driver_name = UC864E_TTY_DRV_NAME;
	uc864e_tty_driver->name = UC864E_TTY_NAME;
	uc864e_tty_driver->major = UC864E_TTY_MAJOR;
	uc864e_tty_driver->minor_start = UC864E_TTY_MINOR_BASE;
	uc864e_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	uc864e_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	uc864e_tty_driver->flags = TTY_DRIVER_REAL_RAW;
	uc864e_tty_driver->init_termios = tty_std_termios;
	uc864e_tty_driver->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;
	
	tty_set_operations (uc864e_tty_driver, &uc864e_tty_ops);

	// register tty driver
	result = tty_register_driver (uc864e_tty_driver);
	
	if (result)
	{
		put_tty_driver (uc864e_tty_driver);
		return result;
	}

	// register to usb subsystem
	result = usb_register (&uc864e_usb_driver);

	if (result)
	{
		printk ("uc864e usb registration failed. error %d\n", result);
	}
	else
	{
		printk ("uc864e usb driver registered\n");
	}

    mutex_init(&mutex);
    at91_set_gpio_input (GPRS_RI_PIN, DISPULLUP);
	at91_set_deglitch(GPRS_RI_PIN, 1);
    if (request_irq (GPRS_RI_PIN, catchRing_irq, IRQT_BOTHEDGE | IRQF_DISABLED, "gprs_ring", NULL))
    {
            printk( KERN_ERR "Request GPRS RING irq failed\n" );
            return -ENODEV;
    }

	return result;
}

static void __exit mod_uc864e_exit (void)
{
	if (uc864e_tty_driver)
	{
		tty_unregister_driver (uc864e_tty_driver);
		put_tty_driver (uc864e_tty_driver);
	}

	// Deregister from the USB subsystem
	usb_deregister (&uc864e_usb_driver);

	if (0x01 == g_ucHW_Init)
	{
		// power off hardware
		uc864e_powerdown ();
	}
    free_irq (GPRS_RI_PIN, NULL);

    printk ("uc864e usb driver unregistered\n");
}

module_init (mod_uc864e_init);
module_exit (mod_uc864e_exit);

MODULE_LICENSE ("GPL");
