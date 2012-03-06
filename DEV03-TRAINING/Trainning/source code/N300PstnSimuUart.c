#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <asm/arch/hardware.h>
#include <asm/atomic.h>
#include <linux/interrupt.h>
#include <asm/arch/at91rm9200.h>
#include <asm/arch/gpio.h>
#include <asm/arch/at91_tc.h>
#include <asm/arch/board.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "N300PstnSimuUart.h"

#define WAIT_COUNT 20  // 9600  work well

static int baud_rate = 9600;
static int irq_type  = 1;
static int msec_sleep= 200;

module_param( baud_rate, int, 0644);   	/* baud rate value */
module_param( irq_type, int, 0644);     /* rxd irq type: 0-lowlevel; 1-falling; 2:highlevel; 3-rasing */
module_param( msec_sleep, int ,0644);   /* read GU delay */

/* global varialbes for devices registering */
static struct cdev g_cdev5;
static struct cdev g_cdev6;
static struct cdev g_cdev7;

static dev_t  g_devno5;
static dev_t  g_devno6;
static dev_t  g_devno7;

static struct class *g_simu_class;

/* global variant for all 3 ttys */
static AT91PS_TCB g_tcbase0 ;  // = ( AT91PS_TCB)AT91C_VA_BASE_TCB0;
static AT91PS_TCB g_tcbase1 ;  // = ( AT91PS_TCB)AT91C_VA_BASE_TCB1;

static atomic_t g_send_int_ref = ATOMIC_INIT(0);      /* send_int route reference cout */

static spinlock_t    g_freq_lock;
static unsigned long g_freq_flag;
static unsigned long g_freq = 59904000/(9600*2);   /* use TIMER_CLOCK1=MCLK/2 */

/* x : 5,6,7 */
#define gu_var(x) 								\
	static int 	g_wait##x=0;						\
	static enum recv_int_status_t g_recv_flag##x=first_into;		\
										\
	static atomic_t g_ttys##x##_rxdirq_ref = ATOMIC_INIT(0);		\
	static atomic_t g_ttys##x##_recv_ref    = ATOMIC_INIT(0);		\
										\
	static struct gu_buf_t 	g_ttys##x##_recv_buf; 				\
	static unsigned char	g_ttys##x##_recv_char;				\
	static struct gu_buf_t 	g_ttys##x##_send_buf;  				\
	static unsigned char   	g_ttys##x##_send_char;	   			\
										\
	static struct gu_setting_t  g_ttys##x##_setting={BAUD_9600,0,8,1};  	\
	static enum   gu_status_t   g_ttys##x##_recv_status=free_bit;		\
	static enum   gu_status_t   g_ttys##x##_send_status=free_bit;		 			

#define file_ops(x)								\
	static struct file_operations	gu_ttys##x##_fops =	\
	{							\
		.owner   = THIS_MODULE,				\
		.open    = gu_ttys##x##_open,			\
		.ioctl   = gu_ttys##x##_ioctl,			\
		.read    = gu_ttys##x##_read,			\
		.write   = gu_ttys##x##_write,			\
		.release = gu_ttys##x##_release			\
	};
	
