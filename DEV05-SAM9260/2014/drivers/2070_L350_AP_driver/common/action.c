/****************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 * (c) Copyright 2002, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ****************************************************************************

    Module Name:
	action.c
 
    Abstract:
    Handle association related requests either from WSTA or from local MLME
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
	Jan Lee		2006	  	created for rt2860
 */

#include "rt_config.h"
#include "action.h"


static VOID ReservedAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem);

/*  
    ==========================================================================
    Description: 
        association state machine init, including state transition and timer init
    Parameters: 
        S - pointer to the association state machine
    Note:
        The state machine looks like the following 
        
                                    ASSOC_IDLE             
        MT2_MLME_DISASSOC_REQ    mlme_disassoc_req_action 
        MT2_PEER_DISASSOC_REQ    peer_disassoc_action     
        MT2_PEER_ASSOC_REQ       drop                     
        MT2_PEER_REASSOC_REQ     drop                     
        MT2_CLS3ERR              cls3err_action           
    ==========================================================================
 */
VOID ActionStateMachineInit(
    IN	PRTMP_ADAPTER	pAd, 
    IN  STATE_MACHINE *S, 
    OUT STATE_MACHINE_FUNC Trans[]) 
{
	StateMachineInit(S, (STATE_MACHINE_FUNC *)Trans, MAX_ACT_STATE, MAX_ACT_MSG, (STATE_MACHINE_FUNC)Drop, ACT_IDLE, ACT_MACHINE_BASE);

	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_SPECTRUM_CATE, (STATE_MACHINE_FUNC)PeerSpectrumAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_QOS_CATE, (STATE_MACHINE_FUNC)PeerQOSAction);

	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_DLS_CATE, (STATE_MACHINE_FUNC)ReservedAction);
#ifdef QOS_DLS_SUPPORT
		StateMachineSetAction(S, ACT_IDLE, MT2_PEER_DLS_CATE, (STATE_MACHINE_FUNC)PeerDLSAction);
#endif // QOS_DLS_SUPPORT //


	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_PUBLIC_CATE, (STATE_MACHINE_FUNC)PeerPublicAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_PEER_RM_CATE, (STATE_MACHINE_FUNC)PeerRMAction);
	
	StateMachineSetAction(S, ACT_IDLE, MT2_MLME_QOS_CATE, (STATE_MACHINE_FUNC)MlmeQOSAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_MLME_DLS_CATE, (STATE_MACHINE_FUNC)MlmeDLSAction);
	StateMachineSetAction(S, ACT_IDLE, MT2_ACT_INVALID, (STATE_MACHINE_FUNC)MlmeInvalidAction);
}


VOID MlmeQOSAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
}

VOID MlmeDLSAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
}

VOID MlmeInvalidAction(
    IN PRTMP_ADAPTER pAd, 
    IN MLME_QUEUE_ELEM *Elem) 
{
	//PUCHAR		   pOutBuffer = NULL;
	//Return the receiving frame except the MSB of category filed set to 1.  7.3.1.11
}

VOID PeerQOSAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
}

#ifdef QOS_DLS_SUPPORT
VOID PeerDLSAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	UCHAR	Action = Elem->Msg[LENGTH_802_11+1];

	switch(Action)
	{
		case ACTION_DLS_REQUEST:
#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
				APPeerDlsReqAction(pAd, Elem);
#endif // CONFIG_AP_SUPPORT //
			break;

		case ACTION_DLS_RESPONSE:
#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
				APPeerDlsRspAction(pAd, Elem);
#endif // CONFIG_AP_SUPPORT //
			break;

		case ACTION_DLS_TEARDOWN:
#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
				APPeerDlsTearDownAction(pAd, Elem);
#endif // CONFIG_AP_SUPPORT //
			break;
	}
}
#endif // QOS_DLS_SUPPORT //


VOID PeerPublicAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	
	if (Elem->Wcid >= MAX_LEN_OF_MAC_TABLE)
		return;


}	


static VOID ReservedAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem)
{
	UCHAR Category;

	if (Elem->MsgLen <= LENGTH_802_11)
	{
		return;
	}

	Category = Elem->Msg[LENGTH_802_11];
	DBGPRINT(RT_DEBUG_TRACE,("Rcv reserved category(%d) Action Frame\n", Category));
	hex_dump("Reserved Action Frame", &Elem->Msg[0], Elem->MsgLen);
}

VOID PeerRMAction(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 

{
	return;
}


VOID ActHeaderInit(
    IN	PRTMP_ADAPTER	pAd, 
    IN OUT PHEADER_802_11 pHdr80211, 
    IN PUCHAR Addr1, 
    IN PUCHAR Addr2,
    IN PUCHAR Addr3) 
{
    NdisZeroMemory(pHdr80211, sizeof(HEADER_802_11));
    pHdr80211->FC.Type = BTYPE_MGMT;
    pHdr80211->FC.SubType = SUBTYPE_ACTION;

    COPY_MAC_ADDR(pHdr80211->Addr1, Addr1);
	COPY_MAC_ADDR(pHdr80211->Addr2, Addr2);
    COPY_MAC_ADDR(pHdr80211->Addr3, Addr3);
}

VOID BarHeaderInit(
	IN	PRTMP_ADAPTER	pAd, 
	IN OUT PFRAME_BAR pCntlBar, 
	IN PUCHAR pDA,
	IN PUCHAR pSA) 
{
//	USHORT	Duration;

	NdisZeroMemory(pCntlBar, sizeof(FRAME_BAR));
	pCntlBar->FC.Type = BTYPE_CNTL;
	pCntlBar->FC.SubType = SUBTYPE_BLOCK_ACK_REQ;
   	pCntlBar->BarControl.MTID = 0;
	pCntlBar->BarControl.Compressed = 1;
	pCntlBar->BarControl.ACKPolicy = 0;


	pCntlBar->Duration = 16 + RTMPCalcDuration(pAd, RATE_1, sizeof(FRAME_BA));

	COPY_MAC_ADDR(pCntlBar->Addr1, pDA);
	COPY_MAC_ADDR(pCntlBar->Addr2, pSA);
}


/*
	==========================================================================
	Description:
		Insert Category and action code into the action frame.
		
	Parametrs:
		1. frame buffer pointer.
		2. frame length.
		3. category code of the frame.
		4. action code of the frame.
	
	Return	: None.
	==========================================================================
 */
VOID InsertActField(
	IN PRTMP_ADAPTER pAd,
	OUT PUCHAR pFrameBuf,
	OUT PULONG pFrameLen,
	IN UINT8 Category,
	IN UINT8 ActCode)
{
	ULONG TempLen;

	MakeOutgoingFrame(	pFrameBuf,		&TempLen,
						1,				&Category,
						1,				&ActCode,
						END_OF_ARGS);

	*pFrameLen = *pFrameLen + TempLen;

	return;
}
