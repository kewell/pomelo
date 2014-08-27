/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2005, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    rtmp_main.c

    Abstract:
    main initialization routines

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Jan Lee		01-10-2005	    modified
	Sample		Jun/01/07		Merge RT2870 and RT2860 drivers.
*/

#include "rt_config.h"

/* added by guowenxue on 2009.06.17 */
#include <asm/arch/gpio.h>
#include <asm/arch-at91/hardware.h>
#include <asm/io.h>

struct proc_dir_entry *MY_PROC_FILE; // add by guowenxue
#define PROCFS_NAME	"wifi_mode_ap"
#define ARCH_L350_V22 1 // Add by KEWELL  2014-08-27 09:03:10 
/* add end */


// Following information will be show when you run 'modinfo'
// *** If you have a solution for the bug in current version of driver, please mail to me.
// Otherwise post to forum in ralinktech's web site(www.ralinktech.com) and let all users help you. ***
MODULE_AUTHOR("Paul Lin <paul_lin@ralinktech.com>");
MODULE_DESCRIPTION("RT2870 Wireless Lan Linux Driver");
MODULE_LICENSE("GPL");


#ifdef MULTIPLE_CARD_SUPPORT
// record whether the card in the card list is used in the card file
extern UINT8  MC_CardUsed[];
#endif // MULTIPLE_CARD_SUPPORT //

/* Kernel thread and vars, which handles packets that are completed. Only
 * packets that have a "complete" function are sent here. This way, the
 * completion is run out of kernel context, and doesn't block the rest of
 * the stack. */
//static int mlme_kill = 0;		// Mlme kernel thread
//static int RTUSBCmd_kill = 0;	// Command kernel thread
//static int TimerFunc_kill = 0;	// TimerQ kernel thread

//static wait_queue_head_t 	timerWaitQ;
//static wait_queue_t 		waitQ;

extern INT __devinit rt28xx_probe(IN void *_dev_p, IN void *_dev_id_p,
									IN UINT argc, OUT PRTMP_ADAPTER *ppAd);


/* module table */
//struct usb_device_id    rtusb_usb_id[] = RT2870_USB_DEVICES;

struct usb_device_id    rtusb_usb_id[] = 
{
        {USB_DEVICE(0x148F,0x2770)}, /* Ralink */
        {USB_DEVICE(0x148F,0x2870)}, /* Ralink */
        {USB_DEVICE(0x07B8,0x2870)}, /* AboCom */
        {USB_DEVICE(0x07B8,0x2770)}, /* AboCom */
        {USB_DEVICE(0x0DF6,0x0039)}, /* Sitecom 2770 */
        {USB_DEVICE(0x0DF6,0x003F)}, /* Sitecom 2770 */
        {USB_DEVICE(0x083A,0x7512)}, /* Arcadyan 2770 */
        {USB_DEVICE(0x0789,0x0162)}, /* Logitec 2870 */
        {USB_DEVICE(0x0789,0x0163)}, /* Logitec 2870 */
        {USB_DEVICE(0x0789,0x0164)}, /* Logitec 2870 */
        {USB_DEVICE(0x177f,0x0302)}, /* lsusb */
        {USB_DEVICE(0x0B05,0x1731)}, /* Asus */
        {USB_DEVICE(0x0B05,0x1732)}, /* Asus */
        {USB_DEVICE(0x0B05,0x1742)}, /* Asus */
        {USB_DEVICE(0x0DF6,0x0017)}, /* Sitecom */
        {USB_DEVICE(0x0DF6,0x002B)}, /* Sitecom */
        {USB_DEVICE(0x0DF6,0x002C)}, /* Sitecom */
        {USB_DEVICE(0x0DF6,0x002D)}, /* Sitecom */
        {USB_DEVICE(0x14B2,0x3C06)}, /* Conceptronic */
        {USB_DEVICE(0x14B2,0x3C28)}, /* Conceptronic */
        {USB_DEVICE(0x2019,0xED06)}, /* Planex Communications, Inc. */
        {USB_DEVICE(0x07D1,0x3C09)}, /* D-Link */
        {USB_DEVICE(0x07D1,0x3C11)}, /* D-Link */
        {USB_DEVICE(0x14B2,0x3C07)}, /* AL */
        {USB_DEVICE(0x050D,0x8053)}, /* Belkin */
        {USB_DEVICE(0x14B2,0x3C23)}, /* Airlink */
        {USB_DEVICE(0x14B2,0x3C27)}, /* Airlink */
        {USB_DEVICE(0x07AA,0x002F)}, /* Corega */
        {USB_DEVICE(0x07AA,0x003C)}, /* Corega */
        {USB_DEVICE(0x07AA,0x003F)}, /* Corega */
        {USB_DEVICE(0x1044,0x800B)}, /* Gigabyte */
        {USB_DEVICE(0x15A9,0x0006)}, /* Sparklan */
        {USB_DEVICE(0x083A,0xB522)}, /* SMC */
        {USB_DEVICE(0x083A,0xA618)}, /* SMC */
        {USB_DEVICE(0x083A,0x8522)}, /* Arcadyan */
        {USB_DEVICE(0x083A,0x7522)}, /* Arcadyan */
        {USB_DEVICE(0x0CDE,0x0022)}, /* ZCOM */
        {USB_DEVICE(0x0586,0x3416)}, /* Zyxel */
        {USB_DEVICE(0x0CDE,0x0025)}, /* Zyxel */
        {USB_DEVICE(0x1740,0x9701)}, /* EnGenius */
        {USB_DEVICE(0x1740,0x9702)}, /* EnGenius */
        {USB_DEVICE(0x0471,0x200f)}, /* Philips */
        {USB_DEVICE(0x14B2,0x3C25)}, /* Draytek */
        {USB_DEVICE(0x13D3,0x3247)}, /* AzureWave */
        {USB_DEVICE(0x083A,0x6618)}, /* Accton */
        {USB_DEVICE(0x15c5,0x0008)}, /* Amit */
        {USB_DEVICE(0x0E66,0x0001)}, /* Hawking */
        {USB_DEVICE(0x0E66,0x0003)}, /* Hawking */
        {USB_DEVICE(0x129B,0x1828)}, /* Siemens */
        {USB_DEVICE(0x157E,0x300E)},    /* U-Media */
        {USB_DEVICE(0x050d,0x805c)},
        {USB_DEVICE(0x050d,0x815c)},
        {USB_DEVICE(0x1482,0x3C09)}, /* Abocom*/
        {USB_DEVICE(0x14B2,0x3C09)}, /* Alpha */
        {USB_DEVICE(0x04E8,0x2018)}, /* samsung */
        {USB_DEVICE(0x5A57,0x0280)}, /* Zinwell */
        {USB_DEVICE(0x5A57,0x0282)}, /* Zinwell */
        {USB_DEVICE(0x7392,0x7718)},
        {USB_DEVICE(0x7392,0x7717)},
        {USB_DEVICE(0x1737,0x0070)}, /* Linksys WUSB100 */
        {USB_DEVICE(0x1737,0x0071)}, /* Linksys WUSB600N */
        {USB_DEVICE(0x0411,0x00e8)}, /* Buffalo WLI-UC-G300N*/
        {USB_DEVICE(0x050d,0x815c)}, /* Belkin F5D8053 */
        {USB_DEVICE(0x100D,0x9031)}, /* Motorola 2770 */
        {USB_DEVICE(0x0DB0,0x6899)},
        {USB_DEVICE(0x148F,0x2070)}, /* Ralink 2070 */
        {USB_DEVICE(0x148F,0x3070)}, /* Ralink 3070 */
        {USB_DEVICE(0x148F,0x3071)}, /* Ralink 3071 */
        {USB_DEVICE(0x148F,0x3072)}, /* Ralink 3072 */
        {USB_DEVICE(0x0DF6,0x003E)}, /* Sitecom 3070 */
        {USB_DEVICE(0x14B2,0x3C12)}, /* AL 3070 */
        {USB_DEVICE(0x18C5,0x0012)}, /* Corega 3070 */
        {USB_DEVICE(0x083A,0x7511)}, /* Arcadyan 3070 */
        {USB_DEVICE(0x1740,0x9703)}, /* EnGenius 3070 */
        {USB_DEVICE(0x1740,0x9705)}, /* EnGenius 3071 */
        {USB_DEVICE(0x1740,0x9706)}, /* EnGenius 3072 */
        {USB_DEVICE(0x13D3,0x3273)}, /* AzureWave 3070*/
        {USB_DEVICE(0x1044,0x800D)}, /* Gigabyte GN-WB32L 3070 */
        {USB_DEVICE(0x2019,0xAB25)}, /* Planex Communications, Inc. RT3070 */
        {USB_DEVICE(0x07B8,0x3070)}, /* AboCom 3070 */
        {USB_DEVICE(0x07B8,0x3071)}, /* AboCom 3071 */
        {USB_DEVICE(0x07B8,0x3072)}, /* Abocom 3072 */
        {USB_DEVICE(0x7392,0x7711)}, /* Edimax 3070 */
        {USB_DEVICE(0x1A32,0x0304)}, /* Quanta 3070 */
        {USB_DEVICE(0x1EDA,0x2310)}, /* AirTies 3070 */
        {USB_DEVICE(0x07D1,0x3C0A)}, /* D-Link 3072*/
        {USB_DEVICE(0x1D4D,0x000C)}, /* Pegatron Corporation 3070 */
        {USB_DEVICE(0x1D4D,0x000C)}, /* Pegatron Corporation 3070 */
        {USB_DEVICE(0x148F,0x3572)}, /* Ralink */
        {USB_DEVICE(0x1740,0x9801)}, /* EnGenius 3572 */
        { }/* Terminating entry */
};