/*
* initialize all global varibles in this file, two variables in .h not initialized here
*/
#define init_ttys_gvar(x)				\
	static inline void init_ttys##x##_gvar(void)	\
	{						\
		g_wait##x=0;				\
		g_recv_flag##x=first_into;		\
							\
		atomic_set(&g_ttys##x##_rxdirq_ref,0);	\
		atomic_set(&g_ttys##x##_recv_ref,0);	\
							\
		buf_init(&g_ttys##x##_recv_buf);	\
		buf_init(&g_ttys##x##_send_buf);	\
							\
		g_ttys##x##_recv_char = 0;		\
		g_ttys##x##_send_char = 0;		\
									\
		baud_set( g_ttys##x##_setting, BAUD_9600, 0, 8, 1 );	\
									\
		g_ttys##x##_recv_status = free_bit; 	\
		g_ttys##x##_send_status = free_bit;	\
	}
/* 
 * x : tty no=5,6,7 ;
 * eirq : external irq-4, 5, 0 ; 
 * tcirq : timer irq-3,4,5
 */
#define gu_ttys_open(x,eirq,tcirq)	    						\
	static int gu_ttys##x##_open( struct inode *inode, struct file *file )		\
	{										\
		init_ttys##x##_gvar();	    					\
										\
		if( 3 == atomic_read(&g_send_int_ref) )				\
                {                                                               \
                        printk(KERN_ERR "GU Driver is busy.\n");                         \
                        return -EBUSY;                                         	\
                }                                                               \
										\
		if( 0 > atomic_read(&g_send_int_ref) || 3 < atomic_read(&g_send_int_ref) )	\
                {                                                               \
                        printk(KERN_ERR "GU Driver error.\n");                         	\
                        return -ENODEV;                                         \
                }                                                               \
									\
		if( !atomic_read(&g_send_int_ref) )			\
		{							\
			/* start send_int */				\
			if ( request_irq(AT91C_ID_TC0, send_int, 0, "gu_send", NULL ))	\
				return -ENODEV;							\
			else									\
			{									\
				start_tc(0, 0, 0, baud_rate);					\
			}									\
		}										\
		atomic_inc( &g_send_int_ref );					\
										\
		/* now, start recv_int route */					\
		if( 1 == atomic_read( &g_ttys##x##_rxdirq_ref ) )		\
		{								\
			if( atomic_dec_and_test(&g_send_int_ref) )		\
			{							\
				disable_tc(0,0);				\
				free_irq(AT91C_ID_TC0, NULL);				\
			}							\
			printk(KERN_ERR "GU Driver is busy.\n");				\
			return -EBUSY;						\
		}								\
		if( 0 > atomic_read( &g_ttys##x##_rxdirq_ref ) || 1 < atomic_read( &g_ttys##x##_rxdirq_ref ) )	\
		{								\
			if( atomic_dec_and_test(&g_send_int_ref) )		\
			{							\
				disable_tc(0,0);				\
				free_irq(AT91C_ID_TC0, NULL);				\
			}							\
			printk(KERN_ERR "GU Driver error.\n");				\
			return -ENODEV;						\
		}								\
		ttys_cfg_rxdirq(x,irq_type);	/*set rxd pin irq mode*/	\
		if( request_irq( AT91C_ID_IRQ##eirq, ttys##x##_rxd_int, IRQF_DISABLED, "rxd_irq", NULL) )	\
		{								\
			if( atomic_dec_and_test(&g_send_int_ref) )		\
				free_irq(AT91C_ID_TC0, NULL);			\
										\
			printk(KERN_ERR "GU Register rxd_irq error.\n");			\
			return -ENODEV;						\
		}								\
		atomic_inc(&g_ttys##x##_rxdirq_ref);				\
										\
		/* regieter recv int route,but timer not start yet */		\
		if( 1 == atomic_read( &g_ttys##x##_recv_ref ) )			\
		{								\
			if( atomic_dec_and_test(&g_send_int_ref) )		\
				free_irq(AT91C_ID_TC0, NULL);			\
										\
			free_irq(AT91C_ID_IRQ##eirq, NULL);			\
			atomic_dec(&g_ttys##x##_rxdirq_ref);                    \
										\
			printk(KERN_ERR "GU Driver is busy.\n");				\
			return -EBUSY;						\
		}								\
		if( 0 > atomic_read( &g_ttys##x##_recv_ref ) || 1 < atomic_read( &g_ttys##x##_recv_ref ) )	\
		{								\
			if( atomic_dec_and_test(&g_send_int_ref) )		\
			free_irq(AT91C_ID_TC0, NULL);				\
										\
			free_irq(AT91C_ID_IRQ##eirq, NULL);			\
			atomic_dec(&g_ttys##x##_rxdirq_ref);                    \
										\
			printk(KERN_ERR "GU Driver error.\n");				\
			return -ENODEV;						\
		}								\
		config_tcirq(x);						\
		if ( request_irq(AT91C_ID_TC##tcirq, ttys##x##_recv_int, IRQF_DISABLED, "ttys_recv", NULL ))	\
		{								\
			if( atomic_dec_and_test(&g_send_int_ref) )		\
				free_irq(AT91C_ID_TC0, NULL);			\
										\
			free_irq(AT91C_ID_IRQ##eirq, NULL);			\
			atomic_dec(&g_ttys##x##_rxdirq_ref);                    \
										\
			printk(KERN_ERR "GU Register recv_irq error.\n");			\
			return -ENODEV;						\
		}								\
		atomic_inc(&g_ttys##x##_recv_ref);				\
										\
		return 0;							\
	}

#define gu_ttys_write(x)		\
	static ssize_t gu_ttys##x##_write( struct file *file, const char __user *buf, size_t size, loff_t *ppos )	\
	{			\
		int ret ;	\
				\
		DECLARE_WAITQUEUE( wait, current );	\
		if( 0 == size )				\
			return  0;			\
		if( !buf || !file || size<0 )		\
			return -EINVAL;			\
							\
		add_wait_queue( &g_ttys##x##_send_buf.wq_head, &wait ); \
									\
		ret = put_to_buf( &g_ttys##x##_send_buf,  buf, size);	\
		while( g_ttys##x##_send_buf.len )  		\
		{						\
			 /*if(file->f_flags & o_nonblock) */ 	\
			 /*{				  */	\
			 /*	 ret = -eagain;		  */	\
			 /*	 goto out1;		  */	\
			 /*}				  */	\
			 					\
			set_current_state(TASK_INTERRUPTIBLE);	\
			schedule();				\
								\
			if(signal_pending(current))		\
			{					\
				ret = -ERESTARTSYS;		\
				goto out1;			\
			}					\
		}						\
								\
		if( 0 > ret )					\
		{						\
			ret = -EINVAL;				\
		}						\
	out1:							\
		remove_wait_queue( &g_ttys##x##_send_buf.wq_head, &wait );	\
		set_current_state(TASK_RUNNING);				\
		buf_clear( &g_ttys##x##_send_buf );				\
		return ret;							\
	}									\

#if 1
#define gu_ttys_read(x)			\
	static ssize_t gu_ttys##x##_read( struct file *file, char __user *buf, size_t count, loff_t *ppos )	\
	{					\
		int ret ;			\
						\
		/*DECLARE_WAITQUEUE( wait, current );*/			\
		if( 0 == count )		\
			return 0;		\
		if( !file || !buf || count<0 )	\
			return -EINVAL;		\
						\
		/*add_wait_queue( &g_ttys##x##_recv_buf.wq_head, &wait );*/  \
								\
		/*while( !g_ttys##x##_recv_buf.can_read )*/  	\
		/*{			*/			\
			 /*if(file->f_flags & o_nonblock) */ 	\
			 /*{				  */	\
			 /*	 ret = -eagain;		  */	\
			 /*	 goto out1;		  */	\
			 /*}				  */	\
								\
		/*	set_current_state(TASK_INTERRUPTIBLE);*/\
		/*	schedule();*/				\
								\
		/*	if(signal_pending(current))	*/	\
		/*	{				*/	\
		/*		ret = -ERESTARTSYS;	*/	\
		/*		goto out1;		*/	\
		/*	}				*/	\
		/*}					*/	\
		msleep(msec_sleep); 	\
				\
		ret = get_from_buf( &g_ttys##x##_recv_buf,  buf, count );	\
										\
		if( 0 > ret )							\
			ret = -EINVAL;						\
	/* out1: */								\
		/*remove_wait_queue( &g_ttys##x##_recv_buf.wq_head, &wait ); */	\
		/*set_current_state(TASK_RUNNING); 	*/			\
		/*buf_clear( &g_ttys##x##_recv_buf ); 	*/			\
		return ret;							\
	}
#else	
#define gu_ttys_read(x)			\
	static ssize_t gu_ttys##x##_read( struct file *file, char __user *buf, size_t count, loff_t *ppos )	\
	{						\
		int ret ;				\
							\
		declare_waitqueue( wait, current );	\
		if( 0 == count )			\
			return 0;			\
		if( !file || !buf || count<0 )		\
			return -EINVAL;			\
									\
		add_wait_queue( &g_ttys##x##_recv_buf.wq_head, &wait ); \
									\
		while( !g_ttys##x##_recv_buf.len )  		\
		{						\
			set_current_state(TASK_INTERRUPTIBLE);	\
			schedule();				\
								\
			if(signal_pending(current))		\
			{					\
				ret = -ERESTARTSYS;		\
				goto out1;			\
			}					\
		}						\
								\
		ret = get_from_buf( &g_ttys##x##_recv_buf,  buf, count );	\
										\
		if( 0 > ret )							\
			ret = -EINVAL;						\
	 out1: 								\
		remove_wait_queue( &g_ttys##x##_recv_buf.wq_head, &wait ); 	\
		set_current_state(TASK_RUNNING); 			\
		buf_clear( &g_ttys##x##_recv_buf ); 			\
									\
		return ret;						\
	}
#endif

#define gu_ttys_ioctl(x)			\
	static int gu_ttys##x##_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )	\
	{														\
		unsigned char	*buf;											\
									\
		/*    unsigned char tempchar; */			\
		char	*pcver = (char *) arg;				\
		char	*sver = MODULE_VER;				\
		struct gu_setting_t new_br;    /* new baud_rate */	\
									\
		/*int port_lsr2 = arg; */				\
		switch ( cmd )						\
		{							\
			case GU_GET_VER:				\
				while ( *sver != 0 )			\
				{					\
					*pcver++ = *sver++;		\
				}					\
									\
				*pcver = 0;				\
				break;					\
									\
			case GU_CLEAR_BUF:				\
				buf_clear( &g_ttys##x##_recv_buf ); 	\
				buf_clear( &g_ttys##x##_send_buf ); 	\
				break;					\
									\
			case GU_GET_LENGTH:		/*get data length */	\
				break;						\
										\
			case GU_SET_UART:			/* uart set */	\
				buf = (unsigned char *) ( &arg );		\
										\
				/* test code only */				\
				if(  buf[2]!=0 || buf[4]!=8 || buf[6]!=1 )	\
				{						\
					/*_info("databits must be 8!\n");*/	\
					break;					\
				}						\
				new_br.baudrate = buf[0];			\
				new_br.parity   = buf[2];			\
				new_br.databits = buf[4];							\
				new_br.stopbits = buf[6];							\
				baud_set(g_ttys##x##_setting, new_br.baudrate, new_br.parity, new_br.databits, new_br.stopbits);	\
														\
				gu_ttys_chgbr( &new_br );							\
				break;										\
														\
			case 0x5459:					\
				* (int *) arg = 1;			\
				break;					\
			default:					\
				return -1;				\
		}							\
		return 0;						\
	}						

/* 
	x : tty no=5,6,7 ;
	eirq : external irq-4, 5, 0 ; 
	tcirq : timer irq-3,4,5
	blk : 0,1
	ch : 0,1,2
*/
#define gu_ttys_release(x,eirq,tcirq,blk,ch)		\
	static int gu_ttys##x##_release( struct inode *inode, struct file *file )	\
	{										\
		ttys##x##_clr_rxdirq;							\
		ttys##x##_dis_rxdirq;							\
		free_irq(AT91C_ID_IRQ##eirq, NULL);					\
											\
		disable_tc(blk,ch);							\
		free_irq(AT91C_ID_TC##tcirq, NULL);		\
								\
		if( atomic_dec_and_test( &g_send_int_ref ))	\
		{						\
			/* stop send_int route */	\
			disable_tc(0,0);		\
			free_irq( AT91C_ID_TC0, NULL);	\
		}					\
							\
		buf_clear( &g_ttys##x##_recv_buf );	\
		buf_clear( &g_ttys##x##_send_buf );	\
							\
		return 0;				\
	}	

#define do_wave			\
	static int is_hi=0;	\
	if( !is_hi )		\
	{			\
		is_hi=1;	\
		at91_set_gpio_value(GU_TTYS5_TXD, LOWLEVEL);	\
	}							\
	else							\
        {							\
                is_hi=0;					\
                at91_set_gpio_value(GU_TTYS5_TXD, HIGHLEVEL);	\
        }	


#define do_send(x)						\
	switch(g_ttys##x##_send_status)				\
	{							\
		case free_bit:					\
			if( 0 == g_ttys##x##_send_buf.len )	\
				break;				\
								\
			if( ++g_wait##x < WAIT_COUNT || !at91_get_gpio_value(GU_TTYS##x##_CTS) )	\
				break;									\
													\
			g_ttys##x##_send_char = g_ttys##x##_send_buf.bufp[g_ttys##x##_send_buf.idx++];	\
			at91_set_gpio_value(GU_TTYS##x##_TXD, LOWLEVEL);				\
			g_ttys##x##_send_status = start_bit;			\
			break;							\
										\
		case start_bit:							\
			at91_set_gpio_value(GU_TTYS##x##_TXD, (g_ttys##x##_send_char & 0x01)?HIGHLEVEL:LOWLEVEL);	\
			g_ttys##x##_send_status = data_bit1;		\
			break;						\
									\
		case data_bit1:						\
			at91_set_gpio_value(GU_TTYS##x##_TXD, (g_ttys##x##_send_char & 0x02)?HIGHLEVEL:LOWLEVEL);	\
			g_ttys##x##_send_status = data_bit2;		\
			break;						\
									\
		case data_bit2:						\
			at91_set_gpio_value(GU_TTYS##x##_TXD, (g_ttys##x##_send_char & 0x04)?HIGHLEVEL:LOWLEVEL);	\
			g_ttys##x##_send_status = data_bit3;		\
			break;						\
									\
		case data_bit3:						\
			at91_set_gpio_value(GU_TTYS##x##_TXD, (g_ttys##x##_send_char & 0x08)?HIGHLEVEL:LOWLEVEL);	\
			g_ttys##x##_send_status = data_bit4;	\
			break;					\
								\
		case data_bit4:					\
			at91_set_gpio_value(GU_TTYS##x##_TXD, (g_ttys##x##_send_char & 0x10)?HIGHLEVEL:LOWLEVEL);	\
			g_ttys##x##_send_status = data_bit5;	\
			break;					\
								\
		case data_bit5:					\
			at91_set_gpio_value(GU_TTYS##x##_TXD,  (g_ttys##x##_send_char & 0x20)?HIGHLEVEL:LOWLEVEL);	\
			g_ttys##x##_send_status = data_bit6;	\
			break;					\
								\
		case data_bit6:					\
			at91_set_gpio_value(GU_TTYS##x##_TXD, (g_ttys##x##_send_char & 0x40)?HIGHLEVEL:LOWLEVEL);	\
			g_ttys##x##_send_status = data_bit7;	\
			break;					\
								\
		case data_bit7:					\
			at91_set_gpio_value(GU_TTYS##x##_TXD, (g_ttys##x##_send_char & 0x80)?HIGHLEVEL:LOWLEVEL);	\
			g_ttys##x##_send_status = data_bit8;	\
			break;					\
								\
		case data_bit8:					\
			at91_set_gpio_value(GU_TTYS##x##_TXD, HIGHLEVEL);	\
			g_ttys##x##_send_status = stop_bit;			\
			break;							\
							\
		case stop_bit:				\
			if( g_ttys##x##_send_buf.idx == g_ttys##x##_send_buf.len )	/* send complete */ \
			{								\
				if( ++g_wait##x < WAIT_COUNT )				\
					break;			/* still free_bit1 */	\
				g_ttys##x##_send_status = free_bit;			\
				g_wait##x = 0;						\
				g_ttys##x##_send_buf.len = 0;		/* prevent send chars which not exist */	\
				wake_up_interruptible( &g_ttys##x##_send_buf.wq_head);					\
				break;			\
			}				\
			g_wait##x = 0; 			\
							\
			g_ttys##x##_send_status = free_bit;	/* at lease 1 wait_bit inserted */	\
			break;				\
							\
		default:				\
			;				\
	}						\
							\
	
		

/*
tty : 5,6,7
blk : 0, 1
ch :  0, 1, 2
tc  :  0 - 6  blk=0:0,1,2 ; blk=1:3,4,5
*/
/* trigger while start_bit arrived */
#define gu_ttys_rxd_int(tty,blk,ch,tc)  							\
	static irqreturn_t  ttys##tty##_rxd_int( int irq, void *id )	\
	{						\
		ttys##tty##_clr_rxdirq;			\
		start_tc( blk, ch, tc, baud_rate*2 );	\
		/*printk(KERN_INFO "In rxd_int:rxd_int entered!.\n");*/					\
		return IRQ_HANDLED;			\
	}


#if 1
/*
blk : 0, 1
ch :  0, 1, 2
tc  :  0 - 6  blk=0:0,1,2 ; blk=1:3,4,5
*/
#define gu_ttys_recv_int(tty,blk,ch)		\
	static irqreturn_t ttys##tty##_recv_int( int irq, void *id )		\
	{											\
		clear_tc_sr(blk,ch);  /* clear interrupt */				\
											\
		if( likely(should_ignore == g_recv_flag##tty))					\
		{									\
			/*g_recv_flag##tty = can_recv;	*/				\
			__asm__(							\
				" mov %0, %1 "			\
				: "=r"(g_recv_flag##tty)	\
				: "r" (can_recv)		\
			);					\
			return IRQ_HANDLED;			\
		}						\
								\
		if( unlikely(first_into == g_recv_flag##tty ))		\
		{						\
			ttys##tty##_dis_rxdirq ;		\
			/*g_ttys##tty##_recv_status = start_bit;*/				\
			/*g_recv_flag##tty = should_ignore;	*/				\
			__asm__(								\
				" mov %0, %2\n "						\
				" mov %1, %3 "							\
				:"=r"(g_ttys##tty##_recv_status),"=r"(g_recv_flag##tty)		\
				:"r"(start_bit),"r"(should_ignore)				\
			);						\
			return IRQ_HANDLED;				\
		}							\
									\
		/*g_recv_flag##tty = should_ignore;		*/	\
		__asm__(						\
			" mov %0, %1 "					\
			: "=r"(g_recv_flag##tty)			\
			: "r" (should_ignore)				\
		);							\
							\
		ttys5_ctrl_led(1);  /* led on */	\
		switch(g_ttys##tty##_recv_status)	\
		{					\
			case start_bit:			\
				if(ttys##tty##_get())	\
					/*g_ttys##tty##_recv_char |= 0x01;	*/	\
					__asm__(					\
						" orr %0, %0, #0x01   "			\
						:"=r"(g_ttys##tty##_recv_char)		\
						:"r"(g_ttys##tty##_recv_char)		\
					);						\
											\
				/*g_ttys##tty##_recv_status = data_bit1;	*/	\
				__asm__(					\
					" mov %0, %1 "				\
					: "=r"(g_ttys##tty##_recv_status)	\
					: "r" (data_bit1)			\
				);						\
				break;						\
										\
			case data_bit1:						\
				if(ttys##tty##_get())				\
					/*g_ttys##tty##_recv_char |= 0x02;	*/	\
					__asm__(					\
						" orr %0, %0, #0x02  "			\
						:"=r"(g_ttys##tty##_recv_char)		\
						:"r"(g_ttys##tty##_recv_char)		\
					);						\
											\
				/*g_ttys##tty##_recv_status = data_bit2;	*/	\
				__asm__(						\
					" mov %0, %1 "					\
					: "=r"(g_ttys##tty##_recv_status)	\
					: "r" (data_bit2)			\
				);						\
				break;						\
										\
			case data_bit2:						\
				if(ttys##tty##_get())				\
					/*g_ttys##tty##_recv_char |= 0x04;	*/	\
					__asm__(					\
						" orr %0, %0, #0x04   "			\
						:"=r"(g_ttys##tty##_recv_char)		\
						:"r"(g_ttys##tty##_recv_char)		\
					);						\
											\
				/*g_ttys##tty##_recv_status = data_bit3;	*/	\
				__asm__(						\
					" mov %0, %1 "					\
					: "=r"(g_ttys##tty##_recv_status)		\
					: "r" (data_bit3)				\
				);	\
				break;	\
					\
			case data_bit3:						\
				if(ttys##tty##_get())				\
					/*g_ttys##tty##_recv_char |= 0x08;	*/	\
					__asm__(					\
						" orr %0, %0, #0x08 "			\
						:"=r"(g_ttys##tty##_recv_char)		\
						:"r"(g_ttys##tty##_recv_char)		\
					);		\
							\
				/*g_ttys##tty##_recv_status = data_bit4;	*/	\
				__asm__(						\
					" mov %0, %1 "					\
					: "=r"(g_ttys##tty##_recv_status)		\
					: "r" (data_bit4)				\
				);							\
				break;							\
											\
			case data_bit4:							\
				if(ttys##tty##_get())					\
					/*g_ttys##tty##_recv_char |= 0x10;	*/	\
					__asm__(					\
						" orr %0, %0, #0x10   "			\
						:"=r"(g_ttys##tty##_recv_char)		\
						:"r"(g_ttys##tty##_recv_char)		\
					);						\
											\
				/*g_ttys##tty##_recv_status = data_bit5;	*/	\
				__asm__(						\
					" mov %0, %1 "					\
					: "=r"(g_ttys##tty##_recv_status)		\
					: "r" (data_bit5)				\
				);							\
				break;							\
											\
			case data_bit5:							\
				if(ttys##tty##_get())					\
					/*g_ttys##tty##_recv_char |= 0x20;	*/	\
					__asm__(					\
						" orr %0, %0, #0x20   "			\
						:"=r"(g_ttys##tty##_recv_char)		\
						:"r"(g_ttys##tty##_recv_char)		\
					);						\
											\
				/*g_ttys##tty##_recv_status = data_bit6;	*/	\
				__asm__(						\
					" mov %0, %1 "					\
					: "=r"(g_ttys##tty##_recv_status)		\
					: "r" (data_bit6)				\
				);							\
				break;							\
											\
			case data_bit6:							\
				if(ttys##tty##_get())					\
					/*g_ttys##tty##_recv_char |= 0x40;	*/	\
					__asm__(					\
						" orr %0, %0, #0x40 "			\
						:"=r"(g_ttys##tty##_recv_char)		\
						:"r"(g_ttys##tty##_recv_char)		\
					);						\
											\
				/*g_ttys##tty##_recv_status = data_bit7;	*/	\
				__asm__(						\
					" mov %0, %1 "					\
					: "=r"(g_ttys##tty##_recv_status)		\
					: "r" (data_bit7)				\
				);							\
				break;							\
											\
			case data_bit7:							\
				if( ttys##tty##_get() )					\
					/*g_ttys##tty##_recv_char |= 0x80;	*/	\
					__asm__(					\
						" orr %0, %0, #0x80 "			\
						:"=r"(g_ttys##tty##_recv_char)		\
						:"r"(g_ttys##tty##_recv_char)		\
					);						\
											\
				/*g_ttys##tty##_recv_buf.bufp[g_ttys##tty##_recv_buf.idx++] = g_ttys##tty##_recv_char;	*/	\
				/*g_ttys##tty##_recv_buf.len++;	*/	\
				/*g_ttys##tty##_recv_char = 0;	*/	\
				/*g_recv_flag##tty = first_into; */	\
				__asm__(				\
					" strb %2, [ %0,%5]\n "		\
					" add %0, %0, #1 \n  "		\
					" add %1, %1, #1 \n  "		\
					" eor %2, %2, %2 \n  "		\
					" mov %3, %4  "			\
					: "=r"(g_ttys##tty##_recv_buf.idx), "=r"(g_ttys##tty##_recv_buf.len),"=r"(g_ttys##tty##_recv_char),"=r"(g_recv_flag##tty)		\
					:"r"(first_into),"r"(g_ttys##tty##_recv_buf.bufp),"0"(g_ttys##tty##_recv_buf.idx), "1"(g_ttys##tty##_recv_buf.len), \
						"2"(g_ttys##tty##_recv_char)	\
				);						\
										\
				disable_tc(blk,ch);  				\
				ttys##tty##_clr_rxdirq;				\
				ttys##tty##_restart_rxdirq;			\
										\
				ttys##tty##_ctrl_led(0);  /* led off */		\
										\
				break;						\
										\
			default:						\
				;						\
		}								\
		return IRQ_HANDLED;						\
	}						
#endif

gu_var(5)
gu_var(6)
gu_var(7)
	
init_ttys_gvar(5)
init_ttys_gvar(6)
init_ttys_gvar(7)

#if 0
static irqreturn_t  send_int( int irq, void *id )	
{
	clear_tc_sr(0,0); 		/* clear_tc_sr(tc_block,tc_channel) */						
	//do_wave	
	do_send(5)	
	do_send(6)
	do_send(7)		
	
	return IRQ_HANDLED;
}
#else
static irqreturn_t  send_int( int irq, void *id )	
{
	clear_tc_sr(0,0); 		/* clear_tc_sr(tc_block,tc_channel) */						
	switch(g_ttys5_send_status)				
	{							
		case free_bit:					
			if( likely(0 == g_ttys5_send_buf.len) )	
				break;				
			if( likely( ++g_wait5 < WAIT_COUNT || ttys5_get_cts() ))	
				break;									
													
			//g_ttys5_send_status = free_bit2;
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(free_bit2), "0" (g_ttys5_send_status)
			);      
			break;				
			
		case free_bit2:
			//g_ttys5_send_char = g_ttys5_send_buf.bufp[g_ttys5_send_buf.idx++];	
			__asm__(
				" ldrb %0, [%2, %1]\n"	
				" add %1, %1, #1 "
				: "=r"(g_ttys5_send_char),"=r"(g_ttys5_send_buf.idx)
				: "r"(g_ttys5_send_buf.bufp), "1"(g_ttys5_send_buf.idx)
			);
			
			ttys5_put(0); 
			ttys5_ctrl_led(1); 
			//g_ttys5_send_status = start_bit;
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(start_bit), "0" (g_ttys5_send_status)
			);      
			break;				
										
		case start_bit:							
			ttys5_put( g_ttys5_send_char & 0x01 );
			
			//g_ttys5_send_status = data_bit1;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(data_bit1), "0" (g_ttys5_send_status)
			);      
			break;						
									
		case data_bit1:						
			ttys5_put( g_ttys5_send_char & 0x02 );
			//g_ttys5_send_status = data_bit2;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(data_bit2), "0" (g_ttys5_send_status)
			);      
			break;						
									
		case data_bit2:						
			ttys5_put( g_ttys5_send_char & 0x04 );
			//g_ttys5_send_status = data_bit3;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(data_bit3), "0" (g_ttys5_send_status)
			);      
			break;						
									
		case data_bit3:						
			ttys5_put( g_ttys5_send_char & 0x08 );
			//g_ttys5_send_status = data_bit4;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(data_bit4), "0" (g_ttys5_send_status)
			);      
			break;					
								
		case data_bit4:					
			ttys5_put( g_ttys5_send_char & 0x10 );
			//g_ttys5_send_status = data_bit5;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(data_bit5), "0" (g_ttys5_send_status)
			);      
			break;					
								
		case data_bit5:					
			ttys5_put( g_ttys5_send_char & 0x20 );
			//g_ttys5_send_status = data_bit6;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(data_bit6), "0" (g_ttys5_send_status)
			);      
			break;					
								
		case data_bit6:					
			ttys5_put( g_ttys5_send_char & 0x40 );
			//g_ttys5_send_status = data_bit7;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(data_bit7), "0" (g_ttys5_send_status)
			);      
			break;					
								
		case data_bit7:					
			ttys5_put( g_ttys5_send_char & 0x80 );
			//g_ttys5_send_status = data_bit8;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(data_bit8), "0" (g_ttys5_send_status)
			);      
			break;					
								
		case data_bit8:					
			ttys5_put(1);
			//g_ttys5_send_status = stop_bit;			
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys5_send_status)
				: "r"(stop_bit), "0" (g_ttys5_send_status)
			);      
			break;							
							
		case stop_bit:				
			if( unlikely(g_ttys5_send_buf.idx == g_ttys5_send_buf.len) )	/* send complete */ 
			{								
				if( likely(++g_wait5 < WAIT_COUNT) )				
					break;			/* still stop_bit1 */	
				//g_ttys5_send_status = free_bit;			
				//g_wait5 = 0;						
				//g_ttys5_send_buf.len = 0;		/* prevent send chars which not exist */	
				__asm__(
					" mov %0, %3\n "
					" eor %1, %1, %1\n "
					" eor %2, %2, %2"
					:"=r"(g_ttys5_send_status),"=r"(g_wait5),"=r"(g_ttys5_send_buf.len)
					:"r"(free_bit)
				);

				ttys5_ctrl_led(0);			/* turn off led*/
				wake_up_interruptible( &g_ttys5_send_buf.wq_head);					
				break;			
			}		

			ttys5_ctrl_led(0);
			//g_wait5 = 0; 			
			//g_ttys5_send_status = free_bit;	/* at lease 1 wait_bit inserted */	
		        __asm__ ( 
				" mov %0, %2\n" 
				" eor %1, %1, %1 "
				: "=r"(g_ttys5_send_status),"=r"(g_wait5)
				: "r"(free_bit), "0" (g_ttys5_send_status)
			);      
			break;				
							
		default:				
			;				
	}		
	
	switch(g_ttys6_send_status)				
	{							
		case free_bit:					
			if( likely(0 == g_ttys6_send_buf.len) )	
				break;				
			if( likely(++g_wait6 < WAIT_COUNT || ttys6_get_cts()) )	
				break;									
													
			//g_ttys6_send_status = free_bit2;			
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(free_bit2), "0" (g_ttys6_send_status)
			);      
			break;							
										
		case free_bit2:
			//g_ttys6_send_char = g_ttys6_send_buf.bufp[g_ttys6_send_buf.idx++];	
			__asm__(
				" ldrb %0, [%2, %1]\n"	
				" add %1, %1, #1 "
				: "=r"(g_ttys6_send_char),"=r"(g_ttys6_send_buf.idx)
				: "r"(g_ttys6_send_buf.bufp), "1"(g_ttys6_send_buf.idx)
			);
			ttys6_put(0); 
			ttys6_ctrl_led(1); 
			
			//g_ttys6_send_status = start_bit;
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(start_bit), "0" (g_ttys6_send_status)
			);      
			break;				
										
		case start_bit:							
			ttys6_put( g_ttys6_send_char & 0x01 );
			//g_ttys6_send_status = data_bit1;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(data_bit1), "0" (g_ttys6_send_status)
			);      
			break;						
									
		case data_bit1:						
			ttys6_put( g_ttys6_send_char & 0x02 );
			//g_ttys6_send_status = data_bit2;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(data_bit2), "0" (g_ttys6_send_status)
			);      
			break;						
									
		case data_bit2:						
			ttys6_put( g_ttys6_send_char & 0x04 );
			//g_ttys6_send_status = data_bit3;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(data_bit3), "0" (g_ttys6_send_status)
			);      
			break;						
									
		case data_bit3:						
			ttys6_put( g_ttys6_send_char & 0x08 );
			//g_ttys6_send_status = data_bit4;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(data_bit4), "0" (g_ttys6_send_status)
			);      
			break;					
								
		case data_bit4:					
			ttys6_put( g_ttys6_send_char & 0x10 );
			//g_ttys6_send_status = data_bit5;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(data_bit5), "0" (g_ttys6_send_status)
			);      
			break;					
								
		case data_bit5:					
			ttys6_put( g_ttys6_send_char & 0x20 );
			//g_ttys6_send_status = data_bit6;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(data_bit6), "0" (g_ttys6_send_status)
			);      
			break;					
								
		case data_bit6:					
			ttys6_put( g_ttys6_send_char & 0x40 );
			//g_ttys6_send_status = data_bit7;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(data_bit7), "0" (g_ttys6_send_status)
			);      
			break;					
								
		case data_bit7:					
			ttys6_put( g_ttys6_send_char & 0x80 );
			//g_ttys6_send_status = data_bit8;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(data_bit8), "0" (g_ttys6_send_status)
			);      
			break;					
								
		case data_bit8:					
			ttys6_put(1);
			//g_ttys6_send_status = stop_bit;			
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys6_send_status)
				: "r"(stop_bit), "0" (g_ttys6_send_status)
			);      
			break;							
							
		case stop_bit:				
			if( unlikely(g_ttys6_send_buf.idx == g_ttys6_send_buf.len) )	/* send complete */ 
			{								
				if( likely( ++g_wait6 < WAIT_COUNT ) )				
					break;			/* still free_bit1 */	
				//g_ttys6_send_status = free_bit;			
				//g_wait6 = 0;						
				//g_ttys6_send_buf.len = 0;		/* prevent send chars which not exist */	
				__asm__(
					" mov %0, %3\n "
					" eor %1, %1, %1\n "
					" eor %2, %2, %2"
					:"=r"(g_ttys6_send_status),"=r"(g_wait6),"=r"(g_ttys6_send_buf.len)
					:"r"(free_bit)
				);
				
				ttys6_ctrl_led(0);
				wake_up_interruptible( &g_ttys6_send_buf.wq_head);					
				break;			
			}				
			ttys6_ctrl_led(0);
			//g_wait6 = 0; 			
			//g_ttys6_send_status = free_bit;	/* at lease 1 wait_bit inserted */	
		        __asm__ ( 
				" mov %0, %2\n" 
				" eor %1, %1, %1 "
				: "=r"(g_ttys6_send_status),"=r"(g_wait6)
				: "r"(free_bit), "0" (g_ttys6_send_status)
			);      
			break;				
							
		default:				
			;				
	}	
		
	switch(g_ttys7_send_status)				
	{							
		case free_bit:					
			if( likely(0 == g_ttys7_send_buf.len) )	
				break;				
			if( likely( ++g_wait7 < WAIT_COUNT || ttys7_get_cts()) )	
				break;									
														
			//g_ttys7_send_status = free_bit2;			
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(free_bit2), "0" (g_ttys7_send_status)
			);      
			break;							
										
		case free_bit2:
			//g_ttys7_send_char = g_ttys7_send_buf.bufp[g_ttys7_send_buf.idx++];	
			__asm__(
				" ldrb %0, [%2, %1]\n"	
				" add %1, %1, #1 "
				: "=r"(g_ttys7_send_char),"=r"(g_ttys7_send_buf.idx)
				: "r"(g_ttys7_send_buf.bufp), "1"(g_ttys7_send_buf.idx)
			);
			
			ttys7_put(0); 
			ttys7_ctrl_led(1); 
			
			//g_ttys7_send_status = start_bit;
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(start_bit), "0" (g_ttys7_send_status)
			);      
			break;				
										
		case start_bit:							
			ttys7_put(g_ttys7_send_char & 0x01);
			//g_ttys7_send_status = data_bit1;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(data_bit1), "0" (g_ttys7_send_status)
			);      
			break;						
									
		case data_bit1:						
			ttys7_put(g_ttys7_send_char & 0x02);
			//g_ttys7_send_status = data_bit2;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(data_bit2), "0" (g_ttys7_send_status)
			);      
			break;						
									
		case data_bit2:						
			ttys7_put(g_ttys7_send_char & 0x04);
			//g_ttys7_send_status = data_bit3;		
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(data_bit3), "0" (g_ttys7_send_status)
			);      
			break;						
									
		case data_bit3:						
			ttys7_put(g_ttys7_send_char & 0x08);
			//g_ttys7_send_status = data_bit4;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(data_bit4), "0" (g_ttys7_send_status)
			);      
			break;					
								
		case data_bit4:					
			ttys7_put(g_ttys7_send_char & 0x10);
			//g_ttys7_send_status = data_bit5;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(data_bit5), "0" (g_ttys7_send_status)
			);      
			break;					
								
		case data_bit5:					
			ttys7_put(g_ttys7_send_char & 0x20);
			//g_ttys7_send_status = data_bit6;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(data_bit6), "0" (g_ttys7_send_status)
			);      
			break;					
								
		case data_bit6:					
			ttys7_put(g_ttys7_send_char & 0x40);
			//g_ttys7_send_status = data_bit7;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(data_bit7), "0" (g_ttys7_send_status)
			);      
			break;					
								
		case data_bit7:					
			ttys7_put(g_ttys7_send_char & 0x80);
			//g_ttys7_send_status = data_bit8;	
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(data_bit8), "0" (g_ttys7_send_status)
			);      
			break;					
								
		case data_bit8:					
			ttys7_put(1);
			//g_ttys7_send_status = stop_bit;			
		        __asm__ ( 
				" mov %0, %1\n" 
				: "=r"(g_ttys7_send_status)
				: "r"(stop_bit), "0" (g_ttys7_send_status)
			);      
			break;							
							
		case stop_bit:				
			if( unlikely(g_ttys7_send_buf.idx == g_ttys7_send_buf.len) )	/* send complete */ 
			{								
				if( likely(++g_wait7 < WAIT_COUNT) )				
					break;			/* still free_bit1 */	
				
				//g_ttys7_send_status = free_bit;			
				//g_wait7 = 0;						
				//g_ttys7_send_buf.len = 0;		/* prevent send chars which not exist */	
				__asm__(
					" mov %0, %3\n "
					" eor %1, %1, %1\n "
					" eor %2, %2, %2"
					:"=r"(g_ttys7_send_status),"=r"(g_wait7),"=r"(g_ttys7_send_buf.len)
					:"r"(free_bit)
				);
				
				ttys7_ctrl_led(0);
				wake_up_interruptible( &g_ttys7_send_buf.wq_head);					
				break;			
			}	

			ttys7_ctrl_led(0);
			//g_wait7 = 0; 			
			//g_ttys7_send_status = free_bit;	/* at lease 1 wait_bit inserted */	
		        __asm__ ( 
				" mov %0, %2\n" 
				" eor %1, %1, %1 "
				: "=r"(g_ttys7_send_status),"=r"(g_wait7)
				: "r"(free_bit), "0" (g_ttys7_send_status)
			);      
			break;				
							
		default:				
			;				
	}	

	return IRQ_HANDLED;
}
#endif

/* change baud_rate */
static int gu_ttys_chgbr(const struct gu_setting_t * brp)
{
	if( g_ttys5_recv_buf.len || g_ttys5_send_buf.len 
			|| g_ttys6_recv_buf.len || g_ttys6_send_buf.len  
			|| g_ttys7_recv_buf.len || g_ttys7_send_buf.len )
	{
		printk(KERN_ERR "GU is busy, can`t change baud rate now.Try it later.\n");
		return -EBUSY;
	}
	
	baud_rate = get_bd_value(brp->baudrate);
	spin_lock_irqsave( &g_freq_lock, g_freq_flag );
	g_freq = 59904000/(2*baud_rate);
	spin_unlock_irqrestore( &g_freq_lock, g_freq_flag );
	
	/* reset send timer setting */
	disable_tc(0,0);					
	start_tc( 0, 0, 0, baud_rate );
		
	return 0;
}

#if 0
static irqreturn_t ttys5_recv_int( int irq, void *id )		
{											
	clear_tc_sr(1,0);  /* clear interrupt */				
										
	if( should_ignore == g_recv_flag5 )					
	{									
		//g_recv_flag5 = can_recv;					
		__asm__(
			" mov %0, %1 "
			: "=r"(g_recv_flag5)
			: "r" (can_recv)
		);
		return IRQ_HANDLED;			
	}						
							
	if( first_into == g_recv_flag5 )		
	{						
		ttys5_dis_rxdirq ;		
		//g_ttys5_recv_status = start_bit;	
		//g_recv_flag5 = should_ignore;	
		__asm__(
			" mov %0, %2\n "
			" mov %1, %3 "
			:"=r"(g_ttys5_recv_status),"=r"(g_recv_flag5)
			:"r"(start_bit),"r"(should_ignore)
		);
		return IRQ_HANDLED;			
	}						
								
	//g_recv_flag5 = should_ignore;		
	__asm__(
		" mov %0, %1 "
		: "=r"(g_recv_flag5)
		: "r" (should_ignore)
	);
	
	ttys5_ctrl_led(1);  /* led on */			
	switch(g_ttys5_recv_status)				
	{								
		case start_bit:						
			if(ttys5_get())				
				//g_ttys5_recv_char |= 0x01;	
				__asm__(
					" orr %0, %0, #0x01   "
					:"=r"(g_ttys5_recv_char)
					:"r"(g_ttys5_recv_char)
				);
			
			//g_ttys5_recv_status = data_bit1;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys5_recv_status)
				: "r" (data_bit1)
			);
			break;						
									
		case data_bit1:						
			if(ttys5_get())				
				//g_ttys5_recv_char |= 0x02;	
				__asm__(
					" orr %0, %0, #0x02  "
					:"=r"(g_ttys5_recv_char)
					:"r"(g_ttys5_recv_char)
				);
			
			//g_ttys5_recv_status = data_bit2;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys5_recv_status)
				: "r" (data_bit2)
			);
			break;						
									
		case data_bit2:						
			if(ttys5_get())				
				//g_ttys5_recv_char |= 0x04;	
				__asm__(
					" orr %0, %0, #0x04   "
					:"=r"(g_ttys5_recv_char)
					:"r"(g_ttys5_recv_char)
				);
			
			//g_ttys5_recv_status = data_bit3;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys5_recv_status)
				: "r" (data_bit3)
			);
			break;						
									
		case data_bit3:						
			if(ttys5_get())				
				//g_ttys5_recv_char |= 0x08;	
				__asm__(
					" orr %0, %0, #0x08 "
					:"=r"(g_ttys5_recv_char)
					:"r"(g_ttys5_recv_char)
				);
			
			//g_ttys5_recv_status = data_bit4;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys5_recv_status)
				: "r" (data_bit4)
			);
			break;						
									
		case data_bit4:						
			if(ttys5_get())				
				//g_ttys5_recv_char |= 0x10;	
				__asm__(
					" orr %0, %0, #0x10   "
					:"=r"(g_ttys5_recv_char)
					:"r"(g_ttys5_recv_char)
				);
			
			//g_ttys5_recv_status = data_bit5;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys5_recv_status)
				: "r" (data_bit5)
			);
			break;						
									
		case data_bit5:						
			if(ttys5_get())				
				//g_ttys5_recv_char |= 0x20;	
				__asm__(
					" orr %0, %0, #0x20   "
					:"=r"(g_ttys5_recv_char)
					:"r"(g_ttys5_recv_char)
				);
			
			//g_ttys5_recv_status = data_bit6;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys5_recv_status)
				: "r" (data_bit6)
			);
			break;						
									
		case data_bit6:						
			if(ttys5_get())				
				//g_ttys5_recv_char |= 0x40;	
				__asm__(
					" orr %0, %0, #0x40 "
					:"=r"(g_ttys5_recv_char)
					:"r"(g_ttys5_recv_char)
				);
			
			//g_ttys5_recv_status = data_bit7;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys5_recv_status)
				: "r" (data_bit7)
			);
			break;						
									
		case data_bit7:						
			if(ttys5_get())				
				//g_ttys5_recv_char |= 0x80;	
				__asm__(
					" orr %0, %0, #0x80 "
					:"=r"(g_ttys5_recv_char)
					:"r"(g_ttys5_recv_char)
				);

			//g_ttys5_recv_buf.bufp[g_ttys5_recv_buf.idx++] = g_ttys5_recv_char;	
			//g_ttys5_recv_buf.len++;								
			//g_ttys5_recv_char = 0;		
			//g_recv_flag5 = first_into; /* !! */	
			__asm__(
				" strb %2, [ %0,%5]\n "
				" add %0, %0, #1 \n  "
				" add %1, %1, #1 \n  "
				" eor %2, %2, %2 \n  "
				" mov %3, %4  "
				: "=r"(g_ttys5_recv_buf.idx), "=r"(g_ttys5_recv_buf.len),  \
					"=r"(g_ttys5_recv_char),"=r"(g_recv_flag5)
				:"r"(first_into),"r"(g_ttys5_recv_buf.bufp),	\
					"0"(g_ttys5_recv_buf.idx), "1"(g_ttys5_recv_buf.len), \
					"2"(g_ttys5_recv_char)	
			);
								
			disable_tc(1,0);  			
			ttys5_clr_rxdirq;			
			ttys5_restart_rxdirq;		
								
			ttys5_ctrl_led(0);  /* led off */	
								
			break;					
								
		default:					
			;					
	}								
	return IRQ_HANDLED;					
}

static irqreturn_t ttys6_recv_int( int irq, void *id )		
{											
	clear_tc_sr(1,1);  /* clear interrupt */				
										
	if( should_ignore == g_recv_flag6 )					
	{									
		//g_recv_flag6 = can_recv;					
		__asm__(
			" mov %0, %1 "
			: "=r"(g_recv_flag6)
			: "r" (can_recv)
		);
		return IRQ_HANDLED;			
	}						
							
	if( first_into == g_recv_flag6 )		
	{						
		ttys6_dis_rxdirq ;		
		//g_ttys6_recv_status = start_bit;	
		//g_recv_flag6 = should_ignore;	
		__asm__(
			" mov %0, %2\n "
			" mov %1, %3 "
			:"=r"(g_ttys6_recv_status),"=r"(g_recv_flag6)
			:"r"(start_bit),"r"(should_ignore)
		);
		return IRQ_HANDLED;			
	}						
								
	//g_recv_flag6 = should_ignore;		
	__asm__(
		" mov %0, %1 "
		: "=r"(g_recv_flag6)
		: "r" (should_ignore)
	);
	
	ttys6_ctrl_led(1);  /* led on */			
	switch(g_ttys6_recv_status)				
	{								
		case start_bit:						
			if(ttys6_get())				
				//g_ttys6_recv_char |= 0x01;	
				__asm__(
					" orr %0, %0, #0x01   "
					:"=r"(g_ttys6_recv_char)
					:"r"(g_ttys6_recv_char)
				);
			
			//g_ttys6_recv_status = data_bit1;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys6_recv_status)
				: "r" (data_bit1)
			);
			break;						
									
		case data_bit1:						
			if(ttys6_get())				
				//g_ttys6_recv_char |= 0x02;	
				__asm__(
					" orr %0, %0, #0x02  "
					:"=r"(g_ttys6_recv_char)
					:"r"(g_ttys6_recv_char)
				);
			
			//g_ttys6_recv_status = data_bit2;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys6_recv_status)
				: "r" (data_bit2)
			);
			break;						
									
		case data_bit2:						
			if(ttys6_get())				
				//g_ttys6_recv_char |= 0x04;	
				__asm__(
					" orr %0, %0, #0x04   "
					:"=r"(g_ttys6_recv_char)
					:"r"(g_ttys6_recv_char)
				);
			
			//g_ttys6_recv_status = data_bit3;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys6_recv_status)
				: "r" (data_bit3)
			);
			break;						
									
		case data_bit3:						
			if(ttys6_get())				
				//g_ttys6_recv_char |= 0x08;	
				__asm__(
					" orr %0, %0, #0x08 "
					:"=r"(g_ttys6_recv_char)
					:"r"(g_ttys6_recv_char)
				);
			
			//g_ttys6_recv_status = data_bit4;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys6_recv_status)
				: "r" (data_bit4)
			);
			break;						
									
		case data_bit4:						
			if(ttys6_get())				
				//g_ttys6_recv_char |= 0x10;	
				__asm__(
					" orr %0, %0, #0x10   "
					:"=r"(g_ttys6_recv_char)
					:"r"(g_ttys6_recv_char)
				);
			
			//g_ttys6_recv_status = data_bit6;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys6_recv_status)
				: "r" (data_bit5)
			);
			break;						
									
		case data_bit5:						
			if(ttys6_get())				
				//g_ttys6_recv_char |= 0x20;	
				__asm__(
					" orr %0, %0, #0x20   "
					:"=r"(g_ttys6_recv_char)
					:"r"(g_ttys6_recv_char)
				);
			
			//g_ttys6_recv_status = data_bit6;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys6_recv_status)
				: "r" (data_bit6)
			);
			break;						
									
		case data_bit6:						
			if(ttys6_get())				
				//g_ttys6_recv_char |= 0x40;	
				__asm__(
					" orr %0, %0, #0x40 "
					:"=r"(g_ttys6_recv_char)
					:"r"(g_ttys6_recv_char)
				);
			
			//g_ttys6_recv_status = data_bit7;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys6_recv_status)
				: "r" (data_bit7)
			);
			break;						
									
		case data_bit7:						
			if(ttys6_get())				
				//g_ttys6_recv_char |= 0x80;	
				__asm__(
					" orr %0, %0, #0x80 "
					:"=r"(g_ttys6_recv_char)
					:"r"(g_ttys6_recv_char)
				);

			//g_ttys6_recv_buf.bufp[g_ttys6_recv_buf.idx++] = g_ttys6_recv_char;	
			//g_ttys6_recv_buf.len++;								
			//g_ttys6_recv_char = 0;		
			//g_recv_flag6 = first_into; /* !! */	
			__asm__(
				" strb %2, [ %0,%5]\n "
				" add %0, %0, #1 \n  "
				" add %1, %1, #1 \n  "
				" eor %2, %2, %2 \n  "
				" mov %3, %4  "
				: "=r"(g_ttys6_recv_buf.idx), "=r"(g_ttys6_recv_buf.len),  \
					"=r"(g_ttys6_recv_char),"=r"(g_recv_flag6)
				:"r"(first_into),"r"(g_ttys6_recv_buf.bufp),	\
					"0"(g_ttys6_recv_buf.idx), "1"(g_ttys6_recv_buf.len), \
					"2"(g_ttys6_recv_char)	
			);
								
			disable_tc(1,1);  			
			ttys6_clr_rxdirq;			
			ttys6_restart_rxdirq;		
								
			ttys6_ctrl_led(0);  /* led off */	
								
			break;					
								
		default:					
			;					
	}								
	return IRQ_HANDLED;					
}

static irqreturn_t ttys7_recv_int( int irq, void *id )		
{											
	clear_tc_sr(1,2);  /* clear interrupt */				
										
	if( should_ignore == g_recv_flag7 )					
	{									
		//g_recv_flag7 = can_recv;					
		__asm__(
			" mov %0, %1 "
			: "=r"(g_recv_flag7)
			: "r" (can_recv)
		);
		return IRQ_HANDLED;			
	}						
							
	if( first_into == g_recv_flag7 )		
	{						
		ttys7_dis_rxdirq ;		
		//g_ttys7_recv_status = start_bit;	
		//g_recv_flag7 = should_ignore;	
		__asm__(
			" mov %0, %2\n "
			" mov %1, %3 "
			:"=r"(g_ttys7_recv_status),"=r"(g_recv_flag7)
			:"r"(start_bit),"r"(should_ignore)
		);
		return IRQ_HANDLED;			
	}						
								
	//g_recv_flag7 = should_ignore;		
	__asm__(
		" mov %0, %1 "
		: "=r"(g_recv_flag7)
		: "r" (should_ignore)
	);
	
	ttys7_ctrl_led(1);  /* led on */			
	switch(g_ttys7_recv_status)				
	{								
		case start_bit:						
			if(ttys7_get())				
				//g_ttys7_recv_char |= 0x01;	
				__asm__(
					" orr %0, %0, #0x01   "
					:"=r"(g_ttys7_recv_char)
					:"r"(g_ttys7_recv_char)
				);
			
			//g_ttys7_recv_status = data_bit1;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys7_recv_status)
				: "r" (data_bit1)
			);
			break;						
									
		case data_bit1:						
			if(ttys7_get())				
				//g_ttys7_recv_char |= 0x02;	
				__asm__(
					" orr %0, %0, #0x02  "
					:"=r"(g_ttys7_recv_char)
					:"r"(g_ttys7_recv_char)
				);
			
			//g_ttys7_recv_status = data_bit2;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys7_recv_status)
				: "r" (data_bit2)
			);
			break;						
									
		case data_bit2:						
			if(ttys7_get())				
				//g_ttys7_recv_char |= 0x04;	
				__asm__(
					" orr %0, %0, #0x04   "
					:"=r"(g_ttys7_recv_char)
					:"r"(g_ttys7_recv_char)
				);
			
			//g_ttys7_recv_status = data_bit3;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys7_recv_status)
				: "r" (data_bit3)
			);
			break;						
									
		case data_bit3:						
			if(ttys7_get())				
				//g_ttys7_recv_char |= 0x08;	
				__asm__(
					" orr %0, %0, #0x08 "
					:"=r"(g_ttys7_recv_char)
					:"r"(g_ttys7_recv_char)
				);
			
			//g_ttys7_recv_status = data_bit4;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys7_recv_status)
				: "r" (data_bit4)
			);
			break;						
									
		case data_bit4:						
			if(ttys7_get())				
				//g_ttys7_recv_char |= 0x10;	
				__asm__(
					" orr %0, %0, #0x10   "
					:"=r"(g_ttys7_recv_char)
					:"r"(g_ttys7_recv_char)
				);
			
			//g_ttys7_recv_status = data_bit6;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys7_recv_status)
				: "r" (data_bit5)
			);
			break;						
									
		case data_bit5:						
			if(ttys7_get())				
				//g_ttys7_recv_char |= 0x20;	
				__asm__(
					" orr %0, %0, #0x20   "
					:"=r"(g_ttys7_recv_char)
					:"r"(g_ttys7_recv_char)
				);
			
			//g_ttys7_recv_status = data_bit6;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys7_recv_status)
				: "r" (data_bit6)
			);
			break;						
									
		case data_bit6:						
			if(ttys7_get())				
				//g_ttys7_recv_char |= 0x40;	
				__asm__(
					" orr %0, %0, #0x40 "
					:"=r"(g_ttys7_recv_char)
					:"r"(g_ttys7_recv_char)
				);
			
			//g_ttys7_recv_status = data_bit7;		
			__asm__(
				" mov %0, %1 "
				: "=r"(g_ttys7_recv_status)
				: "r" (data_bit7)
			);
			break;						
									
		case data_bit7:						
			if(ttys7_get())				
				//g_ttys7_recv_char |= 0x80;	
				__asm__(
					" orr %0, %0, #0x80 "
					:"=r"(g_ttys7_recv_char)
					:"r"(g_ttys7_recv_char)
				);

			//g_ttys7_recv_buf.bufp[g_ttys7_recv_buf.idx++] = g_ttys7_recv_char;	
			//g_ttys7_recv_buf.len++;								
			//g_ttys7_recv_char = 0;		
			//g_recv_flag7 = first_into; /* !! */	
			__asm__(
				" strb %2, [ %0,%5]\n "
				" add %0, %0, #1 \n  "
				" add %1, %1, #1 \n  "
				" eor %2, %2, %2 \n  "
				" mov %3, %4  "
				: "=r"(g_ttys7_recv_buf.idx), "=r"(g_ttys7_recv_buf.len),  \
					"=r"(g_ttys7_recv_char),"=r"(g_recv_flag7)
				:"r"(first_into),"r"(g_ttys7_recv_buf.bufp),	\
					"0"(g_ttys7_recv_buf.idx), "1"(g_ttys7_recv_buf.len), \
					"2"(g_ttys7_recv_char)	
			);
								
			disable_tc(1,2);  			
			ttys7_clr_rxdirq;			
			ttys7_restart_rxdirq;		
								
			ttys7_ctrl_led(0);  /* led off */	
								
			break;					
								
		default:					
			;					
	}								
	return IRQ_HANDLED;					
}
#endif

gu_ttys_recv_int(5,1,0)
gu_ttys_recv_int(6,1,1)
gu_ttys_recv_int(7,1,2)

gu_ttys_rxd_int(5,1,0,3)
gu_ttys_rxd_int(6,1,1,4)
gu_ttys_rxd_int(7,1,2,5)

gu_ttys_open(5,4,3)
gu_ttys_open(6,5,4)
gu_ttys_open(7,0,5)
	
gu_ttys_write(5)
gu_ttys_write(6)
gu_ttys_write(7)
	
gu_ttys_read(5)
gu_ttys_read(6)
gu_ttys_read(7)
	
gu_ttys_ioctl(5)
gu_ttys_ioctl(6)
gu_ttys_ioctl(7)

gu_ttys_release(5,4,3,1,0)
gu_ttys_release(6,5,4,1,1)
gu_ttys_release(7,0,5,1,2)

file_ops(5)
file_ops(6)
file_ops(7)
	
static int __init gu_all_init(void)
{
	int result, error;

	/* map TCB0 TCB1 io resource */
	g_tcbase0 = ioremap( AT91C_BASE_TCB0, SZ_16K );
	g_tcbase1 = ioremap( AT91C_BASE_TCB1, SZ_16K );
	
	/* g_freq lock */
	spin_lock_init( &g_freq_lock );
		
	/* register simu class in sysfs */
	g_simu_class = class_create( THIS_MODULE, "gpio_uart");
	if( IS_ERR(g_simu_class))
	{
		printk(KERN_ERR "Err: failed in creating class.\n");
	      	return -1;
	}	

	/* for ttys5 */
	at91_set_gpio_input(  GU_TTYS5_CTS, PULLUP );
	at91_set_gpio_output( GU_TTYS5_DATA, LOWLEVEL );
	
	at91_set_gpio_output( GU_TTYS5_TXD, HIGHLEVEL );
	at91_set_gpio_input(  GU_TTYS5_RXD, PULLUP );

	g_devno5 = MKDEV(MAJOR_GU_TTYS5, 0);
	
	result = register_chrdev_region( g_devno5, 1, GU_TTYS5_DEVNAME );
	if (result<0)
        {
		printk (KERN_ERR "%s: can't get major number %d\n", GU_TTYS5_DEVNAME, MAJOR_GU_TTYS5 );		     
		return result;
        }
			
	cdev_init( &g_cdev5, &gu_ttys5_fops );
	g_cdev5.owner = THIS_MODULE;
	g_cdev5.ops   = &gu_ttys5_fops;
	
	error = cdev_add(&g_cdev5, g_devno5, 1);
	if (error)
		printk(KERN_WARNING "Error %d adding char_reg_setup_cdev", error);	
	
	device_create( g_simu_class, NULL, g_devno5, "ttyS5" );  /* will add node /dev/ttyS5 */
	
	buf_clear( &g_ttys5_recv_buf ); 	
	buf_clear( &g_ttys5_send_buf ); 	
	
	printk(KERN_NOTICE "gu_ttyS5 registered!.\n");

	/* register ttys6 */
	
	at91_set_gpio_input(  GU_TTYS6_CTS, PULLUP );
	at91_set_gpio_output( GU_TTYS6_DATA, LOWLEVEL );
	
	at91_set_gpio_output( GU_TTYS6_TXD, HIGHLEVEL );
	at91_set_gpio_input(  GU_TTYS6_RXD, PULLUP );
	
	g_devno6 = MKDEV(MAJOR_GU_TTYS6, 0);
	
	result = register_chrdev_region( g_devno6, 1, GU_TTYS6_DEVNAME );
	if (result<0)
        {
		printk (KERN_ERR "%s: can't get major number %d\n", GU_TTYS6_DEVNAME, MAJOR_GU_TTYS6 );		     
		return result;
        }
			
	cdev_init( &g_cdev6, &gu_ttys6_fops );
	g_cdev6.owner = THIS_MODULE;
	g_cdev6.ops   = &gu_ttys6_fops;
	error = cdev_add(&g_cdev6, g_devno6, 1);
	if (error)
		printk(KERN_WARNING "Error %d adding char_reg_setup_cdev", error);	
	
	device_create( g_simu_class, NULL, g_devno6, "ttyS6" );  /* will add node /dev/ttyS6 */
	
	buf_clear( &g_ttys6_recv_buf ); 	
	buf_clear( &g_ttys6_send_buf ); 	

	printk(KERN_NOTICE "gu_ttyS6 registered!.\n");

	/* for ttys7 */
	at91_set_gpio_input(  GU_TTYS7_CTS, PULLUP );
	at91_set_gpio_output( GU_TTYS7_DATA, LOWLEVEL );
	
	at91_set_gpio_output( GU_TTYS7_TXD, HIGHLEVEL );
	at91_set_gpio_input(  GU_TTYS7_RXD, PULLUP );
	
	g_devno7 = MKDEV(MAJOR_GU_TTYS7, 0);
	
	result = register_chrdev_region( g_devno7, 1, GU_TTYS7_DEVNAME );
	if (result<0)
        {
		printk (KERN_ERR "%s: can't get major number %d\n", GU_TTYS7_DEVNAME, MAJOR_GU_TTYS7 );		     
		return result;
        }
			
	cdev_init( &g_cdev7, &gu_ttys7_fops );
	g_cdev7.owner = THIS_MODULE;
	g_cdev7.ops   = &gu_ttys7_fops;
	
	error = cdev_add(&g_cdev7, g_devno7, 1);
	if (error)
		printk(KERN_WARNING "Error %d adding char_reg_setup_cdev", error);	

	device_create( g_simu_class, NULL, g_devno7, "ttyS7" );  /* will add node /dev/ttyS7 */
	
	buf_clear( &g_ttys7_recv_buf ); 	
	buf_clear( &g_ttys7_send_buf ); 	
	
	printk(KERN_NOTICE "gu_ttyS7 registered!.\n");

	return 0;
}

static void __exit gu_all_exit(void)
{
	/* release ttys send */
	if( atomic_read(&g_send_int_ref) )				
	{                                                               
		disable_tc(0,0);					
		free_irq( AT91C_ID_TC0, NULL);				
	}                                                               
		
	/* ttys5 release and exit*/		
	
	if( atomic_read( &g_ttys5_rxdirq_ref ) )		
	{
		ttys5_clr_rxdirq;							
		ttys5_dis_rxdirq;							
		free_irq(AT91C_ID_IRQ4, NULL);					
	}								
								
	if( atomic_read( &g_ttys5_recv_ref ) )			
	{								
		disable_tc(1,0);							
		free_irq(AT91C_ID_TC3, NULL);		
	}								
	
	cdev_del( &g_cdev5 );
	device_destroy( g_simu_class, g_devno5 );
	unregister_chrdev_region( g_devno5, 1 );
	printk(KERN_INFO "gu_ttyS5 unregistered!\n");
	
	/* ttys6 release and exit*/		
	
	if( atomic_read( &g_ttys6_rxdirq_ref ) )		
	{
		ttys6_clr_rxdirq;							
		ttys6_dis_rxdirq;							
		free_irq(AT91C_ID_IRQ5, NULL);					
	}								
								
	if( atomic_read( &g_ttys6_recv_ref ) )			
	{								
		disable_tc(1,1);							
		free_irq(AT91C_ID_TC4, NULL);		
	}								

	cdev_del( &g_cdev6 );
	device_destroy( g_simu_class, g_devno6 );
	unregister_chrdev_region( g_devno6, 1 );
	printk(KERN_INFO "gu_ttys6 unregistered!\n");

	/* ttys7 release and exit*/		
	
	if( atomic_read( &g_ttys7_rxdirq_ref ) )		
	{
		ttys7_clr_rxdirq;							
		ttys7_dis_rxdirq;							
		free_irq(AT91C_ID_IRQ0, NULL);					
	}								
								
	if( atomic_read( &g_ttys7_recv_ref ) )			
	{								
		disable_tc(1,2);							
		free_irq(AT91C_ID_TC5, NULL);		
	}								

	cdev_del( &g_cdev7 );
	device_destroy( g_simu_class, g_devno7 );
	unregister_chrdev_region( g_devno7, 1 );
	printk(KERN_INFO "gu_ttys7 unregistered!\n");

	/* destory class */
	class_destroy( g_simu_class );
	
	iounmap( (void*)g_tcbase0 );
	iounmap( (void*)g_tcbase1 );
}

module_init( gu_all_init );
module_exit( gu_all_exit );
MODULE_LICENSE( "GPL" );

