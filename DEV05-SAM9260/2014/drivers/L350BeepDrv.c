#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <asm/arch-at91/at91_tc.h>
#include <asm/arch-at91/at91_pmc.h>
#include <asm/arch/gpio.h>
#include <asm/arch-at91/hardware.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

#include "L350BeepDrv.h"
#include "L350IoctlPara.h"


#define SET_DEFAULT_BEEP_FREQ 8001

volatile unsigned int *PMC_SCER;
volatile unsigned int *PMC_SCDR;
volatile unsigned int *PMC_PCK1;
volatile unsigned int *PMC_SR;

static unsigned int m_uiPrescaler = 0x5;

static int beep_open( struct inode *inode, struct file *file )
{
     at91_set_B_periph(BEEP_PIN, DISPULLUP);

     *PMC_SCDR |= (0x1 << 9);
     *PMC_PCK1 = ((m_uiPrescaler & 0xff) << 2);
     while ((0x1 << 9) != ((*PMC_SR) & (0x1 << 9)));
     
     return 0;
}

static int beep_release( struct inode *inode, struct file *file )
{
     //*PMC_SCDR |= (0x1 << 9);
     //at91_set_gpio_output (BEEP_PIN, 1);

     return 0;
}

static int beep_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
{
     char		*pcVer = (char *) arg;
     char		*sVer = MODULE_VER;
     unsigned long	prescaler;
     
     switch ( cmd )
     {
     case BEEP_ALARM:				
	  at91_set_B_periph(BEEP_PIN, DISPULLUP);
	  *PMC_SCER |= (0x1 << 9);
	  break;

     case BEEP_DISALARM:
	  *PMC_SCDR |= (0x1 << 9);						
	  at91_set_gpio_output (BEEP_PIN, 1);
	  break;
			
     case SET_DEFAULT_BEEP_FREQ:
	  prescaler = arg;
	  if (prescaler > 0x6 || prescaler < 0x0) 
	  {
	       prescaler = 0x5;	       
	  }
	  *PMC_SCDR |= (0x1 << 9);			
	  m_uiPrescaler = (unsigned int)prescaler;	
	  *PMC_PCK1 = ((m_uiPrescaler & 0xff) << 2);			
	  while ((0x1 << 9) != ((*PMC_SR) & (0x1 << 9)));
	  break;	
			
     case SOFTWARE_VER:
	  while ( *sVer != 0 )
	  {
	       *pcVer++ = *sVer++;
	  }
	  *pcVer = 0;
	  break;
     
     default:
	  break;
     }
     
     return 0;	
}

static struct file_operations	beep_fops = 
{ 	.owner = THIS_MODULE,
	.open = beep_open,
	.release = beep_release,
	.ioctl = beep_ioctl, 
};					 	

int init_module( void )
{
	if ((register_chrdev(MODULE_ID, MODULE_NAME, &beep_fops)) < 0)
	{
		printk( "register beep driver failed\n" );
		return -ENODEV;
	}		
	
	PMC_SCER = ioremap(AT91_PMC + AT91_BASE_SYS + 0x00, 0x04);
	PMC_SCDR = ioremap(AT91_PMC + AT91_BASE_SYS + 0x04, 0x04);
	PMC_PCK1  = ioremap(AT91_PMC + AT91_BASE_SYS + 0x44, 0x04);
   	PMC_SR  = ioremap(AT91_PMC + AT91_BASE_SYS + 0x68, 0x04);     		
	udelay( 100 );
	
	print(("Beep driver registered\n"));
	
	return 0;
}

void cleanup_module( void )
{
	*PMC_SCDR |= (0x1 << 9);						
    at91_set_gpio_output (BEEP_PIN, 1);

    iounmap (PMC_SCER);
	iounmap (PMC_SCDR);
	iounmap (PMC_PCK1);
	iounmap (PMC_SR);	
	
	unregister_chrdev( MODULE_ID, MODULE_NAME );
	
	print(("Beep driver unregistered\n"));
}

MODULE_LICENSE( "GPL" );