INT const               rtusb_usb_id_len = sizeof(rtusb_usb_id) / sizeof(struct usb_device_id);   
MODULE_DEVICE_TABLE(usb, rtusb_usb_id);

#ifndef PF_NOFREEZE
#define PF_NOFREEZE  0
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)

/**************************************************************************/
/**************************************************************************/
//tested for kernel 2.4 series
/**************************************************************************/
/**************************************************************************/
static void *rtusb_probe(struct usb_device *dev, UINT interface,
						const struct usb_device_id *id_table);
static void rtusb_disconnect(struct usb_device *dev, void *ptr);
						
struct usb_driver rtusb_driver = {
		name:"rt2870",
		probe:rtusb_probe,
		disconnect:rtusb_disconnect,
		id_table:rtusb_usb_id,
	};
	
#else

#ifdef CONFIG_PM

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
#define pm_message_t u32
#endif

static int rt2870_suspend(struct usb_interface *intf, pm_message_t state);
static int rt2870_resume(struct usb_interface *intf);
#endif // CONFIG_PM //

/**************************************************************************/
/**************************************************************************/
//tested for kernel 2.6series
/**************************************************************************/
/**************************************************************************/
static int rtusb_probe (struct usb_interface *intf, const struct usb_device_id *id);
static void rtusb_disconnect(struct usb_interface *intf);

struct usb_driver rtusb_driver = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
	.owner = THIS_MODULE,
#endif	
	.name="rt2870",
	.probe=rtusb_probe,
	.disconnect=rtusb_disconnect,
	.id_table=rtusb_usb_id,

#ifdef CONFIG_PM
	suspend:	rt2870_suspend,
	resume:		rt2870_resume,
#endif
	};

#ifdef CONFIG_PM

VOID RT2860RejectPendingPackets(
	IN	PRTMP_ADAPTER	pAd)
{
	// clear PS packets
	// clear TxSw packets
}

static int rt2870_suspend(
	struct usb_interface *intf,
	pm_message_t state)
{
	struct net_device *net_dev;
	PRTMP_ADAPTER pAd = usb_get_intfdata(intf);


	DBGPRINT(RT_DEBUG_TRACE, ("===> rt2870_suspend()\n"));
	net_dev = pAd->net_dev;
			netif_device_detach(net_dev);

	pAd->PM_FlgSuspend = 1;
	if (netif_running(net_dev)) {
		RTUSBCancelPendingBulkInIRP(pAd);
		RTUSBCancelPendingBulkOutIRP(pAd);
	}
	DBGPRINT(RT_DEBUG_TRACE, ("<=== rt2870_suspend()\n"));
	return 0;
}

static int rt2870_resume(
	struct usb_interface *intf)
{
	struct net_device *net_dev;
	PRTMP_ADAPTER pAd = usb_get_intfdata(intf);


	DBGPRINT(RT_DEBUG_TRACE, ("===> rt2870_resume()\n"));

	pAd->PM_FlgSuspend = 0;
	net_dev = pAd->net_dev;
			netif_device_attach(net_dev);
			netif_start_queue(net_dev);
			netif_carrier_on(net_dev);
			netif_wake_queue(net_dev);

	DBGPRINT(RT_DEBUG_TRACE, ("<=== rt2870_resume()\n"));
	return 0;
}
#endif // CONFIG_PM //
#endif // LINUX_VERSION_CODE //

//add by guowenxue, 2009.06.17
#ifdef ARCH_L350_V22
#define SWITCH_WIFI_USB_HOST_A AT91_PIN_PB22

static void set_GPIO_PB22(int level)
{
       	at91_set_gpio_output(SWITCH_WIFI_USB_HOST_A, level);
       	at91_set_gpio_value( SWITCH_WIFI_USB_HOST_A, level );
}
#endif
// add end


// Init driver module
INT __init rtusb_init(void)
{
#ifdef ARCH_L350_V22
/*RT2070 is use the external USB interface to debug till now, so must set GPIO_PB22 as high level*/
/*to let External USB interface work. After product use RT2070, we should change it to low level */
    printk("Will set PB22 as lowlevel by KEWELL\n"); // add by KEWELL 2014-08-27 09:03:56 
	set_GPIO_PB22(0); // 0->internal USB interface work   1->external USB interface work
#endif

// add by guowenxue, add file "$PROCFS_NAME" to identify the WiFi work as AP mode or STA mode.
	MY_PROC_FILE = create_proc_entry(PROCFS_NAME,0644,NULL);
	if(MY_PROC_FILE == NULL)
       	{
		#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33) /* Add by WENJING 2010-12-03 */
	       	remove_proc_entry(PROCFS_NAME, &proc_root);
		#else
	       	remove_proc_entry(PROCFS_NAME, NULL);
		#endif
	       	printk(KERN_INFO "Error:could not initialize /proc/%s\n",PROCFS_NAME);
	       	return -ENOMEM;
       	}
// add end

	DBGPRINT(RT_DEBUG_TRACE, ("rtusb_init()\n"));
	printk("rtusb init --->\n");   
	return usb_register(&rtusb_driver);
}

// Deinit driver module
VOID __exit rtusb_exit(void)
{
/*RT2070 is use the external USB interface to debug till now, so need set GPIO_PB22 as low level*/
/*After product use RT2070, we should set it as high level to let External USB interface work. */

// add by guowenxue
#ifdef ARCH_L350_V22
	set_GPIO_PB22(1);  // 0->internal USB interface work   1->external USB interface work
#endif
	#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33) /* Add by WENJING 2010-12-03 */
		remove_proc_entry(PROCFS_NAME, &proc_root);
	#else
		remove_proc_entry(PROCFS_NAME, NULL);
	#endif
// add end
	usb_deregister(&rtusb_driver);	
	printk("<--- rtusb exit\n");
}

module_init(rtusb_init);
module_exit(rtusb_exit);




/*---------------------------------------------------------------------	*/
/* function declarations												*/
/*---------------------------------------------------------------------	*/

/*
========================================================================
Routine Description:
    MLME kernel thread.

Arguments:
	*Context			the pAd, driver control block pointer

Return Value:
    0					close the thread

Note:
========================================================================
*/
INT MlmeThread(
	IN void *Context)
{
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER)Context;
	POS_COOKIE	pObj;
	int status;

	pObj = (POS_COOKIE)pAd->OS_Cookie;

	rtmp_os_thread_init("rt2870MlmeThread", (PVOID)&(pAd->mlmeComplete));

	while (pAd->mlme_kill == 0)
	{
		/* lock the device pointers */
		//down(&(pAd->mlme_semaphore));
		status = down_interruptible(&(pAd->mlme_semaphore));
			
		/* lock the device pointers , need to check if required*/
		//down(&(pAd->usbdev_semaphore));
		
		if (!pAd->PM_FlgSuspend)
		MlmeHandler(pAd);
		
		/* unlock the device pointers */
		//up(&(pAd->usbdev_semaphore));
		if (status != 0)
		{
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);
			break;
		}	
	}

	/* notify the exit routine that we're actually exiting now 
	 *
	 * complete()/wait_for_completion() is similar to up()/down(),
	 * except that complete() is safe in the case where the structure
	 * is getting deleted in a parallel mode of execution (i.e. just
	 * after the down() -- that's necessary for the thread-shutdown
	 * case.
	 *
	 * complete_and_exit() goes even further than this -- it is safe in
	 * the case that the thread of the caller is going away (not just
	 * the structure) -- this is necessary for the module-remove case.
	 * This is important in preemption kernels, which transfer the flow
	 * of execution immediately upon a complete().
	 */
	DBGPRINT(RT_DEBUG_TRACE,( "<---%s\n",__FUNCTION__));

	pObj->MLMEThr_pid = -1;
	
	complete_and_exit (&pAd->mlmeComplete, 0);
	return 0;

}


