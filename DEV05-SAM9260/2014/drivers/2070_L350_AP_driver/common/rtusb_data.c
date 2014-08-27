/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2005, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rtusb_data.c

	Abstract:
	Ralink USB driver Tx/Rx functions.

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
	Jan            03-25-2006    created

*/
#include	"rt_config.h"

extern  UCHAR Phy11BGNextRateUpward[]; // defined in mlme.c
extern UCHAR	EpToQueue[];


#if 0 // sample
VOID REPORT_ETHERNET_FRAME_TO_LLC(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			p8023hdr,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize)
{
	struct sk_buff	*pSkb;
	ULONG	Now;
	
	/* allocate a rx packet */
	pSkb = dev_alloc_skb(DataSize + LENGTH_802_3 + 2);
	if (pSkb)
	{
		/* convert 802.11 to 802.3 packet */
		pSkb->dev = get_netdev_from_bssid(pAd, BSS0);
		skb_reserve(pSkb, 2);	// 16 byte align the IP header
		memcpy(skb_put(pSkb, LENGTH_802_3), p8023hdr, LENGTH_802_3);
		memcpy(skb_put(pSkb, DataSize), pData, DataSize);

		RTMP_SET_PACKET_SOURCE(OSPKT_TO_RTPKT(pSkb), PKTSRC_NDIS);

		/* pass this packet to upper layer */
		pSkb->protocol = eth_type_trans(pSkb, get_netdev_from_bssid(pAd, 0));
		netif_rx(pSkb);

		/* update status */
		Now = pAd->net_dev->last_rx;
		NdisGetSystemUpTime(&Now);
		pAd->stats.rx_packets++;
		pAd->Counters8023.GoodReceives++;
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,("Can't allocate skb\n"));
	}
}

static UINT deaggregate_AMSDU_announce(
	IN	PRTMP_ADAPTER	pAd,
	PNDIS_PACKET		pPacket,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize)	
{
	USHORT 			PayloadSize;
	USHORT 			SubFrameSize; 		
	UINT			nMSDU;
	UINT			tSize = DataSize;
	PUCHAR			pDataPos = pData;
	UINT			TotalLen = 0;

	nMSDU = 0;
	while (tSize > LENGTH_802_3)
	{
		nMSDU++;

		PayloadSize = (*(pDataPos+13)) + ((*(pDataPos+12))<<8);
		SubFrameSize = PayloadSize + LENGTH_802_3;
		TotalLen += SubFrameSize;

		if ((tSize < SubFrameSize) || (PayloadSize == 0 ) || (TotalLen > 4000))
		{
			DBGPRINT(RT_DEBUG_ERROR,("A-MSDU Size = %d =====>\n", tSize));
			hex_dump("A-MSDU", pDataPos, 32 );
			DBGPRINT(RT_DEBUG_ERROR,("A-MSDU break: %d subframe,  len = %d! \n", nMSDU, SubFrameSize));
			DBGPRINT(RT_DEBUG_ERROR,("<============\n"));
			break;
		}

		pDataPos += LENGTH_802_3;
		if (RTMPEqualMemory(SNAP_802_1H, pDataPos, 6) || RTMPEqualMemory(SNAP_BRIDGE_TUNNEL, pDataPos, 6))
		{
			UCHAR	Header802_3[LENGTH_802_3]={0};
			
			DBGPRINT(RT_DEBUG_INFO,("A-MSDU!Has SNAP LLC format PayloadSize = %d. pro = %x %x %x %x.\n",PayloadSize, *(pDataPos+6),*(pDataPos+7), *(pDataPos+8),*(pDataPos+9)));

			NdisMoveMemory(&Header802_3[0], (pDataPos - LENGTH_802_3), 12);
		
			if ((RTMPEqualMemory(IPX, pDataPos+6, 2) || RTMPEqualMemory(APPLE_TALK, pDataPos+6, 2)) &&  \
				RTMPEqualMemory(SNAP_802_1H, pDataPos, 6))
			{
				CHAR	LLC_Len[2];

				LLC_Len[0] = (UCHAR)(PayloadSize / 256);
				LLC_Len[1] = (UCHAR)(PayloadSize % 256); 
				NdisMoveMemory(&Header802_3[12], LLC_Len, LENGTH_802_3_TYPE);
			}
			else
			{			
				NdisMoveMemory(&Header802_3[12], (pDataPos + 6), LENGTH_802_3_TYPE); 
			}

			append_pkt(pAd, Header802_3, LENGTH_802_3, (pDataPos + LENGTH_802_1_H), (PayloadSize - LENGTH_802_1_H), &pPacket);
			clone_pkt_and_pass_up(pAd, pPacket, (PayloadSize + LENGTH_802_3 - LENGTH_802_1_H));
		}
		else
		{
			DBGPRINT(RT_DEBUG_INFO,("A-MSDU!No SNAP LLC format Payload1Size = %d. pro = %x %x %x %x.\n",PayloadSize, *(pDataPos+6),*(pDataPos+7), *(pDataPos+8),*(pDataPos+9)));

			append_pkt(pAd, pDataPos, LENGTH_802_3, (pDataPos + LENGTH_802_3), (PayloadSize - LENGTH_802_3), &pPacket);
			clone_pkt_and_pass_up(pAd, pPacket, PayloadSize);

		}

		// Add padded bytes
		TotalLen += (((SubFrameSize+3)&(~0x3)) - SubFrameSize);	

		// A-MSDU has padding to multiple of 4 including subframe header.
		// align SubFrameSize up to multiple of 4 
		SubFrameSize = (SubFrameSize+3)&(~0x3);
		
		pAd->Counters8023.GoodReceives++;
		if (tSize > SubFrameSize)
		{
			pDataPos += (SubFrameSize - LENGTH_802_3);
			tSize -= SubFrameSize;
		}
		else
		{   
			// end of A-MSDU
			tSize = 0;
		}
	}
	
	// finally release original rx packet 
	RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);

	return nMSDU;
}
#endif

VOID REPORT_AMSDU_FRAMES_TO_LLC(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pData,
	IN	ULONG			DataSize)
{	
	PNDIS_PACKET	pPacket;
	UINT			nMSDU;
	struct			sk_buff *pSkb;

	nMSDU = 0;
	/* allocate a rx packet */
	pSkb = dev_alloc_skb(RX_BUFFER_AGGRESIZE);
	pPacket = (PNDIS_PACKET)OSPKT_TO_RTPKT(pSkb);
	if (pSkb)
	{

		/* convert 802.11 to 802.3 packet */
		pSkb->dev = get_netdev_from_bssid(pAd, BSS0);		
		RTMP_SET_PACKET_SOURCE(pPacket, PKTSRC_NDIS);
		deaggregate_AMSDU_announce(pAd, pPacket, pData, DataSize);
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR,("Can't allocate skb\n"));
	}
}


#if 0
/*
	========================================================================
	
	Routine Description:
		SendPackets handler

	Arguments:
		skb 			point to sk_buf which upper layer transmit
		net_dev 		point to net_dev
	Return Value:
		None

	Note:
	
	========================================================================
*/
INT STASendPackets(
	IN	struct sk_buff		*pSkb,
	IN	PNET_DEV            net_dev)
{
	PRTMP_ADAPTER		pAd = net_dev->priv;
//	NDIS_STATUS			Status = NDIS_STATUS_SUCCESS;
	struct sk_buff		*pkt = (struct sk_buff *) pSkb;
	PNDIS_PACKET		pPacket = (PNDIS_PACKET) pkt;
	INT					Index;

        DBGPRINT(RT_DEBUG_INFO, ("===> STASendPackets\n"));
        // EapolStart size is 18
	if (pSkb->len < 14) 
	{
		DBGPRINT(RT_DEBUG_ERROR, (">> STASendPackets: bad packet size (%d)\n", pkt->len));
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return 0;
	}
	

#if 0	
	if (pAd->bPromiscuous == TRUE)
	{
		for (Index = 0; Index < NumberOfPackets; Index++)
		{
			// Drop send request since we are in monitor mode
			NdisMSendComplete(
				pAd->AdapterHandle,
				ppPacketArray[Index],
				NDIS_STATUS_FAILURE);
		}
		return;
	}
#endif

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
		RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
	{
		// Drop send request since hardware is in reset state
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);		
		return 0;
	}
	// Drop packets if no associations
	else if (!INFRA_ON(pAd) && !ADHOC_ON(pAd))
	{ 
		// Drop send request since there are no physical connection yet
		// Check the association status for infrastructure mode
		// And Mibss for Ad-hoc mode setup
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return 0;

	}
	else
	{
		RTMP_SET_PACKET_WCID(pPacket, 0); // this field is useless when in STA mode
		RTMP_SET_PACKET_SOURCE(pPacket, PKTSRC_NDIS);
		NDIS_SET_PACKET_STATUS(pPacket, NDIS_STATUS_PENDING);
		pAd->RalinkCounters.PendingNdisPacketCount ++;

		// This function has to manage NdisSendComplete return call within its routine
		// NdisSendComplete will acknowledge upper layer in two steps.
		// 1. Within Packet Enqueue, set the NDIS_STATUS_PENDING
		// 2. Within TxRingTxDone / PrioRingTxDone call NdisSendComplete with final status
		// Be careful about how/when to release this internal allocated NDIS PACKET buffer !
		STASendPacket(pAd, pPacket);
		
	}

	// Dequeue one frame from SendTxWait queue and process it
	// There are two place calling dequeue for TX ring.
	// 1. Here, right after queueing the frame.
	// 2. At the end of TxRingTxDone service routine.
	if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)) && 
		(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF)) &&
		(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
		(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)))
	{
		for (Index = 0; Index < 4; Index++)
		{
			if(pAd->TxSwQueue[Index].Number > 0)
			{
				RTMPUSBDeQueuePacket(pAd, Index);
			}
		}
	}	

	// Kick bulk out
	RTUSBKickBulkOut(pAd);

	return 0;
}

