/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************
 
 	Module Name:
	rtusb_io.c

	Abstract:

	Revision History:
	Who			When	    What
	--------	----------  ----------------------------------------------
	Name		Date	    Modification logs
	Paul Lin    06-25-2004  created
*/

#include	"rt_config.h"


/*
	========================================================================
	
	Routine Description: NIC initialization complete

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/

NTSTATUS	RTUSBFirmwareRun(
	IN	PRTMP_ADAPTER	pAd)
{
	NTSTATUS	Status;

	Status = RTUSB_VendorRequest(
		pAd,
		USBD_TRANSFER_DIRECTION_OUT,
		DEVICE_VENDOR_REQUEST_OUT,
		0x01,
		0x8,
		0,
		NULL,
		0);
	
	return Status;
}



/*
	========================================================================
	
	Routine Description: Write Firmware to NIC.

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS RTUSBFirmwareWrite(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR		pFwImage,
	IN ULONG		FwLen)
{
	UINT32		MacReg;
	NTSTATUS 	Status;
//	ULONG 		i;
	USHORT		writeLen;
	
	Status = RTUSBReadMACRegister(pAd, MAC_CSR0, &MacReg);


	writeLen = FwLen;
	RTUSBMultiWrite(pAd, FIRMWARE_IMAGE_BASE, pFwImage, writeLen);
	
	Status = RTUSBWriteMACRegister(pAd, 0x7014, 0xffffffff);
	Status = RTUSBWriteMACRegister(pAd, 0x701c, 0xffffffff);
	Status = RTUSBFirmwareRun(pAd);

	//2008/11/28:KH add to fix the dead rf frequency offset bug<--
	RTMPusecDelay(10000);
	RTUSBWriteMACRegister(pAd,H2M_MAILBOX_CSR,0);
	AsicSendCommandToMcu(pAd, 0x72, 0x00, 0x00, 0x00);//reset rf by MCU supported by new firmware
	//2008/11/28:KH add to fix the dead rf frequency offset bug-->
	
	return Status;
}


/*
	========================================================================
	
	Routine Description: Get current firmware operation mode (Return Value)

	Arguments:

	Return Value: 
		0 or 1 = Downloaded by host driver
		others = Driver doesn't download firmware

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBFirmwareOpmode(
	IN	PRTMP_ADAPTER	pAd,
	OUT	PUINT32			pValue)
{
	NTSTATUS	Status;

	Status = RTUSB_VendorRequest(
		pAd,
		(USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK),
		DEVICE_VENDOR_REQUEST_IN,
		0x1,
		0x11,
		0,
		pValue,
		4);
	return Status;
}
NTSTATUS	RTUSBVenderReset(
	IN	PRTMP_ADAPTER	pAd)
{
	NTSTATUS	Status;
	DBGPRINT_RAW(RT_DEBUG_ERROR, ("-->RTUSBVenderReset\n"));
	Status = RTUSB_VendorRequest(
		pAd,
		USBD_TRANSFER_DIRECTION_OUT,
		DEVICE_VENDOR_REQUEST_OUT,
		0x01,
		0x1,
		0,
		NULL,
		0);

	DBGPRINT_RAW(RT_DEBUG_ERROR, ("<--RTUSBVenderReset\n"));
	return Status;
}
/*
	========================================================================
	
	Routine Description: Read various length data from RT2573

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBMultiRead(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	OUT	PUCHAR			pData,
	IN	USHORT			length)
{
	NTSTATUS	Status;

	Status = RTUSB_VendorRequest(
		pAd,
		(USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK),
		DEVICE_VENDOR_REQUEST_IN,
		0x7,
		0,
		Offset,
		pData,
		length);
	
	return Status;
}

/*
	========================================================================
	
	Routine Description: Write various length data to RT2573

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBMultiWrite_OneByte(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	IN	PUCHAR			pData)
{
	NTSTATUS	Status;

	// TODO: In 2870, use this funciton carefully cause it's not stable.
	Status = RTUSB_VendorRequest(
		pAd,
		USBD_TRANSFER_DIRECTION_OUT,
		DEVICE_VENDOR_REQUEST_OUT,
		0x6,
		0,
		Offset,
		pData,
		1);

	return Status;
}

NTSTATUS	RTUSBMultiWrite(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	IN	PUCHAR			pData,
	IN	USHORT			length)
{
	NTSTATUS	Status;


        USHORT          index = 0,Value;
        PUCHAR          pSrc = pData;
        USHORT          resude = 0;
		
        resude = length % 2;
		length  += resude;
		do
		{
			Value =(USHORT)( *pSrc  | (*(pSrc + 1) << 8));
		Status = RTUSBSingleWrite(pAd,Offset + index,Value);
            index +=2;
            length -= 2;
            pSrc = pSrc + 2;
        }while(length > 0);

	return Status;
}


NTSTATUS RTUSBSingleWrite(
	IN 	RTMP_ADAPTER 	*pAd,
	IN	USHORT			Offset,
	IN	USHORT			Value)
{
	NTSTATUS	Status;

	Status = RTUSB_VendorRequest(
		pAd,
		USBD_TRANSFER_DIRECTION_OUT,
		DEVICE_VENDOR_REQUEST_OUT,
		0x2,
		Value,
		Offset,
		NULL,
		0);
	
	return Status;

}


/*
	========================================================================
	
	Routine Description: Read 32-bit MAC register

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBReadMACRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	OUT	PUINT32			pValue)
{
	NTSTATUS	Status;
	UINT32		localVal;

	Status = RTUSB_VendorRequest(
		pAd,
		(USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK),
		DEVICE_VENDOR_REQUEST_IN,
		0x7,
		0,
		Offset,
		&localVal,
		4);
	
	*pValue = le2cpu32(localVal);


	if (Status < 0)
		*pValue = 0xffffffff;
	
	return Status;
}


/*
	========================================================================
	
	Routine Description: Write 32-bit MAC register

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBWriteMACRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	IN	UINT32			Value)
{
	NTSTATUS	Status;
	UINT32		localVal;

	localVal = Value;

	Status = RTUSBSingleWrite(pAd, Offset, (USHORT)(localVal & 0xffff));
	Status = RTUSBSingleWrite(pAd, Offset + 2, (USHORT)((localVal & 0xffff0000) >> 16));

	return Status;
}



#if 1
/*
	========================================================================
	
	Routine Description: Read 8-bit BBP register

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBReadBBPRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Id,
	IN	PUCHAR			pValue)
{
	BBP_CSR_CFG_STRUC	BbpCsr;
	UINT			i = 0;
	NTSTATUS		status;
	
	// Verify the busy condition
	do
	{
		status = RTUSBReadMACRegister(pAd, BBP_CSR_CFG, &BbpCsr.word);
		if(status >= 0)
		{
		if (!(BbpCsr.field.Busy == BUSY))
			break;
		}
		printk("RTUSBReadBBPRegister(BBP_CSR_CFG_1):retry count=%d!\n", i);
		i++;
	}
	while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));
	
	if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
	{
		//
		// Read failed then Return Default value.
		//
		*pValue = pAd->BbpWriteLatch[Id];
	
		DBGPRINT_RAW(RT_DEBUG_ERROR, ("Retry count exhausted or device removed!!!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// Prepare for write material
	BbpCsr.word 				= 0;
	BbpCsr.field.fRead			= 1;
	BbpCsr.field.Busy			= 1;
	BbpCsr.field.RegNum 		= Id;
	RTUSBWriteMACRegister(pAd, BBP_CSR_CFG, BbpCsr.word);

	i = 0;	
	// Verify the busy condition
	do
	{
		status = RTUSBReadMACRegister(pAd, BBP_CSR_CFG, &BbpCsr.word);
		if (status >= 0)
		{
		if (!(BbpCsr.field.Busy == BUSY))
		{
			*pValue = (UCHAR)BbpCsr.field.Value;
			break;
		}
		}
		printk("RTUSBReadBBPRegister(BBP_CSR_CFG_2):retry count=%d!\n", i);
		i++;
	}
	while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));
	
	if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
	{
		//
		// Read failed then Return Default value.
		//
		*pValue = pAd->BbpWriteLatch[Id];

		DBGPRINT_RAW(RT_DEBUG_ERROR, ("Retry count exhausted or device removed!!!\n"));
		return STATUS_UNSUCCESSFUL;
	}
	
	return STATUS_SUCCESS;
}
#else
/*
	========================================================================
	
	Routine Description: Read 8-bit BBP register via firmware

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBReadBBPRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Id,
	IN	PUCHAR			pValue)
{																		
	BBP_CSR_CFG_STRUC	BbpCsr;											
	int					i, k;											
	for (i=0; i<MAX_BUSY_COUNT; i++)									
	{																	
		RTUSBReadMACRegister(pAd, H2M_BBP_AGENT, &BbpCsr.word);				
		if (BbpCsr.field.Busy == BUSY)									
		{																
			continue;													
		}																
		BbpCsr.word = 0;												
		BbpCsr.field.fRead = 1;											
		BbpCsr.field.BBP_RW_MODE = 1;									
		BbpCsr.field.Busy = 1;											
		BbpCsr.field.RegNum = Id;										
		RTUSBWriteMACRegister(pAd, H2M_BBP_AGENT, BbpCsr.word);				
		AsicSendCommandToMcu(pAd, 0x80, 0xff, 0x0, 0x0);					
		for (k=0; k<MAX_BUSY_COUNT; k++)								
		{																
			RTUSBReadMACRegister(pAd, H2M_BBP_AGENT, &BbpCsr.word);			
			if (BbpCsr.field.Busy == IDLE)								
				break;													
		}																
		if ((BbpCsr.field.Busy == IDLE) &&								
			(BbpCsr.field.RegNum == Id))								
		{																
			*pValue = (UCHAR)BbpCsr.field.Value;							
			break;														
		}																
	}
	if (BbpCsr.field.Busy == BUSY)										
	{																	
		DBGPRINT_ERR(("BBP read R%d=0x%x fail\n", Id, BbpCsr.word));	
		*pValue = pAd->BbpWriteLatch[Id];								
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}
#endif

#if 1
/*
	========================================================================
	
	Routine Description: Write 8-bit BBP register

	Arguments:

	Return Value:
	
	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBWriteBBPRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Id,
	IN	UCHAR			Value)
{
	BBP_CSR_CFG_STRUC	BbpCsr;
	UINT			i = 0;
	NTSTATUS		status;
	// Verify the busy condition
	do
	{
		status = RTUSBReadMACRegister(pAd, BBP_CSR_CFG, &BbpCsr.word);
		if (status >= 0)
		{
		if (!(BbpCsr.field.Busy == BUSY))
			break;
		}
		printk("RTUSBWriteBBPRegister(BBP_CSR_CFG):retry count=%d!\n", i);
		i++;
	}
	while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));
	
	if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR, ("Retry count exhausted or device removed!!!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// Prepare for write material
	BbpCsr.word 				= 0;
	BbpCsr.field.fRead			= 0;
	BbpCsr.field.Value			= Value;
	BbpCsr.field.Busy			= 1;
	BbpCsr.field.RegNum 		= Id;
	RTUSBWriteMACRegister(pAd, BBP_CSR_CFG, BbpCsr.word);
	
	pAd->BbpWriteLatch[Id] = Value;

	return STATUS_SUCCESS;
}
#else
/*
	========================================================================
	
	Routine Description: Write 8-bit BBP register via firmware

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/

NTSTATUS	RTUSBWriteBBPRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Id,
	IN	UCHAR			Value)

{																		
	BBP_CSR_CFG_STRUC	BbpCsr;											
	int					BusyCnt;										
	for (BusyCnt=0; BusyCnt<MAX_BUSY_COUNT; BusyCnt++)				
	{																
		RTMP_IO_READ32(pAd, H2M_BBP_AGENT, &BbpCsr.word);				
		if (BbpCsr.field.Busy == BUSY)									
			continue;												
		BbpCsr.word = 0;												
		BbpCsr.field.fRead = 0;											
		BbpCsr.field.BBP_RW_MODE = 1;									
		BbpCsr.field.Busy = 1;											
		BbpCsr.field.Value = Value;								
		BbpCsr.field.RegNum = Id;										
		RTMP_IO_WRITE32(pAd, H2M_BBP_AGENT, BbpCsr.word);				
		AsicSendCommandToMcu(pAd, 0x80, 0xff, 0x0, 0x0);				
		pAd->BbpWriteLatch[Id] = Value;									
		break;														
	}																
	if (BusyCnt == MAX_BUSY_COUNT)									
	{																
		DBGPRINT_ERR(("BBP write R%d=0x%x fail\n", Id, BbpCsr.word));	
		return STATUS_UNSUCCESSFUL;
	}									
	return STATUS_SUCCESS;
}
#endif
/*
	========================================================================
	
	Routine Description: Write RF register through MAC

	Arguments:

	Return Value:
	
	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBWriteRFRegister(
	IN	PRTMP_ADAPTER	pAd,
	IN	UINT32			Value)
{
	PHY_CSR4_STRUC	PhyCsr4;
	UINT			i = 0;
	NTSTATUS		status;

	NdisZeroMemory(&PhyCsr4, sizeof(PHY_CSR4_STRUC));
	do
	{
		status = RTUSBReadMACRegister(pAd, RF_CSR_CFG0, &PhyCsr4.word);
		if (status >= 0)
		{
		if (!(PhyCsr4.field.Busy))
			break;
		}
		printk("RTUSBWriteRFRegister(RF_CSR_CFG0):retry count=%d!\n", i);
		i++;
	}
	while ((i < RETRY_LIMIT) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)));

	if ((i == RETRY_LIMIT) || (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR, ("Retry count exhausted or device removed!!!\n"));
		return STATUS_UNSUCCESSFUL;
	}

	RTUSBWriteMACRegister(pAd, RF_CSR_CFG0, Value);
	
	return STATUS_SUCCESS;
}


/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBReadEEPROM(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	OUT	PUCHAR			pData,
	IN	USHORT			length)
{
	NTSTATUS	Status = STATUS_SUCCESS;

#ifdef RT30xx
	if(pAd->bUseEfuse)
	{
		Status =eFuseRead(pAd, Offset, pData, length);
	}
	else
#endif // RT30xx //
	{
	Status = RTUSB_VendorRequest(
		pAd,
		(USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK),
		DEVICE_VENDOR_REQUEST_IN,
		0x9,
		0,
		Offset,
		pData,
		length);
	}
	
	return Status;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS	RTUSBWriteEEPROM(
	IN	PRTMP_ADAPTER	pAd,
	IN	USHORT			Offset,
	IN	PUCHAR			pData,
	IN	USHORT			length)
{
	NTSTATUS	Status = STATUS_SUCCESS;

#ifdef RT30xx
	if(pAd->bUseEfuse)
	{
		Status = eFuseWrite(pAd, Offset, pData, length);
	}
	else
#endif // RT30xx //
	{
	Status = RTUSB_VendorRequest(
		pAd,
		USBD_TRANSFER_DIRECTION_OUT,
		DEVICE_VENDOR_REQUEST_OUT,
		0x8,
		0,
		Offset,
		pData,
		length);
	}
	
	return Status;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	IRQL = 
	
	Note:
	
	========================================================================
*/
VOID RTUSBPutToSleep(
	IN	PRTMP_ADAPTER	pAd)
{
	UINT32		value;
	
	// Timeout 0x40 x 50us
	value = (SLEEPCID<<16)+(OWNERMCU<<24)+ (0x40<<8)+1;
	RTUSBWriteMACRegister(pAd, 0x7010, value);
	RTUSBWriteMACRegister(pAd, 0x404, 0x30);
	//RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS);			
	DBGPRINT_RAW(RT_DEBUG_ERROR, ("Sleep Mailbox testvalue %x\n", value));
	
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	IRQL = 
	
	Note:
	
	========================================================================
*/
NTSTATUS RTUSBWakeUp(
	IN	PRTMP_ADAPTER	pAd)
{
	NTSTATUS	Status;
	
	Status = RTUSB_VendorRequest(
		pAd,
		USBD_TRANSFER_DIRECTION_OUT,
		DEVICE_VENDOR_REQUEST_OUT,
		0x01,
		0x09,
		0,
		NULL,
		0);
	
	return Status;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	IRQL = 
	
	Note:
	
	========================================================================
*/
VOID	RTUSBInitializeCmdQ(
	IN	PCmdQ	cmdq)
{
	cmdq->head = NULL;
	cmdq->tail = NULL;
	cmdq->size = 0;
	cmdq->CmdQState = RT2870_THREAD_INITED;
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	IRQL = 
	
	Note:
	
	========================================================================
*/
NDIS_STATUS	RTUSBEnqueueCmdFromNdis(
	IN	PRTMP_ADAPTER	pAd,
	IN	NDIS_OID		Oid,
	IN	BOOLEAN			SetInformation,
	IN	PVOID			pInformationBuffer,
	IN	UINT32			InformationBufferLength)
{
	NDIS_STATUS	status;
	PCmdQElmt	cmdqelmt = NULL;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	
	if (pObj->RTUSBCmdThr_pid < 0) 
		return (NDIS_STATUS_RESOURCES);
        
	status = RTMPAllocateMemory((PVOID *)&cmdqelmt, sizeof(CmdQElmt));
	if ((status != NDIS_STATUS_SUCCESS) || (cmdqelmt == NULL))
		return (NDIS_STATUS_RESOURCES);

		cmdqelmt->buffer = NULL;
		if (pInformationBuffer != NULL)
		{
			status = RTMPAllocateMemory((PVOID *)&cmdqelmt->buffer, InformationBufferLength);
			if ((status != NDIS_STATUS_SUCCESS) || (cmdqelmt->buffer == NULL))
			{       
				kfree(cmdqelmt);
				return (NDIS_STATUS_RESOURCES);
			}
			else
			{
				NdisMoveMemory(cmdqelmt->buffer, pInformationBuffer, InformationBufferLength);
				cmdqelmt->bufferlength = InformationBufferLength;
			}
		}
		else
			cmdqelmt->bufferlength = 0;
	
	cmdqelmt->command = Oid;
	cmdqelmt->CmdFromNdis = TRUE;
	if (SetInformation == TRUE)
		cmdqelmt->SetOperation = TRUE;
	else
		cmdqelmt->SetOperation = FALSE;

	NdisAcquireSpinLock(&pAd->CmdQLock);
	if (pAd->CmdQ.CmdQState & RT2870_THREAD_CAN_DO_INSERT)
	{
		EnqueueCmd((&pAd->CmdQ), cmdqelmt);
		status = NDIS_STATUS_SUCCESS;
	}
	else
	{
		status = NDIS_STATUS_FAILURE;
	}
	NdisReleaseSpinLock(&pAd->CmdQLock);
	
	if (status == NDIS_STATUS_FAILURE)
	{
		if (cmdqelmt->buffer)
			NdisFreeMemory(cmdqelmt->buffer, cmdqelmt->bufferlength, 0);
		NdisFreeMemory(cmdqelmt, sizeof(CmdQElmt), 0);
	}
	else
	RTUSBCMDUp(pAd);


    return(NDIS_STATUS_SUCCESS);
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:

	IRQL = 
	
	Note:
	
	========================================================================
*/
NDIS_STATUS RTUSBEnqueueInternalCmd(
	IN PRTMP_ADAPTER	pAd,
	IN NDIS_OID			Oid,
	IN PVOID			pInformationBuffer,
	IN UINT32			InformationBufferLength)	
{
	NDIS_STATUS	status;
	PCmdQElmt	cmdqelmt = NULL;
		

	status = RTMPAllocateMemory((PVOID *)&cmdqelmt, sizeof(CmdQElmt));
	if ((status != NDIS_STATUS_SUCCESS) || (cmdqelmt == NULL))
		return (NDIS_STATUS_RESOURCES);
	NdisZeroMemory(cmdqelmt, sizeof(CmdQElmt));

	if(InformationBufferLength > 0)
	{
		status = RTMPAllocateMemory((PVOID *)&cmdqelmt->buffer, InformationBufferLength);
		if ((status != NDIS_STATUS_SUCCESS) || (cmdqelmt->buffer == NULL))
		{
			NdisFreeMemory(cmdqelmt, sizeof(CmdQElmt), 0);
			return (NDIS_STATUS_RESOURCES);
		}
		else
		{
			NdisMoveMemory(cmdqelmt->buffer, pInformationBuffer, InformationBufferLength);
			cmdqelmt->bufferlength = InformationBufferLength;
		}
	}
	else
	{
		cmdqelmt->buffer = NULL;
		cmdqelmt->bufferlength = 0;
	}

	cmdqelmt->command = Oid;
	cmdqelmt->CmdFromNdis = FALSE;

	if (cmdqelmt != NULL)
	{
		NdisAcquireSpinLock(&pAd->CmdQLock);
		if (pAd->CmdQ.CmdQState & RT2870_THREAD_CAN_DO_INSERT)
		{
			EnqueueCmd((&pAd->CmdQ), cmdqelmt);
			status = NDIS_STATUS_SUCCESS;
		}
		else
		{
			status = NDIS_STATUS_FAILURE;
		}
		NdisReleaseSpinLock(&pAd->CmdQLock);

		if (status == NDIS_STATUS_FAILURE)
		{
			if (cmdqelmt->buffer)
				NdisFreeMemory(cmdqelmt->buffer, cmdqelmt->bufferlength, 0);
			NdisFreeMemory(cmdqelmt, sizeof(CmdQElmt), 0);
		}
		else
		RTUSBCMDUp(pAd);
	}
	return(NDIS_STATUS_SUCCESS);
}

/*
	========================================================================
	
	Routine Description:

	Arguments:

	Return Value:
	
	IRQL = 
	
	Note:
	
	========================================================================
*/
VOID	RTUSBDequeueCmd(
	IN	PCmdQ		cmdq,
	OUT	PCmdQElmt	*pcmdqelmt)
{
	*pcmdqelmt = cmdq->head;
	
	if (*pcmdqelmt != NULL)
	{
		cmdq->head = cmdq->head->next;
		cmdq->size--;
		if (cmdq->size == 0)
			cmdq->tail = NULL;
	}
}

/*
    ========================================================================
	  usb_control_msg - Builds a control urb, sends it off and waits for completion
	  @dev: pointer to the usb device to send the message to
	  @pipe: endpoint "pipe" to send the message to
	  @request: USB message request value
	  @requesttype: USB message request type value
	  @value: USB message value
	  @index: USB message index value
	  @data: pointer to the data to send
	  @size: length in bytes of the data to send
	  @timeout: time in jiffies to wait for the message to complete before
			  timing out (if 0 the wait is forever)
	  Context: !in_interrupt ()

	  This function sends a simple control message to a specified endpoint
	  and waits for the message to complete, or timeout.
	  If successful, it returns the number of bytes transferred, otherwise a negative error number.

	 Don't use this function from within an interrupt context, like a
	  bottom half handler.	If you need an asynchronous message, or need to send
	  a message from within interrupt context, use usb_submit_urb()
	  If a thread in your driver uses this call, make sure your disconnect()
	  method can wait for it to complete.  Since you don't have a handle on
	  the URB used, you can't cancel the request.
  
	
	Routine Description:

	Arguments:

	Return Value:
	
	Note:
	
	========================================================================
*/
NTSTATUS    RTUSB_VendorRequest(
	IN	PRTMP_ADAPTER	pAd,
	IN	UINT32			TransferFlags,
	IN	UCHAR			RequestType,
	IN	UCHAR			Request,
	IN	USHORT			Value,
	IN	USHORT			Index,
	IN	PVOID			TransferBuffer,
	IN	UINT32			TransferBufferLength)
{
	int				ret;
	POS_COOKIE		pObj = (POS_COOKIE) pAd->OS_Cookie;

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("device disconnected\n"));
		return -1;
	}
	else if (in_interrupt())	
	{
		DBGPRINT(RT_DEBUG_ERROR, ("in_interrupt, RTUSB_VendorRequest Request%02x Value%04x Offset%04x\n",Request,Value,Index));

		return -1;
	}
	else
	{
#define MAX_RETRY_COUNT  10

		int retryCount = 0;
		void	*tmpBuf = TransferBuffer;
	
		// Acquire Control token
#ifdef INF_AMAZON_SE
		//Semaphore fix INF_AMAZON_SE hang
		//pAd->UsbVendorReqBuf is the swap for DEVICE_VENDOR_REQUEST_IN to fix dma bug.
		ret = down_interruptible(&(pAd->UsbVendorReq_semaphore));
		if (pAd->UsbVendorReqBuf)
		{
			ASSERT(TransferBufferLength <MAX_PARAM_BUFFER_SIZE);
		   
		   	tmpBuf = (void *)pAd->UsbVendorReqBuf;
		   	NdisZeroMemory(pAd->UsbVendorReqBuf, TransferBufferLength);
		   
		   	if (RequestType == DEVICE_VENDOR_REQUEST_OUT)
		   	 NdisMoveMemory(tmpBuf, TransferBuffer, TransferBufferLength);  
		}
#endif // INF_AMAZON_SE //
#ifdef WINBOND
                //Semaphore fix WINBOND hang
                //pAd->UsbVendorReqBuf is the swap for DEVICE_VENDOR_REQUEST_IN to fix dma bug.
                ret = down_interruptible(&(pAd->Winbond_UsbVendorReq_semaphore));
                if (pAd->Winbond_UsbVendorReqBuf)
                {
                        ASSERT(TransferBufferLength <MAX_PARAM_BUFFER_SIZE);

                        tmpBuf = (void *)pAd->Winbond_UsbVendorReqBuf;
                        NdisZeroMemory(pAd->Winbond_UsbVendorReqBuf, TransferBufferLength);

                        if (RequestType == DEVICE_VENDOR_REQUEST_OUT)
                         NdisMoveMemory(tmpBuf, TransferBuffer, TransferBufferLength);
                }
#endif // WINBOND //

		do {
		if( RequestType == DEVICE_VENDOR_REQUEST_OUT)
			ret=usb_control_msg(pObj->pUsb_Dev, usb_sndctrlpipe( pObj->pUsb_Dev, 0 ), Request, RequestType, Value,Index, tmpBuf, TransferBufferLength, CONTROL_TIMEOUT_JIFFIES);
		else if(RequestType == DEVICE_VENDOR_REQUEST_IN)
			ret=usb_control_msg(pObj->pUsb_Dev, usb_rcvctrlpipe( pObj->pUsb_Dev, 0 ), Request, RequestType, Value,Index, tmpBuf, TransferBufferLength, CONTROL_TIMEOUT_JIFFIES);
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("vendor request direction is failed\n"));
			ret = -1;
		}

			retryCount++;
			if (ret < 0) {
//				printk("#\n");  // comment by guowenxue
				RTMPusecDelay(5000);
			}
		} while((ret < 0) && (retryCount < MAX_RETRY_COUNT));
		
#ifdef INF_AMAZON_SE
	  	if ((pAd->UsbVendorReqBuf) && (RequestType == DEVICE_VENDOR_REQUEST_IN))
			NdisMoveMemory(TransferBuffer, tmpBuf, TransferBufferLength);
	  	up(&(pAd->UsbVendorReq_semaphore));
#endif // INF_AMAZON_SE //
#ifdef WINBOND
                if ((pAd->Winbond_UsbVendorReqBuf) && (RequestType == DEVICE_VENDOR_REQUEST_IN))
                        NdisMoveMemory(TransferBuffer, tmpBuf, TransferBufferLength);
                up(&(pAd->Winbond_UsbVendorReq_semaphore));
#endif // WINBOND //

        if (ret < 0) {
//			DBGPRINT(RT_DEBUG_ERROR, ("USBVendorRequest failed ret=%d \n",ret));
			DBGPRINT(RT_DEBUG_ERROR, ("RTUSB_VendorRequest failed(%d),TxFlags=0x%x, ReqType=%s, Req=0x%x, Index=0x%x\n",
						ret, TransferFlags, (RequestType == DEVICE_VENDOR_REQUEST_OUT ? "OUT" : "IN"), Request, Index));
			if (Request == 0x2)
				DBGPRINT(RT_DEBUG_ERROR, ("\tRequest Value=0x%04x!\n", Value));
			
			if ((TransferBuffer!= NULL) && (TransferBufferLength > 0))
				hex_dump("Failed TransferBuffer value", TransferBuffer, TransferBufferLength);
        }
			
#if 0	    
        // retry
		if (ret < 0) {
			int temp_i=0;
			DBGPRINT(RT_DEBUG_ERROR, ("USBVendorRequest failed ret=%d, \n",ret));
			ret = 0;
			do
			{
				if( RequestType == DEVICE_VENDOR_REQUEST_OUT)
					ret=usb_control_msg(pObj->pUsb_Dev, usb_sndctrlpipe( pObj->pUsb_Dev, 0 ), Request, RequestType, Value,Index, TransferBuffer, TransferBufferLength, CONTROL_TIMEOUT_JIFFIES);
				else if(RequestType == DEVICE_VENDOR_REQUEST_IN)
					ret=usb_control_msg(pObj->pUsb_Dev, usb_rcvctrlpipe( pObj->pUsb_Dev, 0 ), Request, RequestType, Value,Index, TransferBuffer, TransferBufferLength, CONTROL_TIMEOUT_JIFFIES);
				temp_i++;
			} while( (ret < 0) && (temp_i <= 1) );

			if( ret >= 0)
				return ret;
			
		}
#endif

	}
	return ret;
}