/*
========================================================================
Routine Description:
    USB command kernel thread.

Arguments:
	*Context			the pAd, driver control block pointer

Return Value:
    0					close the thread

Note:
========================================================================
*/
INT RTUSBCmdThread(
	IN void * Context)
{
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER)Context;
	POS_COOKIE		pObj;
	int status;

	pObj = (POS_COOKIE)pAd->OS_Cookie;
	
	rtmp_os_thread_init("rt2870CmdThread", (PVOID)&(pAd->CmdQComplete));

	NdisAcquireSpinLock(&pAd->CmdQLock);
	pAd->CmdQ.CmdQState = RT2870_THREAD_RUNNING;
	NdisReleaseSpinLock(&pAd->CmdQLock);
	
	while (pAd->CmdQ.CmdQState == RT2870_THREAD_RUNNING)
	{
		/* lock the device pointers */
		//down(&(pAd->RTUSBCmd_semaphore));
		status = down_interruptible(&(pAd->RTUSBCmd_semaphore));

		if (pAd->CmdQ.CmdQState == RT2870_THREAD_STOPED)
			break;
		
		if (status != 0)
		{
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);
			break;
		}
		/* lock the device pointers , need to check if required*/
		//down(&(pAd->usbdev_semaphore));

		if (!pAd->PM_FlgSuspend)
		CMDHandler(pAd);

		/* unlock the device pointers */
		//up(&(pAd->usbdev_semaphore));
	}

	if (!pAd->PM_FlgSuspend)
	{	// Clear the CmdQElements.
		CmdQElmt	*pCmdQElmt = NULL;

		NdisAcquireSpinLock(&pAd->CmdQLock);
		pAd->CmdQ.CmdQState = RT2870_THREAD_STOPED;
		while(pAd->CmdQ.size)
		{
			RTUSBDequeueCmd(&pAd->CmdQ, &pCmdQElmt);
			if (pCmdQElmt)
			{
				if (pCmdQElmt->CmdFromNdis == TRUE)
				{
					if (pCmdQElmt->buffer != NULL)
						NdisFreeMemory(pCmdQElmt->buffer, pCmdQElmt->bufferlength, 0);

					NdisFreeMemory(pCmdQElmt, sizeof(CmdQElmt), 0);
				}
				else
				{
					if ((pCmdQElmt->buffer != NULL) && (pCmdQElmt->bufferlength != 0))
						NdisFreeMemory(pCmdQElmt->buffer, pCmdQElmt->bufferlength, 0);
		            {
						NdisFreeMemory(pCmdQElmt, sizeof(CmdQElmt), 0);
					}
				}
			}
		}

		NdisReleaseSpinLock(&pAd->CmdQLock);
	}
	/* notify the exit routine that we're actually exiting now 
	 *
	 * complete()/wait_for_completion() is similar to up()/down(),
	 * except that complete() is safe in the case where the structure
	 * is getting deleted in a parallel mode of execution (i.e. just
	 * after the down() -- that's necessary for the thread-shutdown
	 * case.
	 *
	 * complete_and_exit() goes even further than this -- it is safe in
	 * the case that the thread of the caller is going away (not just
	 * the structure) -- this is necessary for the module-remove case.
	 * This is important in preemption kernels, which transfer the flow
	 * of execution immediately upon a complete().
	 */
	DBGPRINT(RT_DEBUG_TRACE,( "<---RTUSBCmdThread\n"));

	pObj->RTUSBCmdThr_pid = -1;
	
	complete_and_exit (&pAd->CmdQComplete, 0);
	return 0;

}


static void RT2870_TimerQ_Handle(RTMP_ADAPTER *pAd)
{
	int status;
	RALINK_TIMER_STRUCT	*pTimer;
	RT2870_TIMER_ENTRY	*pEntry;
	unsigned long	irqFlag;

	while(!pAd->TimerFunc_kill)
	{
//		printk("waiting for event!\n");
		pTimer = NULL;

		status = down_interruptible(&(pAd->RTUSBTimer_semaphore));

		if (pAd->TimerQ.status == RT2870_THREAD_STOPED)
			break;
		
		// event happened.
		while(pAd->TimerQ.pQHead)
		{
			RTMP_IRQ_LOCK(&pAd->TimerQLock, irqFlag);
			pEntry = pAd->TimerQ.pQHead;
			if (pEntry)
			{
				pTimer = pEntry->pRaTimer;

				// update pQHead
				pAd->TimerQ.pQHead = pEntry->pNext;
				if (pEntry == pAd->TimerQ.pQTail)
					pAd->TimerQ.pQTail = NULL;
			
				// return this queue entry to timerQFreeList.
				pEntry->pNext = pAd->TimerQ.pQPollFreeList;
				pAd->TimerQ.pQPollFreeList = pEntry;
			}
			RTMP_IRQ_UNLOCK(&pAd->TimerQLock, irqFlag);

			if (pTimer)
			{
				if (pTimer->handle != NULL)
				if (!pAd->PM_FlgSuspend)
					pTimer->handle(NULL, (PVOID) pTimer->cookie, NULL, pTimer);
				if ((pTimer->Repeat) && (pTimer->State == FALSE))
					RTMP_OS_Add_Timer(&pTimer->TimerObj, pTimer->TimerValue);
			}
		}
		
		if (status != 0)
		{
			pAd->TimerQ.status = RT2870_THREAD_STOPED;
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);
			break;
		}
	}
}


INT TimerQThread(
	IN OUT PVOID Context)
{
	PRTMP_ADAPTER	pAd;
	POS_COOKIE	pObj;

	pAd = (PRTMP_ADAPTER)Context;
	pObj = (POS_COOKIE) pAd->OS_Cookie;

	rtmp_os_thread_init("rt2870TimerQHandle", (PVOID)&(pAd->TimerQComplete));

	RT2870_TimerQ_Handle(pAd);

	/* notify the exit routine that we're actually exiting now 
	 *
	 * complete()/wait_for_completion() is similar to up()/down(),
	 * except that complete() is safe in the case where the structure
	 * is getting deleted in a parallel mode of execution (i.e. just
	 * after the down() -- that's necessary for the thread-shutdown
	 * case.
	 *
	 * complete_and_exit() goes even further than this -- it is safe in
	 * the case that the thread of the caller is going away (not just
	 * the structure) -- this is necessary for the module-remove case.
	 * This is important in preemption kernels, which transfer the flow
	 * of execution immediately upon a complete().
	 */
	DBGPRINT(RT_DEBUG_TRACE,( "<---%s\n",__FUNCTION__));

	pObj->TimerQThr_pid = -1;
	
	complete_and_exit(&pAd->TimerQComplete, 0);
	return 0;

}


RT2870_TIMER_ENTRY *RT2870_TimerQ_Insert(
	IN RTMP_ADAPTER *pAd, 
	IN RALINK_TIMER_STRUCT *pTimer)
{
	RT2870_TIMER_ENTRY *pQNode = NULL, *pQTail;
	unsigned long irqFlags;


	RTMP_IRQ_LOCK(&pAd->TimerQLock, irqFlags);
	if (pAd->TimerQ.status & RT2870_THREAD_CAN_DO_INSERT)
	{
		if(pAd->TimerQ.pQPollFreeList)
		{
			pQNode = pAd->TimerQ.pQPollFreeList;
			pAd->TimerQ.pQPollFreeList = pQNode->pNext;

			pQNode->pRaTimer = pTimer;
			pQNode->pNext = NULL;

			pQTail = pAd->TimerQ.pQTail;
			if (pAd->TimerQ.pQTail != NULL)
				pQTail->pNext = pQNode;
			pAd->TimerQ.pQTail = pQNode;
			if (pAd->TimerQ.pQHead == NULL)
				pAd->TimerQ.pQHead = pQNode;
		}
		RTMP_IRQ_UNLOCK(&pAd->TimerQLock, irqFlags);

		if (pQNode)
			up(&pAd->RTUSBTimer_semaphore);
			//wake_up(&timerWaitQ);
	}
	else
	{
		RTMP_IRQ_UNLOCK(&pAd->TimerQLock, irqFlags);
	}
	return pQNode;
}


BOOLEAN RT2870_TimerQ_Remove(
	IN RTMP_ADAPTER *pAd, 
	IN RALINK_TIMER_STRUCT *pTimer)
{
	RT2870_TIMER_ENTRY *pNode, *pPrev = NULL;
	unsigned long irqFlags;

	RTMP_IRQ_LOCK(&pAd->TimerQLock, irqFlags);
	if (pAd->TimerQ.status >= RT2870_THREAD_INITED)
	{
		pNode = pAd->TimerQ.pQHead;
		while (pNode)
		{
			if (pNode->pRaTimer == pTimer)
				break;
			pPrev = pNode;
			pNode = pNode->pNext;
		}

		// Now move it to freeList queue.
		if (pNode)
		{	
			if (pNode == pAd->TimerQ.pQHead)
				pAd->TimerQ.pQHead = pNode->pNext;
			if (pNode == pAd->TimerQ.pQTail)
				pAd->TimerQ.pQTail = pPrev;
			if (pPrev != NULL)
				pPrev->pNext = pNode->pNext;
			
			// return this queue entry to timerQFreeList.
			pNode->pNext = pAd->TimerQ.pQPollFreeList;
			pAd->TimerQ.pQPollFreeList = pNode;
		}
	}
	RTMP_IRQ_UNLOCK(&pAd->TimerQLock, irqFlags);
			
	return TRUE;
}


void RT2870_TimerQ_Exit(RTMP_ADAPTER *pAd)
{
	RT2870_TIMER_ENTRY *pTimerQ;
	unsigned long irqFlags;
	
	RTMP_IRQ_LOCK(&pAd->TimerQLock, irqFlags);
	while (pAd->TimerQ.pQHead)
	{
		pTimerQ = pAd->TimerQ.pQHead;
		pAd->TimerQ.pQHead = pTimerQ->pNext;
		// remove the timeQ
	}
	pAd->TimerQ.pQPollFreeList = NULL;
	os_free_mem(pAd, pAd->TimerQ.pTimerQPoll);
	pAd->TimerQ.pQTail = NULL;
	pAd->TimerQ.pQHead = NULL;
	pAd->TimerQ.status = RT2870_THREAD_STOPED;
	RTMP_IRQ_UNLOCK(&pAd->TimerQLock, irqFlags);
	
}


void RT2870_TimerQ_Init(RTMP_ADAPTER *pAd)
{
	int 	i;
	RT2870_TIMER_ENTRY *pQNode, *pEntry;
	unsigned long irqFlags;
	
	NdisAllocateSpinLock(&pAd->TimerQLock);

	RTMP_IRQ_LOCK(&pAd->TimerQLock, irqFlags);
	NdisZeroMemory(&pAd->TimerQ, sizeof(pAd->TimerQ));
	//InterlockedExchange(&pAd->TimerQ.count, 0);
	
	/* Initialise the wait q head */
	//init_waitqueue_head(&timerWaitQ);

	os_alloc_mem(pAd, &pAd->TimerQ.pTimerQPoll, sizeof(RT2870_TIMER_ENTRY) * TIMER_QUEUE_SIZE_MAX);
	if (pAd->TimerQ.pTimerQPoll)
	{
		pEntry = NULL;
		pQNode = (RT2870_TIMER_ENTRY *)pAd->TimerQ.pTimerQPoll;
		for (i = 0 ;i <TIMER_QUEUE_SIZE_MAX; i++)
		{
			pQNode->pNext = pEntry;
			pEntry = pQNode;
			pQNode++;
		}
		pAd->TimerQ.pQPollFreeList = pEntry;
		pAd->TimerQ.pQHead = NULL;
		pAd->TimerQ.pQTail = NULL;
		pAd->TimerQ.status = RT2870_THREAD_INITED;
	}
	RTMP_IRQ_UNLOCK(&pAd->TimerQLock, irqFlags);
}


