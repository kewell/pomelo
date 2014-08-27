#include "L350GTM900Drv.h"
#include <linux/irq.h>
#include <linux/proc_fs.h>

struct mutex          mutex; /* Used to lock g_ucRing */
static unsigned char  g_ucRing = RING_NONE;   /* 0x01 Means SMS, 0X02 Means Incoming call */

static unsigned char  g_ucDebug = 0,
           	      g_ucHW_Init = 0;

static int ri_status = RING_NONE;
static struct proc_dir_entry * gtm900_dir;
static int sms_cnt = 0;
static int call_cnt = 0;

static int read_sms_cnt(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
     sprintf(buffer, "%04d\n", sms_cnt);

     return 5;
}

static int read_call_cnt(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
     sprintf(buffer, "%04d\n", call_cnt);

     return 5;
}

static int read_ri_status(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
     if (ri_status == RING_NONE)
     {
	  sprintf(buffer, "NONE\n");
	  return 5;
     }
     else if (ri_status == RING_SMS)
     {
	  sprintf(buffer, "SMS\n");
	  return 4;
     }
     else if (ri_status == RING_CALL)
     {
	  sprintf(buffer, "CALL\n");
	  return 5;
     }
     else
     {
	  sprintf(buffer, "UNKNOWN\n");
	  return 8;
     }
}

static void gprs_hw_init (void)
{
	unsigned int *US_CR;
	
	at91_set_A_periph (GPRS_CTS_PIN, DISPULLUP);
	at91_set_A_periph (GPRS_RTS_PIN, DISPULLUP);

	at91_set_gpio_output (GPRS_DTR_PIN, LOWLEVEL);
	at91_set_gpio_value (GPRS_DTR_PIN, LOWLEVEL);
	
	at91_set_gpio_input (GPRS_DSR_PIN, DISPULLUP);
	at91_set_gpio_input (GPRS_RI_PIN, DISPULLUP);
	at91_set_gpio_input (GPRS_DCD_PIN, DISPULLUP);

	at91_set_gpio_input (GPRS_CHK_SIM1_PIN, DISPULLUP);
	at91_set_gpio_input (GPRS_CHK_SIM2_PIN, DISPULLUP);
	
	at91_set_A_periph (GPRS_RXD_PIN, DISPULLUP);
	at91_set_A_periph (GPRS_TXD_PIN, ENPULLUP);

	at91_sys_write (AT91_PMC_PCER, 1 << AT91C_ID_US1);
	US_CR = ioremap (AT91SAM9260_BASE_US1 + 0x00, 0x04);
	*US_CR = AT91C_US_RXEN;
	*US_CR = AT91C_US_TXEN;
	*US_CR = AT91C_US_DTREN;
	
	// Set the default sim to slot 1
	at91_set_gpio_output (GPRS_SELECT_SIM_PIN, HIGHLEVEL);
	at91_set_gpio_value (GPRS_SELECT_SIM_PIN, HIGHLEVEL);

	at91_set_gpio_output (GPRS_ON_PIN, HIGHLEVEL);		
	at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);		
	at91_set_gpio_output (GPRS_RESET_PIN, HIGHLEVEL);
	at91_set_gpio_value (GPRS_RESET_PIN, HIGHLEVEL);

}

// return 0 = have sim, else means dont have sim
static int gprs_chk_sim (int sim)
{
	if (1 == sim)
			return at91_get_gpio_value (GPRS_CHK_SIM1_PIN);
	else
			return at91_get_gpio_value (GPRS_CHK_SIM2_PIN);
}

// return 0 = sim 2, else means sim 1
static int gprs_get_sim (void)
{
	if (LOWLEVEL == at91_get_gpio_value (GPRS_SELECT_SIM_PIN))
		return 2;
	else
		return 1;
}

static void gprs_set_sim (int sim)
{
	if (2 == sim)
		at91_set_gpio_output (GPRS_SELECT_SIM_PIN, LOWLEVEL);
	else
		at91_set_gpio_output (GPRS_SELECT_SIM_PIN, HIGHLEVEL);

	SLEEP (10);
}

static void gprs_powerup (int sim)
{
	// 0 = use back current sim slot
	if (0 != sim)
	{
		// Check current selected sim slot with wanted sim slot
		if (sim != gprs_get_sim ())
		{
			gprs_set_sim (sim);
		}
	}
	
	at91_set_gpio_value (GPRS_ON_PIN, LOWLEVEL);		
	SLEEP(700);//GTM900B, 700ms for Fibcom G600
	at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);
}

static void gprs_powerdown (void)
{
	at91_set_gpio_value (GPRS_ON_PIN, LOWLEVEL);
	SLEEP (2500);//GTM900B		
	at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);						
}