/*
	========================================================================
	
	Routine Description:
	  Creates an IRP to submite an IOCTL_INTERNAL_USB_RESET_PORT
	  synchronously. Callers of this function must be running at
	  PASSIVE LEVEL.

	Arguments:

	Return Value:

	Note:
	
	========================================================================
*/
NTSTATUS	RTUSB_ResetDevice(
	IN	PRTMP_ADAPTER	pAd)
{
	NTSTATUS		Status = TRUE;

	DBGPRINT_RAW(RT_DEBUG_TRACE, ("--->USB_ResetDevice\n"));
	//RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS);
	return Status;
}
                                                                                                                                                               
VOID CMDHandler(                                                                                                                                                
    IN PRTMP_ADAPTER pAd)                                                                                                                                       
{                                                                                                                                                               
	PCmdQElmt		cmdqelmt;                                                                                                                                       
	PUCHAR			pData;                                                                                                                                          
	NDIS_STATUS		NdisStatus = NDIS_STATUS_SUCCESS;                                                                                                               
//	ULONG			Now = 0;
	NTSTATUS		ntStatus;
//	unsigned long	IrqFlags;
	
	while (pAd->CmdQ.size > 0)                                                                                                                                  
	{                                                                                                                                                           
		NdisStatus = NDIS_STATUS_SUCCESS;
		                                                                                                                      
		NdisAcquireSpinLock(&pAd->CmdQLock);
		RTUSBDequeueCmd(&pAd->CmdQ, &cmdqelmt);
		NdisReleaseSpinLock(&pAd->CmdQLock);
		                                                                                                        
		if (cmdqelmt == NULL)                                                                                                                                   
			break; 
			                                                                                                                                             
		pData = cmdqelmt->buffer;                                      
		                                                                                         
		if(!(RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST) || RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)))
		{
			switch (cmdqelmt->command)
			{
				case CMDTHREAD_CHECK_GPIO:
					{
#ifdef RALINK_ATE			
       					if(ATE_ON(pAd))
						{
							DBGPRINT(RT_DEBUG_TRACE, ("The driver is in ATE mode now\n"));
							break;
						}	
#endif // RALINK_ATE //

					}
					break;                                                                                                                                         
                                                                                                                                            

				case CMDTHREAD_RESET_BULK_OUT:                                                                                                                     
					{
						UINT32		MACValue;
						UCHAR		Index;
						int			ret=0;
						PHT_TX_CONTEXT	pHTTXContext;
//						RTMP_TX_RING *pTxRing;
						unsigned long IrqFlags;
#ifdef RALINK_ATE
						PTX_CONTEXT		pNullContext = &(pAd->NullContext);
#endif // RALINK_ATE //						
						DBGPRINT_RAW(RT_DEBUG_TRACE, ("CmdThread : CMDTHREAD_RESET_BULK_OUT(ResetPipeid=0x%0x)===>\n", pAd->bulkResetPipeid));
						// All transfers must be aborted or cancelled before attempting to reset the pipe.						
						//RTUSBCancelPendingBulkOutIRP(pAd);
						// Wait 10ms to let previous packet that are already in HW FIFO to clear. by MAXLEE 12-25-2007
						Index = 0;
						do 
						{
							RTUSBReadMACRegister(pAd, TXRXQ_PCNT, &MACValue);
							if ((MACValue & 0xf00000/*0x800000*/) == 0)
								break;
							Index++;
							RTMPusecDelay(10000);
						}while(Index < 100);
						MACValue = 0;
						RTUSBReadMACRegister(pAd, USB_DMA_CFG, &MACValue);
						// To prevent Read Register error, we 2nd check the validity.
						if ((MACValue & 0xc00000) == 0)
							RTUSBReadMACRegister(pAd, USB_DMA_CFG, &MACValue);
						// To prevent Read Register error, we 3rd check the validity.
						if ((MACValue & 0xc00000) == 0)
							RTUSBReadMACRegister(pAd, USB_DMA_CFG, &MACValue);
						MACValue |= 0x80000;
						RTUSBWriteMACRegister(pAd, USB_DMA_CFG, MACValue);

						// Wait 1ms to prevent next URB to bulkout before HW reset. by MAXLEE 12-25-2007
						RTMPusecDelay(1000);

						MACValue &= (~0x80000);
						RTUSBWriteMACRegister(pAd, USB_DMA_CFG, MACValue);
						DBGPRINT_RAW(RT_DEBUG_TRACE, ("\tSet 0x2a0 bit19. Clear USB DMA TX path\n"));
						
						// Wait 5ms to prevent next URB to bulkout before HW reset. by MAXLEE 12-25-2007
						//RTMPusecDelay(5000);

						if ((pAd->bulkResetPipeid & BULKOUT_MGMT_RESET_FLAG) == BULKOUT_MGMT_RESET_FLAG)
						{
							RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
							if (pAd->MgmtRing.TxSwFreeIdx < MGMT_RING_SIZE /* pMLMEContext->bWaitingBulkOut == TRUE */)
							{
								RTUSB_SET_BULK_FLAG(pAd, fRTUSB_BULK_OUT_MLME);
							}
							RTUSBKickBulkOut(pAd);
							
							DBGPRINT_RAW(RT_DEBUG_TRACE, ("\tTX MGMT RECOVER Done!\n"));
						}
						else
						{
							pHTTXContext = &(pAd->TxContext[pAd->bulkResetPipeid]);
							//NdisAcquireSpinLock(&pAd->BulkOutLock[pAd->bulkResetPipeid]);
							RTMP_INT_LOCK(&pAd->BulkOutLock[pAd->bulkResetPipeid], IrqFlags);
							if ( pAd->BulkOutPending[pAd->bulkResetPipeid] == FALSE)
							{
								pAd->BulkOutPending[pAd->bulkResetPipeid] = TRUE;
								pHTTXContext->IRPPending = TRUE;
								pAd->watchDogTxPendingCnt[pAd->bulkResetPipeid] = 1;
								
								// no matter what, clean the flag
								RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);
								
								//NdisReleaseSpinLock(&pAd->BulkOutLock[pAd->bulkResetPipeid]);
								RTMP_INT_UNLOCK(&pAd->BulkOutLock[pAd->bulkResetPipeid], IrqFlags);
/*-----------------------------------------------------------------------------------------------*/
#ifdef RALINK_ATE
								if(ATE_ON(pAd))				
							    {
									pNullContext->IRPPending = TRUE;
									//
									// If driver is still in ATE TXFRAME mode, 
									// keep on transmitting ATE frames.
									//
									DBGPRINT_RAW(RT_DEBUG_TRACE, ("pAd->ate.Mode == %d\npAd->ContinBulkOut == %d\npAd->BulkOutRemained == %d\n", pAd->ate.Mode, pAd->ContinBulkOut, atomic_read(&pAd->BulkOutRemained)));
									if((pAd->ate.Mode == ATE_TXFRAME) && ((pAd->ContinBulkOut == TRUE) || (atomic_read(&pAd->BulkOutRemained) > 0)))
								    {
										DBGPRINT_RAW(RT_DEBUG_TRACE, ("After CMDTHREAD_RESET_BULK_OUT, continue to bulk out frames !\n"));

										// Init Tx context descriptor
										RTUSBInitTxDesc(pAd, pNullContext, 0/* pAd->bulkResetPipeid */, (usb_complete_t)ATE_RTUSBBulkOutDataPacketComplete);
										
										if((ret = RTUSB_SUBMIT_URB(pNullContext->pUrb))!=0)
										{
											DBGPRINT(RT_DEBUG_ERROR, ("ATE_RTUSBBulkOutDataPacket: Submit Tx URB failed %d\n", ret));
										}

										pAd->BulkOutReq++;
									}
								}
								else
#endif // RALINK_ATE //
/*-----------------------------------------------------------------------------------------------*/
								{
								RTUSBInitHTTxDesc(pAd, pHTTXContext, pAd->bulkResetPipeid, pHTTXContext->BulkOutSize, (usb_complete_t)RTUSBBulkOutDataPacketComplete);							
							
								if((ret = RTUSB_SUBMIT_URB(pHTTXContext->pUrb))!=0)
								{
										RTMP_INT_LOCK(&pAd->BulkOutLock[pAd->bulkResetPipeid], IrqFlags);
									pAd->BulkOutPending[pAd->bulkResetPipeid] = FALSE;
									pHTTXContext->IRPPending = FALSE;
										pAd->watchDogTxPendingCnt[pAd->bulkResetPipeid] = 0;
										RTMP_INT_UNLOCK(&pAd->BulkOutLock[pAd->bulkResetPipeid], IrqFlags);

										DBGPRINT(RT_DEBUG_ERROR, ("CmdThread : CMDTHREAD_RESET_BULK_OUT: Submit Tx URB failed %d\n", ret));
								} 
									else
									{
										RTMP_IRQ_LOCK(&pAd->BulkOutLock[pAd->bulkResetPipeid], IrqFlags);
										DBGPRINT_RAW(RT_DEBUG_TRACE,("\tCMDTHREAD_RESET_BULK_OUT: TxContext[%d]:CWPos=%ld, NBPos=%ld, ENBPos=%ld, bCopy=%d, pending=%d!\n", 
												pAd->bulkResetPipeid, pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition, 
															pHTTXContext->ENextBulkOutPosition, pHTTXContext->bCopySavePad, pAd->BulkOutPending[pAd->bulkResetPipeid]));
										DBGPRINT_RAW(RT_DEBUG_TRACE,("\t\tBulkOut Req=0x%lx, Complete=0x%lx, Other=0x%lx\n", 
															pAd->BulkOutReq, pAd->BulkOutComplete, pAd->BulkOutCompleteOther));
										RTMP_IRQ_UNLOCK(&pAd->BulkOutLock[pAd->bulkResetPipeid], IrqFlags);
										DBGPRINT_RAW(RT_DEBUG_TRACE, ("\tCMDTHREAD_RESET_BULK_OUT: Submit Tx DATA URB for failed BulkReq(0x%lx) Done, status=%d!\n", pAd->bulkResetReq[pAd->bulkResetPipeid], pHTTXContext->pUrb->status));

									}
								}
							}
							else
							{
								//NdisReleaseSpinLock(&pAd->BulkOutLock[pAd->bulkResetPipeid]);
								//RTMP_INT_UNLOCK(&pAd->BulkOutLock[pAd->bulkResetPipeid], IrqFlags);
								
								DBGPRINT_RAW(RT_DEBUG_ERROR, ("CmdThread : TX DATA RECOVER FAIL for BulkReq(0x%lx) because BulkOutPending[%d] is TRUE!\n", pAd->bulkResetReq[pAd->bulkResetPipeid], pAd->bulkResetPipeid));
								if (pAd->bulkResetPipeid == 0)
								{
									UCHAR	pendingContext = 0;
									PHT_TX_CONTEXT pHTTXContext = (PHT_TX_CONTEXT)(&pAd->TxContext[pAd->bulkResetPipeid ]);
									PTX_CONTEXT pMLMEContext = (PTX_CONTEXT)(pAd->MgmtRing.Cell[pAd->MgmtRing.TxDmaIdx].AllocVa);
									PTX_CONTEXT pNULLContext = (PTX_CONTEXT)(&pAd->PsPollContext);
									PTX_CONTEXT pPsPollContext = (PTX_CONTEXT)(&pAd->NullContext);

									if (pHTTXContext->IRPPending)
										pendingContext |= 1;
									else if (pMLMEContext->IRPPending)
										pendingContext |= 2;
									else if (pNULLContext->IRPPending)
										pendingContext |= 4;
									else if (pPsPollContext->IRPPending)
										pendingContext |= 8;
									else
										pendingContext = 0;
									
									DBGPRINT_RAW(RT_DEBUG_ERROR, ("\tTX Occupied by %d!\n", pendingContext));
								}

							// no matter what, clean the flag
							RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);

								RTMP_INT_UNLOCK(&pAd->BulkOutLock[pAd->bulkResetPipeid], IrqFlags);
								
								RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << pAd->bulkResetPipeid));
							}

							RTMPDeQueuePacket(pAd, FALSE, NUM_OF_TX_RING, MAX_TX_PROCESS);
							//RTUSBKickBulkOut(pAd);
						}

					}
					/*
						// Don't cancel BULKIN.	
						while ((atomic_read(&pAd->PendingRx) > 0) && 
								(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))) 
						{
							if (atomic_read(&pAd->PendingRx) > 0)
							{
								DBGPRINT_RAW(RT_DEBUG_ERROR, ("BulkIn IRP Pending!!cancel it!\n"));
								RTUSBCancelPendingBulkInIRP(pAd);
							}
							RTMPusecDelay(100000);
						}
						
						if ((atomic_read(&pAd->PendingRx) == 0) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)))
						{
							UCHAR	i;
							RTUSBRxPacket(pAd);
							pAd->NextRxBulkInReadIndex = 0;	// Next Rx Read index
							pAd->NextRxBulkInIndex		= 0;	// Rx Bulk pointer
							for (i = 0; i < (RX_RING_SIZE); i++)
							{
								PRX_CONTEXT  pRxContext = &(pAd->RxContext[i]);

								pRxContext->pAd	= pAd;
								pRxContext->InUse		= FALSE;
								pRxContext->IRPPending	= FALSE;
								pRxContext->Readable	= FALSE;
								pRxContext->ReorderInUse = FALSE;
							
							}
							RTUSBBulkReceive(pAd);
							DBGPRINT_RAW(RT_DEBUG_ERROR, ("RTUSBBulkReceive\n"));
						}*/
					DBGPRINT_RAW(RT_DEBUG_TRACE, ("CmdThread : CMDTHREAD_RESET_BULK_OUT<===\n"));
    	   			break;                                                                                                                                              
            	                                                                                                                                                    
				case CMDTHREAD_RESET_BULK_IN:                                                                                                                      
					DBGPRINT_RAW(RT_DEBUG_TRACE, ("CmdThread : CMDTHREAD_RESET_BULK_IN === >\n"));

					// All transfers must be aborted or cancelled before attempting to reset the pipe.
					{
						UINT32		MACValue;
/*-----------------------------------------------------------------------------------------------*/
#ifdef RALINK_ATE
						if (ATE_ON(pAd))
						{
							if((pAd->PendingRx > 0) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
							{
								DBGPRINT_RAW(RT_DEBUG_ERROR, ("ATE : BulkIn IRP Pending!!!\n"));
								ATE_RTUSBCancelPendingBulkInIRP(pAd);
								RTMPusecDelay(100000);
								pAd->PendingRx = 0;
							}
						}
						else
#endif // RALINK_ATE //
/*-----------------------------------------------------------------------------------------------*/
						{
						//while ((atomic_read(&pAd->PendingRx) > 0) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))) 
						if((pAd->PendingRx > 0) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))
						{
							DBGPRINT_RAW(RT_DEBUG_ERROR, ("BulkIn IRP Pending!!!\n"));
							RTUSBCancelPendingBulkInIRP(pAd);
							RTMPusecDelay(100000);
							pAd->PendingRx = 0;
						}
						}						
						
						// Wait 10ms before reading register.
						RTMPusecDelay(10000);
						ntStatus = RTUSBReadMACRegister(pAd, MAC_CSR0, &MACValue);

						if ((NT_SUCCESS(ntStatus) == TRUE) && 
							(!(RTMP_TEST_FLAG(pAd, (fRTMP_ADAPTER_RESET_IN_PROGRESS | fRTMP_ADAPTER_RADIO_OFF | 
													fRTMP_ADAPTER_HALT_IN_PROGRESS | fRTMP_ADAPTER_NIC_NOT_EXIST)))))
						{		
							UCHAR	i;

							if (RTMP_TEST_FLAG(pAd, (fRTMP_ADAPTER_RESET_IN_PROGRESS | fRTMP_ADAPTER_RADIO_OFF | 
														fRTMP_ADAPTER_HALT_IN_PROGRESS | fRTMP_ADAPTER_NIC_NOT_EXIST)))
								break;
							pAd->NextRxBulkInPosition = pAd->RxContext[pAd->NextRxBulkInIndex].BulkInOffset;
							DBGPRINT(RT_DEBUG_TRACE, ("BULK_IN_RESET: NBIIdx=0x%x,NBIRIdx=0x%x, BIRPos=0x%lx. BIReq=x%lx, BIComplete=0x%lx, BICFail0x%lx\n", 
									pAd->NextRxBulkInIndex,  pAd->NextRxBulkInReadIndex, pAd->NextRxBulkInPosition, pAd->BulkInReq, pAd->BulkInComplete, pAd->BulkInCompleteFail));
							for (i = 0; i < RX_RING_SIZE; i++)
							{
 								DBGPRINT(RT_DEBUG_TRACE, ("\tRxContext[%d]: IRPPending=%d, InUse=%d, Readable=%d!\n"
									, i, pAd->RxContext[i].IRPPending, pAd->RxContext[i].InUse, pAd->RxContext[i].Readable));
							}
 							/*

							DBGPRINT_RAW(RT_DEBUG_ERROR, ("==========================================\n"));

							pAd->NextRxBulkInReadIndex = 0;	// Next Rx Read index
							pAd->NextRxBulkInIndex		= 0;	// Rx Bulk pointer
							for (i = 0; i < (RX_RING_SIZE); i++)
							{
								PRX_CONTEXT  pRxContext = &(pAd->RxContext[i]);

								pRxContext->pAd	= pAd;
								pRxContext->InUse		= FALSE;
								pRxContext->IRPPending	= FALSE;
								pRxContext->Readable	= FALSE;
								pRxContext->ReorderInUse = FALSE;
									
							}*/
							RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET);
							for (i = 0; i < pAd->CommonCfg.NumOfBulkInIRP; i++)
							{
								//RTUSBBulkReceive(pAd);
								PRX_CONTEXT		pRxContext;
								PURB			pUrb;
								int				ret = 0;
								unsigned long	IrqFlags;

	
								RTMP_IRQ_LOCK(&pAd->BulkInLock, IrqFlags);
								pRxContext = &(pAd->RxContext[pAd->NextRxBulkInIndex]);
								if ((pAd->PendingRx > 0) || (pRxContext->Readable == TRUE) || (pRxContext->InUse == TRUE))
								{
									RTMP_IRQ_UNLOCK(&pAd->BulkInLock, IrqFlags);
									break;
								}
								pRxContext->InUse = TRUE;
								pRxContext->IRPPending = TRUE;
								pAd->PendingRx++;
								pAd->BulkInReq++;
								RTMP_IRQ_UNLOCK(&pAd->BulkInLock, IrqFlags);

								// Init Rx context descriptor
								RTUSBInitRxDesc(pAd, pRxContext);
								pUrb = pRxContext->pUrb;
								if ((ret = RTUSB_SUBMIT_URB(pUrb))!=0)
								{	// fail
								
									RTMP_IRQ_LOCK(&pAd->BulkInLock, IrqFlags);
									pRxContext->InUse = FALSE;
									pRxContext->IRPPending = FALSE;
									pAd->PendingRx--;
									pAd->BulkInReq--;
									RTMP_IRQ_UNLOCK(&pAd->BulkInLock, IrqFlags);
									DBGPRINT(RT_DEBUG_ERROR, ("CMDTHREAD_RESET_BULK_IN: Submit Rx URB failed(%d), status=%d\n", ret, pUrb->status));
								}
								else
								{	// success
#if 0								
									RTMP_IRQ_LOCK(&pAd->BulkInLock, IrqFlags);
									pRxContext->IRPPending = TRUE;
									//NdisInterlockedIncrement(&pAd->PendingRx);
									pAd->PendingRx++;
									RTMP_IRQ_UNLOCK(&pAd->BulkInLock, IrqFlags);
									pAd->BulkInReq++;
#endif
									//printk("BIDone, Pend=%d,BIIdx=%d,BIRIdx=%d!\n", pAd->PendingRx, pAd->NextRxBulkInIndex, pAd->NextRxBulkInReadIndex);
									DBGPRINT_RAW(RT_DEBUG_TRACE, ("CMDTHREAD_RESET_BULK_IN: Submit Rx URB Done, status=%d!\n", pUrb->status));
									ASSERT((pRxContext->InUse == pRxContext->IRPPending));
								}
							}					

						}
						else
						{
							// Card must be removed
							if (NT_SUCCESS(ntStatus) != TRUE)
							{
							RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST);
								DBGPRINT_RAW(RT_DEBUG_ERROR, ("CMDTHREAD_RESET_BULK_IN: Read Register Failed!Card must be removed!!\n\n"));
							}
							else
							{
								DBGPRINT_RAW(RT_DEBUG_ERROR, ("CMDTHREAD_RESET_BULK_IN: Cannot do bulk in because flags(0x%lx) on !\n", pAd->Flags));
						}
					}                                                                                                                                                   
					}
					DBGPRINT_RAW(RT_DEBUG_TRACE, ("CmdThread : CMDTHREAD_RESET_BULK_IN <===\n"));
					break;                                                                                                                                              
            	                                                                                                                                                    
				case CMDTHREAD_SET_ASIC_WCID:
					{
						RT_SET_ASIC_WCID	SetAsicWcid;
						USHORT		offset;
						UINT32		MACValue, MACRValue = 0;
						SetAsicWcid = *((PRT_SET_ASIC_WCID)(pData));
						
						if (SetAsicWcid.WCID >= MAX_LEN_OF_MAC_TABLE)
							return;
						
						offset = MAC_WCID_BASE + ((UCHAR)SetAsicWcid.WCID)*HW_WCID_ENTRY_SIZE;

						DBGPRINT_RAW(RT_DEBUG_TRACE, ("CmdThread : CMDTHREAD_SET_ASIC_WCID : WCID = %ld, SetTid  = %lx, DeleteTid = %lx.\n", SetAsicWcid.WCID, SetAsicWcid.SetTid, SetAsicWcid.DeleteTid));
						MACValue = (pAd->MacTab.Content[SetAsicWcid.WCID].Addr[3]<<24)+(pAd->MacTab.Content[SetAsicWcid.WCID].Addr[2]<<16)+(pAd->MacTab.Content[SetAsicWcid.WCID].Addr[1]<<8)+(pAd->MacTab.Content[SetAsicWcid.WCID].Addr[0]);
						DBGPRINT_RAW(RT_DEBUG_TRACE, ("1-MACValue= %x,\n", MACValue));
						RTUSBWriteMACRegister(pAd, offset, MACValue);
						// Read bitmask
						RTUSBReadMACRegister(pAd, offset+4, &MACRValue);
						if ( SetAsicWcid.DeleteTid != 0xffffffff)
							MACRValue &= (~SetAsicWcid.DeleteTid);
						if (SetAsicWcid.SetTid != 0xffffffff)
							MACRValue |= (SetAsicWcid.SetTid);
						MACRValue &= 0xffff0000;
					
						MACValue = (pAd->MacTab.Content[SetAsicWcid.WCID].Addr[5]<<8)+pAd->MacTab.Content[SetAsicWcid.WCID].Addr[4];
						MACValue |= MACRValue;
						RTUSBWriteMACRegister(pAd, offset+4, MACValue);
						
						DBGPRINT_RAW(RT_DEBUG_TRACE, ("2-MACValue= %x,\n", MACValue));
					}
					break;
					
				case CMDTHREAD_SET_ASIC_WCID_CIPHER:
					{
					}
					break;