VOID RT2870_WatchDog(IN RTMP_ADAPTER *pAd)
{
	PHT_TX_CONTEXT		pHTTXContext;
	int 					idx;
	ULONG				irqFlags;
	PURB		   		pUrb;
	BOOLEAN				needDumpSeq = FALSE;
	UINT32          	MACValue;


	idx = 0;
	RTMP_IO_READ32(pAd, TXRXQ_PCNT, &MACValue);
	if ((MACValue & 0xff) !=0 )
	{
		DBGPRINT(RT_DEBUG_TRACE, ("TX QUEUE 0 Not EMPTY(Value=0x%0x). !!!!!!!!!!!!!!!\n", MACValue));
		RTMP_IO_WRITE32(pAd, PBF_CFG, 0xf40012);
		while((MACValue &0xff) != 0 && (idx++ < 10))
		{
		        RTMP_IO_READ32(pAd, TXRXQ_PCNT, &MACValue);
		        NdisMSleep(1);
		}
		RTMP_IO_WRITE32(pAd, PBF_CFG, 0xf40006);
	}

//PS packets use HCCA queue when dequeue from PS unicast queue (WiFi WPA2 MA9_DT1 for Marvell B STA)
	
	if (pAd->watchDogRxOverFlowCnt >= 2)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Maybe the Rx Bulk-In hanged! Cancel the pending Rx bulks request!\n"));
		if ((!RTMP_TEST_FLAG(pAd, (fRTMP_ADAPTER_RESET_IN_PROGRESS |
									fRTMP_ADAPTER_BULKIN_RESET |
									fRTMP_ADAPTER_HALT_IN_PROGRESS |
									fRTMP_ADAPTER_NIC_NOT_EXIST))))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Call CMDTHREAD_RESET_BULK_IN to cancel the pending Rx Bulk!\n"));
			RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET);
			RTUSBEnqueueInternalCmd(pAd, CMDTHREAD_RESET_BULK_IN, NULL, 0);
			needDumpSeq = TRUE;
		}
		pAd->watchDogRxOverFlowCnt = 0;
	}


	for (idx = 0; idx < NUM_OF_TX_RING; idx++)
	{
		pUrb = NULL;
		
		RTMP_IRQ_LOCK(&pAd->BulkOutLock[idx], irqFlags);
		if ((pAd->BulkOutPending[idx] == TRUE) && pAd->watchDogTxPendingCnt)
		{
			pAd->watchDogTxPendingCnt[idx]++;

			if ((pAd->watchDogTxPendingCnt[idx] > 2) && 
				 (!RTMP_TEST_FLAG(pAd, (fRTMP_ADAPTER_RESET_IN_PROGRESS | fRTMP_ADAPTER_HALT_IN_PROGRESS | fRTMP_ADAPTER_NIC_NOT_EXIST | fRTMP_ADAPTER_BULKOUT_RESET)))
				)
			{
				// FIXME: Following code just support single bulk out. If you wanna support multiple bulk out. Modify it!
				pHTTXContext = (PHT_TX_CONTEXT)(&pAd->TxContext[idx]);
				if (pHTTXContext->IRPPending)
				{	// Check TxContext.
					pUrb = pHTTXContext->pUrb;
				}
				else if (idx == MGMTPIPEIDX)
				{
					PTX_CONTEXT pMLMEContext, pNULLContext, pPsPollContext;
					
					//Check MgmtContext.
					pMLMEContext = (PTX_CONTEXT)(pAd->MgmtRing.Cell[pAd->MgmtRing.TxDmaIdx].AllocVa);
					pPsPollContext = (PTX_CONTEXT)(&pAd->PsPollContext);
					pNULLContext = (PTX_CONTEXT)(&pAd->NullContext);
					
					if (pMLMEContext->IRPPending)
					{
						ASSERT(pMLMEContext->IRPPending);
						pUrb = pMLMEContext->pUrb;
					}
					else if (pNULLContext->IRPPending)
					{	
						ASSERT(pNULLContext->IRPPending);
						pUrb = pNULLContext->pUrb;
					}
					else if (pPsPollContext->IRPPending)
					{	
						ASSERT(pPsPollContext->IRPPending);
						pUrb = pPsPollContext->pUrb;
					}
				}
				
				RTMP_IRQ_UNLOCK(&pAd->BulkOutLock[idx], irqFlags);
				
				DBGPRINT(RT_DEBUG_TRACE, ("Maybe the Tx Bulk-Out hanged! Cancel the pending Tx bulks request of idx(%d)!\n", idx));
				if (pUrb)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("Unlink the pending URB!\n"));
					// unlink it now
					RTUSB_UNLINK_URB(pUrb);
					// Sleep 200 microseconds to give cancellation time to work
					RTMPusecDelay(200);
					needDumpSeq = TRUE;
				}
				else
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Unkonw bulkOut URB maybe hanged!!!!!!!!!!!!\n"));
				}
			}
			else
			{
				RTMP_IRQ_UNLOCK(&pAd->BulkOutLock[idx], irqFlags);
			}
		}
		else
		{
			RTMP_IRQ_UNLOCK(&pAd->BulkOutLock[idx], irqFlags);
		}
	}

}

/*
========================================================================
Routine Description:
    Release allocated resources.

Arguments:
    *dev				Point to the PCI or USB device
	pAd					driver control block pointer

Return Value:
    None

Note:
========================================================================
*/
static void _rtusb_disconnect(struct usb_device *dev, PRTMP_ADAPTER pAd)
{
	struct net_device	*net_dev = NULL;


	DBGPRINT(RT_DEBUG_ERROR, ("rtusb_disconnect: unregister usbnet usb-%s-%s\n",
				dev->bus->bus_name, dev->devpath));
	if (!pAd)
	{
#ifdef MULTIPLE_CARD_SUPPORT
		if ((pAd->MC_RowID >= 0) && (pAd->MC_RowID <= MAX_NUM_OF_MULTIPLE_CARD))
			MC_CardUsed[pAd->MC_RowID] = 0; // not clear MAC address
#endif // MULTIPLE_CARD_SUPPORT //

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* kernel 2.4 series */
		while(MOD_IN_USE > 0)
		{
			MOD_DEC_USE_COUNT;
		}
#else
		usb_put_dev(dev);
#endif // LINUX_VERSION_CODE //

		printk("rtusb_disconnect: pAd == NULL!\n");
		return;
	}
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST);


#ifdef WSC_INCLUDED
	// This kernel thread init in the probe fucntion, so kill it when do remove module.
    if (pAd->write_dat_file_pid >= 0)
    {
        int ret;
        pAd->time_to_die = 1;
        up(&(pAd->write_dat_file_semaphore));
        wmb(); // need to check
		ret = kill_proc (pAd->write_dat_file_pid, SIGTERM, 1);
		if (ret)
		{
			printk (KERN_ERR "%s: unable to signal thread\n", pAd->net_dev->name);
			return;
		}
        wait_for_completion (&pAd->write_dat_file_notify);
    }

    if (pAd->pHmacData)
        kfree(pAd->pHmacData);
#endif // WSC_INCLUDED //

	// for debug, wait to show some messages to /proc system
	udelay(1);


	
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
	// remove all AP-client virtual interfaces.
	RT28xx_ApCli_Remove(pAd);
#endif // APCLI_SUPPORT //

#ifdef WDS_SUPPORT
	// remove all WDS virtual interfaces.
	RT28xx_WDS_Remove(pAd);
#endif // WDS_SUPPORT //

#ifdef MBSS_SUPPORT
    RT28xx_MBSS_Remove(pAd);
#endif // MBSS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

	net_dev = pAd->net_dev;
	if (pAd->net_dev != NULL)
	{
		printk("rtusb_disconnect: unregister_netdev(), dev->name=%s!\n", net_dev->name);
		unregister_netdev (pAd->net_dev);
	}
	udelay(1);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* kernel 2.4 series */
#else
	flush_scheduled_work();
#endif // LINUX_VERSION_CODE //
	udelay(1);

	// free net_device memory
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* kernel 2.4 series */
	kfree(net_dev);
#else
	free_netdev(net_dev);
#endif // LINUX_VERSION_CODE //

	// free adapter memory
	RTMPFreeAdapter(pAd);

	// release a use of the usb device structure
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* kernel 2.4 series */
	while(MOD_IN_USE > 0)
	{
		MOD_DEC_USE_COUNT;
	}
#else
	usb_put_dev(dev);
#endif // LINUX_VERSION_CODE //
	udelay(1);

	DBGPRINT(RT_DEBUG_ERROR, (" RTUSB disconnect successfully\n"));
}