/*
	========================================================================

	Routine	Description:
		This routine classifies outgoing frames into several AC (Access
		Category) and enqueue them into corresponding s/w waiting queues.
		
	Arguments:
		pAd Pointer to our adapter
		pPacket 	Pointer to send packet
		
	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
NDIS_STATUS	STASendPacket(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket)
{
	PACKET_INFO			PacketInfo;
	PUCHAR				pSrcBufVA;
	UINT				SrcBufLen;
	UINT				AllowFragSize;
	UCHAR				NumberOfFrag;
	UCHAR				RTSRequired;
	NDIS_STATUS			Status = NDIS_STATUS_FAILURE;
	UCHAR				PsMode;
	UCHAR				QueIdx, UserPriority;
	PQUEUE_HEADER		pTxQueue;
	
	DBGPRINT(RT_DEBUG_INFO, ("===> STASendPacket\n"));

	// Prepare packet information structure for buffer descriptor 
	// chained within a single NDIS packet.
#ifdef WIN_NDIS
	NdisQueryPacket(
		pPacket,							// Ndis packet
		&PacketInfo.PhysicalBufferCount,	// Physical buffer count
		&PacketInfo.BufferCount,			// Number of buffer descriptor
		&PacketInfo.pFirstBuffer,			// Pointer to first buffer descripotr
		&PacketInfo.TotalPacketLength);		// Ndis packet length

	NDIS_QUERY_BUFFER(PacketInfo.pFirstBuffer, &pSrcBufVA, &SrcBufLen);
#else
	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);
#endif

	// Check for virtual address allocation, it might fail !!!
	if (pSrcBufVA == NULL)
	{
		DBGPRINT(RT_DEBUG_ERROR,("STASendPacket --> pSrcBufVA == NULL !!!SrcBufLen=%x\n",SrcBufLen));
		// Resourece is low, system did not allocate virtual address
		// return NDIS_STATUS_FAILURE directly to upper layer
		RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		return NDIS_STATUS_FAILURE;
	}

	//
	// Check for multicast or broadcast (First byte of DA)
	//
	if ((*((PUCHAR) pSrcBufVA) & 0x01) != 0)
	{
		// For multicast & broadcast, there is no fragment allowed
		NumberOfFrag = 1;
	}
	else if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED))
	{
		NumberOfFrag = 1;   // Aggregation overwhelms fragmentation
	}
	else if ((pAd->CommonCfg.HTPhyMode.field.MODE == MODE_HTMIX) || (pAd->CommonCfg.HTPhyMode.field.MODE == MODE_HTGREENFIELD))
	{
		NumberOfFrag = 1;	// MIMO RATE overwhelms fragmentation
	}
#ifdef LEAP_SUPPORT	
	else if (CKIP_CMIC_ON(pAd))
	{
		//
		// Need to Insert CKIP MIC on the beginning of the payload.
		// It's to complex to do fragment, and it's ok on CCX1 & CC2.
		//
		NumberOfFrag = 1;
	}
#endif // LEAP_SUPPORT //
	else
	{
		// Check for payload allowed for each fragment
		AllowFragSize = (pAd->CommonCfg.FragmentThreshold) - LENGTH_802_11 - LENGTH_CRC;

		// Calculate fragments required
		NumberOfFrag = ((PacketInfo.TotalPacketLength - LENGTH_802_3 + LENGTH_802_1_H) / AllowFragSize) + 1;
		// Minus 1 if the size just match to allowable fragment size
		if (((PacketInfo.TotalPacketLength - LENGTH_802_3 + LENGTH_802_1_H) % AllowFragSize) == 0)
		{
			NumberOfFrag--;
		}
	}

	// STEP 2. 	Check the requirement of RTS:
	//			If multiple fragment required, RTS is required only for the first fragment
	//			if the fragment size large than RTS threshold
	
	if (NumberOfFrag > 1)
		RTSRequired = (pAd->CommonCfg.FragmentThreshold > pAd->CommonCfg.RtsThreshold) ? 1 : 0;
	else
		RTSRequired = (PacketInfo.TotalPacketLength > pAd->CommonCfg.RtsThreshold) ? 1 : 0;

	DBGPRINT(RT_DEBUG_INFO, ("Number of fragments include RTS :%d\n", NumberOfFrag + RTSRequired));

	// RTS/CTS may also be required in order to protect OFDM frame
	if ((pAd->CommonCfg.TxRate >= RATE_FIRST_OFDM_RATE) && 
		(pAd->CommonCfg.TxRate <= RATE_LAST_OFDM_RATE) &&
		OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED))
	{
		RTSRequired = 1;
	}
	else
	{
		//RT2860b MIMO RTS protection rule
	}
	
	// Save fragment number to Ndis packet reserved field
	RTMP_SET_PACKET_FRAGMENTS(pPacket, NumberOfFrag);

	RTSRequired = 0;	// Let ASIC  send RTS/CTS
	// Save RTS requirement to Ndis packet reserved field
	RTMP_SET_PACKET_RTS(pPacket, RTSRequired);
	RTMP_SET_PACKET_TXRATE(pPacket, pAd->CommonCfg.TxRate);

	//
	// STEP 3. Traffic classification. outcome = <UserPriority, QueIdx>
	//
	UserPriority = 0;
	QueIdx       = QID_AC_BE;

	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED))
	{
		USHORT Protocol;
		UCHAR  LlcSnapLen = 0, Byte0, Byte1;

		do
		{
			// get Ethernet protocol field
			Protocol = (USHORT)((pSrcBufVA[12] << 8) + pSrcBufVA[13]);
			if (Protocol <= 1500)
			{
				// get Ethernet protocol field from LLC/SNAP
				if (Sniff2BytesFromNdisBuffer(PacketInfo.pFirstBuffer, LENGTH_802_3 + 6, &Byte0, &Byte1) != NDIS_STATUS_SUCCESS)
					break;

				Protocol = (USHORT)((Byte0 << 8) + Byte1);
				LlcSnapLen = 8;
			}

			// always AC_BE for non-IP packet
			if (Protocol != 0x0800)
				break;

			// get IP header
			if (Sniff2BytesFromNdisBuffer(PacketInfo.pFirstBuffer, LENGTH_802_3 + LlcSnapLen, &Byte0, &Byte1) != NDIS_STATUS_SUCCESS)
				break;

			// return AC_BE if packet is not IPv4
			if ((Byte0 & 0xf0) != 0x40)
				break;

			UserPriority = (Byte1 & 0xe0) >> 5;
			QueIdx = MapUserPriorityToAccessCategory[UserPriority];
#if 0 // Not support yet!
			// TODO: have to check ACM bit. apply TSPEC if ACM is ON
			// TODO: downgrade UP & QueIdx before passing ACM
			if (pAd->CommonCfg.APEdcaParm.bACM[QueIdx])
			{
				UserPriority = 0;
				QueIdx       = QID_AC_BE;
			}
#endif
		} while (FALSE);
	}

	RTMP_SET_PACKET_UP(pPacket, UserPriority);

	// Make sure SendTxWait queue resource won't be used by other threads
	NdisAcquireSpinLock(&pAd->SendTxWaitQueueLock[QueIdx]);

	pTxQueue = &pAd->TxSwQueue[QueIdx];
	
	//
	// For infrastructure mode, enqueue this frame immediately to sendwaitqueue
	// For Ad-hoc mode, check the DA power state, then decide which queue to enqueue
	//
	if (INFRA_ON(pAd))
	{
		// In infrastructure mode, simply enqueue the packet into Tx waiting queue.
		DBGPRINT(RT_DEBUG_INFO, ("Infrastructure -> Enqueue one frame\n"));
		
		// Enqueue Ndis packet to end of Tx wait queue
		InsertTailQueue(pTxQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
		Status = NDIS_STATUS_SUCCESS;
	}
	else
	{
		// In IBSS mode, power state of destination should be considered.
		PsMode = PWR_ACTIVE;		// Faked
		if (PsMode == PWR_ACTIVE)
		{
			// Enqueue Ndis packet to end of Tx wait queue
			InsertTailQueue(pTxQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
			Status = NDIS_STATUS_SUCCESS;
		}
	}
	
	NdisReleaseSpinLock(&pAd->SendTxWaitQueueLock[QueIdx]);

	return NDIS_STATUS_SUCCESS;
}


/*
	========================================================================

	Routine	Description:
		To do the enqueue operation and extract the first item of waiting 
		list. If a number of available shared memory segments could meet 
		the request of extracted item, the extracted item will be fragmented
		into shared memory segments.
		
	Arguments:
		pAd	Pointer	to our adapter
		pQueue		Pointer to Waiting Queue
		
	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	Note:
	
	========================================================================
*/
VOID	RTMPUSBDeQueuePacket(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			BulkOutPipeId)
{
	PQUEUE_ENTRY		pEntry;
	PNDIS_PACKET		pPacket;
	UCHAR				FragmentRequired;
	NDIS_STATUS			Status;
	UCHAR				Count = 0;
	PQUEUE_HEADER		pQueue;
	UCHAR				QueIdx;
	
	NdisAcquireSpinLock(&pAd->DeQueueLock[BulkOutPipeId]);
	if (pAd->DeQueueRunning[BulkOutPipeId])
	{
		NdisReleaseSpinLock(&pAd->DeQueueLock[BulkOutPipeId]);
		return;
	}
	else
	{
		pAd->DeQueueRunning[BulkOutPipeId] = TRUE;
		NdisReleaseSpinLock(&pAd->DeQueueLock[BulkOutPipeId]);
	}

	QueIdx = BulkOutPipeId;
	

	// Make sure SendTxWait queue resource won't be used by other threads
	NdisAcquireSpinLock(&pAd->SendTxWaitQueueLock[BulkOutPipeId]);

	// Select Queue
	pQueue = &pAd->TxSwQueue[BulkOutPipeId];
	
	// Check queue before dequeue
	while ((pQueue->Head != NULL) && (Count < MAX_TX_PROCESS))
	{
		// Reset is in progress, stop immediately
		if ( RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
			 RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS) ||
			 RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
		{
			DBGPRINT(RT_DEBUG_ERROR,("--RTMPUSBDeQueuePacket %d reset-in-progress !!--\n", BulkOutPipeId));
			break;
		}
			
		// Dequeue the first entry from head of queue list
		pEntry = RemoveHeadQueue(pQueue);

		// Retrieve Ndis packet pointer from MiniportReservedEx field
		pPacket = QUEUE_ENTRY_TO_PKT(pEntry);
		// RTS or CTS-to-self for B/G protection mode has been set already.
		// There is no need to re-do it here. 
		// Total fragment required = number of fragment + RST if required
		FragmentRequired = RTMP_GET_PACKET_FRAGMENTS(pPacket) + RTMP_GET_PACKET_RTS(pPacket);
		
		if ((RTUSBFreeDescriptorRequest(pAd, TX_RING, BulkOutPipeId, FragmentRequired) == NDIS_STATUS_SUCCESS)) 
		{
			// Avaliable ring descriptors are enough for this frame
			// Call hard transmit
			// Nitro mode / Normal mode selection
			NdisReleaseSpinLock(&pAd->SendTxWaitQueueLock[BulkOutPipeId]);
			
			Status = RTUSBHardTransmit(pAd, pPacket, FragmentRequired, QueIdx);

			// Make sure SendTxWait queue resource won't be used by other threads
			NdisAcquireSpinLock(&pAd->SendTxWaitQueueLock[BulkOutPipeId]);
			// check status
			if (Status == NDIS_STATUS_FAILURE)
			{
				// Packet failed due to various Ndis Packet error
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
				break;
			}
			else if (Status == NDIS_STATUS_RESOURCES)
			{
				// Not enough free tx ring, it might happen due to free descriptor inquery might be not correct
				// It also might change to NDIS_STATUS_FAILURE to simply drop the frame
				// Put the frame back into head of queue
				DBGPRINT(RT_DEBUG_ERROR,("RESOURSE NOT ENOUGH %d queue, pPacket = 0x%lx\n", BulkOutPipeId,(ULONG)pPacket));
				InsertHeadQueue(pQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
				break;
			}			
			Count++;
		}	
		else
		{
			DBGPRINT(RT_DEBUG_ERROR,("--RTMPUSBDeQueuePacket %d queue full !!  pPacket = 0x%lx\n", BulkOutPipeId,(ULONG)pPacket));
			InsertHeadQueue(pQueue, PACKET_TO_QUEUE_ENTRY(pPacket));
			break;
		}
	}

	// Release TxSwQueue0 resources
	NdisReleaseSpinLock(&pAd->SendTxWaitQueueLock[BulkOutPipeId]);
	
	NdisAcquireSpinLock(&pAd->DeQueueLock[BulkOutPipeId]);
	pAd->DeQueueRunning[BulkOutPipeId] = FALSE;
	NdisReleaseSpinLock(&pAd->DeQueueLock[BulkOutPipeId]);
}
#endif 


/*
	========================================================================

	Routine	Description:
		This subroutine will scan through releative ring descriptor to find
		out avaliable free ring descriptor and compare with request size.
		
	Arguments:
		pAd	Pointer	to our adapter
		RingType	Selected Ring
		
	Return Value:
		NDIS_STATUS_FAILURE		Not enough free descriptor
		NDIS_STATUS_SUCCESS		Enough free descriptor

	Note:
	
	========================================================================
*/
#if 0
NDIS_STATUS	RTUSBFreeDescriptorRequest(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			RingType,
	IN	UCHAR			BulkOutPipeId,
	IN	UINT32			NumberRequired)
{
	UCHAR			FreeNumber = 0;
	UINT			Index;
	NDIS_STATUS		Status = NDIS_STATUS_FAILURE;
	unsigned long   IrqFlags;
	HT_TX_CONTEXT	*pHTTXContext;

	
	switch (RingType)
	{
		case TX_RING:
			pHTTXContext = &pAd->TxContext[BulkOutPipeId];
			RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);
			if ((pHTTXContext->CurWritePosition < pHTTXContext->NextBulkOutPosition) && ((pHTTXContext->CurWritePosition + LOCAL_TXBUF_SIZE) > pHTTXContext->NextBulkOutPosition))
			{
				DBGPRINT(RT_DEBUG_INFO,("RTUSBFreeD c1 --> CurWritePosition = %ld, NextBulkOutPosition = %ld. \n", pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition));
				printk("RTUSBFreeD c1 --> CurWritePosition = %ld, NextBulkOutPosition = %ld. \n", pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition);
				RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
			}
			else if ((pHTTXContext->CurWritePosition == 8) && (pHTTXContext->NextBulkOutPosition < LOCAL_TXBUF_SIZE))
			{
				DBGPRINT(RT_DEBUG_INFO,("RTUSBFreeD c2 --> CurWritePosition = %ld, NextBulkOutPosition = %ld. \n", pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition));
				printk("RTUSBFreeD c2 --> CurWritePosition = %ld, NextBulkOutPosition = %ld. \n", pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition);
				RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
			}
			else
			{
				Status = NDIS_STATUS_SUCCESS;
			}
 			RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);
			
			break;
			
		case PRIO_RING:
			Index = pAd->NextMLMEIndex;
			do
			{
				PTX_CONTEXT	pTxD  = &pAd->MLMEContext[Index];
				
				// While Owner bit is NIC, obviously ASIC still need it.
				// If valid bit is TRUE, indicate that TxDone has not process yet
				// We should not use it until TxDone finish cleanup job
				if (pTxD->InUse == FALSE)
				{
					// This one is free
					FreeNumber++;
				}
				else
				{
					break;
				}
					
				Index = (Index + 1) % MGMT_RING_SIZE;				
			}	while (FreeNumber < NumberRequired);	// Quit here ! Free number is enough !

			if (FreeNumber >= NumberRequired)
			{
				Status = NDIS_STATUS_SUCCESS;
			}
			break;
			
		default:
			DBGPRINT_RAW(RT_DEBUG_ERROR, ("--->RTUSBFreeDescriptorRequest() -----!! \n"));
			
			break;
	}
	
	return (Status);
}

#else

NDIS_STATUS	RTUSBFreeDescriptorRequest(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			BulkOutPipeId,
	IN	UINT32			NumberRequired)
{
//	UCHAR			FreeNumber = 0;
//	UINT			Index;
	NDIS_STATUS		Status = NDIS_STATUS_FAILURE;
	unsigned long   IrqFlags;
	HT_TX_CONTEXT	*pHTTXContext;


	pHTTXContext = &pAd->TxContext[BulkOutPipeId];
	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);
	if ((pHTTXContext->CurWritePosition < pHTTXContext->NextBulkOutPosition) && ((pHTTXContext->CurWritePosition + NumberRequired + LOCAL_TXBUF_SIZE) > pHTTXContext->NextBulkOutPosition))
	{
		
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
	}
	else if ((pHTTXContext->CurWritePosition == 8) && (pHTTXContext->NextBulkOutPosition < (NumberRequired + LOCAL_TXBUF_SIZE)))
	{
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
	}
	else if (pHTTXContext->bCurWriting == TRUE)
	{
		DBGPRINT(RT_DEBUG_TRACE,("RTUSBFreeD c3 --> QueIdx=%d, CWPos=%ld, NBOutPos=%ld!\n", BulkOutPipeId, pHTTXContext->CurWritePosition, pHTTXContext->NextBulkOutPosition));
		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << BulkOutPipeId));
	}
	else
	{
		Status = NDIS_STATUS_SUCCESS;
	}
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);

	
	return (Status);
}

#endif


NDIS_STATUS RTUSBFreeDescriptorRelease(
	IN RTMP_ADAPTER *pAd, 
	IN UCHAR		BulkOutPipeId)
{
	unsigned long   IrqFlags;
	HT_TX_CONTEXT	*pHTTXContext;
	
	pHTTXContext = &pAd->TxContext[BulkOutPipeId];
	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);
	pHTTXContext->bCurWriting = FALSE;
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);

	return (NDIS_STATUS_SUCCESS);
}


BOOLEAN	RTUSBNeedQueueBackForAgg(
	IN RTMP_ADAPTER *pAd, 
	IN UCHAR		BulkOutPipeId)
{
	unsigned long   IrqFlags;
	HT_TX_CONTEXT	*pHTTXContext;
	BOOLEAN			needQueBack = FALSE;
	
	pHTTXContext = &pAd->TxContext[BulkOutPipeId];
	
	RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);
	if ((pHTTXContext->IRPPending == TRUE)  /*&& (pAd->TxSwQueue[BulkOutPipeId].Number == 0) */)
	{
#if 0
		if ((pHTTXContext->CurWritePosition <= 8) && 
			(pHTTXContext->NextBulkOutPosition > 8 && (pHTTXContext->NextBulkOutPosition+MAX_AGGREGATION_SIZE) < MAX_TXBULK_LIMIT))
		{
			needQueBack = TRUE;
		}
		else if ((pHTTXContext->CurWritePosition < pHTTXContext->NextBulkOutPosition) &&
				 ((pHTTXContext->NextBulkOutPosition + MAX_AGGREGATION_SIZE) < MAX_TXBULK_LIMIT))
		{
			needQueBack = TRUE;
		}
#else
		if ((pHTTXContext->CurWritePosition < pHTTXContext->ENextBulkOutPosition) && 
			(((pHTTXContext->ENextBulkOutPosition+MAX_AGGREGATION_SIZE) < MAX_TXBULK_LIMIT) || (pHTTXContext->CurWritePosition > MAX_AGGREGATION_SIZE)))
		{
			needQueBack = TRUE;
		}
#endif
		else if ((pHTTXContext->CurWritePosition > pHTTXContext->ENextBulkOutPosition) && 
				 ((pHTTXContext->ENextBulkOutPosition + MAX_AGGREGATION_SIZE) < pHTTXContext->CurWritePosition))
		{
			needQueBack = TRUE;
		}
	}
	RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[BulkOutPipeId], IrqFlags);

	return needQueBack;

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
VOID	RTUSBRejectPendingPackets(
	IN	PRTMP_ADAPTER	pAd)
{
	UCHAR			Index;
	PQUEUE_ENTRY	pEntry;
	PNDIS_PACKET	pPacket;
	PQUEUE_HEADER	pQueue;
	

	for (Index = 0; Index < 4; Index++)
	{
		NdisAcquireSpinLock(&pAd->TxSwQueueLock[Index]);
		while (pAd->TxSwQueue[Index].Head != NULL)
		{
			pQueue = (PQUEUE_HEADER) &(pAd->TxSwQueue[Index]);
			pEntry = RemoveHeadQueue(pQueue);
			pPacket = QUEUE_ENTRY_TO_PACKET(pEntry);
			RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
		}
		NdisReleaseSpinLock(&pAd->TxSwQueueLock[Index]);

	}
	
}

/*
	========================================================================

	Routine	Description:
		Suspend MSDU transmission
		
	Arguments:
		pAd		Pointer	to our adapter
		
	Return Value:
		None
		
	Note:
	
	========================================================================
*/
#if 0
VOID    RTUSBSuspendMsduTransmission(
	IN	PRTMP_ADAPTER	pAd)
{
	DBGPRINT(RT_DEBUG_TRACE,("SCANNING, suspend MSDU transmission ...\n"));

	//
	// Before BSS_SCAN_IN_PROGRESS, we need to keep Current R66 value and
	// use Lowbound as R66 value on ScanNextChannel(...)
	//
	RTUSBReadBBPRegister(pAd, BBP_R66, &pAd->BbpTuning.R66CurrentValue);

	if (pAd->LatchRfRegs.Channel <= 14)
		RTUSBWriteBBPRegister(pAd, BBP_R66, 0x30);
	else
		RTUSBWriteBBPRegister(pAd, BBP_R66, 0x40);
	
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS);
}


/*
	========================================================================

	Routine	Description:
		Resume MSDU transmission
		
	Arguments:
		pAd		Pointer	to our adapter
		
	Return Value:
		None
		
	Note:
	
	========================================================================
*/
VOID    RTUSBResumeMsduTransmission(
	IN	PRTMP_ADAPTER	pAd)
{
	UCHAR			Index;


	DBGPRINT(RT_DEBUG_ERROR,("SCAN done, resume MSDU transmission ...\n"));
	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS);

	//
	// After finish BSS_SCAN_IN_PROGRESS, we need to restore Current R66 value
	//
	RTUSBWriteBBPRegister(pAd, BBP_R66, pAd->BbpTuning.R66CurrentValue);
	
	if ((!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
		(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS)) &&
		(!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF)))
	{
		// Dequeue all Tx software queue, if have been queued.
		for (Index = 0; Index < 4; Index++)
		{
			if(pAd->TxSwQueue[Index].Number > 0)
			{
				RTMPUSBDeQueuePacket(pAd, Index);
			}
		}
	}

	// Kick bulk out
	RTUSBKickBulkOut(pAd);
}
#endif