//Benson modified for USB interface, avoid in interrupt when write key, 20080724 -->
				case RT_CMD_SET_KEY_TABLE: //General call for AsicAddPairwiseKeyEntry()                    
				{
					RT_ADD_PAIRWISE_KEY_ENTRY KeyInfo;
					KeyInfo = *((PRT_ADD_PAIRWISE_KEY_ENTRY)(pData));
					AsicAddPairwiseKeyEntry(pAd,
											KeyInfo.MacAddr,
											(UCHAR)KeyInfo.MacTabMatchWCID,
											&KeyInfo.CipherKey);
				}
					break;
				case RT_CMD_SET_RX_WCID_TABLE: //General call for RTMPAddWcidAttributeEntry()
				{
					PMAC_TABLE_ENTRY pEntry;
					UCHAR KeyIdx;
					UCHAR CipherAlg;
					UCHAR ApIdx;

					pEntry = (PMAC_TABLE_ENTRY)(pData);
#ifdef CONFIG_AP_SUPPORT
					/* carella modify: write WCID Attribute Table in Cmd Thread. 
					 * ApCli / WDS Set WCID Attribute Table as: 
					 * Reset WcidAttribute -> Set Encryption Key -> Set WcidAttribute
					 * in CPU slow platform(ex:AMAZON_SE), the Cmd Thread(Reset WcidAttribute)
					 * will be executed after set and overwrite the WcidAttribute.
					 */
#ifdef APCLI_SUPPORT
					if ((pEntry->ValidAsApCli) && (pEntry->WepStatus == Ndis802_11WEPEnabled))
					{
						KeyIdx = pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx].DefaultKeyId;
						CipherAlg = pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx].SharedKey[KeyIdx].CipherAlg;
						ApIdx = pAd->ApCfg.BssidNum + MAX_MESH_NUM + MIN_NET_DEVICE_FOR_APCLI;
					}
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
					if ((pEntry->ValidAsWDS) && 
						(pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].WdsKey.KeyLen > 0) &&
						((pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].WepStatus == Ndis802_11Encryption1Enabled) ||
						(pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].WepStatus == Ndis802_11Encryption2Enabled) ||
						(pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].WepStatus == Ndis802_11Encryption3Enabled)))
					{
						KeyIdx = pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].KeyIdx;
						CipherAlg = pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].WdsKey.CipherAlg;
						ApIdx = MAIN_MBSSID + MIN_NET_DEVICE_FOR_WDS;
					}