/*
========================================================================
Routine Description:
    Probe RT28XX chipset.

Arguments:
    *dev				Point to the PCI or USB device
	interface			
	*id_table			Point to the PCI or USB device ID

Return Value:
    None

Note:
========================================================================
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* kernel 2.4 series */
static void *rtusb_probe(struct usb_device *dev, UINT interface,
						const struct usb_device_id *id)
{
    printk("[kernel]rtusb_probe:");
    printk("vendor: %x Product %x\n", id->idVendor, id->idProduct);
	PRTMP_ADAPTER pAd;
	rt28xx_probe((void *)dev, (void *)id, interface, &pAd);
	return (void *)pAd;
}

//Disconnect function is called within exit routine
static void rtusb_disconnect(struct usb_device *dev, void *ptr)
{
	_rtusb_disconnect(dev, ((PRTMP_ADAPTER)ptr));
}

#else	/* kernel 2.6 series */
static int rtusb_probe (struct usb_interface *intf, const struct usb_device_id *id)
{	
	PRTMP_ADAPTER pAd;
	return (int)rt28xx_probe((void *)intf, (void *)id, 0, &pAd);
}


static void rtusb_disconnect(struct usb_interface *intf)
{
	struct usb_device   *dev = interface_to_usbdev(intf);
	PRTMP_ADAPTER       pAd;

	pAd = usb_get_intfdata(intf);
	usb_set_intfdata(intf, NULL);	

	_rtusb_disconnect(dev, pAd);
}
#endif // LINUX_VERSION_CODE //


/*
========================================================================
Routine Description:
    Close kernel threads.

Arguments:
	*pAd				the raxx interface data pointer

Return Value:
    NONE

Note:
========================================================================
*/
VOID RT28xxThreadTerminate(
	IN RTMP_ADAPTER *pAd)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT			ret;


	// Sleep 50 milliseconds so pending io might finish normally
	RTMPusecDelay(50000);

	// We want to wait until all pending receives and sends to the
	// device object. We cancel any
	// irps. Wait until sends and receives have stopped.
	RTUSBCancelPendingIRPs(pAd);

	// Terminate Threads
#if 0 //Add by Zero:Sep.04.2008 Move to back. Fix Amazon_SE interface down hang bug.
	if (pObj->TimerQThr_pid >= 0) 
	{
		POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;

		printk("Terminate the TimerQThr_pid=%d!\n", pObj->TimerQThr_pid);
		mb();
		pAd->TimerFunc_kill = 1;
		mb();
		ret = kill_proc(pObj->TimerQThr_pid, SIGTERM, 1);
		if (ret)
		{
			printk(KERN_WARNING "%s: unable to stop TimerQThread, pid=%d, ret=%d!\n", 
					pAd->net_dev->name, pObj->TimerQThr_pid, ret);
		}
		else 
		{
			wait_for_completion(&pAd->TimerQComplete);
			pObj->TimerQThr_pid = -1;
		}
	}
#endif	
	if (pObj->MLMEThr_pid >= 0) 
	{
		printk("Terminate the MLMEThr_pid=%d!\n", pObj->MLMEThr_pid);
		mb();
		pAd->mlme_kill = 1;
		//RT28XX_MLME_HANDLER(pAd);
		mb();
		ret = kill_proc (pObj->MLMEThr_pid, SIGTERM, 1);
		if (ret) 
		{
			printk (KERN_WARNING "%s: unable to Mlme thread, pid=%d, ret=%d!\n",
					pAd->net_dev->name, pObj->MLMEThr_pid, ret);
		} 
		else
		{
			//wait_for_completion (&pAd->notify);
			wait_for_completion (&pAd->mlmeComplete);
			pObj->MLMEThr_pid = -1;
		}
	}

	if (pObj->RTUSBCmdThr_pid >= 0) 
	{
		printk("Terminate the RTUSBCmdThr_pid=%d!\n", pObj->RTUSBCmdThr_pid);
		mb();
		NdisAcquireSpinLock(&pAd->CmdQLock);
		pAd->CmdQ.CmdQState = RT2870_THREAD_STOPED;
		NdisReleaseSpinLock(&pAd->CmdQLock);
		mb();
		//RTUSBCMDUp(pAd);
		ret = kill_proc(pObj->RTUSBCmdThr_pid, SIGTERM, 1);
		if (ret) 
		{
			printk(KERN_WARNING "%s: unable to RTUSBCmd thread, pid=%d, ret=%d!\n",
					pAd->net_dev->name, pObj->RTUSBCmdThr_pid, ret);
		}
		else
		{
			//wait_for_completion (&pAd->notify);
			wait_for_completion (&pAd->CmdQComplete);
			pObj->RTUSBCmdThr_pid = -1;
		}
	}
#if 1 //Add by Zero:Sep.04.2008  Fix Amazon_SE interface down hang bug.
	if (pObj->TimerQThr_pid >= 0) 
	{
		POS_COOKIE pObj = (POS_COOKIE)pAd->OS_Cookie;

		printk("Terminate the TimerQThr_pid=%d!\n", pObj->TimerQThr_pid);
		mb();
		pAd->TimerFunc_kill = 1;
		mb();
		ret = kill_proc(pObj->TimerQThr_pid, SIGTERM, 1);
		if (ret)
		{
			printk(KERN_WARNING "%s: unable to stop TimerQThread, pid=%d, ret=%d!\n", 
					pAd->net_dev->name, pObj->TimerQThr_pid, ret);
		}
		else 
		{
			printk("wait_for_completion TimerQThr\n");
			wait_for_completion(&pAd->TimerQComplete);
			pObj->TimerQThr_pid = -1;
		}
	}
#endif
	// Kill tasklets
	pAd->mlme_kill = 0;
	pAd->CmdQ.CmdQState = RT2870_THREAD_UNKNOWN;
	pAd->TimerFunc_kill = 0;
}


void kill_thread_task(IN PRTMP_ADAPTER pAd)
{
	POS_COOKIE pObj;

	pObj = (POS_COOKIE) pAd->OS_Cookie;

	tasklet_kill(&pObj->rx_done_task);
	tasklet_kill(&pObj->mgmt_dma_done_task);
	tasklet_kill(&pObj->ac0_dma_done_task);
	tasklet_kill(&pObj->ac1_dma_done_task);
	tasklet_kill(&pObj->ac2_dma_done_task);
	tasklet_kill(&pObj->ac3_dma_done_task);
//	tasklet_kill(&pObj->hcca_dma_done_task);
	tasklet_kill(&pObj->tbtt_task);

}


/*
========================================================================
Routine Description:
    Check the chipset vendor/product ID.

Arguments:
    _dev_p				Point to the PCI or USB device

Return Value:
    TRUE				Check ok
	FALSE				Check fail

Note:
========================================================================
*/
BOOLEAN RT28XXChipsetCheck(
	IN void *_dev_p)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* kernel 2.4 series */
	struct usb_device *dev_p = (struct usb_device *)_dev_p;
#else
	struct usb_interface *intf = (struct usb_interface *)_dev_p;
	struct usb_device *dev_p = interface_to_usbdev(intf);
#endif // LINUX_VERSION_CODE //
	UINT32 i;


	for(i=0; i<rtusb_usb_id_len; i++)
	{
		if (dev_p->descriptor.idVendor == rtusb_usb_id[i].idVendor &&
			dev_p->descriptor.idProduct == rtusb_usb_id[i].idProduct)
		{
			printk("rt2870: idVendor = 0x%x, idProduct = 0x%x\n",
					dev_p->descriptor.idVendor, dev_p->descriptor.idProduct);
			break;
		}
	}

	if (i == rtusb_usb_id_len) 
	{
		printk("rt2870: Error! Device Descriptor not matching!\n");
		return FALSE;
	}

	return TRUE;
}


/*
========================================================================
Routine Description:
    Init net device structure.

Arguments:
    _dev_p				Point to the PCI or USB device
    *net_dev			Point to the net device
	*pAd				the raxx interface data pointer

Return Value:
    TRUE				Init ok
	FALSE				Init fail

Note:
========================================================================
*/
BOOLEAN RT28XXNetDevInit(
	IN void 				*_dev_p,
	IN struct  net_device	*net_dev,
	IN RTMP_ADAPTER 		*pAd)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* kernel 2.4 series */
	struct usb_device *dev_p = (struct usb_device *)_dev_p;
#else
	struct usb_interface *intf = (struct usb_interface *)_dev_p;
	struct usb_device *dev_p = interface_to_usbdev(intf);
#endif // LINUX_VERSION_CODE //


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	/* kernel 2.4 series */
	pAd->config = dev_p->config;
#else
	pAd->config = &dev_p->config->desc;
#endif // LINUX_VERSION_CODE //
	return TRUE;
}