/*
	========================================================================
	
	Routine	Description:
		Calculates the duration which is required to transmit out frames 
	with given size and specified rate.
		
	Arguments:
		pTxD		Pointer to transmit descriptor
		Ack			Setting for Ack requirement bit
		Fragment	Setting for Fragment bit
		RetryMode	Setting for retry mode
		Ifs			Setting for IFS gap
		Rate		Setting for transmit rate
		Service		Setting for service
		Length		Frame length
		TxPreamble  Short or Long preamble when using CCK rates
		QueIdx - 0-3, according to 802.11e/d4.4 June/2003
		
	Return Value:
		None
		
	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
#if 0 // sample
VOID RTMPWriteTxWI(
	IN	PRTMP_ADAPTER	pAd,
	IN	PTXWI_STRUC 	pTxWI,
	IN	BOOLEAN			FRAG,	
	IN	BOOLEAN			InsTimestamp,
	IN	BOOLEAN 		AMPDU,
	IN	BOOLEAN 		Ack,
	IN	BOOLEAN 		NSeq,		// HW new a sequence.
	IN	UCHAR			BASize,
	IN	UCHAR			WCID,
	IN	ULONG			Length,
	IN	UCHAR 			PID,
	IN	UCHAR			MIMOps,
	IN	UCHAR			Txopmode,	
	IN	BOOLEAN			CfAck,	
	IN	HTTRANSMIT_SETTING	Transmit)
{

	PMAC_TABLE_ENTRY	pMac = NULL;
	
	if (WCID < MAX_LEN_OF_MAC_TABLE)
		pMac = &pAd->MacTab.Content[WCID];

	//
	// Always use Long preamble before verifiation short preamble functionality works well.
	// Todo: remove the following line if short preamble functionality works
	//
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);
	pTxWI->FRAG= FRAG;
	pTxWI->TS= InsTimestamp;
	pTxWI->AMPDU = AMPDU;
	if (pMac)
	{
		pTxWI->MIMOps = (pMac->PsMode == PWR_MMPS)? 1:0;
		pTxWI->MpduDensity = pMac->MpduDensity;
	}
	pTxWI->ACK = Ack;
	pTxWI->txop = Txopmode;
	pTxWI->NSEQ = NSeq;
	pTxWI->BAWinSize = BASize;	

	pTxWI->WirelessCliID = WCID;
	pTxWI->MPDUtotalByteCount = Length; 
	pTxWI->PacketId = PID; 
	
	pTxWI->BW = Transmit.field.BW;
	pTxWI->ShortGI = Transmit.field.ShortGI;
	pTxWI->STBC= Transmit.field.STBC;
	
	pTxWI->MCS = Transmit.field.MCS;
	pTxWI->PHYMODE= Transmit.field.MODE;
	// MMPS is 802.11n features. Because TxWI->MCS > 7 must be HT mode, so need not check if it's HT rate.
	if ((MIMOps == MMPS_STATIC) && (pTxWI->MCS > 7))	  
		pTxWI->MCS = 7;
	
	if ((MIMOps == MMPS_DYNAMIC) && (pTxWI->MCS > 7)) // SMPS protect 2 spatial.
		pTxWI->MIMOps = 1;
	
	pTxWI->CFACK = CfAck;
}
#endif

VOID RTMPWriteTxInfo(
	IN	PRTMP_ADAPTER	pAd,
	IN	PTXINFO_STRUC 	pTxInfo,
	IN	  USHORT		USBDMApktLen,
	IN	  BOOLEAN		bWiv,
	IN	  UCHAR			QueueSel,
	IN	  UCHAR			NextValid,
	IN	  UCHAR			TxBurst)
{
	pTxInfo->USBDMATxPktLen = USBDMApktLen;
	pTxInfo->QSEL = QueueSel;
	if (QueueSel != FIFO_EDCA)
		DBGPRINT(RT_DEBUG_TRACE, ("====> QueueSel != FIFO_EDCA<============\n"));
	pTxInfo->USBDMANextVLD = FALSE; //NextValid;  // Need to check with Jan about this.
	pTxInfo->USBDMATxburst = TxBurst;
	pTxInfo->WIV = bWiv;
	pTxInfo->SwUseLastRound = 0;
	pTxInfo->rsv = 0;
	pTxInfo->rsv2 = 0;
}
// should be called only when -
// 1. MEADIA_CONNECTED
// 2. AGGREGATION_IN_USED
// 3. Fragmentation not in used
// 4. either no previous frame (pPrevAddr1=NULL) .OR. previoud frame is aggregatible
#if 0
BOOLEAN TxFrameIsAggregatible(
	IN  PRTMP_ADAPTER   pAd,
	IN  PUCHAR          pPrevAddr1,
	IN  PUCHAR          p8023hdr)
{
	// can't aggregate EAPOL (802.1x) frame
	if ((p8023hdr[12] == 0x88) && (p8023hdr[13] == 0x8e))
		return FALSE;

	// can't aggregate multicast/broadcast frame
	if (p8023hdr[0] & 0x01)
		return FALSE;

	//
	// Adhoc mode will not support Aggregation.
	//    
	if (INFRA_ON(pAd) && (pAd->bUsbTxBulkAggre == FALSE)) // must be unicast to AP
		return TRUE;
	else
		return FALSE;
}


/*
	========================================================================

	Routine	Description:
		Copy frame from waiting queue into relative ring buffer and set 
	appropriate ASIC register to kick hardware encryption before really
	sent out to air.
		
	Arguments:
		pAd		Pointer	to our adapter
		PNDIS_PACKET	Pointer to outgoing Ndis frame
		NumberOfFrag	Number of fragment required
		
	Return Value:
		None

	Note:
	
	========================================================================
*/
NDIS_STATUS RTUSBHardTransmit(
	IN	PRTMP_ADAPTER	pAd,
	IN	PNDIS_PACKET	pPacket,
	IN	UCHAR			NumberRequired,
	IN    UCHAR			QueIdx)
{
	UINT			LengthQosPAD =0;
	UINT			This80211HdrLen = LENGTH_802_11;
	PACKET_INFO		PacketInfo;
	UINT			BytesCopied;
	UINT			TxSize;
	UINT			InitFreeMpduSize, FreeMpduSize;
	INT				SrcRemainingBytes;
	USHORT			Protocol;
	UCHAR			FrameGap;
	HEADER_802_11	Header_802_11;
	UCHAR			Header_802_3[LENGTH_802_3];
	PHEADER_802_11	pHeader80211;	
	PUCHAR			pDest;
	PHT_TX_CONTEXT		pTxContext;
	PTXWI_STRUC		pTxWI;
	PTXINFO_STRUC		pTxInfo;
	BOOLEAN			StartOfFrame;
	BOOLEAN			bEAPOLFrame=FALSE;
	BOOLEAN			MICFrag;
//	PCIPHER_KEY		pWpaKey = NULL;
//	UCHAR			RetryLimit = 0;
	BOOLEAN			Cipher;
	ULONG			TransferBufferLength;
	USHORT			AckDuration = 0;
	USHORT			EncryptionOverhead = 0;
	UCHAR			CipherAlg;
	BOOLEAN			bAckRequired;
//	UCHAR			RetryMode = SHORT_RETRY;
	UCHAR			UserPriority;
	UCHAR			MpduRequired, RtsRequired, MimoPs = MMPS_ENABLE;
	UCHAR			TxRate;		
	PCIPHER_KEY		pKey = NULL ;
	PUCHAR			pSrcBufVA = NULL;
	UINT			SrcBufLen;
	PUCHAR			pExtraLlcSnapEncap = NULL; // NULL: no extra LLC/SNAP is required
	UCHAR			KeyIdx;
	PUCHAR			pWirelessPacket;
	BOOLEAN			bAggregatible = FALSE;
	ULONG			DataOffSet = 0;
	ULONG 			NextMpduSize;
	ULONG 			FillOffset;
	BOOLEAN			bMcast;		
	BOOLEAN 		bTXBA = FALSE; // TRUE if use block ack.
	UCHAR			PID;
	BOOLEAN 		TXWIAMPDU = FALSE;	   //AMPDU bit in TXWI
	UCHAR			TXWIBAWinSize  = 0; //BAWinSize field in TXWI
	BOOLEAN 		bHTC = FALSE; // TRUE if HT AMSDU used for this MPDU.
	UCHAR			RAWcid = BSSID_WCID;	// default in infra mode or adhoc mode using legacy rate, RA is BSSID. 
	UCHAR			RABAOriIdx = 0;	//The RA's BA Originator table index. 
	BOOLEAN			bPiggyBack = FALSE;
	BOOLEAN			letusprint = FALSE;
	PULONG			ptemp;
//	BOOLEAN			bAmsdu = FALSE;	// use HT AMSDU
	BOOLEAN			bHTRate = FALSE, bDHCPFrame = FALSE;
	BOOLEAN			bIncreaseCurWriPos = FALSE;
	UCHAR			NextValid=0;
	
	if ((pAd->CommonCfg.bIEEE80211H == 1) && (pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE))
	{
		DBGPRINT(RT_DEBUG_INFO,("RTUSBHardTransmit --> radar detect not in normal mode !!!\n"));
		return (NDIS_STATUS_FAILURE);
	}

	if (pAd->bForcePrintTX == TRUE)
		letusprint = TRUE;
	
	TxRate = RTMP_GET_PACKET_TXRATE(pPacket);
	MpduRequired = RTMP_GET_PACKET_FRAGMENTS(pPacket);
	RtsRequired = RTMP_GET_PACKET_RTS(pPacket);
	UserPriority = RTMP_GET_PACKET_UP(pPacket);
#ifdef WIN_NDIS
	//
	// Prepare packet information structure which will be query for buffer descriptor
	//
	NdisQueryPacket(
		pPacket,							// Ndis packet
		&PacketInfo.PhysicalBufferCount,	// Physical buffer count
		&PacketInfo.BufferCount,			// Number of buffer descriptor
		&PacketInfo.pFirstBuffer,			// Pointer to first buffer descripotr
		&PacketInfo.TotalPacketLength);		// Ndis packet length		

	NDIS_QUERY_BUFFER(PacketInfo.pFirstBuffer, &pSrcBufVA, &SrcBufLen);
#else
	RTMP_QueryPacketInfo(pPacket, &PacketInfo, &pSrcBufVA, &SrcBufLen);
#endif

	DBGPRINT_RAW(RT_DEBUG_INFO, ("packetsize:%d\n", PacketInfo.TotalPacketLength));

 	// Check for virtual address allocation, it might fail !!!
	if (pSrcBufVA == NULL)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("pSrcBufVA == NULL\n"));
		return(NDIS_STATUS_RESOURCES);
	}
	if (SrcBufLen < 14)
	{
		DBGPRINT_RAW(RT_DEBUG_ERROR, ("RTUSBHardTransmit --> Ndis Packet buffer error !!!\n"));
		return (NDIS_STATUS_FAILURE);
	}
	else
	{
		//
		// Backup Header_802_3
		//
		NdisMoveMemory(Header_802_3, pSrcBufVA, LENGTH_802_3);
	}

	// In in HT rate adhoc mode, A-MPDU is often used. So need to lookup BA Table and MAC Entry. 
	// Note multicast packets in adhoc also use BSSID_WCID index. 
	if (ADHOC_ON(pAd))		// only unicast data use Block Ack
	{
		MAC_TABLE_ENTRY *pEntry;
		pEntry = MacTableLookup(pAd, pSrcBufVA);
		if (pEntry)
		{
			RAWcid = (UCHAR)pEntry->Aid;
			// update Originator index in adhoc mode. 
			// Becasue adhoc mode set many BA seesion with many stations.  
			//The RA's BA Originator index.
			RABAOriIdx = pEntry->BAOriWcidArray[UserPriority];
		}
	}

	bHTRate = FALSE;
	if (pAd->MacTab.Content[RAWcid].HTPhyMode.field.MODE & 0x2)
	{
		bHTRate = TRUE;
		MimoPs = pAd->MacTab.Content[RAWcid].MmpsMode;
	}
	//
	// If DHCP datagram or ARP datagram , we need to send it as Low rates.
	//
	if (RTMPCheckDHCPFrame(pAd, pPacket))
	{
		bDHCPFrame = TRUE;
	}
	if (pAd->CommonCfg.Channel <= 14)
	{
		//
		// Case 802.11 b/g
		// basic channel means that we can use CCKM's low rate as RATE_1.
		//		
		if ((TxRate != RATE_1) && RTMPCheckDHCPFrame(pAd, pPacket))
			TxRate = RATE_1;
	}
	else
	{
		//
		// Case 802.11a
		// We must used OFDM's low rate as RATE_6, note RATE_1 is not allow
		// Only OFDM support on Channel > 14
		//
		if ((TxRate != RATE_6) && RTMPCheckDHCPFrame(pAd, pPacket))
			TxRate = RATE_6;
	}

	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED) 	&&
		(bHTRate == FALSE)										&&
		(pAd->MacTab.Content[RAWcid].HTPhyMode.field.MODE == MODE_OFDM)	&&
		(bDHCPFrame == FALSE)							&&
		TxFrameIsAggregatible(pAd, NULL, pSrcBufVA))
	{
		bAggregatible = TRUE;
	}
	

	if (KeyIdx == 0xff)
		CipherAlg = CIPHER_NONE;
	else if ((Cipher == Ndis802_11EncryptionDisabled) || (pAd->SharedKey[BSS0][KeyIdx].KeyLen == 0))
		CipherAlg = CIPHER_NONE;
	else
	{
		Header_802_11.FC.Wep = 1;
		CipherAlg = pAd->SharedKey[BSS0][KeyIdx].CipherAlg;
		pKey = &pAd->SharedKey[BSS0][KeyIdx];
		DBGPRINT(RT_DEBUG_INFO,("RTMPHardTransmit(bEAP=%d) - %s key#%d, KeyLen=%d\n", 
			bEAPOLFrame, CipherName[CipherAlg], KeyIdx, pAd->SharedKey[BSS0][KeyIdx].KeyLen));
	}

	// STEP 3.1 if TKIP is used and fragmentation is required. Driver has to
	//          append TKIP MIC at tail of the scatter buffer (This must be the
	//          ONLY scatter buffer in the NDIS PACKET). 
	//          MAC ASIC will only perform IV/EIV/ICV insertion but no TKIP MIC
	if ((MpduRequired > 1) && (CipherAlg == CIPHER_TKIP))
	{
		//SrcBufLen += 8;
		PacketInfo.TotalPacketLength += 8;
		CipherAlg = CIPHER_TKIP_NO_MIC;	
	}

	if (CipherAlg == CIPHER_TKIP_NO_MIC)
	{
		//
		// On this case, we don't support Aggregation, it's diffcult to implement.
		//
		bAggregatible = FALSE;
	}

	// ----------------------------------------------------------------
	// STEP 4. Make RTS frame or CTS-to-self frame if required
	// ----------------------------------------------------------------

	//
	// calcuate the overhead bytes that encryption algorithm may add. This
	// affects the calculate of "duration" field
	//
	if ((CipherAlg == CIPHER_WEP64) || (CipherAlg == CIPHER_WEP128)) 
		EncryptionOverhead = 8; //WEP: IV[4] + ICV[4];
	else if (CipherAlg == CIPHER_TKIP_NO_MIC)
		EncryptionOverhead = 12;//TKIP: IV[4] + EIV[4] + ICV[4], MIC will be added to TotalPacketLength
	else if (CipherAlg == CIPHER_TKIP)
		EncryptionOverhead = 20;//TKIP: IV[4] + EIV[4] + ICV[4] + MIC[8]
	else if (CipherAlg == CIPHER_AES)
		EncryptionOverhead = 16;    // AES: IV[4] + EIV[4] + MIC[8]
	else
		EncryptionOverhead = 0;
	// decide how much time an ACK/CTS frame will consume in the air
	AckDuration = RTMPCalcDuration(pAd, pAd->CommonCfg.ExpectedACKRate[TxRate], 14);

	// If fragment required, MPDU size is maximum fragment size
	// Else, MPDU size should be frame with 802.11 header & CRC
	if (MpduRequired > 1)
		NextMpduSize = pAd->CommonCfg.FragmentThreshold;
	else
	{
		NextMpduSize = PacketInfo.TotalPacketLength + LENGTH_802_11 + LENGTH_CRC - LENGTH_802_3;
		if (pExtraLlcSnapEncap)
			NextMpduSize += LENGTH_802_1_H;
	}