#endif // WDS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //



						RTMPAddWcidAttributeEntry(
										  pAd,
										  ApIdx,
										  KeyIdx,
										  CipherAlg,
										  pEntry);
					}
						break;
//Benson modified for USB interface, avoid in interrupt when write key, 20080724 <--

				case CMDTHREAD_SET_CLIENT_MAC_ENTRY:
					{
						MAC_TABLE_ENTRY *pEntry;
						pEntry = (MAC_TABLE_ENTRY *)pData;

#ifdef CONFIG_AP_SUPPORT
						IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
						{
#ifdef APCLI_SUPPORT
							if (pEntry->ValidAsApCli)
								AsicRemovePairwiseKeyEntry(pAd, (pAd->ApCfg.BssidNum + MAX_MESH_NUM + pEntry->apidx), (UCHAR)pEntry->Aid);
							else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
							if (pEntry->ValidAsWDS)
								AsicRemovePairwiseKeyEntry(pAd, pEntry->apidx, (UCHAR)pEntry->Aid);
							else
#endif // WDS_SUPPORT //
								AsicRemovePairwiseKeyEntry(pAd, pEntry->apidx, (UCHAR)pEntry->Aid);

							if ((pEntry->ValidAsCLI) && (pEntry->WepStatus == Ndis802_11WEPEnabled))
							{
								UCHAR KeyIdx	= pAd->ApCfg.MBSSID[pEntry->apidx].DefaultKeyId;
								UCHAR CipherAlg = pAd->SharedKey[pEntry->apidx][KeyIdx].CipherAlg;
							
								RTMPAddWcidAttributeEntry(
															pAd,
															pEntry->apidx,
															KeyIdx,
															CipherAlg,
															pEntry);
							}
						}
#endif // CONFIG_AP_SUPPORT //


						AsicUpdateRxWCIDTable(pAd, pEntry->Aid, pEntry->Addr);
						printk("UpdateRxWCIDTable(): Aid=%d, Addr=%02x:%02x:%02x:%02x:%02x:%02x!\n", pEntry->Aid, 
								pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2], pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
					}
					break;