static void gprs_reset (void)
{
	at91_set_gpio_value( GPRS_RESET_PIN, LOWLEVEL );
	SLEEP( 100 );
	at91_set_gpio_value( GPRS_RESET_PIN, HIGHLEVEL );
	
}

static irqreturn_t catchRing_irq (int irq, void * id)
{
    int                     value;
    static unsigned long    start = 0;

    value = at91_get_gpio_value(GPRS_RI_PIN);
    if(LOWLEVEL == value)  /* It's a FALLING EDGE */
    {
	 start = jiffies;     /* FALLING to lowlevel time start */
    }
    else if(HIGHLEVEL==value) /* It's a RISING EDGE */
    {
	 if (time_after(jiffies, start))
	 {
	      unsigned int timeval;
	      timeval =jiffies_to_msecs(jiffies - start);
	      
	      if (timeval > 0 && timeval <= 150)
	      {
		   sms_cnt++;		   
		   g_ucRing = RING_SMS;
	      }
	      else if (timeval > 0 && timeval < 1000)
	      {
		   call_cnt++;
		   g_ucRing = RING_CALL;
	      }
	      else 
		   g_ucRing = RING_UNKNOWN;

	 }
	 else
	      g_ucRing = RING_UNKNOWN;
    }

    return IRQ_HANDLED;
}

static int gprs_open( struct inode *inode, struct file *file )
{
     struct proc_dir_entry * res;

     if (0x00 == g_ucHW_Init)
     {
	  g_ucHW_Init = 0x01;
	  gprs_hw_init ();
     }

	 at91_set_gpio_input (GPRS_RI_PIN, DISPULLUP);
	 at91_set_deglitch(GPRS_RI_PIN, 1);
	 if (request_irq (GPRS_RI_PIN, catchRing_irq, IRQT_BOTHEDGE | IRQF_DISABLED, "gprs_ring", NULL))
     {
	  printk( KERN_ERR "Request GPRS RING irq failed\n" );
	  return -ENODEV;
     }
     gtm900_dir = proc_mkdir("gtm900", 0);
     if (!gtm900_dir)
	  return -ENOMEM;
     
     res = create_proc_read_entry("sms_cnt", S_IRUGO, gtm900_dir, read_sms_cnt, 0);
     if (!res)
	  return -ENOMEM;
     res = create_proc_read_entry("call_cnt", S_IRUGO, gtm900_dir, read_call_cnt, 0);
     if (!res)
	  return -ENOMEM;
     res = create_proc_read_entry("ri_status", S_IRUGO, gtm900_dir, read_ri_status, 0);
     if (!res)
	  return -ENOMEM;
     
     return 0;
}

static int gprs_release( struct inode *inode, struct file *file )
{
     free_irq(GPRS_RI_PIN, 0);

     return 0;
}

static int gprs_ioctl ( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
{
	int flag, retVal;
	
	switch (cmd)
	{
	case GPRS_POWERON:
		if (!(0 == arg || 1 == arg || 2 == arg))
			return -1;

		// power up with selected sim
		gprs_powerup (arg);

		if (g_ucDebug)
			printk ("gprs powered up module with sim %ld\n", arg);
		return 0;
			
	case GPRS_POWERDOWN:
		gprs_powerdown ();

		if (g_ucDebug)
			printk ("gprs powered down module\n");
		return 0;
		
	case GPRS_RESET:
		if (!(0 == arg || 1 == arg || 2 == arg))
			return -1;

		if (0 != arg)
			gprs_set_sim (arg);
		
		gprs_reset ();
		
		if (g_ucDebug)
			printk ("gprs reset module\n");
		return 0;
	
	case GET_SIM_SLOT:
		flag = gprs_get_sim ();
		
		return flag;

	case SET_SIM_SLOT:
		if (g_ucDebug)
			printk("\n\nSET_SIM_SLOT: arg=%ld\n\n", arg);
		// 1, 11 = Select SIM 1
		// 2, 12 = Select SIM 2
		if (!(1 == arg || 2 == arg))
			return -1;

		// power up with selected sim
		gprs_powerup (arg);

		if (g_ucDebug)
			printk ("gprs selected sim %ld\n", arg);
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

		flag = gprs_chk_sim (arg);
		
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

static struct file_operations	gprs_fops =
{
	.owner = THIS_MODULE,
	.open = gprs_open,
	.release = gprs_release,
	.ioctl = gprs_ioctl,
	
};

                                              
static int __init mod_gprs_init (void)
{
        mutex_init(&mutex);

	if ((register_chrdev(MODULE_ID, MODULE_NAME, &gprs_fops)) < 0)
	{
		return -ENODEV;
	}	

	//gprs_hw_init ();	

	return 0;
}

static void __exit mod_gprs_exit (void)
{
	unregister_chrdev(MODULE_ID, MODULE_NAME);
}

module_init (mod_gprs_init);
module_exit (mod_gprs_exit);

MODULE_LICENSE ("GPL");