/*	if (RtsRequired || OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_RTS_PROTECTION_ENABLE))
	{
		RTMPSendRTSCTSFrame(pAd, 
						 	Header_802_11.Addr1, 
						 	NextMpduSize + EncryptionOverhead, 
						 	TxRate,
						 	pAd->CommonCfg.RtsRate, 
						 	AckDuration,
						 	QueIdx,
						 	FrameGap,
						 	SUBTYPE_RTS);
		
		// RTS/CTS-protected frame should use LONG_RETRY (=4) and SIFS
		RetryMode = LONG_RETRY;
		FrameGap = IFS_SIFS;
		if (RtsRequired)
			NumberRequired--;
	}
	else if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED))
	{
		RTMPSendRTSCTSFrame(pAd, 
						 	Header_802_11.Addr1, 
						 	NextMpduSize + EncryptionOverhead, 
						 	TxRate,
						 	pAd->CommonCfg.RtsRate, 
						 	AckDuration,
						 	QueIdx,
						 	FrameGap,
						 	SUBTYPE_CTS);
		
		// RTS/CTS-protected frame should use LONG_RETRY (=4) and SIFS
		RetryMode = LONG_RETRY;
		FrameGap = IFS_SIFS;
	}
*/
	// --------------------------------------------------------
	// STEP 5. START MAKING MPDU(s)
	//      Start Copy Ndis Packet into Ring buffer.
	//      For frame required more than one ring buffer (fragment), all ring buffers
	//      have to be filled before kicking start tx bit.
	//      Make sure TX ring resource won't be used by other threads
	// --------------------------------------------------------
	if (bTXBA)
	{
		TXWIAMPDU = TRUE;
		TXWIBAWinSize = pAd->BATable.BAOriEntry[RABAOriIdx].BAWinSize - 1;
		Header_802_11.Frag = 0;
	}
	Header_802_11.Sequence = (pAd->MacTab.Content[RAWcid].BAOriSequence[UserPriority]++);
	pAd->MacTab.Content[RAWcid].BAOriSequence[UserPriority] %= 4096;
	SrcRemainingBytes = PacketInfo.TotalPacketLength - LENGTH_802_3;
	SrcBufLen        -= LENGTH_802_3;  // skip 802.3 header

	//
	// Fragmentation is not allowed on multicast & broadcast
	// So, we need to used the MAX_FRAG_THRESHOLD instead of pAd->CommonCfg.FragmentThreshold
	// otherwise if PacketInfo.TotalPacketLength > pAd->CommonCfg.FragmentThreshold then
	// packet will be fragment on multicast & broadcast.
	//
	// MpduRequired equals to 1 means this could be Aggretaion case.
	//
	if ((*pSrcBufVA & 0x01) || (pAd->CommonCfg.HTPhyMode.field.MODE >= MODE_HTMIX)|| MpduRequired == 1)
	{
		InitFreeMpduSize = MAX_FRAG_THRESHOLD - sizeof(Header_802_11) - LENGTH_CRC;
	}
	else
	{
		InitFreeMpduSize = pAd->CommonCfg.FragmentThreshold - sizeof(Header_802_11) - LENGTH_CRC;
	}

	StartOfFrame = TRUE;
	MICFrag = FALSE;	// Flag to indicate MIC shall spread into two MPDUs
	// Start Copy Ndis Packet into Ring buffer.
	// For frame required more than one ring buffer (fragment), all ring buffers
	// have to be filled before kicking start tx bit.
	do
	{
		unsigned long	IrqFlags;
		//
		// STEP 5.1 Get the Tx Ring descriptor & Dma Buffer address
		//
		RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
		pTxContext  = &pAd->TxContext[QueIdx];
		FillOffset = pTxContext->CurWritePosition;
		// Check ring full.
		if (((pTxContext->CurWritePosition) < pTxContext->NextBulkOutPosition) && (pTxContext->CurWritePosition + LOCAL_TXBUF_SIZE) > pTxContext->NextBulkOutPosition)
		{
			DBGPRINT(RT_DEBUG_INFO,("RTUSBHardTransmit c1 --> CurWritePosition = %ld, NextBulkOutPosition = %ld. \n", pTxContext->CurWritePosition, pTxContext->NextBulkOutPosition));
			RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx));
			pTxContext->bCurWriting = FALSE;
			RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
			return (NDIS_STATUS_RESOURCES);
		}
		else if ((pTxContext->CurWritePosition == 8) && (pTxContext->NextBulkOutPosition < LOCAL_TXBUF_SIZE))
		{
			DBGPRINT(RT_DEBUG_TRACE,("RTUSBHardTransmit c2 --> CurWritePosition = %ld, NextBulkOutPosition = %ld. \n", pTxContext->CurWritePosition, pTxContext->NextBulkOutPosition));
			RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx));
			pTxContext->bCurWriting = FALSE;
			RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
			return (NDIS_STATUS_RESOURCES);
		}
		else if (pTxContext->bCurWriting == TRUE)
		{
			DBGPRINT(RT_DEBUG_TRACE,("RTUSBHardTransmit c3 --> CurWritePosition = %ld, NextBulkOutPosition = %ld. \n", pTxContext->CurWritePosition, pTxContext->NextBulkOutPosition));
			pTxContext->bCurWriting = FALSE;
			RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
			return (NDIS_STATUS_RESOURCES);
		}
		
		if ((pTxContext->ENextBulkOutPosition == pTxContext->CurWritePosition))
		{
			bIncreaseCurWriPos = TRUE;
			pTxContext->ENextBulkOutPosition += 8;
			FillOffset += 8;
		}
		
		pTxContext->bCurWriting = TRUE;
		RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
		pTxInfo= (PTXINFO_STRUC)&pTxContext->TransferBuffer->WirelessPacket[FillOffset];
		NdisZeroMemory(pTxInfo, TXINFO_SIZE);
		pTxWI= (PTXWI_STRUC)&pTxContext->TransferBuffer->WirelessPacket[FillOffset+TXINFO_SIZE];
		NdisZeroMemory(pTxWI, TXWI_SIZE);
		pWirelessPacket = &pTxContext->TransferBuffer->WirelessPacket[FillOffset+TXINFO_SIZE+TXWI_SIZE];
		pAd->LastTxRate = (USHORT)(pAd->MacTab.Content[RAWcid].HTPhyMode.word) ;

		//
		// STEP 5.2 COPY 802.11 HEADER INTO 1ST DMA BUFFER
		//
		pDest = pWirelessPacket;		
		NdisMoveMemory(pDest, &Header_802_11, sizeof(Header_802_11));
		pDest       += sizeof(Header_802_11);
		DataOffSet  += sizeof(Header_802_11);

		// Init FreeMpduSize
		FreeMpduSize = InitFreeMpduSize;

		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED))
		{
			// copy QOS CONTROL bytes
			// HT rate must has QOS CONTROL bytes
			if (bTXBA)
				*pDest		  =  (UserPriority & 0x0f) ;
			else
				*pDest		  =  (UserPriority & 0x0f) | pAd->CommonCfg.AckPolicy[QueIdx];
			*(pDest+1)    =  0;
			pDest         += 2;
			This80211HdrLen =  LENGTH_802_11 + 2;
			FreeMpduSize  -= 2;
			LengthQosPAD = 2;
			// Pad 2 bytes to make 802.11 header 4-byte alignment
			if (bHTC == TRUE)
			{
				// HTC Control field is following QOS field.
				NdisZeroMemory(pDest, 6);
				This80211HdrLen        += 4;
				if (CLIENT_STATUS_TEST_FLAG(&pAd->MacTab.Content[RAWcid], fCLIENT_STATUS_RDG_CAPABLE) )
					*(pDest+3)|=0x80;
				if (pAd->bLinkAdapt == TRUE)
				{	//bit2MRQ=1
					*(pDest)|=0x4;
					// Set identifier as 2
					*(pDest)|=0x10;
					//Set MFB field = MCS. MFB is at bit9-15
					*(pDest+1)|=(pAd->CommonCfg.HTPhyMode.field.MCS<<1);
				}
				pDest		  += 6;
			}
			else
			{
				NdisZeroMemory(pDest, 2);
				pDest		  += 2;
			}
		}

		//
		// STEP 5.4 COPY LLC/SNAP, CKIP MIC INTO 1ST DMA BUFFER ONLY WHEN THIS 
		//          MPDU IS THE 1ST OR ONLY FRAGMENT 
		//
		if (Header_802_11.Frag == 0)
		{
			if (pExtraLlcSnapEncap)
			{
				if ((CipherAlg == CIPHER_TKIP_NO_MIC) && (pKey != NULL))
				{
					// Calculate MSDU MIC Value
					RTMPCalculateMICValue(pAd, pPacket, pExtraLlcSnapEncap, pKey, BSS0); // sample
					DBGPRINT(RT_DEBUG_TRACE, ("1 RTMPCalculateMICValue \n"));
				}
					// Insert LLC-SNAP encapsulation
					NdisMoveMemory(pDest, pExtraLlcSnapEncap, 6);
					pDest += 6;
					DataOffSet += 6;
					NdisMoveMemory(pDest, pSrcBufVA + 12, 2);
					pDest += 2;
					DataOffSet += 2;
				pSrcBufVA += LENGTH_802_3;
				FreeMpduSize -= LENGTH_802_1_H;
			}
			else
			{
				if ((CipherAlg == CIPHER_TKIP_NO_MIC) && (pKey != NULL))
				{
					// Calculate MSDU MIC Value
					RTMPCalculateMICValue(pAd, pPacket, pExtraLlcSnapEncap, pKey, BSS0); //sample
					DBGPRINT(RT_DEBUG_TRACE, (" 2RTMPCalculateMICValue \n"));
				}
				pSrcBufVA += LENGTH_802_3;
			}
		}

		// Start copying payload
		BytesCopied = 0;
		do
		{
			if (SrcBufLen >= FreeMpduSize)
			{
				// Copy only the free fragment size, and save the pointer
				// of current buffer descriptor for next fragment buffer.
				NdisMoveMemory(pDest, pSrcBufVA, FreeMpduSize);
				BytesCopied += FreeMpduSize;
				pSrcBufVA   += FreeMpduSize;
				pDest       += FreeMpduSize;
				SrcBufLen   -= FreeMpduSize;
				break;
			}
			else if (SrcBufLen > 0)
			{
				// Copy the rest of this buffer descriptor pointed data
				// into ring buffer.
				if (pSrcBufVA == NULL)
				{
					ASSERT(0);
				}
				NdisMoveMemory(pDest, pSrcBufVA, SrcBufLen);
				BytesCopied  += SrcBufLen;
				pDest        += SrcBufLen;
				FreeMpduSize -= SrcBufLen;
			}
			else 
			{
				if (pSrcBufVA == NULL)
					DBGPRINT(RT_DEBUG_TRACE, (" 2pSrcBufVA IS NULL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n"));
			}
		
			// No more buffer descriptor
			// Add MIC value if needed

			//if ((pAd->CommonCfg.WepStatus == Ndis802_11Encryption2Enabled) && 
			//	(MICFrag == FALSE) &&
			//	(pKey != NULL))

			if((CipherAlg == CIPHER_TKIP_NO_MIC) &&
				(MICFrag == FALSE) &&
				(pKey != NULL))
			{
				// Fregment and TKIP//
				INT i;

				SrcBufLen = 8;		// Set length to MIC length
				DBGPRINT_RAW(RT_DEBUG_TRACE, ("Calculated TX MIC value =  BytesCopied = %d", BytesCopied));  
				for (i = 0; i < 8; i++)
				{
					DBGPRINT_RAW(RT_DEBUG_INFO, ("%02x:", pAd->PrivateInfo.Tx.MIC[i]));  
				}
				DBGPRINT_RAW(RT_DEBUG_TRACE, ("\n")); 
								
				if (FreeMpduSize >= SrcBufLen)
				{
					NdisMoveMemory(pDest, pAd->PrivateInfo.Tx.MIC, SrcBufLen);
					BytesCopied  += SrcBufLen;
					pDest		 += SrcBufLen;
					FreeMpduSize -= SrcBufLen;
					SrcBufLen = 0;
				}
				else
				{
					NdisMoveMemory(pDest, pAd->PrivateInfo.Tx.MIC, FreeMpduSize);
					BytesCopied  += FreeMpduSize;
					pSrcBufVA     = pAd->PrivateInfo.Tx.MIC + FreeMpduSize;
					pDest		 += FreeMpduSize;
					SrcBufLen		 -= FreeMpduSize;
					MICFrag 	  = TRUE;
				}						
			}
			break;
				
		}	while (TRUE);		// End of copying payload
				
		// Real packet size, No 802.1H header for fragments except the first one.
		if ((StartOfFrame == TRUE) && (pExtraLlcSnapEncap != NULL))
		{
			TxSize = BytesCopied + This80211HdrLen + LENGTH_802_1_H;
		}
		else
		{
			TxSize = BytesCopied + This80211HdrLen;
		}

		SrcRemainingBytes -=  BytesCopied;
	
		//
		// STEP 5.6 MODIFY MORE_FRAGMENT BIT & DURATION FIELD. WRITE TXD
		//
		RTMP_IRQ_LOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);
		pHeader80211 = (PHEADER_802_11)pWirelessPacket;
		if (SrcRemainingBytes > 0) // more fragment is required
		{
			 ULONG	ThisNextMpduSize;

			 pHeader80211->FC.MoreFrag = 1;
			 ThisNextMpduSize = min((ULONG) SrcRemainingBytes, (ULONG) pAd->CommonCfg.FragmentThreshold);

			 if (ThisNextMpduSize < pAd->CommonCfg.FragmentThreshold)
			 {
				// In this case, we need to include LENGTH_802_11 and LENGTH_CRC for calculating Duration.
				pHeader80211->Duration = (3 * pAd->CommonCfg.Dsifs) + 
									(2 * AckDuration) + 
									RTMPCalcDuration(pAd, TxRate, ThisNextMpduSize + EncryptionOverhead + LENGTH_802_11 + LENGTH_CRC);
			 }
			 else
			 {
				pHeader80211->Duration = (3 * pAd->CommonCfg.Dsifs) + 
								(2 * AckDuration) + 
								RTMPCalcDuration(pAd, TxRate, ThisNextMpduSize + EncryptionOverhead);
			 }

			//as Sta, the RA always is BSSID. So always use BSSID_WCID.
			// sample
			RTMPWriteTxWI(pAd, pTxWI,  FALSE, FALSE, FALSE, TXWIAMPDU, bAckRequired, FALSE, 
				TXWIBAWinSize, RAWcid, TxSize,
				PID, UserPriority, (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, IFS_SIFS, bPiggyBack,  &pAd->MacTab.Content[RAWcid].HTPhyMode);
			RTMPWriteTxInfo(pAd, pTxInfo, (USHORT)(ThisNextMpduSize+TXWI_SIZE + LengthQosPAD), FALSE, FIFO_EDCA, 0,  0);
			
			if (letusprint == TRUE)
			{
				ptemp = (PULONG)pTxWI;
				DBGPRINT(RT_DEBUG_TRACE,("More Frag: HardTx TxWI= %08lx: %08lx: %08lx: %08lx. CurWritePos = %ld ", *ptemp, *(ptemp+1), *(ptemp+2), *(ptemp+3), pTxContext->CurWritePosition));
			}
			FrameGap = IFS_SIFS;     // use SIFS for all subsequent fragments
			Header_802_11.Frag ++;   // increase Frag #
		}
		else
		{
			pHeader80211->FC.MoreFrag = 0;
			if (pHeader80211->Addr1[0] & 0x01) // multicast/broadcast
				pHeader80211->Duration = 0;
			else
				pHeader80211->Duration = pAd->CommonCfg.Dsifs + AckDuration;

			if ((bEAPOLFrame) && (TxRate > RATE_6))
			{
				TxRate = RATE_6;
			}

			if ((bDHCPFrame == TRUE) || (bEAPOLFrame ==TRUE))
			{
				RTMPWriteTxWI(pAd, pTxWI,  FALSE, FALSE, FALSE, FALSE, bAckRequired, FALSE, 
					TXWIBAWinSize, RAWcid, TxSize, 
					PID, UserPriority, (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, IFS_BACKOFF, bPiggyBack, &pAd->MacTab.Content[MCAST_WCID].HTPhyMode);
				pAd->LastTxRate = (USHORT)(pAd->MacTab.Content[MCAST_WCID].HTPhyMode.word);
				RTMPWriteTxInfo(pAd, pTxInfo, (TxSize+TXWI_SIZE + LengthQosPAD), FALSE, FIFO_EDCA, FALSE,  FALSE);
			}
			else
			{
				RTMPWriteTxWI(pAd, pTxWI,  FALSE, FALSE, FALSE, TXWIAMPDU, bAckRequired, FALSE, 
					TXWIBAWinSize, RAWcid, TxSize, 
					PID, UserPriority, (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, FrameGap, bPiggyBack, &pAd->MacTab.Content[RAWcid].HTPhyMode);
				pAd->LastTxRate = (USHORT)(pAd->MacTab.Content[RAWcid].HTPhyMode.word);
				if ( pAd->TxSwQueue[QueIdx].Number > 1)
					NextValid= 1;
				else
					NextValid = 0;
				RTMPWriteTxInfo(pAd, pTxInfo, (TxSize+TXWI_SIZE + LengthQosPAD), FALSE, FIFO_EDCA, NextValid,  FALSE);
			}
			if (letusprint == TRUE)
			{
				ptemp = (PULONG)pTxWI;
				DBGPRINT(RT_DEBUG_TRACE,("Last: HardTx TxWI= %08lx: %08lx: %08lx: %08lx. CurWritePos = %ld ", *ptemp, *(ptemp+1), *(ptemp+2), *(ptemp+3), pTxContext->CurWritePosition));
			}

			//if ( pAd->TxSwQueue[QueIdx].Number > 1)
			//	pTxD->Burst = 1;

		}

		TransferBufferLength = TxSize + TXWI_SIZE + TXINFO_SIZE+ LengthQosPAD;
		if ((TransferBufferLength % 4) == 1)	
		{
			NdisZeroMemory(pDest, 11);
			TransferBufferLength  += 3;
			pDest  += 3;
		}
		else if ((TransferBufferLength % 4) == 2)	
		{
			NdisZeroMemory(pDest, 10);
			TransferBufferLength  += 2;
			pDest  += 2;
		}
		else if ((TransferBufferLength % 4) == 3)	
		{
			NdisZeroMemory(pDest, 9);
			TransferBufferLength  += 1;
			pDest  += 1;
		}

		// USBDMATxPktLen should be multiple of 4-bytes. And it doesn't include sizeof (TXINFO_STRUC).
		pTxInfo->USBDMATxPktLen = TransferBufferLength - TXINFO_SIZE;
		
		NdisZeroMemory(pTxContext->TransferBuffer->Aggregation, 4);
		NdisMoveMemory(pTxContext->Header_802_3, Header_802_3, LENGTH_802_3);
		pTxContext->bCurWriting = FALSE;

		pTxContext->TxRate = TxRate;
		// Set frame gap for the rest of fragment burst.
		// It won't matter if there is only one fragment (single fragment frame).
		StartOfFrame = FALSE;
		NumberRequired--;
		if (NumberRequired == 0)
		{
			pTxContext->LastOne = TRUE;
		}
		else
		{
			pTxContext->LastOne = FALSE;
			pTxInfo->USBDMATxburst = 1;
		}

		RTUSB_SET_BULK_FLAG(pAd, (fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx));
		
		if (bIncreaseCurWriPos == TRUE)
		{
			pTxContext->CurWritePosition += 8;
			bIncreaseCurWriPos = FALSE;
		}
		pTxContext->CurWritePosition += TransferBufferLength;
		if (pTxContext->CurWritePosition > MAX_TXBULK_LIMIT)
		{
			pTxContext->CurWritePosition = 8;
			pTxInfo->SwUseLastRound = 1;
		}
		RTMP_IRQ_UNLOCK(&pAd->TxContextQueueLock[QueIdx], IrqFlags);

		if (letusprint == TRUE)
		{
			ptemp = (PULONG)pHeader80211;
			DBGPRINT(RT_DEBUG_TRACE,("HardTx Header= %08lx: %08lx: %08lx: %08lx: %08lx: %08lx: %08lx: %08lx: %08lx [ CurWritePosition = %ld] \n", *ptemp, *(ptemp+1), *(ptemp+2), *(ptemp+3), *(ptemp+4), *(ptemp+5), *(ptemp+6), *(ptemp+7), *(ptemp+8), pTxContext->CurWritePosition));
		}

		InterlockedIncrement(&pAd->TxCount);
		
	}	while (NumberRequired > 0); //while (SrcRemainingBytes > 0);
	
	// Acknowledge protocol send complete of pending packet.
	NDIS_SET_PACKET_STATUS(OSPKT_TO_RTPKT(pPacket), NDIS_STATUS_SUCCESS);

	//	
	// Check if MIC error twice within 60 seconds and send EAPOL MIC error to TX queue
	// then we enqueue a message for disasociating with the current AP
	//
 

	// succeed and release the skb buffer
	//
	RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_SUCCESS);
	
	return (NDIS_STATUS_SUCCESS);
}