/*
========================================================================
Routine Description:
    Init net device structure.

Arguments:
    _dev_p				Point to the PCI or USB device
	*pAd				the raxx interface data pointer

Return Value:
    TRUE				Config ok
	FALSE				Config fail

Note:
========================================================================
*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
BOOLEAN RT28XXProbePostConfig(
	IN void 				*_dev_p,
	IN RTMP_ADAPTER 		*pAd,
	IN INT32				interface)
{
	struct usb_device *dev_p = (struct usb_device *)_dev_p;
	struct usb_interface *intf;
	struct usb_interface_descriptor *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	ULONG BulkOutIdx;
	UINT32 i;


	/* get the active interface descriptor */
	intf = &dev_p->actconfig->interface[interface];
	iface_desc = &intf->altsetting[0];

	/* get # of enpoints */
	pAd->NumberOfPipes = iface_desc->bNumEndpoints;
	DBGPRINT(RT_DEBUG_TRACE, ("NumEndpoints=%d\n", iface_desc->bNumEndpoints));		 

	/* Configure Pipes */
	endpoint = &iface_desc->endpoint[0];
	BulkOutIdx = 0;

	for(i=0; i<pAd->NumberOfPipes; i++)
	{
		if ((endpoint[i].bmAttributes == USB_ENDPOINT_XFER_BULK) && 
			((endpoint[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN))
		{
			pAd->BulkInEpAddr = endpoint[i].bEndpointAddress;
			pAd->BulkInMaxPacketSize = endpoint[i].wMaxPacketSize;

			DBGPRINT_RAW(RT_DEBUG_TRACE,
				("BULK IN MaximumPacketSize = %d\n", pAd->BulkInMaxPacketSize));
			DBGPRINT_RAW(RT_DEBUG_TRACE,
				("EP address = 0x%2x  \n", endpoint[i].bEndpointAddress));
		}
		else if ((endpoint[i].bmAttributes == USB_ENDPOINT_XFER_BULK) && 
				((endpoint[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT))
		{
			// There are 6 bulk out EP. EP6 highest priority.
			// EP1-4 is EDCA.  EP5 is HCCA.
			pAd->BulkOutEpAddr[BulkOutIdx++] = endpoint[i].bEndpointAddress;
			pAd->BulkOutMaxPacketSize = endpoint[i].wMaxPacketSize;

			DBGPRINT_RAW(RT_DEBUG_TRACE,
				("BULK OUT MaximumPacketSize = %d\n", pAd->BulkOutMaxPacketSize));
			DBGPRINT_RAW(RT_DEBUG_TRACE,
				("EP address = 0x%2x  \n", endpoint[i].bEndpointAddress));
		}
	}

	if (!(pAd->BulkInEpAddr && pAd->BulkOutEpAddr[0])) 
	{
		printk("Could not find both bulk-in and bulk-out endpoints\n");
		return FALSE;
	}

	return TRUE;
}

#else
BOOLEAN RT28XXProbePostConfig(
	IN void 				*_dev_p,
	IN RTMP_ADAPTER 		*pAd,
	IN INT32				interface)
{
	struct usb_interface *intf = (struct usb_interface *)_dev_p;
	struct usb_host_interface *iface_desc;
	ULONG BulkOutIdx;
	UINT32 i;


	/* get the active interface descriptor */
	iface_desc = intf->cur_altsetting;

	/* get # of enpoints  */
	pAd->NumberOfPipes = iface_desc->desc.bNumEndpoints;
	DBGPRINT(RT_DEBUG_TRACE,
			("NumEndpoints=%d\n", iface_desc->desc.bNumEndpoints));		  

	/* Configure Pipes */
	BulkOutIdx = 0;

	for(i=0; i<pAd->NumberOfPipes; i++)
	{ 
		if ((iface_desc->endpoint[i].desc.bmAttributes == 
				USB_ENDPOINT_XFER_BULK) && 
			((iface_desc->endpoint[i].desc.bEndpointAddress &
				USB_ENDPOINT_DIR_MASK) == USB_DIR_IN))
		{
			pAd->BulkInEpAddr = iface_desc->endpoint[i].desc.bEndpointAddress;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
			pAd->BulkInMaxPacketSize = le2cpu16(iface_desc->endpoint[i].desc.wMaxPacketSize);
#else
			pAd->BulkInMaxPacketSize = iface_desc->endpoint[i].desc.wMaxPacketSize;
#endif // LINUX_VERSION_CODE //

			DBGPRINT_RAW(RT_DEBUG_TRACE,
				("BULK IN MaximumPacketSize = %d\n", pAd->BulkInMaxPacketSize));
			DBGPRINT_RAW(RT_DEBUG_TRACE,
				("EP address = 0x%2x\n", iface_desc->endpoint[i].desc.bEndpointAddress));
		}
		else if ((iface_desc->endpoint[i].desc.bmAttributes ==
					USB_ENDPOINT_XFER_BULK) && 
				((iface_desc->endpoint[i].desc.bEndpointAddress &
					USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT))
		{
			// there are 6 bulk out EP. EP6 highest priority.
			// EP1-4 is EDCA.  EP5 is HCCA.
			pAd->BulkOutEpAddr[BulkOutIdx++] = iface_desc->endpoint[i].desc.bEndpointAddress;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
			pAd->BulkOutMaxPacketSize = le2cpu16(iface_desc->endpoint[i].desc.wMaxPacketSize);
#else
			pAd->BulkOutMaxPacketSize = iface_desc->endpoint[i].desc.wMaxPacketSize;
#endif // LINUX_VERSION_CODE //

			DBGPRINT_RAW(RT_DEBUG_TRACE,
				("BULK OUT MaximumPacketSize = %d\n", pAd->BulkOutMaxPacketSize));
			DBGPRINT_RAW(RT_DEBUG_TRACE,
				("EP address = 0x%2x  \n", iface_desc->endpoint[i].desc.bEndpointAddress));
		}
	}

	if (!(pAd->BulkInEpAddr && pAd->BulkOutEpAddr[0])) 
	{
		printk("%s: Could not find both bulk-in and bulk-out endpoints\n", __FUNCTION__);
		return FALSE;
	}

	return TRUE;
}
#endif // LINUX_VERSION_CODE //


/*
========================================================================
Routine Description:
    Disable DMA.

Arguments:
	*pAd				the raxx interface data pointer

Return Value:
	None

Note:
========================================================================
*/
VOID RT28XXDMADisable(
	IN RTMP_ADAPTER 		*pAd)
{
	// no use
}



/*
========================================================================
Routine Description:
    Enable DMA.

Arguments:
	*pAd				the raxx interface data pointer

Return Value:
	None

Note:
========================================================================
*/
VOID RT28XXDMAEnable(
	IN RTMP_ADAPTER 		*pAd)
{
	WPDMA_GLO_CFG_STRUC	GloCfg;
	USB_DMA_CFG_STRUC	UsbCfg;
	int					i = 0;
	
	
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x4);
	do
	{
		RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &GloCfg.word);
		if ((GloCfg.field.TxDMABusy == 0)  && (GloCfg.field.RxDMABusy == 0))
			break;
		
		DBGPRINT(RT_DEBUG_TRACE, ("==>  DMABusy\n"));
		RTMPusecDelay(1000);
		i++;
	}while ( i <200);


	RTMPusecDelay(50);
	GloCfg.field.EnTXWriteBackDDONE = 1;
	GloCfg.field.EnableRxDMA = 1;
	GloCfg.field.EnableTxDMA = 1;
	DBGPRINT(RT_DEBUG_TRACE, ("<== WRITE DMA offset 0x208 = 0x%x\n", GloCfg.word));	
	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, GloCfg.word);
	
	UsbCfg.word = 0;
	UsbCfg.field.phyclear = 0;
	/* usb version is 1.1,do not use bulk in aggregation */
	if (pAd->BulkInMaxPacketSize == 512)
			UsbCfg.field.RxBulkAggEn = 1;
	/* for last packet, PBF might use more than limited, so minus 2 to prevent from error */
	UsbCfg.field.RxBulkAggLmt = (MAX_RXBULK_SIZE /1024)-3;
	UsbCfg.field.RxBulkAggTOut = 0x80; /* 2006-10-18 */
	UsbCfg.field.RxBulkEn = 1;
	UsbCfg.field.TxBulkEn = 1;

	RTUSBWriteMACRegister(pAd, USB_DMA_CFG, UsbCfg.word);

}

/*
========================================================================
Routine Description:
    Write Beacon buffer to Asic.

Arguments:
	*pAd				the raxx interface data pointer

Return Value:
	None

Note:
========================================================================
*/
VOID RT28xx_UpdateBeaconToAsic(
	IN RTMP_ADAPTER		*pAd,
	IN INT				apidx,
	IN ULONG			FrameLen,
	IN ULONG			UpdatePos)
{
	PUCHAR        	pBeaconFrame = NULL;
	UCHAR  			*ptr;
	UINT  			i, padding;
	BEACON_SYNC_STRUCT	*pBeaconSync = pAd->CommonCfg.pBeaconSync;
	UINT32			longValue;
//	USHORT			shortValue;
	BOOLEAN			bBcnReq = FALSE;
	UCHAR			bcn_idx = 0;

#ifdef CONFIG_AP_SUPPORT
	if (apidx < pAd->ApCfg.BssidNum)
	{
		bcn_idx = pAd->ApCfg.MBSSID[apidx].BcnBufIdx;
		pBeaconFrame = pAd->ApCfg.MBSSID[apidx].BeaconBuf;
		bBcnReq = BeaconTransmitRequired(pAd, apidx);
	}
#endif // CONFIG_AP_SUPPORT //

	if (pBeaconFrame == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR,("pBeaconFrame is NULL!\n"));
		return;
	}

	if (pBeaconSync == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR,("pBeaconSync is NULL!\n"));
		return;
	}
	
	//if ((pAd->WdsTab.Mode == WDS_BRIDGE_MODE) || 
	//	((pAd->ApCfg.MBSSID[apidx].MSSIDDev == NULL) || !(pAd->ApCfg.MBSSID[apidx].MSSIDDev->flags & IFF_UP))
	//	)
	if (bBcnReq == FALSE)
	{
		/* when the ra interface is down, do not send its beacon frame */
		/* clear all zero */
		for(i=0; i<TXWI_SIZE; i+=4) {
			RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[bcn_idx] + i, 0x00);
		}
		pBeaconSync->BeaconBitMap &= (~(BEACON_BITMAP_MASK & (1 << bcn_idx)));
		NdisZeroMemory(pBeaconSync->BeaconTxWI[bcn_idx], TXWI_SIZE);
	}
	else
	{
		ptr = (PUCHAR)&pAd->BeaconTxWI;
#ifdef RT_BIG_ENDIAN
		RTMPWIEndianChange(ptr, TYPE_TXWI);
#endif
		if (NdisEqualMemory(pBeaconSync->BeaconTxWI[bcn_idx], &pAd->BeaconTxWI, TXWI_SIZE) == FALSE)
		{	// If BeaconTxWI changed, we need to rewrite the TxWI for the Beacon frames.
			pBeaconSync->BeaconBitMap &= (~(BEACON_BITMAP_MASK & (1 << bcn_idx)));
			NdisMoveMemory(pBeaconSync->BeaconTxWI[bcn_idx], &pAd->BeaconTxWI, TXWI_SIZE);
		}
		
		if ((pBeaconSync->BeaconBitMap & (1 << bcn_idx)) != (1 << bcn_idx))
		{
			for (i=0; i<TXWI_SIZE; i+=4)  // 16-byte TXWI field
			{
				longValue =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
				RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[bcn_idx] + i, longValue);
				ptr += 4;
			}
		}

		ptr = pBeaconSync->BeaconBuf[bcn_idx];
		padding = (FrameLen & 0x01);
		NdisZeroMemory((PUCHAR)(pBeaconFrame + FrameLen), padding);
		FrameLen += padding;
		for (i = 0 ; i < FrameLen /*HW_BEACON_OFFSET*/; i += 2)
		{
			if (NdisEqualMemory(ptr, pBeaconFrame, 2) == FALSE)
			{
				NdisMoveMemory(ptr, pBeaconFrame, 2);
				//shortValue = *ptr + (*(ptr+1)<<8);
				//RTMP_IO_WRITE8(pAd, pAd->BeaconOffset[bcn_idx] + TXWI_SIZE + i, shortValue);
				RTUSBMultiWrite(pAd, pAd->BeaconOffset[bcn_idx] + TXWI_SIZE + i, ptr, 2);
			}
			ptr +=2;
			pBeaconFrame += 2;
		}

		pBeaconSync->BeaconBitMap |= (1 << bcn_idx);

		// For AP interface, set the DtimBitOn so that we can send Bcast/Mcast frame out after this beacon frame.