// add by johnli, fix "in_interrupt" error when call "MacTableDeleteEntry" in Rx tasklet
				case CMDTHREAD_UPDATE_PROTECT:
					{
						AsicUpdateProtect(pAd, 0, (ALLN_SETPROTECT), TRUE, 0);
					}
					break;
#ifdef CONFIG_AP_SUPPORT
				case CMDTHREAD_AP_UPDATE_CAPABILITY_AND_ERPIE:
					{
						APUpdateCapabilityAndErpIe(pAd);
					}
					break;
#endif // CONFIG_AP_SUPPORT //
// end johnli
				    
				case OID_802_11_ADD_WEP:
					{
					}
					break;
					
				case CMDTHREAD_802_11_COUNTER_MEASURE:
#ifdef CONFIG_AP_SUPPORT
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
					{
						MAC_TABLE_ENTRY *pEntry;
						
						pEntry = (MAC_TABLE_ENTRY *)pData;
						HandleCounterMeasure(pAd, pEntry);
					}
#endif // CONFIG_AP_SUPPORT //
					break;
					
#if 0 // Not used currently but maybe used latter, so keep it here.
				case CMDTHREAD_MULTI_READ_MAC:
					{
						PCMDHandler_TLV	p = (PCMDHandler_TLV)cmdqelmt->buffer;
						RTUSBMultiRead(pAd, p->Offset, &p->DataFirst, p->Length);
					}
					break; 
					
				case CMDTHREAD_MULTI_WRITE_MAC:
					{
						PCMDHandler_TLV	p = (PCMDHandler_TLV)cmdqelmt->buffer;
						DBGPRINT_RAW(RT_DEBUG_TRACE, ("CMDTHREAD_MULTI_WRITE_MAC Offset%04x Length%04x\n",p->Offset,p->Length));
						RTUSBMultiWrite(pAd, p->Offset, &p->DataFirst, p->Length);
					}
					break;
        	                                                                                                                                                        
				case CMDTHREAD_VENDOR_EEPROM_READ:
					{
						PCMDHandler_TLV	p = (PCMDHandler_TLV)cmdqelmt->buffer;
						RTUSBReadEEPROM(pAd, p->Offset, &p->DataFirst, p->Length);
					}
					break;
					
				case CMDTHREAD_VENDOR_EEPROM_WRITE:
					{
						PCMDHandler_TLV	p = (PCMDHandler_TLV)cmdqelmt->buffer;
						pData = &p->DataFirst;
						while (p->Length > 62)
						{
							RTUSBWriteEEPROM(pAd, p->Offset, pData, 62);
							p->Offset += 62;
							p->Length -= 62;
							pData += 62;
						}
						RTUSBWriteEEPROM(pAd, p->Offset, pData, p->Length);
					}
					break;
                                                                                                                                                                
				case CMDTHREAD_VENDOR_ENTER_TESTMODE:
						RTUSB_VendorRequest(pAd,
											0,
											DEVICE_VENDOR_REQUEST_OUT,
											0x1,
											0x4,
											0x1,
											NULL,
											0);
						break;                                                                                                                                   
            	                                                                                                                                                    
					case CMDTHREAD_VENDOR_EXIT_TESTMODE:
						RTUSB_VendorRequest(pAd,
											0,
											DEVICE_VENDOR_REQUEST_OUT,
											0x1,
											0x4,
											0x0,
											NULL,
											0);
						break;
            	                                                                                                                                                    

				case CMDTHREAD_SET_RADIO:
					break;
                                                                                                                                          
				case CMDTHREAD_RESET_FROM_ERROR:                                                                                                                       
				case CMDTHREAD_RESET_FROM_NDIS:                                                                                                                        
					/*{
						UINT	i = 0;                                                                                                                                  
            	                                                                                                                                                    
						RTUSBRejectPendingPackets(pAd);//reject all NDIS packets waiting in TX queue                                                                    
						RTUSBCleanUpDataBulkOutQueue(pAd);                                                                                                              
						MlmeSuspend(pAd, FALSE);                                                                                                                        
            	                                                                                                                                                    
						//Add code to access necessary registers here.                                                                                                  
						//disable Rx                                                                                                                                    
						RTUSBWriteMACRegister(pAd, TXRX_CSR2, 1);                                                                                                       
						//Ask our device to complete any pending bulk in IRP.                                                                                           
						while ((atomic_read(&pAd->PendingRx) > 0) ||                                                                                                    
								(pAd->BulkOutPending[0] == TRUE) ||                                                                                                      
						   		(pAd->BulkOutPending[1] == TRUE) ||                                                                                                      
						   		(pAd->BulkOutPending[2] == TRUE) ||                                                                                                      
						   		(pAd->BulkOutPending[3] == TRUE))                                                                                                        
            	                                                                                                                                                    
						{                                                                                                                                               
							if (atomic_read(&pAd->PendingRx) > 0)                                                                                                       
							{                                                                                                                                           
								DBGPRINT_RAW(RT_DEBUG_TRACE, "BulkIn IRP Pending!!!\n");                                                                                
								RTUSB_VendorRequest(pAd,                                                                                                                
													0,                                                                                                                  
													DEVICE_VENDOR_REQUEST_OUT,                                                                                          
													0x0C,                                                                                                               
													0x0,                                                                                                                
													0x0,                                                                                                                
													NULL,                                                                                                               
													0);                                                                                                                 
							}                                                                                                                                           
            	        	                                                                                                                                            
							if ((pAd->BulkOutPending[0] == TRUE) ||                                                                                                     
							(pAd->BulkOutPending[1] == TRUE) ||                                                                                                     
							(pAd->BulkOutPending[2] == TRUE) ||                                                                                                     
							(pAd->BulkOutPending[3] == TRUE))                                                                                                       
							{                                                                                                                                           
								DBGPRINT_RAW(RT_DEBUG_TRACE, ("BulkOut IRP Pending!!!\n"));
								if (i == 0)                                                                                                                             
								{                                                                                                                                       
									RTUSBCancelPendingBulkOutIRP(pAd);                                                                                                  
									i++;                                                                                                                                
								}                                                                                                                                       
							}                                                                                                                                           
							RTMPusecDelay(500000);
						}                                                                                                                                               
            	                                                                                                                                                    
						NICResetFromError(pAd);                                                                                                                         
						if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HARDWARE_ERROR))                                                                                          
						{                                                                                                                                               
							RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_HARDWARE_ERROR);                                                                                         
						}                                                                                                                                               
						if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET))                                                                                            
						{                                                                                                                                               
							RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKIN_RESET);                                                                                           
						}                                                                                                                                               
						if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET))                                                                                           
						{                                                                                                                                               
							RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BULKOUT_RESET);                                                                                          
						}                                                                                                                                               
            	                                                                                                                                                    
						RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS);                                                                                          
            	                                                                                                                                                    
						if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF)) &&                                                                                          
							(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&                                                                                   
							(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST)))                                                                                        
						{                                                                                                                                               
							MlmeResume(pAd);                                                                                                                            
							//
							// Support multiple BulkIn IRP, the value on pAd->CommonCfg.NumOfBulkInIRP may be large than 1.
							//
							for(i = 0; i < pAd->CommonCfg.NumOfBulkInIRP; i ++)
							{
								RTUSBBulkReceive(pAd);
							}      
							RTUSBWriteMACRegister(pAd, TXRX_CSR2, 0x7e);                                                                                                
						}                                                                                                                                               
					}*/                                                                                                                                                   
					break;                                                                                                                                              
					

				case CMDTHREAD_VENDOR_WRITE_BBP:
					{
						UCHAR	Offset, Value, bitmask, BBPValue;
						Offset = *((PUCHAR)pData);
						Value = *((PUCHAR)(pData + 1));
						bitmask = *((PUCHAR)(pData + 2));
						DBGPRINT_RAW(RT_DEBUG_INFO, ("offset = 0x%02x	value = 0x%02x\n", Offset, Value));
						if (bitmask != 0xff)
						{
							RTUSBReadBBPRegister(pAd, Offset, &BBPValue);
							BBPValue&= (~bitmask);
							BBPValue |= Value;
							RTUSBWriteBBPRegister(pAd, Offset, BBPValue);
						}
						else
						{
							RTUSBWriteBBPRegister(pAd, Offset, Value);
						}
					}
					break;
					
				case CMDTHREAD_SET_BW:
					{
						UCHAR	Value, extoffset, centralchannel;
						UCHAR	BBPValue;
						UINT32	MACValue;
						Value = *((PUCHAR)(pData));
						extoffset = *((PUCHAR)(pData+1));
						centralchannel = *((PUCHAR)(pData+2));
						RTUSBReadMACRegister(pAd, TX_BAND_CFG, &MACValue);
						RTUSBReadBBPRegister(pAd, BBP_R3, &BBPValue);
						if ((extoffset == EXTCHA_BELOW))
						{
							MACValue |= 0x1;
							BBPValue |= 0x20;
							RTUSBWriteMACRegister(pAd, TX_BAND_CFG, MACValue);
							RTUSBWriteBBPRegister(pAd, BBP_R3, BBPValue);
							DBGPRINT_RAW(RT_DEBUG_TRACE, (" extoffset == EXTCHA_BELOW\n"));
						}
						else
						{
							MACValue &= 0xfe;
							BBPValue &= (~0x20);
							RTUSBWriteMACRegister(pAd, TX_BAND_CFG, MACValue);
							RTUSBWriteBBPRegister(pAd, BBP_R3, BBPValue);
							DBGPRINT_RAW(RT_DEBUG_TRACE, (" extoffset == EXTCHA_ABOVE\n"));
						} 
						if ((Value%0xf) == BW_40)
						{
							RTUSBReadBBPRegister(pAd, BBP_R4, &BBPValue);
							BBPValue&= (~0x18);
							BBPValue|= (0x10);
							RTUSBWriteBBPRegister(pAd, BBP_R4, BBPValue);
							pAd->CommonCfg.BBPCurrentBW = BW_40;
							AsicSwitchChannel(pAd, centralchannel,FALSE);
							AsicLockChannel(pAd, centralchannel);
							DBGPRINT_RAW(RT_DEBUG_TRACE, (" BBPCurrentBW = BW_40\n"));
						}
						else
						{
							RTUSBReadBBPRegister(pAd, BBP_R4, &BBPValue);
							BBPValue&= (~0x18);
							RTUSBWriteBBPRegister(pAd, BBP_R4, BBPValue);
							pAd->CommonCfg.BBPCurrentBW = BW_20;
							DBGPRINT_RAW(RT_DEBUG_TRACE, (" BBPCurrentBW = BW_20\n"));
						}
					}
					break;

				case CMDTHREAD_VENDOR_READ_BBP:
					{
						UCHAR	Offset = *((PUCHAR)pData);
						PUCHAR	pValue = (PUCHAR)(pData + 1);

						DBGPRINT_RAW(RT_DEBUG_INFO, ("offset = 0x%02x\n", Offset));
						RTUSBReadBBPRegister(pAd, Offset, pValue);
						DBGPRINT_RAW(RT_DEBUG_INFO, ("value = 0x%02x\n", *pValue));
					}
					break;

				case CMDTHREAD_VENDOR_WRITE_RF:
					{
						UINT32	Value = *((PUINT32)pData);

						DBGPRINT_RAW(RT_DEBUG_INFO, ("value = 0x%08x\n", Value));
						RTUSBWriteRFRegister(pAd, Value);
					}
					break;
				    
				case RT_OID_802_11_RESET_COUNTERS:
					{
//						UCHAR	Value[22];

						//RTUSBMultiRead(pAd, STA_CSR0, Value, 24);
					}
					break;

				case CMDTHREAD_VENDOR_RESET:
					RTUSB_VendorRequest(pAd,
											0,
											DEVICE_VENDOR_REQUEST_OUT,
											1,
											1,
											0,
											NULL,
											0);
					break;

				case CMDTHREAD_VENDOR_UNPLUG:
					RTUSB_VendorRequest(pAd,
										0,
										DEVICE_VENDOR_REQUEST_OUT,
										1,
										2,
										0,
										NULL,
										0);
					break;
      	                                                                                                                                                    
				case CMDTHREAD_VENDOR_SWITCH_FUNCTION:                                                                                                             
					DBGPRINT_RAW(RT_DEBUG_ERROR, ("CMDTHREAD_VENDOR_SWITCH_FUNCTION -- NOT SUPPORT !!\n"));
					break;                                                                                                                                              
			                                                                                                                                                
				case CMDTHREAD_VENDOR_FLIP_IQ:
					{
						UINT32	Value1, Value2;
						//RTUSBReadMACRegister(pAd, PHY_CSR5, &Value1);
						//RTUSBReadMACRegister(pAd, PHY_CSR6, &Value2);
						if (*pData == 1)
						{
							DBGPRINT_RAW(RT_DEBUG_ERROR, ("I/Q Flip\n"));
							Value1 = Value1 | 0x0004;
							Value2 = Value2 | 0x0004;
						}
						else
						{
							DBGPRINT_RAW(RT_DEBUG_ERROR, ("I/Q Not Flip\n"));
							Value1 = Value1 & 0xFFFB;
							Value2 = Value2 & 0xFFFB;
						}
						///RTUSBWriteMACRegister(pAd, PHY_CSR5, Value1);
						//RTUSBWriteMACRegister(pAd, PHY_CSR6, Value2);
					}
					break;

				case CMDTHREAD_UPDATE_TX_RATE:					
					NdisZeroMemory(pAd->CommonCfg.DesireRate, MAX_LEN_OF_SUPPORTED_RATES);
					NdisMoveMemory(pAd->CommonCfg.DesireRate, (PUCHAR)pData, sizeof(NDIS_802_11_RATES));
					MlmeUpdateTxRates(pAd, FALSE);
					break;

				case CMDTHREAD_802_11_SET_PREAMBLE:
					{
						USHORT	Preamble = *((PUSHORT)(cmdqelmt->buffer));
						if (Preamble == Rt802_11PreambleShort)
						{
							pAd->CommonCfg.TxPreamble = Preamble;
							MlmeSetTxPreamble(pAd, Rt802_11PreambleShort);
						}
						else if ((Preamble == Rt802_11PreambleLong) || (Preamble == Rt802_11PreambleAuto))
						{
							// if user wants AUTO, initialize to INT32 here, then change according to AP's
							// capability upon association.
							pAd->CommonCfg.TxPreamble = Preamble;
							MlmeSetTxPreamble(pAd, Rt802_11PreambleLong);
						}
						else
							NdisStatus = NDIS_STATUS_INVALID_DATA;
						DBGPRINT(RT_DEBUG_ERROR, ("Set::CMDTHREAD_802_11_SET_PREAMBLE (=%d)\n", Preamble));
					}
					break;

				case OID_802_11_NETWORK_TYPE_IN_USE:
					{
						NDIS_802_11_NETWORK_TYPE	NetType = *(PNDIS_802_11_NETWORK_TYPE)(cmdqelmt->buffer);
						if (NetType == Ndis802_11DS)
							RTMPSetPhyMode(pAd, PHY_11B);
						else if (NetType == Ndis802_11OFDM24)
							RTMPSetPhyMode(pAd, PHY_11BG_MIXED);
						else if (NetType == Ndis802_11OFDM5)
							RTMPSetPhyMode(pAd, PHY_11A);
						else
							NdisStatus = NDIS_STATUS_INVALID_DATA;
						DBGPRINT(RT_DEBUG_ERROR, ("CmdThread::OID_802_11_NETWORK_TYPE_IN_USE (=%d)\n",NetType));
					}
					break;

				case CMDTHREAD_802_11_SET_PHY_MODE:
					{
						UINT32	phymode = *(UINT32 *)(cmdqelmt->buffer);
						pAd->CommonCfg.PhyMode = 0xff;
						RTMPSetPhyMode(pAd, phymode);
						DBGPRINT(RT_DEBUG_ERROR, ("CmdThread::CMDTHREAD_802_11_SET_PHY_MODE (=%x)\n", phymode));
					}
					break;            	                                                                                                                                                    

				case OID_802_11_REMOVE_WEP:
					{
						UINT32		KeyIdx;

						
						KeyIdx = *(NDIS_802_11_KEY_INDEX *) pData;
						if (KeyIdx & 0x80000000)
						{
							NdisStatus = NDIS_STATUS_INVALID_DATA;
							DBGPRINT(RT_DEBUG_ERROR, ("CmdThread::OID_802_11_REMOVE_WEP, INVALID_DATA!!\n"));
						}
						else
						{
							KeyIdx = KeyIdx & 0x0fffffff;
							if (KeyIdx >= 4)
							{
								NdisStatus = NDIS_STATUS_INVALID_DATA;
								DBGPRINT(RT_DEBUG_ERROR, ("CmdThread::OID_802_11_REMOVE_WEP, Invalid KeyIdx[=%d]!!\n", KeyIdx));
							}
							else
							{
								pAd->SharedKey[BSS0][KeyIdx].KeyLen = 0;
								pAd->SharedKey[BSS0][KeyIdx].CipherAlg = CIPHER_NONE;
								AsicRemoveSharedKeyEntry(pAd, BSS0, (UCHAR)KeyIdx);
								DBGPRINT(RT_DEBUG_TRACE, ("CmdThread::OID_802_11_REMOVE_WEP (KeyIdx=%d)\n", KeyIdx));
							}
						}	
					}
					break;

				case CMDTHREAD_802_11_ADD_KEY_WEP:
					{
						PNDIS_802_11_KEY		pKey;
						UINT32					KeyIdx;						
						USHORT					i;						

						pKey = (PNDIS_802_11_KEY) pData;
						KeyIdx = pKey->KeyIndex & 0x0fffffff;
							 
						 // it is a shared key
						 if (KeyIdx >= 4)
						 {
							 NdisStatus = NDIS_STATUS_INVALID_DATA;
							 DBGPRINT(RT_DEBUG_ERROR, ("CmdThread::CMDTHREAD_802_11_ADD_KEY_WEP, Invalid KeyIdx[=%d]!!\n", KeyIdx));
						 }
						 else 
						 {
							 UCHAR CipherAlg;
							 
							 pAd->SharedKey[BSS0][KeyIdx].KeyLen = (UCHAR) pKey->KeyLength;
							 NdisMoveMemory(pAd->SharedKey[BSS0][KeyIdx].Key, &pKey->KeyMaterial, pKey->KeyLength);

							 if (pKey->KeyLength == 5)
								 CipherAlg = CIPHER_WEP64;
							 else
								 CipherAlg = CIPHER_WEP128;
							pAd->SharedKey[BSS0][KeyIdx].CipherAlg = CipherAlg;
							if (pAd->OpMode == OPMODE_STA)
							{
								pAd->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = pAd->SharedKey[BSS0][KeyIdx].CipherAlg;
								pAd->MacTab.Content[BSSID_WCID].PairwiseKey.KeyLen = pAd->SharedKey[BSS0][KeyIdx].KeyLen;
						 	}
							if (pKey->KeyIndex & 0x80000000)
							{
							}

							AsicAddSharedKeyEntry(pAd, BSS0, (UCHAR)KeyIdx, CipherAlg, pAd->SharedKey[BSS0][KeyIdx].Key, NULL, NULL);
							DBGPRINT(RT_DEBUG_TRACE, ("CmdThread::CMDTHREAD_802_11_ADD_KEY_WEP (KeyLen=%d, CipherAlg=%d)\n", pAd->SharedKey[BSS0][KeyIdx].KeyLen, pAd->SharedKey[BSS0][KeyIdx].CipherAlg));
							// ad-hoc mode need to specify group key with WCID index=BSS0Mcast_WCID. Let's always set this key here.
							if (pKey->KeyIndex & 0x80000000)
							{
								UCHAR	IVEIV[8];
								UINT32	WCIDAttri, Value;
								USHORT	offset;
								NdisZeroMemory(IVEIV, 8);
								// 1. IV/EIV
								// Specify key index to find shared key.
								IVEIV[3] = (UCHAR)(KeyIdx<< 6);	//WEP Eiv bit off. groupkey index is not 0
								offset = PAIRWISE_IVEIV_TABLE_BASE + (BSS0Mcast_WCID * HW_IVEIV_ENTRY_SIZE);
								for (i=0; i<8;)
								{
									Value = IVEIV[i];
									Value += (IVEIV[i+1]<<8);
									Value += (IVEIV[i+2]<<16);
									Value += (IVEIV[i+3]<<24);
									RTUSBWriteMACRegister(pAd, offset+i, Value);
									i+=4;
								}

								// 2. WCID Attribute UDF:3, BSSIdx:3, Alg:3, Keytable:use share key, BSSIdx is 0
								WCIDAttri = (pAd->SharedKey[BSS0][KeyIdx].CipherAlg<<1)|SHAREDKEYTABLE;
								offset = MAC_WCID_ATTRIBUTE_BASE + (BSS0Mcast_WCID* HW_WCID_ATTRI_SIZE);
							 	DBGPRINT(RT_DEBUG_TRACE, ("BSS0Mcast_WCID : offset = %x, WCIDAttri = %x\n", offset, WCIDAttri));
								RTUSBWriteMACRegister(pAd, offset, WCIDAttri);
								
							}

						}
					}
					break;

				case OID_802_11_ADD_KEY:
					{  
//						NdisStatus = RTMPWPAAddKeyProc(pAd, pData); // sample
						DBGPRINT(RT_DEBUG_TRACE, ("CmdThread::OID_802_11_ADD_KEY\n"));
					}
					break;
                                                                                                                                                                                                                                                                              
				case OID_802_11_REMOVE_KEY:
					{
					}
					break;

				case RT_OID_802_11_RSSI_1:
					{
					}
						
					break;

				case RT_OID_802_11_RSSI_2:
					{
					}
					break;

				case OID_802_11_POWER_MODE:
					{
					}					
					break;

				case CMDTHREAD_FORCE_WAKE_UP:
					AsicForceWakeup(pAd);
					break;
#endif
				default:
					DBGPRINT(RT_DEBUG_ERROR, ("--> Control Thread !! ERROR !! Unknown(cmdqelmt->command=0x%x) !! \n", cmdqelmt->command));
					break;                                                                                                                                              
			}                                                                                                                                                       
		}                                                                                                                                                          
                                                                                                                                                                
		if (cmdqelmt->CmdFromNdis == TRUE)
		{
				if (cmdqelmt->buffer != NULL)
					NdisFreeMemory(cmdqelmt->buffer, cmdqelmt->bufferlength, 0);
			                                                                                                                                                    
			NdisFreeMemory(cmdqelmt, sizeof(CmdQElmt), 0);
		}
		else
		{
			if ((cmdqelmt->buffer != NULL) && (cmdqelmt->bufferlength != 0))
				NdisFreeMemory(cmdqelmt->buffer, cmdqelmt->bufferlength, 0);
            {
				NdisFreeMemory(cmdqelmt, sizeof(CmdQElmt), 0);
			}
		}
	}	/* end of while */
}