/*
	========================================================================

	Routine	Description:
		This is the completion routine for the USB_RxPacket which submits
		a URB to USBD for a transmission. 

		If the status is SUCCESS and the length indicated by the URB is
		correct according to the length of the header in the data, and
		packetCRC is correct,and if it is DATA packer origin form the 
		BSS where currently is joinned, then if it is a fraggmented packet
		collects it first, and then performs the address translation. 
		Finaly stores this packet in the RxQ and triggers the RXThread to 
		indicate this packet within the NDIS context.
		If the packet is Management packet, calls the MgmtFrameRxProcessing
		function to process it.

		If the length mismatch, the reception is not correct and a reset to
		the pipe must be scheduled.      
		
	Arguments:
		DeviceObject        Pointer to the device object for next lower
							device. DeviceObject passed in here belongs to
							the next lower driver in the stack because we
							were invoked via IoCallDriver in USB_RxPacket
							AND it is not OUR device object
		Irp                 Ptr to completed IRP
		Context             Ptr to our Adapter object (context specified
							in IoSetCompletionRoutine
		
	Return Value:
		Always returns STATUS_MORE_PROCESSING_REQUIRED

	Note:
		Always returns STATUS_MORE_PROCESSING_REQUIRED
	========================================================================
*/
NTSTATUS	RTUSBRxPacket(
	IN	PRTMP_ADAPTER  pAd,
	IN    BOOLEAN          bBulkReceive)
{
	PRX_CONTEXT		pRxContext;
	PRXWI_STRUC		pRxWI;
	PRXINFO_STRUC	pRxInfo;
	PHEADER_802_11	pHeader;
	PUCHAR			pData,pStart, pTmpBuf;
	PUCHAR			pDA, pSA;
	ULONG			i;
	NDIS_STATUS		Status;
	USHORT			DataSize, Msdu2Size;
//	PUCHAR			pEncap;
//	UCHAR			LLC_Len[2];
	UCHAR			Header802_3[14];
	PCIPHER_KEY 	pWpaKey;
	BOOLEAN			EAPOLFrame;
	ULONG			TransferBufferLength;
	ULONG			ThisFrameLen = 0;
	ULONG			ReadPosition = 0;
	PULONG			ptemp;
	PUCHAR			ptr;
	BOOLEAN			bAMsdu;
	BOOLEAN			bHTRate = FALSE;


	/* device had been closed */
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_REMOVE_IN_PROGRESS)) 
		return 0;
	
	//
	// We have a number of cases:
	//      1) The USB read timed out and we received no data.
	//      2) The USB read timed out and we received some data.
	//      3) The USB read was successful and fully filled our irp buffer.
	//      4) The irp was cancelled.
	//      5) Some other failure from the USB device object.
	//
		
	//if Status != STATUS_SUCCESS, Readable = FALSE . So doesn't need check success here.
	do
	{
		pRxContext = &pAd->RxContext[pAd->NextRxBulkInReadIndex];
		if ((pRxContext->Readable == FALSE) || (pRxContext->InUse == TRUE))
			break;

		pAd->NextRxBulkInReadIndex = (pAd->NextRxBulkInReadIndex + 1) % (RX_RING_SIZE);

		TransferBufferLength = pRxContext->pUrb->actual_length + pRxContext->BulkInOffset;
		if (TransferBufferLength == 0)
		{
			DBGPRINT(RT_DEBUG_TRACE,("RTUSBRxPacket TransferBufferLength = %ld \n",  TransferBufferLength));
			DBGPRINT_RAW(RT_DEBUG_ERROR, ("R = 0x%x/0x%x/ 0x%lx. ::x%lx/0x%lx/ 0x%lx.n", pAd->NextRxBulkInIndex,  pAd->NextRxBulkInReadIndex, pAd->NextRxBulkInPosition, pAd->BulkInReq, pAd->BulkInComplete, pAd->BulkInCompleteFail));
				DBGPRINT_RAW(RT_DEBUG_ERROR, ("[0] IRPPending=%d:  InUse=%d: Readable=%d: [1] IRPPending=%d:  InUse=%d: Readable=%d: [2 IRPPending=%d:  InUse=%d: Readable=%d: [3] IRPPending=%d:  InUse=%d: Readable=%d: \n"
				, pAd->RxContext[0].IRPPending, pAd->RxContext[0].InUse, pAd->RxContext[0].Readable
				, pAd->RxContext[1].IRPPending, pAd->RxContext[1].InUse, pAd->RxContext[1].Readable
				, pAd->RxContext[2].IRPPending, pAd->RxContext[2].InUse, pAd->RxContext[2].Readable
				, pAd->RxContext[3].IRPPending, pAd->RxContext[3].InUse, pAd->RxContext[3].Readable));
			DBGPRINT_RAW(RT_DEBUG_ERROR, ("R = [4] IRPPending=%d:  InUse=%d: Readable=%d: [5] IRPPending=%d:  InUse=%d: Readable=%d:[6] IRPPending=%d:  InUse=%d: Readable=%d: [7] IRPPending=%d:  InUse=%d: Readable=%d: \n"
				, pAd->RxContext[4].IRPPending, pAd->RxContext[4].InUse, pAd->RxContext[4].Readable
				, pAd->RxContext[5].IRPPending, pAd->RxContext[5].InUse, pAd->RxContext[5].Readable
				, pAd->RxContext[6].IRPPending, pAd->RxContext[6].InUse, pAd->RxContext[6].Readable
				, pAd->RxContext[7].IRPPending, pAd->RxContext[7].InUse, pAd->RxContext[7].Readable));
		}

		while (TransferBufferLength > 0)
		{
			// minimus size = Length + RXWI
			if (TransferBufferLength >= (sizeof(RXWI_STRUC) + 8))
			{
				pData = &pRxContext->TransferBuffer[ReadPosition]; /* 4KB */
				pStart = &pRxContext->TransferBuffer[ReadPosition];

				// after shift First Word is  RXDMA Length
				ThisFrameLen = *pData + (*(pData+1)<<8);
				if ((ThisFrameLen&0x3) != 0)
				{
					DBGPRINT(RT_DEBUG_TRACE,("RXDMALen not multiple of 4. [%ld] (Total TransferBufferLength = %d) \n", ThisFrameLen, pRxContext->pUrb->actual_length));
					ThisFrameLen = (ThisFrameLen&0x3);
				}
				if (ThisFrameLen >= TransferBufferLength)
				{
					DBGPRINT(RT_DEBUG_TRACE,("This Frame Len(0x%lx) outranges. \n", ThisFrameLen));
					DBGPRINT(RT_DEBUG_TRACE,("Total BulkIn length=%x, Now left  TransferBufferLength = %lx, ReadPosition=%lx\n", pRxContext->pUrb->actual_length, TransferBufferLength, ReadPosition));
					ThisFrameLen = (ThisFrameLen&0x3);
					//Error frame. finish this loop
					ThisFrameLen = 0;
					TransferBufferLength = 0;
					break;
				}
				pData += 4;
				// Next is RxWI
				pRxWI = (PRXWI_STRUC)pData;
				// Cast to 802.11 header for flags checking
				pHeader	= (PHEADER_802_11) (pData + RXWI_SIZE);
				// pRxInfo
				pRxInfo = (PRXINFO_STRUC)(pData + ThisFrameLen);
				DataSize = (USHORT) pRxWI->MPDUtotalByteCount;
				if (pRxWI->PHYMODE & 0x2)
					bHTRate = TRUE;
				
				if (pRxInfo == NULL )
				{
//					DBGPRINT(RT_DEBUG_EMU,("RxInfo=NULL Error!! ThisFrameLen = %ld\n", ThisFrameLen));
					// Finish 
					TransferBufferLength = 0;
					Status = NDIS_STATUS_FAILURE;
				}
				else if (pRxWI == NULL)
				{
//					DBGPRINT(RT_DEBUG_EMU,("pRxWI=NULL Error!! ThisFrameLen = %ld\n", ThisFrameLen));
					// Finish 
					TransferBufferLength = 0;
					Status = NDIS_STATUS_FAILURE;
				}
				else
				{
					// Increase Total receive byte counter after real data received no mater any error or not
					pAd->RalinkCounters.ReceivedByteCount += (pRxWI->MPDUtotalByteCount- 4);
					pAd->RalinkCounters.RxCount ++;
					
					// Check for all RxD errors
					Status = RTMPCheckRxWI(pAd, pHeader, pRxWI, pRxInfo);
				}

				if (pRxWI->MPDUtotalByteCount < 14)
					Status = NDIS_STATUS_FAILURE;

				if (Status == NDIS_STATUS_SUCCESS)
				{
					// Apply packet filtering rule based on microsoft requirements.
					Status = RTMPApplyPacketFilter(pAd, pRxInfo, pHeader);
				}	
				else if (pAd->bPromiscuous == FALSE)
				{
					//DBGPRINT(RT_DEBUG_INFO,("RX err filtered1 [MPDUtotalByteCount = 0x%x]RxInfo= 0x%08x\n", pRxWI->MPDUtotalByteCount, *pRxInfo));
				}

				// Add receive counters
				if (Status == NDIS_STATUS_SUCCESS)
				{
					// Increase 802.11 counters & general receive counters
					INC_COUNTER64(pAd->WlanCounters.ReceivedFragmentCount);
				}
				else if (pAd->bPromiscuous == FALSE)
				{
					// Increase general counters
					pAd->Counters8023.RxErrors++;
					//DBGPRINT(RT_DEBUG_INFO,("RX err filtered2 [MPDUtotalByteCount = 0x%x]RxInfo= 0x%08x\n", pRxWI->MPDUtotalByteCount, *pRxInfo));
				}

				//
				// Do RxD release operation	for	all	failure	frames
				//
				if (Status == NDIS_STATUS_SUCCESS)
				{
					do
					{
						// pData : Pointer skip	the RxD Descriptior and the first 24 bytes,	802.11 HEADER
						pData += LENGTH_802_11 + sizeof(RXWI_STRUC);
						DataSize = (USHORT) pRxWI->MPDUtotalByteCount- LENGTH_802_11;

						//
						// CASE I. receive a DATA frame
						//				
						if (pHeader->FC.Type == BTYPE_DATA)
						{
							if (RTMPEqualMemory(EAPOL, pData + 6, 2))
							{
								EAPOLFrame = TRUE;
								DBGPRINT(RT_DEBUG_TRACE,("RxDone- EAPOL Frame\n"));
								
								//
								// On WPAPSK & WPA2PSK, it will not send EAPOL-Start to re-start 4way handshaking.
								// Since we didn't indicate media connect event to upper layer,
								// if we indicate this EAPOL frame to upper layer, this packet will be ignore
								// So we queue this packet and wait for media connect.
								//
								/*if (((pAd->CommonCfg.AuthMode == Ndis802_11AuthModeWPAPSK) || (pAd->CommonCfg.AuthMode == Ndis802_11AuthModeWPA2PSK)) && 
									 (pAd->bQueueEAPOL == TRUE))
								{
									pAd->RxEAPOLLen = 0;								
								}
								*/								
							}
							else
								EAPOLFrame = FALSE;
							pAd->MacTab.Content[BSSID_WCID].NoDataIdleCount = 0;

							pAd->BulkInDataOneSecCount++;
							// before LINK UP, all DATA frames are rejected
							if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
							{
								break;
							}

							// Drop not my BSS frame
							//
							// Not drop EAPOL frame, since this have happen on the first time that we link up
							// And need some more time to set BSSID to asic
							// So pRxD->MyBss may be 0
							//
							if ((pRxInfo->MyBss == 0) && (EAPOLFrame != TRUE))
								break; // give up this frame

							// Drop NULL (+CF-POLL) (+CF-ACK) data frame
							if ((pHeader->FC.SubType & 0x04) == 0x04)
							{
								DBGPRINT(RT_DEBUG_TRACE,("RxDone- drop NULL frame(subtype=%d)\n",pHeader->FC.SubType));
								break;
							}
							
							// remove the 2 extra QOS CNTL bytes
							bAMsdu = FALSE;
							if (pHeader->FC.SubType & 0x08)
							{
								// bit 7 in Qos Control field signals the HT A-MSDU format
								if ((*pData) & 0x80)
								{
									bAMsdu = TRUE;
								}
								pData += 2;
								DataSize -= 2;
							}

							// remove the 2 extra AGGREGATION bytes
							Msdu2Size = 0;
							if ((pHeader->FC.Order) && (bHTRate == FALSE)&& (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED)))
							{
								Msdu2Size = *pData + (*(pData+1) << 8);
								if ((Msdu2Size <= 1536) && (Msdu2Size < DataSize))
								{
									pData += 2;
									DataSize -= 2;
								}
								else
									Msdu2Size = 0;
							}
							else if ((pHeader->FC.Order) && (bHTRate == TRUE) )
							{
								ptemp = (PULONG)pData;
								if (*(ptemp+3) & 0x80)
									DBGPRINT(RT_DEBUG_TRACE,("Received HTC = %08lx : %08lx : %08lx\n", *ptemp, *(ptemp+1), *(ptemp+2)));
								pData += 4;
								DataSize -= 4;
							}

							if ((pAd->MacTab.Content[BSSID_WCID].bIAmBadAtheros == FALSE)&& (pRxInfo->AMPDU == 1) && (pHeader->FC.Retry))
							{
								pAd->MacTab.Content[BSSID_WCID].bIAmBadAtheros = TRUE;
								pAd->CommonCfg.IOTestParm.bLastAtheros = TRUE;
								if (!STA_AES_ON(pAd)) // sample
									AsicUpdateProtect(pAd, 8, ALLN_SETPROTECT, FALSE, FALSE);
								DBGPRINT(RT_DEBUG_TRACE, ("Atheros Problem. Turn on RTS/CTS!!!\n"));
								if (pAd->MacTab.Content[BSSID_WCID].BaSizeInUse != 0x10)
								{
								//	pAd->MacTab.Content[BSSID_WCID].BaSizeInUse = 0x10;
								//	BATableTearORIEntry(pAd, 0, (UCHAR)BSSID_WCID, FALSE, TRUE);
								}
							}
							// L2PAD bit on will pad 2 bytes at LLC
							if (pRxInfo->L2PAD == 1)
							{
								pData += 2;
							}
							// prepare 802.3 header: DA=addr1; SA=addr3 in INFRA mode, DA=addr2 in ADHOC mode
							pDA = pHeader->Addr1; 
							if (INFRA_ON(pAd))
								pSA	= pHeader->Addr3;
							else
								pSA	= pHeader->Addr2;

							if (pHeader->FC.Wep) // frame received in encrypted format
							{
								if (pRxInfo->CipherAlg == CIPHER_NONE) // unsupported cipher suite
								{
								}
								else if (pAd->SharedKey[BSS0][pRxWI->KeyIndex].KeyLen == 0)
								{
									break; // give up this frame since the keylen is invalid.
								}
								else
								{
#ifdef LEAP_SUPPORT
{
									// Remove CKIP LLC headers and make it as aa-aa-03-00-00-00
									if (NdisEqualMemory(pData, CKIP_LLC_SNAP, sizeof(CKIP_LLC_SNAP)))
									{
										NdisZeroMemory(pData + 3, 3);
										NdisMoveMemory(pData + 6, pData + sizeof(CKIP_LLC_SNAP) + 4 + 4, DataSize - (sizeof(CKIP_LLC_SNAP) + 4 + 4));
										DataSize -= 10;	// skip 4-byte CMIC + 4-byte TSC + 2-byte EtherType
									}
}
#endif	
								}			
							}
							else
							{// frame received in clear text
								// encryption in-use but receive a non-EAPOL clear text frame, drop it
								if (((pAd->StaCfg.WepStatus == Ndis802_11Encryption1Enabled) ||
									(pAd->StaCfg.WepStatus == Ndis802_11Encryption2Enabled) ||
									(pAd->StaCfg.WepStatus == Ndis802_11Encryption3Enabled)) &&
									(pAd->StaCfg.PrivacyFilter == Ndis802_11PrivFilter8021xWEP) &&
									(!NdisEqualMemory(EAPOL_LLC_SNAP, pData, LENGTH_802_1_H)))
								{
									break; // give up this frame
								}				
							}
							
							// 
							// Case I.1  Process Broadcast & Multicast data frame
							//
							if (pRxInfo->Bcast || pRxInfo->Mcast)
							{
								PUCHAR pRemovedLLCSNAP;

								INC_COUNTER64(pAd->WlanCounters.MulticastReceivedFrameCount);

								// Drop Mcast/Bcast frame with fragment bit on
								if (pHeader->FC.MoreFrag)
								{
									break; // give up this frame
								}

								// Filter out Bcast frame which AP relayed for us
								if (pHeader->FC.FrDs && MAC_ADDR_EQUAL(pHeader->Addr3, pAd->CurrentAddress))
								{
									break; // give up this frame
								}

								// build 802.3 header and decide if remove the 8-byte LLC/SNAP encapsulation
								CONVERT_TO_802_3(Header802_3, pDA, pSA, pData, DataSize, pRemovedLLCSNAP);
//								REPORT_ETHERNET_FRAME_TO_LLC(pAd,Header802_3, pData, DataSize, BSS0); // sample
								if (pAd->bForcePrintRX)
										DBGPRINT_RAW(RT_DEBUG_TRACE, ("!!! report BCASTDATA (no frag) to LLC (len=%d, proto=%02x:%02x) %02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x\n",
													DataSize, Header802_3[12], Header802_3[13],
													*pData, *(pData+1),*(pData+2),*(pData+3),*(pData+4),*(pData+5),*(pData+6),*(pData+7)));
							}
							//
							// Case I.2  Process unicast-to-me DATA frame
							//
							else if	(pRxInfo->U2M)
							{
								pAd->LastRxRate = (USHORT)((pRxWI->MCS) + (pRxWI->BW <<7) + (pRxWI->ShortGI <<8)+ (pRxWI->PHYMODE <<14)) ;
								if (pRxWI->RSSI0 != 0)
								{
									pAd->StaCfg.RssiSample.LastRssi0	= ConvertToRssi(pAd, (CHAR) pRxWI->RSSI0, RSSI_0);
									pAd->StaCfg.RssiSample.AvgRssi0X8	= (pAd->StaCfg.RssiSample.AvgRssi0X8 - pAd->StaCfg.RssiSample.AvgRssi0) + pAd->StaCfg.RssiSample.LastRssi0;
									pAd->StaCfg.RssiSample.AvgRssi0  	= pAd->StaCfg.RssiSample.AvgRssi0X8 >> 3;
								}
								if (pRxWI->RSSI1 != 0)
								{
									pAd->StaCfg.RssiSample.LastRssi1	= ConvertToRssi(pAd, (CHAR) pRxWI->RSSI1, RSSI_1);
									pAd->StaCfg.RssiSample.AvgRssi1X8	= (pAd->StaCfg.RssiSample.AvgRssi1X8 - pAd->StaCfg.RssiSample.AvgRssi1) + pAd->StaCfg.RssiSample.LastRssi1;
									pAd->StaCfg.RssiSample.AvgRssi1	= pAd->StaCfg.RssiSample.AvgRssi1X8 >> 3;
								}
								if (pRxWI->RSSI2 != 0)
								{
									pAd->StaCfg.RssiSample.LastRssi2	= ConvertToRssi(pAd, (CHAR) pRxWI->RSSI2, RSSI_2);
									pAd->StaCfg.RssiSample.AvgRssi2X8	= (pAd->StaCfg.RssiSample.AvgRssi2X8 - pAd->StaCfg.RssiSample.AvgRssi2) + pAd->StaCfg.RssiSample.LastRssi2;
									pAd->StaCfg.RssiSample.AvgRssi2	= pAd->StaCfg.RssiSample.AvgRssi2X8 >> 3;
								}

								pAd->StaCfg.LastSNR0 = (UCHAR)(pRxWI->SNR0);
								pAd->StaCfg.LastSNR1 = (UCHAR)(pRxWI->SNR1);

#if WPA_SUPPLICANT_SUPPORT
								if (pAd->StaCfg.WpaSupplicantUP == TRUE) 
								{
									// All EAPoL frames have to pass to upper layer (ex. WPA_SUPPLICANT daemon)
									// TBD : process fragmented EAPol frames
									if(NdisEqualMemory(EAPOL_LLC_SNAP, pData, LENGTH_802_1_H))
									{
										PUCHAR pRemovedLLCSNAP;
										int		success = 0;
					
										// In 802.1x mode, if the received frame is EAP-SUCCESS packet, turn on the PortSecured variable
										if ((pAd->StaCfg.IEEE8021X == TRUE) &&
											(EAP_CODE_SUCCESS == RTMPCheckWPAframeForEapCode(pAd, pData, DataSize, LENGTH_802_1_H)))
										{
											DBGPRINT_RAW(RT_DEBUG_TRACE, ("Receive EAP-SUCCESS Packet\n"));
											pAd->StaCfg.PortSecured = WPA_802_1X_PORT_SECURED;	

											success = 1;
										}
								
										// build 802.3 header and decide if remove the 8-byte LLC/SNAP encapsulation
										CONVERT_TO_802_3(Header802_3, pDA, pSA, pData, DataSize, pRemovedLLCSNAP);
										REPORT_ETHERNET_FRAME_TO_LLC(pAd, Header802_3, pData, DataSize);
										DBGPRINT_RAW(RT_DEBUG_TRACE, ("!!! report EAPoL DATA to LLC (len=%d) !!!\n", DataSize));
						
										if(success)
										{
											// For static wep mode, need to set wep key to Asic again
											if(pAd->StaCfg.IEEE8021x_required_keys == 0)
											{
												int idx;
								
												idx = pAd->CommonCfg.DefaultKeyId;
												for (idx=0; idx < 4; idx++)
												{
													DBGPRINT_RAW(RT_DEBUG_TRACE, ("Set WEP key to Asic again =>\n"));
						     
													if(pAd->StaCfg.DesireSharedKey[idx].KeyLen != 0)
													{
														union 
														{
															char buf[sizeof(NDIS_802_11_WEP)+MAX_LEN_OF_KEY- 1];
															NDIS_802_11_WEP keyinfo;
														}   WepKey;
														int len;

                                        
														NdisZeroMemory(&WepKey, sizeof(WepKey));
														len =pAd->StaCfg.DesireSharedKey[idx].KeyLen;
                                        
														NdisMoveMemory(WepKey.keyinfo.KeyMaterial, 
															pAd->StaCfg.DesireSharedKey[idx].Key, 
															pAd->StaCfg.DesireSharedKey[idx].KeyLen);
			              			                        
														WepKey.keyinfo.KeyIndex = 0x80000000 + idx; 
														WepKey.keyinfo.KeyLength = len;
														pAd->SharedKey[BSS0][idx].KeyLen =(UCHAR) (len <= 5 ? 5 : 13);
			
														// need to enqueue cmd to thread
														RTUSBEnqueueCmdFromNdis(pAd, OID_802_11_ADD_WEP, TRUE, &WepKey, sizeof(WepKey.keyinfo) + len - 1);
													}
												}														
											}																																				
										}
										break;	// end of processing this frame
									}
								}/* end of (pAd->StaCfg.WpaSupplicantUP == TRUE)*/
#endif

								// Special DATA frame that has to pass to MLME
								//   1. Cisco Aironet frames for CCX2. We need pass it to MLME for special process
								//   2. EAPOL handshaking frames when driver supplicant enabled, pass to MLME for special process
#ifdef LEAP_SUPPORT
								if (NdisEqualMemory(SNAP_AIRONET, pData, LENGTH_802_1_H) ||
									(NdisEqualMemory(EAPOL_LLC_SNAP, pData, LENGTH_802_1_H) && 
									((pAd->StaCfg.WpaState != SS_NOTUSE) || (pAd->StaCfg.LeapAuthMode == CISCO_AuthModeLEAP))))
#else
								if ( NdisEqualMemory(EAPOL_LLC_SNAP, pData, LENGTH_802_1_H) && (pAd->StaCfg.WpaState != SS_NOTUSE))
#endif									
								{
									pTmpBuf = (PUCHAR)(pData - LENGTH_802_11);
									DataSize += LENGTH_802_11;
									hex_dump("EAPOL Frame:", (PUCHAR)pHeader, (UINT)DataSize);
									NdisMoveMemory(pTmpBuf, (PUCHAR)pHeader, LENGTH_802_11);
									// append 802.11 header and skip Qos, padded bytes..
									REPORT_MGMT_FRAME_TO_MLME(pAd, pRxWI->WirelessCliID, pTmpBuf/*pHeader*/, DataSize, pRxWI->RSSI0,pRxWI->RSSI1, pRxWI->RSSI2, pRxInfo->PlcpSignal);
									DBGPRINT_RAW(RT_DEBUG_TRACE, ("!!! report EAPOL/AIRONET DATA to MLME (len=%d) !!!\n", DataSize));
									break;	// end of processing this frame
								}

								if (pHeader->Frag == 0) 	// First or Only fragment
								{
									PUCHAR pRemovedLLCSNAP;

									CONVERT_TO_802_3(Header802_3, pDA, pSA, pData, DataSize, pRemovedLLCSNAP);
									pAd->FragFrame.Flags &= 0xFFFFFFFE;

									// Firt Fragment & LLC/SNAP been removed. Keep the removed LLC/SNAP for later on
									// TKIP MIC verification.
									if (pHeader->FC.MoreFrag && pRemovedLLCSNAP)
									{
//										NdisMoveMemory(pAd->FragFrame.Header_LLC, pRemovedLLCSNAP, LENGTH_802_1_H); // sample
										pAd->FragFrame.Flags |= 0x01;								
									}

									// One & The only fragment
									if (pHeader->FC.MoreFrag == FALSE)
									{
										if ((pHeader->FC.Order == 1)  && (Msdu2Size > 0)) // this is an aggregation
										{
											ULONG Payload1Size, Payload2Size;
											PUCHAR pData2;

											pAd->RalinkCounters.OneSecRxAggregationCount ++;
											Payload1Size = DataSize - Msdu2Size;
											Payload2Size = Msdu2Size - LENGTH_802_3;

//											REPORT_ETHERNET_FRAME_TO_LLC(pAd, Header802_3, pData, Payload1Size, BSS0); // sample
											DBGPRINT(RT_DEBUG_INFO, ("!!! report segregated MSDU1 to LLC (len=%ld, proto=%02x:%02x) %02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x\n",
																	LENGTH_802_3+Payload1Size, Header802_3[12], Header802_3[13],
																	*pData, *(pData+1),*(pData+2),*(pData+3),*(pData+4),*(pData+5),*(pData+6),*(pData+7)));									

											pData2 = pData + Payload1Size + LENGTH_802_3;
//											REPORT_ETHERNET_FRAME_TO_LLC(pAd, pData + Payload1Size, pData2, Payload2Size, BSS0); // sample
											DBGPRINT_RAW(RT_DEBUG_INFO, ("!!! report segregated MSDU2 to LLC (len=%ld, proto=%02x:%02x) %02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x\n",
																	LENGTH_802_3+Payload2Size, *(pData2 -2), *(pData2 - 1),
																	*pData2, *(pData2+1),*(pData2+2),*(pData2+3),*(pData2+4),*(pData2+5),*(pData2+6),*(pData2+7)));									
										}
#if 0 // sample
										else if (bAMsdu)
										{
											UCHAR	DataOffset = (UCHAR)(pData - pStart);
											if ((pRxInfo->BA == TRUE) && (pHeader->FC.SubType != SUBTYPE_QOS_NULL))
											{
												if (!QosBADataParse(pAd, TRUE, Header802_3, (UCHAR)pRxWI->WirelessCliID, (UCHAR)pRxWI->TID,
																	(USHORT)pRxWI->SEQUENCE, DataOffset, pStart, DataSize))
												{
													REPORT_AMSDU_FRAMES_TO_LLC(pAd, pData, DataSize);
												}
											}
											else
											{
												REPORT_AMSDU_FRAMES_TO_LLC(pAd, pData, DataSize);
											}							
										}
										else
										{
											UCHAR		DataOffset =(UCHAR)(pData - pStart);
											BOOLEAN		Returnbrc;
											if ((pRxInfo->BA == TRUE) && (pHeader->FC.SubType != SUBTYPE_QOS_NULL))
											{
												Returnbrc = QosBADataParse(pAd, FALSE, Header802_3,  (UCHAR)pRxWI->WirelessCliID, 
														(UCHAR)pRxWI->TID, (USHORT)pRxWI->SEQUENCE, DataOffset, pStart, DataSize);
												if (Returnbrc == FALSE)
												{
													REPORT_ETHERNET_FRAME_TO_LLC(pAd, Header802_3, pData, DataSize);
												}
											}
											else
											{											
												/*if (pAd->bQueueEAPOL   == TRUE)
												{
													pAd->bQueueEAPOL  = FALSE;
												
													if (pAd->pRxEAPOL == NULL)
													{
														MlmeAllocateMemory(pAd, (PVOID *)&pAd->pRxEAPOL);
													}
												
													if ((pAd->pRxEAPOL != NULL) && (DataSize <= (MAX_LEN_OF_MLME_BUFFER - LENGTH_802_3)))
													{
														NdisMoveMemory(pAd->pRxEAPOL, Header802_3, LENGTH_802_3);
														NdisMoveMemory(pAd->pRxEAPOL + LENGTH_802_3, pData, DataSize);
														pAd->RxEAPOLLen = DataSize;
														DBGPRINT(RT_DEBUG_TRACE, ("Queue This Rx EAPOL Frame SN=%d\n", pHeader->Sequence));
													}
												}
												else*/												
												{										
											
													REPORT_ETHERNET_FRAME_TO_LLC(pAd, Header802_3, pData, DataSize);
												}
											}
											if (pAd->bForcePrintRX)
												DBGPRINT_RAW(RT_DEBUG_TRACE, ("!!! report DATA (no frag) to LLC (len=%d, proto=%02x:%02x) %02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x\n",
														DataSize, Header802_3[12], Header802_3[13],
														*pData, *(pData+1),*(pData+2),*(pData+3),*(pData+4),*(pData+5),*(pData+6),*(pData+7)));
										}
#endif
									}
									// First fragment - record the 802.3 header and frame body
									else
									{
//										NdisMoveMemory(&pAd->FragFrame.Buffer[LENGTH_802_3], pData, DataSize); // sample
//										NdisMoveMemory(pAd->FragFrame.Header802_3, Header802_3, LENGTH_802_3); // sample
										pAd->FragFrame.RxSize	 = DataSize;
										pAd->FragFrame.Sequence = pHeader->Sequence;
										pAd->FragFrame.LastFrag = pHeader->Frag;		// Should be 0
									}
								} //First or Only fragment
								// Middle & End of fragment burst fragments
								else
								{
									// No LLC-SNAP header in except the first fragment frame
									if ((pHeader->Sequence != pAd->FragFrame.Sequence) ||
										(pHeader->Frag != (pAd->FragFrame.LastFrag + 1)))
									{
										// Fragment is not the same sequence or out of fragment number order
										// Clear Fragment frame contents
										NdisZeroMemory(&pAd->FragFrame, sizeof(FRAGMENT_FRAME));
										break;
									}
									else if ((pAd->FragFrame.RxSize + DataSize) > MAX_FRAME_SIZE)
									{
										// Fragment frame is too large, it exeeds the maximum frame size.
										// Clear Fragment frame contents
										NdisZeroMemory(&pAd->FragFrame, sizeof(FRAGMENT_FRAME));
										break; // give up this frame
									}
									
									// concatenate this fragment into the re-assembly buffer
//									NdisMoveMemory(&pAd->FragFrame.Buffer[LENGTH_802_3 + pAd->FragFrame.RxSize], pData, DataSize); // sample
									pAd->FragFrame.RxSize	+= DataSize;
									pAd->FragFrame.LastFrag = pHeader->Frag;		// Update fragment number
	 
									// Last fragment
									if (pHeader->FC.MoreFrag == FALSE)
									{
										// For TKIP frame, calculate the MIC value
										if (pRxInfo->Decrypted &&
											(pRxWI->WirelessCliID < MAX_LEN_OF_MAC_TABLE)  &&
											(pAd->MacTab.Content[pRxWI->WirelessCliID].WepStatus == Ndis802_11Encryption2Enabled))
										{
											pWpaKey = &pAd->SharedKey[BSS0][pRxWI->KeyIndex];

											// Minus MIC length
											pAd->FragFrame.RxSize -= 8;

											if (pAd->FragFrame.Flags & 0x00000001)
											{
												// originally there's an LLC/SNAP field in the first fragment
												// but been removed in re-assembly buffer. here we have to include
												// this LLC/SNAP field upon calculating TKIP MIC
												// pData = pAd->FragFrame.Header_LLC;
												// Copy LLC data to the position in front of real data for MIC calculation
//												NdisMoveMemory(&pAd->FragFrame.Buffer[LENGTH_802_3 - LENGTH_802_1_H], // sample
//																pAd->FragFrame.Header_LLC, 
//																LENGTH_802_1_H);
//												pData = (PUCHAR) &pAd->FragFrame.Buffer[LENGTH_802_3 - LENGTH_802_1_H]; // sample
												DataSize = (USHORT)pAd->FragFrame.RxSize + LENGTH_802_1_H;
											}
											else
											{
//												pData = (PUCHAR) &pAd->FragFrame.Buffer[LENGTH_802_3]; // sample
												DataSize = (USHORT)pAd->FragFrame.RxSize;
											}
											
											if (RTMPTkipCompareMICValue(
													pAd,
													pData,
													pDA,
													pSA,
													pWpaKey->RxMic,
													DataSize) == FALSE)
											{
												DBGPRINT_RAW(RT_DEBUG_ERROR,("Rx MIC Value error 2\n"));
												RTMPReportMicError(pAd, pWpaKey);
												break;  // give up this frame
											}
										}

//										pData = &pAd->FragFrame.Buffer[LENGTH_802_3]; // sample
//										REPORT_ETHERNET_FRAME_TO_LLC(pAd, pAd->FragFrame.Header802_3, pData, pAd->FragFrame.RxSize, BSS0); // sample
										if (pAd->bForcePrintRX)
											DBGPRINT(RT_DEBUG_TRACE, ("!!! report DATA (fragmented) to LLC (len=%ld) !!!\n", pAd->FragFrame.RxSize));
										// Clear Fragment frame contents
										NdisZeroMemory(&pAd->FragFrame, sizeof(FRAGMENT_FRAME));
									}
								}
							}
						} // FC.Type == BTYPE_DATA
						//
						// CASE II. receive a MGMT frame
						//
						else if (pHeader->FC.Type == BTYPE_MGMT)
						{
							REPORT_MGMT_FRAME_TO_MLME(pAd, pRxWI->WirelessCliID, pHeader, pRxWI->MPDUtotalByteCount, pRxWI->RSSI0, pRxWI->RSSI1, pRxWI->RSSI2, pRxInfo->PlcpSignal);
							break;  // end of processing this frame
						}
						//
						// CASE III. receive a CNTL frame
						//
						else if (pHeader->FC.Type == BTYPE_CNTL)
						{
							if ((pHeader->FC.SubType == SUBTYPE_BLOCK_ACK_REQ))
							{
								CntlEnqueueForRecv(pAd, pRxWI->WirelessCliID, (pRxWI->MPDUtotalByteCount), (PFRAME_BA_REQ)pHeader);

							}
							break; // give up this frame
						}
						//
						// CASE IV. receive a frame of invalid type
						//
						else
							break; // give up this frame
					} while (FALSE); // ************* exit point *********
				}//if (Status == NDIS_STATUS_SUCCESS)

			}
				
			// for next packet from RXBULKAggregation, position is at ThisFrameLen+8, so add 8 here.
			ThisFrameLen += 8;
			// update next packet read position. 
			ReadPosition += ThisFrameLen;
			if (TransferBufferLength >= (ThisFrameLen))
				TransferBufferLength -= (ThisFrameLen);
			else
				TransferBufferLength = 0;
			
			if (TransferBufferLength <=  4)
				break;
		}

		pRxContext->Readable = FALSE;
		pRxContext->BulkInOffset = 0;
		pRxContext = &pAd->RxContext[pAd->NextRxBulkInReadIndex];
	}	while (1);

	//
	// We return STATUS_MORE_PROCESSING_REQUIRED so that the completion
	// routine (IofCompleteRequest) will stop working on the irp.
	//
	if (bBulkReceive == TRUE)
		RTUSBBulkReceive(pAd);

	return 0;//STATUS_MORE_PROCESSING_REQUIRED;
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
VOID	RTUSBDequeueMLMEPacket(
	IN	PRTMP_ADAPTER	pAd)
{
	PMGMT_STRUC		pMgmt;
	
	NdisAcquireSpinLock(&pAd->MLMEWaitQueueLock);
	while ((pAd->PopMgmtIndex != pAd->PushMgmtIndex) || (atomic_read(&pAd->MgmtQueueSize) > 0 ))
	{
		pMgmt = &pAd->USB_MgmtRing[pAd->PopMgmtIndex];

		if (RTUSBFreeDescriptorRequest(pAd, PRIO_RING, 0, 1) == NDIS_STATUS_SUCCESS)
		{
			InterlockedDecrement(&pAd->MgmtQueueSize);
			pAd->PopMgmtIndex = (pAd->PopMgmtIndex + 1) % MGMT_RING_SIZE;
			NdisReleaseSpinLock(&pAd->MLMEWaitQueueLock);

			RTUSBMlmeHardTransmit(pAd, pMgmt);

			MlmeFreeMemory(pAd, pMgmt->pBuffer);
			pMgmt->pBuffer = NULL;
			pMgmt->Valid = FALSE;

			NdisAcquireSpinLock(&pAd->MLMEWaitQueueLock);
		}
		else
		{
			DBGPRINT(RT_DEBUG_TRACE, ("not enough space in PrioRing[pAdapter->MgmtQueueSize=%d]\n", atomic_read(&pAd->MgmtQueueSize)));
			DBGPRINT(RT_DEBUG_TRACE, ("RTUSBDequeueMLMEPacket::PrioRingFirstIndex = %d, PrioRingTxCnt = %ld, PopMgmtIndex = %d, PushMgmtIndex = %d, NextMLMEIndex = %d\n", 
					pAd->PrioRingFirstIndex, pAd->PrioRingTxCnt, 
					pAd->PopMgmtIndex, pAd->PushMgmtIndex, pAd->NextMLMEIndex));			
			break;
		}
	}
	NdisReleaseSpinLock(&pAd->MLMEWaitQueueLock);
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
VOID	RTUSBCleanUpMLMEWaitQueue(
	IN	PRTMP_ADAPTER	pAd)
{
	PMGMT_STRUC		pMgmt;
	
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("--->CleanUpMLMEWaitQueue\n"));

	NdisAcquireSpinLock(&pAd->MLMEWaitQueueLock);
	while (pAd->PopMgmtIndex != pAd->PushMgmtIndex)
	{
		pMgmt = (PMGMT_STRUC)&pAd->USB_MgmtRing[pAd->PopMgmtIndex];
		MlmeFreeMemory(pAd, pMgmt->pBuffer);
		pMgmt->pBuffer = NULL;
		pMgmt->Valid = FALSE;
		InterlockedDecrement(&pAd->MgmtQueueSize);

		pAd->PopMgmtIndex++;
		if (pAd->PopMgmtIndex >= MGMT_RING_SIZE)
		{
			pAd->PopMgmtIndex = 0;
		}
	}
	NdisReleaseSpinLock(&pAd->MLMEWaitQueueLock);

	DBGPRINT_RAW(RT_DEBUG_TRACE, ("<---CleanUpMLMEWaitQueue\n"));
}