#ifdef CONFIG_AP_SUPPORT
		{
			ptr = pAd->ApCfg.MBSSID[apidx].BeaconBuf + pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon;
			if ((*(ptr + 4)) & 0x01)
				pBeaconSync->DtimBitOn |= (1 << apidx);
			else
				pBeaconSync->DtimBitOn &= ~(1 << apidx);
		}
#endif // CONFIG_AP_SUPPORT //
	}
	
}

#ifdef CONFIG_AP_SUPPORT
/*
    ========================================================================
    Routine Description:
        For device work as AP mode but didn't have TBTT interrupt event, we need a mechanism 
        to update the beacon context in each Beacon interval. Here we use a periodical timer 
        to simulate the TBTT interrupt to handle the beacon context update.
        
    Arguments:
        SystemSpecific1         - Not used.
        FunctionContext         - Pointer to our Adapter context.
        SystemSpecific2         - Not used.
        SystemSpecific3         - Not used.
        
    Return Value:
        None
        
    ========================================================================
*/
#if 0
static int BeaconCount = 0;
static LARGE_INTEGER expectedTime;

#define BEACON_ADJUST_PERIOD		3

BOOLEAN getDeltaTime(LARGE_INTEGER src, LARGE_INTEGER base, LARGE_INTEGER *delta)
{
	BOOLEAN			positive;

	delta->HighPart = base.HighPart - src.HighPart;
		
	if (base.HighPart > src.HighPart)
	{
		delta->HighPart = base.HighPart - src.HighPart;
		if (base.LowPart >= src.LowPart)
			delta->LowPart = base.LowPart - src.LowPart;
		else
		{
			delta->LowPart = (0xffffffff - src.LowPart) + base.LowPart;
			delta->HighPart--;
		}
		positive = FALSE;
	}
	else if (base.HighPart == src.HighPart)
	{
		if (base.LowPart >= src.LowPart)
		{
			delta->LowPart = base.LowPart - src.LowPart;
			positive = FALSE;
		}
		else
		{
			delta->LowPart = src.LowPart - base.LowPart;
			positive = TRUE;
		}
	}
	else 
	{
		delta->HighPart = src.HighPart - base.HighPart;
		if (src.LowPart >= base.LowPart)
			delta->LowPart = src.LowPart - base.LowPart;
		else
		{
			delta->LowPart = (0xffffffff - base.LowPart) + src.LowPart;
			delta->HighPart--;
		}
		positive = TRUE;
	}

	return positive;
	
}


VOID APBeaconUpdateExec(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3)
{
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER)FunctionContext;
	LARGE_INTEGER	tsfTime_b, tsfTime_a, deltaTime_exp, deltaTime_ab;
	UINT32			delta, remain;
	BOOLEAN			positive;


	ReSyncBeaconTime(pAd);

	if (pAd->ApCfg.DtimCount == 0)
	{
		POS_COOKIE pObj;
		
		pObj = (POS_COOKIE) pAd->OS_Cookie;
		tasklet_hi_schedule(&pObj->tbtt_task);
	}


	RTMP_IO_READ32(pAd, TSF_TIMER_DW0, &tsfTime_b.LowPart);
	RTMP_IO_READ32(pAd, TSF_TIMER_DW1, &tsfTime_b.HighPart);

	APUpdateAllBeaconFrame(pAd);

	RTMP_IO_READ32(pAd, TSF_TIMER_DW0, &tsfTime_a.LowPart);
	RTMP_IO_READ32(pAd, TSF_TIMER_DW1, &tsfTime_a.HighPart);
	
	getDeltaTime(tsfTime_b, tsfTime_a, &deltaTime_ab);
	if (deltaTime_ab.LowPart > 5000)
		printk("Alerting_A_B(%d): Update time larger than 5000(=%d)!\n", BeaconCount, deltaTime_ab.LowPart);
	
	positive=getDeltaTime(tsfTime_b, expectedTime, &deltaTime_exp);
	if (!positive)
	{
		printk("Alerting_EXP_B(%d): timer fire time is early than expected time! tsfTime=%d%d, exp=%d%d, delta=%d%d!\n", 
				BeaconCount, tsfTime_b.HighPart, tsfTime_b.LowPart, expectedTime.HighPart, expectedTime.LowPart, 
				deltaTime_exp.HighPart,deltaTime_exp.LowPart);
	}
	else
	{
		if (deltaTime_exp.LowPart > (pAd->CommonCfg.BeaconPeriod << 10))
			printk("Alerting_EX_B(%d): More than one beacon period was missed before we do update!(delta=%d)!\n", BeaconCount,deltaTime_exp.LowPart);
		else if (deltaTime_exp.LowPart > (pAd->CommonCfg.BeaconPeriod << 10))
			printk("Alerting_EX_B(%d): Update Timer run too fast(delta=%d)??!\n", deltaTime_exp.LowPart);
			
	}
	
	positive=getDeltaTime(tsfTime_a, expectedTime, &deltaTime_exp);
	if (!positive)
	{
		printk("Alerting_EXP_A(%d): timer fire time is early than expected time! tsfTime=%d%d, exp=%d%d, delta=%d%d!\n", 
				BeaconCount, tsfTime_b.HighPart, tsfTime_b.LowPart, expectedTime.HighPart, expectedTime.LowPart, 
				deltaTime_exp.HighPart,deltaTime_exp.LowPart);
	}
	else
	{
		if (deltaTime_exp.LowPart > (pAd->CommonCfg.BeaconPeriod << 10))
			printk("Alerting_EXP_A(%d): More than one beacon period may missed when we do update!(delta=%d)!\n", BeaconCount, deltaTime_exp.LowPart);
	}

#if 0
	printk("BeaconCount=%d, tb=%d%d, ta=%d%d, exp=%d%d, dt_exp=(%c)%d%d, dt_ab=%d%d\n", 
				BeaconCount, tsfTime_b.HighPart, tsfTime_b.LowPart, tsfTime_a.HighPart, tsfTime_a.LowPart, 
				expectedTime.HighPart, expectedTime.LowPart, 
				(positive == TRUE? '+' : '-'),deltaTime_exp.HighPart, deltaTime_exp.LowPart,
				deltaTime_ab.HighPart, deltaTime_ab.LowPart);
#endif

	
	expectedTime.HighPart = tsfTime_a.HighPart;
	remain = tsfTime_a.LowPart % (pAd->CommonCfg.BeaconPeriod << 10);
	delta = (pAd->CommonCfg.BeaconPeriod << 10) - remain;
	expectedTime.LowPart = tsfTime_a.LowPart + delta;

	BeaconCount++;

	pAd->CommonCfg.BeaconUpdateTimer.TimerValue = (delta >> 10) + 20;
#if 0
	printk("Adj:b=%d%d, a=%d%d, exp=%d%d, dc=%d, d=%d, np=%ld\n", 
			tsfTime_b.HighPart, tsfTime_b.LowPart, tsfTime_a.HighPart, tsfTime_a.LowPart, 
			expectedTime.HighPart, expectedTime.LowPart, pAd->ApCfg.DtimCount, delta, pAd->CommonCfg.BeaconUpdateTimer.TimerValue);