/*
	========================================================================

	Routine	Description:
		API for MLME to transmit management frame to AP (BSS Mode)
	or station (IBSS Mode)
	
	Arguments:
		pAd	Pointer	to our adapter
		Buffer		Pointer to  memory of outgoing frame
		Length		Size of outgoing management frame
		
	Return Value:
		NDIS_STATUS_FAILURE
		NDIS_STATUS_PENDING
		NDIS_STATUS_SUCCESS

	Note:
	
	========================================================================
*/
VOID	MiniportMMRequest(
	IN	PRTMP_ADAPTER	pAd,
	IN	PVOID			pBuffer,
	IN	ULONG			Length)
{
	DBGPRINT_RAW(RT_DEBUG_INFO, ("---> MiniportMMRequest\n"));
	
	if (pBuffer)
	{
		PMGMT_STRUC	pMgmt;

		// Check management ring free avaliability
		NdisAcquireSpinLock(&pAd->MLMEWaitQueueLock);
		pMgmt = (PMGMT_STRUC)&pAd->USB_MgmtRing[pAd->PushMgmtIndex];
		// This management cell has been occupied
		if (pMgmt->Valid == TRUE)
		{
			NdisReleaseSpinLock(&pAd->MLMEWaitQueueLock);
			MlmeFreeMemory(pAd, pBuffer);
			pAd->RalinkCounters.MgmtRingFullCount++;
			DBGPRINT_RAW(RT_DEBUG_WARN, ("MiniportMMRequest (error:: MgmtRing full)\n"));
		}
		// Insert this request into software managemnet ring
		else
		{
			pMgmt->pBuffer = pBuffer;
			pMgmt->Length  = Length;
			pMgmt->Valid   = TRUE;
			pAd->PushMgmtIndex++;
			InterlockedIncrement(&pAd->MgmtQueueSize);
			if (pAd->PushMgmtIndex >= MGMT_RING_SIZE)
			{
				pAd->PushMgmtIndex = 0;
			}
			NdisReleaseSpinLock(&pAd->MLMEWaitQueueLock);
		}
	}
	else
		DBGPRINT(RT_DEBUG_WARN, ("MiniportMMRequest (error:: NULL msg)\n"));
	
	RTUSBDequeueMLMEPacket(pAd);

	// If pAd->PrioRingTxCnt is larger than 0, this means that prio_ring have something to transmit.
	// Then call KickBulkOut to transmit it
	if (pAd->PrioRingTxCnt > 0)
	{
		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE))
			AsicForceWakeup(pAd);
		RTUSBKickBulkOut(pAd);
	}
	
	DBGPRINT_RAW(RT_DEBUG_INFO, ("<--- MiniportMMRequest\n"));
}


/*
	========================================================================

	Routine	Description:
		Apply packet filter policy, return NDIS_STATUS_FAILURE if this frame
		should be dropped.
		
	Arguments:
		pAd		Pointer	to our adapter
		pRxD			Pointer	to the Rx descriptor
		pHeader			Pointer to the 802.11 frame header
		
	Return Value:
		NDIS_STATUS_SUCCESS		Accept frame
		NDIS_STATUS_FAILURE		Drop Frame
		
	Note:
		Maganement frame should bypass this filtering rule.
	
	========================================================================
*/
NDIS_STATUS	RTMPApplyPacketFilter(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PRXINFO_STRUC	pRxInfo, 
	IN	PHEADER_802_11	pHeader)
{
//	UCHAR	i;
	PULONG	ptemp;
	
	// 0. Management frame should bypass all these filtering rules.
	if (pHeader->FC.Type == BTYPE_MGMT)
	{
		if ((pRxInfo->U2M) || (pRxInfo->Bcast) || (pRxInfo->Mcast))//steven:for ASIC Bug Workaround
		{
			//ASSERT(FALSE);
			return(NDIS_STATUS_SUCCESS);
		}
	}

	// 0.1	Drop all Rx frames if MIC countermeasures kicks in
	if (pAd->StaCfg.MicErrCnt >= 2)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE,("Rx dropped by MIC countermeasure\n"));
		return(NDIS_STATUS_FAILURE);
	}

	// 1. Drop unicast to me packet if NDIS_PACKET_TYPE_DIRECTED is FALSE
	if (pRxInfo->U2M) 
	{
		if (!RX_FILTER_TEST_FLAG(pAd, fRX_FILTER_ACCEPT_DIRECT))
		{
			DBGPRINT_RAW(RT_DEBUG_TRACE,("Rx U2M dropped by RX_FILTER\n"));
			return(NDIS_STATUS_FAILURE);
		}
	}
		
	// 2. Drop broadcast packet if NDIS_PACKET_TYPE_BROADCAST is FALSE
	else if (pRxInfo->Bcast)
	{
		if (!RX_FILTER_TEST_FLAG(pAd, fRX_FILTER_ACCEPT_BROADCAST))
		{
			DBGPRINT_RAW(RT_DEBUG_TRACE,("Rx BCAST dropped by RX_FILTER\n"));
			return(NDIS_STATUS_FAILURE);
		}
	}
			
	// 3. Drop (non-Broadcast) multicast packet if NDIS_PACKET_TYPE_ALL_MULTICAST is false
	//    and NDIS_PACKET_TYPE_MULTICAST is false.
	//    If NDIS_PACKET_TYPE_MULTICAST is true, but NDIS_PACKET_TYPE_ALL_MULTICAST is false.
	//    We have to deal with multicast table lookup & drop not matched packets.
	else if (pRxInfo->Mcast)
	{
		if (!RX_FILTER_TEST_FLAG(pAd, fRX_FILTER_ACCEPT_ALL_MULTICAST))
		{
			if (!RX_FILTER_TEST_FLAG(pAd, fRX_FILTER_ACCEPT_MULTICAST))
			{
				DBGPRINT_RAW(RT_DEBUG_INFO,("Rx MCAST dropped by RX_FILTER\n"));
				return(NDIS_STATUS_FAILURE);
			}
#ifdef WIN_NDIS
			else
			{
				// Selected accept multicast packet based on multicast table
				for (i = 0; i < pAd->NumberOfMcastAddresses; i++)
				{
					if (MAC_ADDR_EQUAL(pHeader->Addr1, pAd->McastTable[i]))
					break;		// Matched
				}

				// Not matched
				if (i == pAd->NumberOfMcastAddresses)
				{
					DBGPRINT(RT_DEBUG_INFO,("Rx MCAST %02x:%02x:%02x:%02x:%02x:%02x dropped by RX_FILTER\n",
											pHeader->Addr1[0], pHeader->Addr1[1], pHeader->Addr1[2], 
											pHeader->Addr1[3], pHeader->Addr1[4], pHeader->Addr1[5]));
					return(NDIS_STATUS_FAILURE);
				}
				else
				{
					DBGPRINT(RT_DEBUG_INFO,("Accept multicast %02x:%02x:%02x:%02x:%02x:%02x\n",
											pHeader->Addr1[0], pHeader->Addr1[1], pHeader->Addr1[2], 
											pHeader->Addr1[3], pHeader->Addr1[4], pHeader->Addr1[5]));
				}
			}
#endif // WIN_NDIS //
		}
	}

	// 4. Not U2M, not Mcast, not Bcast, must be unicast to other DA.
	//    Since we did not implement promiscuous mode, just drop this kind of packet for now.
	else
	{
		ptemp = (PULONG)pRxInfo;
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("not-to-me unicast ,RxInfo = 0x%08lx\n", *ptemp));
		return(NDIS_STATUS_FAILURE);
	}

	return(NDIS_STATUS_SUCCESS);	
}

	
#if WPA_SUPPLICANT_SUPPORT
static void ralink_michael_mic_failure(struct net_device *dev, PCIPHER_KEY pWpaKey)
{
	union iwreq_data wrqu;
	char buf[128];

	/* TODO: needed parameters: count, keyid, key type, TSC */
	
	//Check for Group or Pairwise MIC error
	if (pWpaKey->Type == PAIRWISE_KEY)
		sprintf(buf, "MLME-MICHAELMICFAILURE.indication unicast");
	else
		sprintf(buf, "MLME-MICHAELMICFAILURE.indication broadcast");
	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = strlen(buf);
	//send mic error event to wpa_supplicant
	wireless_send_event(dev, IWEVCUSTOM, &wrqu, buf);
}
#endif


/*
	========================================================================

	Routine	Description:
		Process MIC error indication and record MIC error timer.
		
	Arguments:
		pAd		Pointer	to our adapter
		pWpaKey			Pointer	to the WPA key structure
		
	Return Value:
		None
		
	Note:
	
	========================================================================
*/
VOID	RTMPReportMicError(
	IN	PRTMP_ADAPTER	pAd, 
	IN PCIPHER_KEY pWpaKey)
{
	ULONG	Now;
	struct
	{
		NDIS_802_11_STATUS_INDICATION		Status;
		NDIS_802_11_AUTHENTICATION_REQUEST	Request;
	}	Report;

#if WPA_SUPPLICANT_SUPPORT
	if (pAd->StaCfg.WpaSupplicantUP == TRUE) 
	{
		//report mic error to wpa_supplicant
		ralink_michael_mic_failure(get_netdev_from_bssid(pAd, 0), pWpaKey);
	}
#endif

	// 0. Set Status to indicate auth error
	Report.Status.StatusType = Ndis802_11StatusType_Authentication;
	
	// 1. Check for Group or Pairwise MIC error
	if (pWpaKey->Type == PAIRWISE_KEY)
		Report.Request.Flags = NDIS_802_11_AUTH_REQUEST_PAIRWISE_ERROR;
	else
		Report.Request.Flags = NDIS_802_11_AUTH_REQUEST_GROUP_ERROR;

	// 2. Copy AP MAC address
	NdisMoveMemory(Report.Request.Bssid, pWpaKey->BssId, 6);

	// 3. Calculate length
	Report.Request.Length = sizeof(NDIS_802_11_AUTHENTICATION_REQUEST);
#ifdef WIN_NDIS
	// 4. Indicate to NDIS
	NdisMIndicateStatus(pAd->AdapterHandle, NDIS_STATUS_MEDIA_SPECIFIC_INDICATION, (PVOID) &Report, sizeof(Report));
	NdisMIndicateStatusComplete(pAd->AdapterHandle);
#endif
	// 5. Record Last MIC error time and count
	NdisGetSystemUpTime(&Now);
	if (pAd->StaCfg.MicErrCnt == 0)
	{
		pAd->StaCfg.MicErrCnt++;
		pAd->StaCfg.LastMicErrorTime = Now;
	}
	else if (pAd->StaCfg.MicErrCnt == 1)
	{
		if ((pAd->StaCfg.LastMicErrorTime + (60 * OS_HZ)) > Now)
		{
			// Update Last MIC error time, this did not violate two MIC errors within 60 seconds
			pAd->StaCfg.LastMicErrorTime = Now;			
		}
		else
		{
			pAd->StaCfg.LastMicErrorTime = Now;			
			// Violate MIC error counts, MIC countermeasures kicks in
			pAd->StaCfg.MicErrCnt++;			
			// We shall block all reception
			// We shall clean all Tx ring and disassoicate from AP after next EAPOL frame
			RTUSBRejectPendingPackets(pAd);
			//RTUSBCleanUpDataBulkOutQueue(pAd);
		}
	}
	else
	{
		// MIC error count >= 2
		// This should not happen
		;
	}
}