#endif	
	if ((pAd->CommonCfg.Channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& (pAd->CommonCfg.RadarDetect.RDMode == RD_SWITCHING_MODE))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("APBeaconUpdateExec::Channel Switching...(%d/%d)\n", pAd->CommonCfg.RadarDetect.CSCount, pAd->CommonCfg.RadarDetect.CSPeriod));
		
		pAd->CommonCfg.RadarDetect.CSCount++;
		if (pAd->CommonCfg.RadarDetect.CSCount >= pAd->CommonCfg.RadarDetect.CSPeriod)
		{
			APStop(pAd);
			APStartUp(pAd);
		}
	}
}
#endif
#endif // CONFIG_AP_SUPPORT //

VOID RT2870_BssBeaconStop(
	IN RTMP_ADAPTER *pAd)
{
	BEACON_SYNC_STRUCT	*pBeaconSync;
	int i, offset;
	BOOLEAN	Cancelled = TRUE;

	pBeaconSync = pAd->CommonCfg.pBeaconSync;
	if (pBeaconSync && pBeaconSync->EnableBeacon)
	{
		INT NumOfBcn;

#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			NumOfBcn = pAd->ApCfg.BssidNum + MAX_MESH_NUM;
		}
#endif // CONFIG_AP_SUPPORT //


		RTMPCancelTimer(&pAd->CommonCfg.BeaconUpdateTimer, &Cancelled);

		for(i=0; i<NumOfBcn; i++)
		{
			NdisZeroMemory(pBeaconSync->BeaconBuf[i], HW_BEACON_OFFSET);
			NdisZeroMemory(pBeaconSync->BeaconTxWI[i], TXWI_SIZE);

			for (offset=0; offset<HW_BEACON_OFFSET; offset+=4)
				RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[i] + offset, 0x00);
			
			pBeaconSync->CapabilityInfoLocationInBeacon[i] = 0;
			pBeaconSync->TimIELocationInBeacon[i] = 0;
		}
		pBeaconSync->BeaconBitMap = 0;
		pBeaconSync->DtimBitOn = 0;
	}
}


VOID RT2870_BssBeaconStart(
	IN RTMP_ADAPTER *pAd)
{
	int apidx;
	BEACON_SYNC_STRUCT	*pBeaconSync;
//	LARGE_INTEGER 	tsfTime, deltaTime;

	pBeaconSync = pAd->CommonCfg.pBeaconSync;
	if (pBeaconSync && pBeaconSync->EnableBeacon)
	{
		INT NumOfBcn;

#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			NumOfBcn = pAd->ApCfg.BssidNum + MAX_MESH_NUM;
		}
#endif // CONFIG_AP_SUPPORT //


		for(apidx=0; apidx<NumOfBcn; apidx++)
		{
			UCHAR CapabilityInfoLocationInBeacon = 0;
			UCHAR TimIELocationInBeacon = 0;
#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			{
				{
					CapabilityInfoLocationInBeacon = pAd->ApCfg.MBSSID[apidx].CapabilityInfoLocationInBeacon;
					TimIELocationInBeacon = pAd->ApCfg.MBSSID[apidx].TimIELocationInBeacon;
				}
			}
#endif // CONFIG_AP_SUPPORT //


			NdisZeroMemory(pBeaconSync->BeaconBuf[apidx], HW_BEACON_OFFSET);
			pBeaconSync->CapabilityInfoLocationInBeacon[apidx] = CapabilityInfoLocationInBeacon;
			pBeaconSync->TimIELocationInBeacon[apidx] = TimIELocationInBeacon;
			NdisZeroMemory(pBeaconSync->BeaconTxWI[apidx], TXWI_SIZE);
		}
		pBeaconSync->BeaconBitMap = 0;
		pBeaconSync->DtimBitOn = 0;
		pAd->CommonCfg.BeaconUpdateTimer.Repeat = TRUE;

		pAd->CommonCfg.BeaconAdjust = 0;
		pAd->CommonCfg.BeaconFactor = 0xffffffff / (pAd->CommonCfg.BeaconPeriod << 10);
		pAd->CommonCfg.BeaconRemain = (0xffffffff % (pAd->CommonCfg.BeaconPeriod << 10)) + 1;
		printk("RT2870_BssBeaconStart:BeaconFactor=%d, BeaconRemain=%d!\n", pAd->CommonCfg.BeaconFactor, pAd->CommonCfg.BeaconRemain);
		RTMPSetTimer(&pAd->CommonCfg.BeaconUpdateTimer, pAd->CommonCfg.BeaconPeriod);

	}
}


VOID RT2870_BssBeaconInit(
	IN RTMP_ADAPTER *pAd)
{
	BEACON_SYNC_STRUCT	*pBeaconSync;
	int i;

	NdisAllocMemory(pAd->CommonCfg.pBeaconSync, sizeof(BEACON_SYNC_STRUCT), MEM_ALLOC_FLAG);
	if (pAd->CommonCfg.pBeaconSync)
	{
		pBeaconSync = pAd->CommonCfg.pBeaconSync;
		NdisZeroMemory(pBeaconSync, sizeof(BEACON_SYNC_STRUCT));
		for(i=0; i < HW_BEACON_MAX_COUNT; i++)
		{
			NdisZeroMemory(pBeaconSync->BeaconBuf[i], HW_BEACON_OFFSET);
			pBeaconSync->CapabilityInfoLocationInBeacon[i] = 0;
			pBeaconSync->TimIELocationInBeacon[i] = 0;
			NdisZeroMemory(pBeaconSync->BeaconTxWI[i], TXWI_SIZE);
		}
		pBeaconSync->BeaconBitMap = 0;
		
		//RTMPInitTimer(pAd, &pAd->CommonCfg.BeaconUpdateTimer, GET_TIMER_FUNCTION(BeaconUpdateExec), pAd, TRUE);
		pBeaconSync->EnableBeacon = TRUE;
	}
}


VOID RT2870_BssBeaconExit(
	IN RTMP_ADAPTER *pAd)
{
	BEACON_SYNC_STRUCT	*pBeaconSync;
	BOOLEAN	Cancelled = TRUE;
	int i;

	if (pAd->CommonCfg.pBeaconSync)
	{
		pBeaconSync = pAd->CommonCfg.pBeaconSync;
		pBeaconSync->EnableBeacon = FALSE;
		RTMPCancelTimer(&pAd->CommonCfg.BeaconUpdateTimer, &Cancelled);
		pBeaconSync->BeaconBitMap = 0;

		for(i=0; i<HW_BEACON_MAX_COUNT; i++)
		{
			NdisZeroMemory(pBeaconSync->BeaconBuf[i], HW_BEACON_OFFSET);
			pBeaconSync->CapabilityInfoLocationInBeacon[i] = 0;
			pBeaconSync->TimIELocationInBeacon[i] = 0;
			NdisZeroMemory(pBeaconSync->BeaconTxWI[i], TXWI_SIZE);
		}
		
		NdisFreeMemory(pAd->CommonCfg.pBeaconSync, HW_BEACON_OFFSET * HW_BEACON_MAX_COUNT, 0);
		pAd->CommonCfg.pBeaconSync = NULL;
	}
}

VOID BeaconUpdateExec(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3)
{
	PRTMP_ADAPTER	pAd = (PRTMP_ADAPTER)FunctionContext;
	LARGE_INTEGER	tsfTime_a;//, tsfTime_b, deltaTime_exp, deltaTime_ab;
	UINT32			delta, remain, remain_low, remain_high;
//	BOOLEAN			positive;

	ReSyncBeaconTime(pAd);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		BEACON_SYNC_STRUCT	*pBeaconSync = pAd->CommonCfg.pBeaconSync;
		
		/* update channel utilization */
		QBSS_LoadUpdate(pAd);
		
		if (pAd->ApCfg.DtimCount == 0 && pBeaconSync->DtimBitOn)
		{
			POS_COOKIE pObj;
			
			pObj = (POS_COOKIE) pAd->OS_Cookie;
			tasklet_hi_schedule(&pObj->tbtt_task);
		}


		APUpdateAllBeaconFrame(pAd);
	}
#endif // CONFIG_AP_SUPPORT //


	RTMP_IO_READ32(pAd, TSF_TIMER_DW0, &tsfTime_a.u.LowPart);
	RTMP_IO_READ32(pAd, TSF_TIMER_DW1, &tsfTime_a.u.HighPart);
	

	//positive=getDeltaTime(tsfTime_a, expectedTime, &deltaTime_exp);
	remain_high = pAd->CommonCfg.BeaconRemain * tsfTime_a.u.HighPart;
	remain_low = tsfTime_a.u.LowPart % (pAd->CommonCfg.BeaconPeriod << 10);
	remain = (remain_high + remain_low)%(pAd->CommonCfg.BeaconPeriod << 10);
	delta = (pAd->CommonCfg.BeaconPeriod << 10) - remain;

	pAd->CommonCfg.BeaconUpdateTimer.TimerValue = (delta >> 10) + 10;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if ((pAd->CommonCfg.Channel > 14)
			&& (pAd->CommonCfg.bIEEE80211H == 1)
			&& (pAd->CommonCfg.RadarDetect.RDMode == RD_SWITCHING_MODE))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("BeaconUpdateExec::Channel Switching...(%d/%d)\n", pAd->CommonCfg.RadarDetect.CSCount, pAd->CommonCfg.RadarDetect.CSPeriod));
			
			pAd->CommonCfg.RadarDetect.CSCount++;
			if (pAd->CommonCfg.RadarDetect.CSCount >= pAd->CommonCfg.RadarDetect.CSPeriod)
			{
				APStop(pAd);
				APStartUp(pAd);
			}
		}
	}
#endif // CONFIG_AP_SUPPORT //
}