/*
	========================================================================

	Routine	Description:
		Copy frame from waiting queue into relative ring buffer and set 
	appropriate ASIC register to kick hardware transmit function
	
	Arguments:
		pAd	Pointer	to our adapter
		pBuffer		Pointer to  memory of outgoing frame
		Length		Size of outgoing management frame
		
	Return Value:
		NDIS_STATUS_FAILURE
		NDIS_STATUS_PENDING
		NDIS_STATUS_SUCCESS

	Note:
	
	========================================================================
*/
VOID	RTUSBMlmeHardTransmit(
	IN	PRTMP_ADAPTER	pAd,
	IN	PMGMT_STRUC		pMgmt)
{
	PTX_CONTEXT		pMLMEContext;
	PTXWI_STRUC		pTxWI;
	PTXINFO_STRUC		pTxInfo;
	PUCHAR			pDest;	
	PHEADER_802_11	pHeader_802_11;
	BOOLEAN         	AckRequired, InsertTimestamp;
	ULONG			TransferBufferLength;
	PVOID			pBuffer = pMgmt->pBuffer;
	ULONG			Length = pMgmt->Length;
	UCHAR 			QueIdx;
	UCHAR			MlmeRate;
	DBGPRINT_RAW(RT_DEBUG_INFO, ("--->MlmeHardTransmit\n"));

	pAd->PrioRingTxCnt++;

	pMLMEContext = &pAd->MLMEContext[pAd->NextMLMEIndex];
	pMLMEContext->InUse = TRUE;
	pMLMEContext->bWaitingBulkOut = TRUE;

	// Increase & maintain Tx Ring Index
	pAd->NextMLMEIndex++;
	if (pAd->NextMLMEIndex >= MGMT_RING_SIZE)
	{
		pAd->NextMLMEIndex = 0;
	}

	pTxInfo				= (PTXINFO_STRUC)(pMLMEContext->TransferBuffer->WirelessPacket);
	NdisZeroMemory(pTxInfo, sizeof(TXINFO_STRUC));
	
	pTxWI				= (PTXWI_STRUC)(&pMLMEContext->TransferBuffer->WirelessPacket[4]);
	NdisZeroMemory(pTxWI, sizeof(TXWI_STRUC));
	pDest				= &pMLMEContext->TransferBuffer->WirelessPacket[4+TXWI_SIZE];              
	
	// Verify Mlme rate for a / g bands.
	MlmeRate = pAd->CommonCfg.MlmeRate;
	if ((pAd->LatchRfRegs.Channel > 14) && (MlmeRate < RATE_6)) // 11A band
		MlmeRate = RATE_6;

	pHeader_802_11 = (PHEADER_802_11) pBuffer;
	
	// Before radar detection done, mgmt frame can not be sent but probe req
	// Because we need to use probe req to trigger driver to send probe req in passive scan
	if ((pHeader_802_11->FC.SubType != SUBTYPE_PROBE_REQ) && (pAd->CommonCfg.bIEEE80211H == 1) && (pAd->CommonCfg.RadarDetect.RDMode != RD_NORMAL_MODE))
	{
		DBGPRINT(RT_DEBUG_ERROR,("RTUSBMlmeHardTransmit --> radar detect not in normal mode !!!\n"));
		return;
	}
	
	if (pHeader_802_11->FC.PwrMgmt != PWR_SAVE)
	{
		pHeader_802_11->FC.PwrMgmt = (pAd->StaCfg.Psm == PWR_SAVE);
	}
	
	InsertTimestamp = FALSE;
	if (pHeader_802_11->FC.Type == BTYPE_CNTL) // must be PS-POLL
	{
		AckRequired = FALSE;
	}
	else // BTYPE_MGMT or BMGMT_DATA(must be NULL frame)
	{
		pAd->Sequence       = ((pAd->Sequence) + 1) & (MAX_SEQ_NUMBER);
		pHeader_802_11->Sequence = pAd->Sequence;

		if (pHeader_802_11->Addr1[0] & 0x01) // MULTICAST, BROADCAST
		{
			INC_COUNTER64(pAd->WlanCounters.MulticastTransmittedFrameCount);
			AckRequired = FALSE;
			pHeader_802_11->Duration = 0;
		}
		else
		{
			AckRequired = TRUE;
			pHeader_802_11->Duration = RTMPCalcDuration(pAd, MlmeRate, 14);
			if (pHeader_802_11->FC.SubType == SUBTYPE_PROBE_RSP)
			{
				InsertTimestamp = TRUE;
			}
		}
	}
	
	NdisMoveMemory(pDest, pBuffer, Length);

	// Initialize Priority Descriptor
	// For inter-frame gap, the number is for this frame and next frame
	// For MLME rate, we will fix as 2Mb to match other vendor's implement
	
	QueIdx = QID_AC_BE;
	RTMPWriteTxWI(pAd, pTxWI,  FALSE, FALSE, FALSE, FALSE, AckRequired, FALSE, 0, MLME_WCID, (Length),
		0, 0, (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, IFS_BACKOFF, FALSE, &pAd->CommonCfg.MlmeTransmit);
	RTMPWriteTxInfo(pAd, pTxInfo, (USHORT)(Length+TXWI_SIZE), TRUE, EpToQueue[MGMTPIPEIDX], FALSE,  FALSE);

	// Always add 4 extra bytes at every packet 
	pDest+= Length;
	// Build our URB for USBD
	TransferBufferLength = TXWI_SIZE + Length + sizeof(TXINFO_STRUC);
	
	if ((TransferBufferLength % 4) == 1)	
	{
		NdisZeroMemory(pDest, 7);
		pDest += 7;
		TransferBufferLength  += 3;
	}
	else if ((TransferBufferLength % 4) == 2)	
	{
		NdisZeroMemory(pDest, 6);
		pDest += 6;
		TransferBufferLength  += 2;
	}
	else if ((TransferBufferLength % 4) == 3)	
	{
		NdisZeroMemory(pDest, 5);
		pDest += 5;
		TransferBufferLength  += 1;
	}
	pTxInfo->USBDMATxPktLen = TransferBufferLength - TXINFO_SIZE;

	TransferBufferLength += 4;
	// If TransferBufferLength is multiple of 64, add extra 4 bytes again.
	if ((TransferBufferLength % pAd->BulkOutMaxPacketSize) == 0)
	{
		NdisZeroMemory(pDest, 4);
		TransferBufferLength += 4;
	}
	// Length in TxInfo should be 8 less than bulkout size.
	pMLMEContext->BulkOutSize = TransferBufferLength;
	RTUSB_SET_BULK_FLAG(pAd, fRTUSB_BULK_OUT_MLME);
	
	DBGPRINT(RT_DEBUG_INFO, ("<---MlmeHardTransmit\n"));
}


/* 
	==========================================================================
	Description:
		Send out a NULL frame to AP. The purpose is to inform AP this client 
		current PSM bit.
	NOTE:
		This routine should only be used in infrastructure mode.
	==========================================================================
 */
VOID	USBTmpRTMPSendNullFrame(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			TxRate,
	IN	BOOLEAN 		bQosNull)
{
	UCHAR	NullFrame[48];
	ULONG	Length;
	PHEADER_802_11	pHeader_802_11;

#ifdef RALINK_ATE
	if(ATE_ON(pAd))
	{
		return;
	}
#endif // RALINK_ATE //

	if (RTMP_TEST_FLAG(pAd ,fRTMP_ADAPTER_NEED_STOP_TX))
		return;
			
	// WPA 802.1x secured port control
	if (((pAd->StaCfg.AuthMode == Ndis802_11AuthModeWPA) || 
		 (pAd->StaCfg.AuthMode == Ndis802_11AuthModeWPAPSK) ||
		 (pAd->StaCfg.AuthMode == Ndis802_11AuthModeWPA2) || 
		 (pAd->StaCfg.AuthMode == Ndis802_11AuthModeWPA2PSK)
        ) &&
		(pAd->StaCfg.PortSecured == WPA_802_1X_PORT_NOT_SECURED))
	{
		return;
	}

	NdisZeroMemory(NullFrame, 48);
	Length = sizeof(HEADER_802_11);

	pHeader_802_11 = (PHEADER_802_11) NullFrame;
	
	pHeader_802_11->FC.Type = BTYPE_DATA;
	pHeader_802_11->FC.SubType = SUBTYPE_NULL_FUNC;
	pHeader_802_11->FC.ToDs = 1;
	COPY_MAC_ADDR(pHeader_802_11->Addr1, pAd->CommonCfg.Bssid);
	COPY_MAC_ADDR(pHeader_802_11->Addr2, pAd->CurrentAddress);
	COPY_MAC_ADDR(pHeader_802_11->Addr3, pAd->CommonCfg.Bssid);

	if (pAd->CommonCfg.bAPSDForcePowerSave)
	{
		pHeader_802_11->FC.PwrMgmt = PWR_SAVE;
	}
	else
	{
		pHeader_802_11->FC.PwrMgmt = (pAd->StaCfg.Psm == PWR_SAVE) ? 1: 0;
	}
	pHeader_802_11->Duration = pAd->CommonCfg.Dsifs + RTMPCalcDuration(pAd, TxRate, 14);

	pAd->Sequence++;
	pHeader_802_11->Sequence = pAd->Sequence;

	// Prepare QosNull function frame
	if (bQosNull)
	{
		pHeader_802_11->FC.SubType = SUBTYPE_QOS_NULL;
		
		// copy QOS control bytes
		NullFrame[Length]	=  0;
		NullFrame[Length+1] =  0;
		Length += 2;// if pad with 2 bytes for alignment, APSD will fail
	}


	if (pAd->NullContext.InUse == FALSE)
	{
		PTX_CONTEXT		pNullContext;
		PTXINFO_STRUC	pTxInfo;
		PTXWI_STRUC		pTxWI;

	pNullContext = &(pAd->NullContext);

		// Set the in use bit
		pNullContext->InUse = TRUE;

		RTMPZeroMemory(&pAd->NullContext.TransferBuffer->WirelessPacket[0], 100);
		pTxInfo = (PTXINFO_STRUC)&pAd->NullContext.TransferBuffer->WirelessPacket[0];
		RTMPWriteTxInfo(pAd, pTxInfo, (USHORT)(sizeof(HEADER_802_11)+TXWI_SIZE), TRUE, EpToQueue[MGMTPIPEIDX], FALSE,  FALSE);
		pTxInfo->QSEL = FIFO_EDCA;
		pTxWI = (PTXWI_STRUC)&pAd->NullContext.TransferBuffer->WirelessPacket[TXINFO_SIZE];
		RTMPWriteTxWI(pAd, pTxWI,  FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, 0, BSSID_WCID, (sizeof(HEADER_802_11)),
			0, 0, (UCHAR)pAd->CommonCfg.MlmeTransmit.field.MCS, IFS_HTTXOP, FALSE, &pAd->CommonCfg.MlmeTransmit);
		RTMPMoveMemory(&pAd->NullContext.TransferBuffer->WirelessPacket[TXWI_SIZE+TXINFO_SIZE], &pAd->NullFrame, sizeof(HEADER_802_11));
		pAd->NullContext.BulkOutSize =  TXINFO_SIZE + TXWI_SIZE + sizeof(pAd->NullFrame) + 4;				

		// Fill out frame length information for global Bulk out arbitor
		//pNullContext->BulkOutSize = TransferBufferLength;
		DBGPRINT(RT_DEBUG_TRACE, ("SYNC - send NULL Frame @%d Mbps...\n", RateIdToMbps[TxRate]));
		RTUSB_SET_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NULL);

		// Kick bulk out 
		RTUSBKickBulkOut(pAd);
	}

}


VOID RTMPSendRTSCTSFrame(
	IN  PRTMP_ADAPTER	pAd,
	IN  PUCHAR			pDA,
	IN  ULONG			NextMpduSize,
	IN  UCHAR			TxRate,
	IN  UCHAR			RTSRate,
	IN  USHORT			AckDuration,
	IN  UCHAR			QueIdx,
	IN  UCHAR			FrameGap,
	IN  UCHAR			Type)
{
	PTX_CONTEXT     		pTxContext;
	PTXD_STRUC			pTxD;
	PRTS_FRAME			pRtsFrame;
	PUCHAR				pBuf;
	ULONG				Length = 0;
	ULONG				TransferBufferLength = 0;

	if ((Type != SUBTYPE_RTS) && ( Type != SUBTYPE_CTS))
	{
		DBGPRINT(RT_DEBUG_WARN,("Making RTS/CTS Frame failed, type not matched!\n"));
		return;
	}
	else if ((Type == SUBTYPE_RTS) && ((*pDA) & 0x01))
	{
		if ((*pDA) & 0x01)
		{
			// should not use RTS/CTS to protect MCAST frame since no one will reply CTS
			DBGPRINT(RT_DEBUG_INFO,("Not use RTS Frame to proect MCAST frame\n"));
			return;
		}
	}

	pTxContext  = &pAd->TxContext[QueIdx][pAd->NextTxIndex[QueIdx]];
	if (pTxContext->InUse == FALSE)
	{
		pTxContext->InUse   = TRUE;
		pAd->TxRingTotalNumber[QueIdx]++;		
		pAd->NextTxIndex[QueIdx]++;
		if (pAd->NextTxIndex[QueIdx] >= TX_RING_SIZE)
		{
			pAd->NextTxIndex[QueIdx] = 0;
		}
		pTxD = (PTXD_STRUC) &pTxContext->TransferBuffer->TxDesc;
		pRtsFrame = (PRTS_FRAME) &pTxContext->TransferBuffer->RTSFrame;
		pBuf = (PUCHAR) pRtsFrame;

		NdisZeroMemory(pRtsFrame, sizeof(RTS_FRAME));
		pRtsFrame->FC.Type    = BTYPE_CNTL;
		// CTS-to-self's duration = SIFS + MPDU
		pRtsFrame->Duration = (2 * pAd->CommonCfg.Dsifs) + RTMPCalcDuration(pAd, TxRate, NextMpduSize) + AckDuration;// SIFS + Data + SIFS + ACK

		// Write Tx descriptor
		// Don't kick tx start until all frames are prepared
		// RTS has to set more fragment bit for fragment burst
		// RTS did not encrypt		

		if (Type == SUBTYPE_RTS)
		{
			DBGPRINT(RT_DEBUG_INFO,("Making RTS Frame\n"));

			pRtsFrame->FC.SubType = SUBTYPE_RTS;        
			COPY_MAC_ADDR(pRtsFrame->Addr1, pDA);
			COPY_MAC_ADDR(pRtsFrame->Addr2, pAd->CurrentAddress);
			
			// RTS's duration need to include and extra (SIFS + CTS) time
			pRtsFrame->Duration += (pAd->CommonCfg.Dsifs + RTMPCalcDuration(pAd, RTSRate, 14)); // SIFS + CTS-Duration

			Length = sizeof(RTS_FRAME);
			RTMPWriteTxWI(pAd, pTxD, CIPHER_NONE, 0,0, TRUE, TRUE, FALSE, SHORT_RETRY,
				FrameGap, RTSRate, Length, QueIdx, 0);

			// Fill out frame length information for global Bulk out arbitor
			pTxContext->BulkOutSize = sizeof(TXD_STRUC) + sizeof(RTS_FRAME);
		}
		else if (Type == SUBTYPE_CTS)
		{
			DBGPRINT(RT_DEBUG_INFO,("Making CTS-to-self Frame\n"));
			pRtsFrame->FC.SubType = SUBTYPE_CTS;		
			COPY_MAC_ADDR(pRtsFrame->Addr1, pAd->CurrentAddress);

			Length = 10;  //CTS frame length.
			RTMPWriteTxWI(pAd, pTxD, CIPHER_NONE, 0,0, FALSE, TRUE, FALSE, SHORT_RETRY,
				FrameGap, RTSRate, Length, QueIdx, 0);
		}

		// Build our URB for USBD
		TransferBufferLength = sizeof(TXD_STRUC) + Length;
		if ((TransferBufferLength % 2) == 1)
			TransferBufferLength++;
		if ((TransferBufferLength % pAd->BulkOutMaxPacketSize) == 0)
			TransferBufferLength += 2;

		// Fill out frame length information for global Bulk out arbitor
		pTxContext->BulkOutSize = TransferBufferLength;
		pTxContext->bWaitingBulkOut = TRUE;
		pTxContext->LastOne = TRUE;

		NdisInterlockedIncrement(&pAd->TxCount);
		RTUSB_SET_BULK_FLAG(pAd, fRTUSB_BULK_OUT_DATA_NORMAL << QueIdx);

	}
}
#endif
