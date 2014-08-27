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
    mlme.c
 
    Abstract:
    Major MLME state machiones here
 
    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    John Chang  08-04-2003    created for 11g soft-AP
 */

#include "rt_config.h"
#include <stdarg.h>




/*
    ==========================================================================
    Description:
        This routine is executed every second -
        1. Decide the overall channel quality
        2. Check if need to upgrade the TX rate to any client
        3. perform MAC table maintenance, including ageout no-traffic clients, 
           and release packet buffer in PSQ is fail to TX in time.
    ==========================================================================
 */
VOID APMlmePeriodicExec(
    PRTMP_ADAPTER pAd)
{
    // Reqeust by David 2005/05/12
    // It make sense to disable Adjust Tx Power on AP mode, since we can't take care all of the client's situation    
    // ToDo: need to verify compatibility issue with WiFi product.
    // 
//
// We return here in ATE mode, because the statistics 
// that ATE need are not collected via this routine.
//
#ifdef RALINK_ATE
	if (ATE_ON(pAd))
		return;
#endif // RALINK_ATE //

	// Disable Adjust Tx Power for WPA WiFi-test. 
	// Because high TX power results in the abnormal disconnection of Intel BG-STA.  
//#ifndef WIFI_TEST    
	if (pAd->CommonCfg.bWiFiTest == FALSE)	
		AsicAdjustTxPower(pAd);
//#endif // WIFI_TEST //

	/* BBP TUNING: dynamic tune BBP R66 to find a balance between sensibility
		and noise isolation */
//	AsicBbpTuning2(pAd);

    // walk through MAC table, see if switching TX rate is required
#if 0 // move to mlme.c::MlmePeriodicExec
    if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
        APMlmeDynamicTxRateSwitching(pAd);
#endif

    // MAC table maintenance
	if (pAd->Mlme.PeriodicRound % MLME_TASK_EXEC_MULTIPLE == 0)
	{
		// one second timer
	    MacTableMaintenance(pAd);
		RTMPMaintainPMKIDCache(pAd);

#ifdef WDS_SUPPORT
		WdsTableMaintenance(pAd);
#endif // WDS_SUPPORT //

	}
	
	APUpdateCapabilityAndErpIe(pAd);

#ifdef APCLI_SUPPORT
	if (pAd->Mlme.OneSecPeriodicRound % 2 == 0)
		ApCliIfMonitor(pAd);

	if (pAd->Mlme.OneSecPeriodicRound % 2 == 1)
		ApCliIfUp(pAd);
#endif // APCLI_SUPPORT //


	if ( (pAd->CommonCfg.Channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		)
	{
		ApRadarDetectPeriodic(pAd);
	}

#ifdef DFS_SUPPORT
	if ( (pAd->CommonCfg.Channel > 14)
		&& (pAd->CommonCfg.bIEEE80211H == 1)
		&& RadarChannelCheck(pAd, pAd->CommonCfg.Channel)
		&& OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)
		&& (pAd->CommonCfg.RadarDetect.RDMode == RD_NORMAL_MODE))
	{
		AdaptRadarDetection(pAd);
	}
#endif // DFS_SUPPORT //

}

VOID APMlmeSelectTxRateTable(
	IN PRTMP_ADAPTER		pAd,
	IN PMAC_TABLE_ENTRY		pEntry,
	IN PUCHAR				*ppTable,
	IN PUCHAR				pTableSize,
	IN PUCHAR				pInitTxRateIdx)
{
	MlmeSelectTxRateTable(pAd, pEntry, ppTable, pTableSize, pInitTxRateIdx);
}

VOID APMlmeSetTxRate(
	IN PRTMP_ADAPTER		pAd,
	IN PMAC_TABLE_ENTRY		pEntry,
	IN PRTMP_TX_RATE_SWITCH	pTxRate)
{

	if (pTxRate->CurrMCS < MCS_AUTO)
		pEntry->HTPhyMode.field.MCS = pTxRate->CurrMCS;

		pEntry->HTPhyMode.field.MODE = pTxRate->Mode;


	pAd->LastTxRate = (USHORT)(pEntry->HTPhyMode.word);

	
}

/*
    ==========================================================================
    Description:
        This routine walks through the MAC table, see if TX rate change is 
        required for each associated client. 
    Output:
        pEntry->CurrTxRate - 
    NOTE:
        call this routine every second
    ==========================================================================
 */
VOID APMlmeDynamicTxRateSwitching(
    IN PRTMP_ADAPTER pAd)
{
	ULONG					i;
	PUCHAR					pTable;
	UCHAR					TableSize = 0;
	UCHAR					UpRateIdx, DownRateIdx, CurrRateIdx;
	ULONG					AccuTxTotalCnt, TxTotalCnt;
	ULONG					TxErrorRatio = 0;
	MAC_TABLE_ENTRY		*pEntry;
	PRTMP_TX_RATE_SWITCH	pCurrTxRate, pNextTxRate = NULL;
	BOOLEAN					bTxRateChanged = TRUE, bUpgradeQuality = FALSE;
	UCHAR					InitTxRateIdx, TrainUp, TrainDown;
	TX_STA_CNT1_STRUC		StaTx1;
	TX_STA_CNT0_STRUC		TxStaCnt0;
	CHAR					Rssi, RssiOffset = 0;
	ULONG					TxRetransmit = 0, TxSuccess = 0, TxFailCount = 0;
	
#ifdef RALINK_ATE
   	if (ATE_ON(pAd))
   	{
		return;
   	}
#endif // RALINK_ATE //

    //
    // walk through MAC table, see if need to change AP's TX rate toward each entry
    //
    
	  

	for (i = 1; i < MAX_LEN_OF_MAC_TABLE; i++) 
	{
		pEntry = &pAd->MacTab.Content[i];

		// only associated STA counts
		if (((pEntry->ValidAsCLI == FALSE) || ((pEntry->ValidAsCLI == TRUE) && (pEntry->Sst != SST_ASSOC)))
#ifdef APCLI_SUPPORT
			&&(pEntry->ValidAsApCli == FALSE)
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
			&& ((pEntry->ValidAsWDS == FALSE) || !WDS_IF_UP_CHECK(pAd, pEntry->MatchWDSTabIdx))
#endif // WDS_SUPPORT //
			)
			continue;

		// check if this entry need to switch rate automatically
		if (RTMPCheckEntryEnableAutoRateSwitch(pAd, pEntry) == FALSE)
			continue;

		//NICUpdateFifoStaCounters(pAd);

		if (pAd->MacTab.Size == 1)
		{
#if 0	// test by gary
			TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
						pAd->RalinkCounters.OneSecTxRetryOkCount + 
						pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) * 100) / TxTotalCnt;
#else

			// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			pAd->bUpdateBcnCntDone = TRUE;
			TxRetransmit = StaTx1.field.TxRetransmit;
			TxSuccess = StaTx1.field.TxSuccess;
			TxFailCount = TxStaCnt0.field.TxFailCount;
			TxTotalCnt = TxRetransmit + TxSuccess + TxFailCount;

			pAd->RalinkCounters.OneSecBeaconSentCnt += TxStaCnt0.field.TxBeaconCount;
			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;
			pAd->WlanCounters.TransmittedFragmentCount.u.LowPart += StaTx1.field.TxSuccess;
			pAd->WlanCounters.RetryCount.u.LowPart += StaTx1.field.TxRetransmit;
			pAd->WlanCounters.FailedCount.u.LowPart += TxStaCnt0.field.TxFailCount;

			AccuTxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((TxRetransmit + TxFailCount) * 100) / TxTotalCnt;			
#endif
		}
		else
		{
			TxTotalCnt = pEntry->OneSecTxNoRetryOkCount + 
				 pEntry->OneSecTxRetryOkCount + 
				 pEntry->OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((pEntry->OneSecTxRetryOkCount + pEntry->OneSecTxFailCount) * 100) / TxTotalCnt;
		}

		CurrRateIdx = UpRateIdx = DownRateIdx = pEntry->CurrTxRateIndex;

		Rssi = RTMPMaxRssi(pAd, (CHAR)pEntry->RssiSample.AvgRssi0, (CHAR)pEntry->RssiSample.AvgRssi1, (CHAR)pEntry->RssiSample.AvgRssi2);

		APMlmeSelectTxRateTable(pAd, pEntry, &pTable, &TableSize, &InitTxRateIdx);

		// decide the next upgrade rate and downgrade rate, if any
		if ((CurrRateIdx > 0) && (CurrRateIdx < (TableSize - 1)))
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx -1;
		}
		else if (CurrRateIdx == 0)
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx;
		}
		else if (CurrRateIdx == (TableSize - 1))
		{
			UpRateIdx = CurrRateIdx;
			DownRateIdx = CurrRateIdx - 1;
		}

		pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(CurrRateIdx+1)*5];
		{
			TrainUp		= pCurrTxRate->TrainUp;
			TrainDown	= pCurrTxRate->TrainDown;
		}

		pEntry->LastTimeTxRateChangeAction = pEntry->LastSecTxRateChangeAction;
/*
		if (pAd->CommonCfg.bAutoTxRateSwitch == FALSE)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: Fixed - CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, BW=%d, PER=%ld%% \n\n",
				pEntry->CurrTxRateIndex, pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.STBC,
				pEntry->HTPhyMode.field.ShortGI, pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW, TxErrorRatio));
			return;
		}
		else
 */		
		//if (! OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
		//	continue;

        if (TxTotalCnt <= 15)
        {
   			CHAR	idx = 0;
			UCHAR	TxRateIdx;
			UCHAR	MCS0 = 0, MCS1 = 0, MCS2 = 0, MCS3 = 0, MCS4 = 0,  MCS5 =0, MCS6 = 0, MCS7 = 0;

			// check the existence and index of each needed MCS
			while (idx < pTable[0])
			{
				pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(idx+1)*5];

				if (pCurrTxRate->CurrMCS == MCS_0)
				{
					MCS0 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_1)
				{
					MCS1 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_2)
				{
					MCS2 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_3)
				{
					MCS3 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_4)
				{
					MCS4 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_5)
				{
					MCS5 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_6)
				{
					MCS6 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_7)
				{
					MCS7 = idx;
				}

				idx ++;
			}
#if 1 // test by gary //
			{
				RssiOffset = 5;
			}
#endif // test by gary //
			{// Legacy mode
				if (MCS7 && (Rssi > -70))
				TxRateIdx = MCS7;
				else if (MCS6 && (Rssi > -74))
					TxRateIdx = MCS6;
				else if (MCS5 && (Rssi > -78))
					TxRateIdx = MCS5;
				else if (MCS4 && (Rssi > -82))
				TxRateIdx = MCS4;
				else if (MCS4 == 0)							// for B-only mode
					TxRateIdx = MCS3;
				else if (MCS3 && (Rssi > -85))
					TxRateIdx = MCS3;
				else if (MCS2 && (Rssi > -87))
					TxRateIdx = MCS2;
				else if (MCS1 && (Rssi > -90))
					TxRateIdx = MCS1;
				else
				TxRateIdx = MCS0;
			}

			if (TxRateIdx != pEntry->CurrTxRateIndex)
			{
				pEntry->CurrTxRateIndex = TxRateIdx;
				pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
				APMlmeSetTxRate(pAd, pEntry, pNextTxRate);
			}

			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);
			pEntry->fLastSecAccordingRSSI = TRUE;


			// reset all OneSecTx counters
			RESET_ONE_SEC_TX_CNT(pEntry);

			continue;
        }

		if (pEntry->fLastSecAccordingRSSI == TRUE)
		{
			pEntry->fLastSecAccordingRSSI = FALSE;
			pEntry->LastSecTxRateChangeAction = 0;
			// reset all OneSecTx counters
			RESET_ONE_SEC_TX_CNT(pEntry);

			continue;
		}

		do
		{
			BOOLEAN	bTrainUpDown = FALSE;
			
			pEntry->CurrTxRateStableTime ++;

			// downgrade TX quality if PER >= Rate-Down threshold
			if (TxErrorRatio >= TrainDown)
			{
				bTrainUpDown = TRUE;
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}
			// upgrade TX quality if PER <= Rate-Up threshold
			else if (TxErrorRatio <= TrainUp)
			{
				bTrainUpDown = TRUE;
				bUpgradeQuality = TRUE;
				if (pEntry->TxQuality[CurrRateIdx])
					pEntry->TxQuality[CurrRateIdx] --;  // quality very good in CurrRate

				if (pEntry->TxRateUpPenalty)
					pEntry->TxRateUpPenalty --;
				else if (pEntry->TxQuality[UpRateIdx])
					pEntry->TxQuality[UpRateIdx] --;    // may improve next UP rate's quality
			}

			pEntry->PER[CurrRateIdx] = (UCHAR)TxErrorRatio;

			if (bTrainUpDown == TRUE)
			{
				// perform DRS - consider TxRate Down first, then rate up.
				if ((CurrRateIdx != DownRateIdx) && (pEntry->TxQuality[CurrRateIdx] >= DRS_TX_QUALITY_WORST_BOUND))
				{
					pEntry->CurrTxRateIndex = DownRateIdx;
				}
				else if ((CurrRateIdx != UpRateIdx) && (pEntry->TxQuality[UpRateIdx] <= 0))
				{
					pEntry->CurrTxRateIndex = UpRateIdx;
				}
			}
		}while (FALSE);

		// if rate-up happen, clear all bad history of all TX rates
		if (pEntry->CurrTxRateIndex > CurrRateIdx)
		{
			pEntry->CurrTxRateStableTime = 0;
			pEntry->TxRateUpPenalty = 0;
			pEntry->LastSecTxRateChangeAction = 1; // rate UP
			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);

			//
			// For TxRate fast train up
			// 
			if (!pAd->ApCfg.ApQuickResponeForRateUpTimerRunning)
			{				
				RTMPSetTimer(&pAd->ApCfg.ApQuickResponeForRateUpTimer, 100);

				pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = TRUE;
			}
		}
		// if rate-down happen, only clear DownRate's bad history
		else if (pEntry->CurrTxRateIndex < CurrRateIdx)
		{
			pEntry->CurrTxRateStableTime = 0;
			pEntry->TxRateUpPenalty = 0;           // no penalty
			pEntry->LastSecTxRateChangeAction = 2; // rate DOWN
			pEntry->TxQuality[pEntry->CurrTxRateIndex] = 0;
			pEntry->PER[pEntry->CurrTxRateIndex] = 0;

			//
			// For TxRate fast train down
			// 
			if (!pAd->ApCfg.ApQuickResponeForRateUpTimerRunning)
			{
				RTMPSetTimer(&pAd->ApCfg.ApQuickResponeForRateUpTimer, 100);

				pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = TRUE;
			}
		}
		else
		{
			pEntry->LastSecTxRateChangeAction = 0; // rate no change
			bTxRateChanged = FALSE;
		}
			
		if (pAd->MacTab.Size == 1)
		{
			//test by gary 
       		//pEntry->LastTxOkCount = pAd->RalinkCounters.OneSecTxNoRetryOkCount;
			pEntry->LastTxOkCount = TxSuccess;
		}
		else
		{
			pEntry->LastTxOkCount = pEntry->OneSecTxNoRetryOkCount;
		}

		// reset all OneSecTx counters
		RESET_ONE_SEC_TX_CNT(pEntry);

		pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
		if (bTxRateChanged && pNextTxRate)
		{
			APMlmeSetTxRate(pAd, pEntry, pNextTxRate);
		}
    }
}

/*
    ========================================================================
    Routine Description:
        AP side, Auto TxRate faster train up timer call back function.
        
    Arguments:
        SystemSpecific1         - Not used.
        FunctionContext         - Pointer to our Adapter context.
        SystemSpecific2         - Not used.
        SystemSpecific3         - Not used.
        
    Return Value:
        None
        
    ========================================================================
*/
VOID APQuickResponeForRateUpExec(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3) 
{
	PRTMP_ADAPTER			pAd = (PRTMP_ADAPTER)FunctionContext;
	ULONG					i;
	PUCHAR					pTable;
	UCHAR					TableSize = 0;
	UCHAR					UpRateIdx, DownRateIdx, CurrRateIdx;
	ULONG					AccuTxTotalCnt, TxTotalCnt, TxCnt;
	ULONG					TxErrorRatio = 0;
	MAC_TABLE_ENTRY			*pEntry;
	PRTMP_TX_RATE_SWITCH	pCurrTxRate, pNextTxRate = NULL;
	BOOLEAN					bTxRateChanged = TRUE;
	UCHAR					InitTxRateIdx, TrainUp, TrainDown;
	TX_STA_CNT1_STRUC		StaTx1;
	TX_STA_CNT0_STRUC		TxStaCnt0;
	CHAR					Rssi, ratio;
	ULONG					TxRetransmit = 0, TxSuccess = 0, TxFailCount = 0;

	pAd->ApCfg.ApQuickResponeForRateUpTimerRunning = FALSE;
	
    //
    // walk through MAC table, see if need to change AP's TX rate toward each entry
    //
   	for (i = 1; i < MAX_LEN_OF_MAC_TABLE; i++) 
	{
        pEntry = &pAd->MacTab.Content[i];

        // only associated STA counts
//        if ((pEntry->ValidAsCLI == FALSE) || (pEntry->Sst != SST_ASSOC))
        if (((pEntry->ValidAsCLI == FALSE) || ((pEntry->ValidAsCLI == TRUE) && (pEntry->Sst != SST_ASSOC)))
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
			&& (pEntry->ValidAsApCli == FALSE)
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
			&& (pEntry->ValidAsWDS == FALSE || !WDS_IF_UP_CHECK(pAd, pEntry->MatchWDSTabIdx))
#endif // WDS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
			)
            continue;

    	Rssi = RTMPMaxRssi(pAd, (CHAR)pEntry->RssiSample.AvgRssi0, (CHAR)pEntry->RssiSample.AvgRssi1, (CHAR)pEntry->RssiSample.AvgRssi2);

		CurrRateIdx = UpRateIdx = DownRateIdx = pEntry->CurrTxRateIndex;


		if (pAd->MacTab.Size == 1)
		{
#if 0	// test by gary
            TX_STA_CNT1_STRUC		StaTx1;
			TX_STA_CNT0_STRUC		TxStaCnt0;

       		// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;

			TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
				pAd->RalinkCounters.OneSecTxRetryOkCount + 
				pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) * 100) / TxTotalCnt;
#else
			// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			TxRetransmit = StaTx1.field.TxRetransmit;
			TxSuccess = StaTx1.field.TxSuccess;
			TxFailCount = TxStaCnt0.field.TxFailCount;
			TxTotalCnt = TxRetransmit + TxSuccess + TxFailCount;

			pAd->RalinkCounters.OneSecBeaconSentCnt += TxStaCnt0.field.TxBeaconCount;
			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;
			pAd->WlanCounters.TransmittedFragmentCount.u.LowPart += StaTx1.field.TxSuccess;
			pAd->WlanCounters.RetryCount.u.LowPart += StaTx1.field.TxRetransmit;
			pAd->WlanCounters.FailedCount.u.LowPart += TxStaCnt0.field.TxFailCount;

			AccuTxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((TxRetransmit + TxFailCount) * 100) / TxTotalCnt;

			if (pAd->Antenna.field.TxPath > 1)
				Rssi = (pEntry->RssiSample.AvgRssi0 + pEntry->RssiSample.AvgRssi1) >> 1;
			else
				Rssi = pEntry->RssiSample.AvgRssi0;

			TxCnt = AccuTxTotalCnt;
#endif
		}
		else
		{
		TxTotalCnt = pEntry->OneSecTxNoRetryOkCount + 
			 pEntry->OneSecTxRetryOkCount + 
			 pEntry->OneSecTxFailCount;

		if (TxTotalCnt)
			TxErrorRatio = ((pEntry->OneSecTxRetryOkCount + pEntry->OneSecTxFailCount) * 100) / TxTotalCnt;
	
			TxCnt = TxTotalCnt;	
		}

		// decide the rate table for tuning
		APMlmeSelectTxRateTable(pAd, pEntry, &pTable, &TableSize, &InitTxRateIdx);

		// decide the next upgrade rate and downgrade rate, if any
		if ((CurrRateIdx > 0) && (CurrRateIdx < (TableSize - 1)))
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx -1;
		}
		else if (CurrRateIdx == 0)
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx;
		}
		else if (CurrRateIdx == (TableSize - 1))
		{
			UpRateIdx = CurrRateIdx;
			DownRateIdx = CurrRateIdx - 1;
		}

		pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(CurrRateIdx+1)*5];

		{
			TrainUp		= pCurrTxRate->TrainUp;
			TrainDown	= pCurrTxRate->TrainDown;
		}


//		if (! OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
//			continue;

        if (/*TxTotalCnt*/ TxCnt <= 15)
        {
			NdisZeroMemory(pAd->DrsCounters.TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pAd->DrsCounters.PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);

			// perform DRS - consider TxRate Down first, then rate up.
			if ((pEntry->LastSecTxRateChangeAction == 1) && (CurrRateIdx != DownRateIdx))
			{
				pEntry->CurrTxRateIndex = DownRateIdx;
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}
			else if ((pEntry->LastSecTxRateChangeAction == 2) && (CurrRateIdx != UpRateIdx))
			{
				pEntry->CurrTxRateIndex = UpRateIdx;
			}
            
			continue;
        }

		do
		{
			ULONG		OneSecTxNoRetryOKRationCount;

			// test by gary
			//if (pEntry->LastSecTxRateChangeAction == 0)
			if (pEntry->LastTimeTxRateChangeAction == 0)
				ratio = 5;
			else
				ratio = 4;

			// downgrade TX quality if PER >= Rate-Down threshold
			if (TxErrorRatio >= TrainDown)
			{
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}

			pEntry->PER[CurrRateIdx] = (UCHAR)TxErrorRatio;

			if (pAd->MacTab.Size == 1)
			{
   				//OneSecTxNoRetryOKRationCount = pAd->RalinkCounters.OneSecTxNoRetryOkCount * ratio + (pAd->RalinkCounters.OneSecTxNoRetryOkCount >> 1);
				// test by gary
				OneSecTxNoRetryOKRationCount = (TxSuccess * ratio);
			}
			else
			{
				OneSecTxNoRetryOKRationCount = pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1);
			}

			// perform DRS - consider TxRate Down first, then rate up.
			if ((pEntry->LastSecTxRateChangeAction == 1) && (CurrRateIdx != DownRateIdx))
			{
				//if ((pEntry->LastTxOkCount + 2) >= (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1)))
				if ((pEntry->LastTxOkCount + 2) >= OneSecTxNoRetryOKRationCount)
				{
					pEntry->CurrTxRateIndex = DownRateIdx;
					pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
					
				}
				else
				{
					//DBGPRINT_RAW(RT_DEBUG_TRACE,("QuickDRS: (Up) keep rate-up (L:%ld, C:%d)\n", pEntry->LastTxOkCount, (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1))));
					DBGPRINT_RAW(RT_DEBUG_TRACE,("QuickDRS: (Up) keep rate-up (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
			}
			else if ((pEntry->LastSecTxRateChangeAction == 2) && (CurrRateIdx != UpRateIdx))
			{
				//if ((TxErrorRatio >= 50) || (TxErrorRatio >= TrainDown))
				if ((TxErrorRatio >= 50) && (TxErrorRatio >= TrainDown))
				{
					
				}
				//else if ((pEntry->LastTxOkCount + 2) >= (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1)))
				else if ((pEntry->LastTxOkCount + 2) >= OneSecTxNoRetryOKRationCount)
				{
					pEntry->CurrTxRateIndex = UpRateIdx;
					
				}
				
			}
		}while (FALSE);

		// if rate-up happen, clear all bad history of all TX rates
		if (pEntry->CurrTxRateIndex > CurrRateIdx)
		{

			pEntry->TxRateUpPenalty = 0;
			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);
		}
		// if rate-down happen, only clear DownRate's bad history
		else if (pEntry->CurrTxRateIndex < CurrRateIdx)
		{
			
			pEntry->TxRateUpPenalty = 0;           // no penalty
			pEntry->TxQuality[pEntry->CurrTxRateIndex] = 0;
			pEntry->PER[pEntry->CurrTxRateIndex] = 0;
		}
		else
		{
			bTxRateChanged = FALSE;
		}

		pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
		if (bTxRateChanged && pNextTxRate)
		{
			APMlmeSetTxRate(pAd, pEntry, pNextTxRate);
		}
    }
}

/*! \brief   To substitute the message type if the message is coming from external
 *  \param  *Fr            The frame received
 *  \param  *Machine       The state machine
 *  \param  *MsgType       the message type for the state machine
 *  \return TRUE if the substitution is successful, FALSE otherwise
 *  \pre
 *  \post
 */
BOOLEAN APMsgTypeSubst(
    IN PRTMP_ADAPTER pAd,
    IN PFRAME_802_11 pFrame, 
    OUT INT *Machine, 
    OUT INT *MsgType) 
{
    USHORT Seq;
    UCHAR  EAPType;
    BOOLEAN     Return = FALSE;
#ifdef WSC_AP_SUPPORT
	UCHAR EAPCode;
    PMAC_TABLE_ENTRY pEntry;
#endif // WSC_AP_SUPPORT //

//TODO:
// only PROBE_REQ can be broadcast, all others must be unicast-to-me && is_mybssid; otherwise, 
// ignore this frame

    // wpa EAPOL PACKET
    if (pFrame->Hdr.FC.Type == BTYPE_DATA) 
    {    
#ifdef WSC_AP_SUPPORT    
        //WSC EAPOL PACKET        
        pEntry = MacTableLookup(pAd, pFrame->Hdr.Addr2);
        if ((pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.EntryApIdx != WSC_INIT_ENTRY_APIDX) &&
            pEntry && 
            (pEntry->ValidAsCLI) && 
            (pAd->ApCfg.MBSSID[pEntry->apidx].WscControl.WscConfMode != WSC_DISABLE))
        {
            *Machine = WSC_STATE_MACHINE;
            EAPType = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 1);
            EAPCode = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 4);
            Return = WscMsgTypeSubst(EAPType, EAPCode, MsgType);
        }
        if (!Return)
        {
#endif // WSC_AP_SUPPORT //
        *Machine = AP_WPA_STATE_MACHINE;
        EAPType = *((UCHAR*)pFrame + LENGTH_802_11 + LENGTH_802_1_H + 1);
            Return = APWpaMsgTypeSubst(EAPType, (INT *) MsgType);
#ifdef WSC_AP_SUPPORT            
        }
#endif // WSC_AP_SUPPORT //
        return Return;
    }
    
    if (pFrame->Hdr.FC.Type != BTYPE_MGMT)
        return FALSE;
    
    switch (pFrame->Hdr.FC.SubType) 
    {
        case SUBTYPE_ASSOC_REQ:
            *Machine = AP_ASSOC_STATE_MACHINE;
            *MsgType = APMT2_PEER_ASSOC_REQ;
            
            break;
//      case SUBTYPE_ASSOC_RSP:
//          *Machine = AP_ASSOC_STATE_MACHINE;
//          *MsgType = APMT2_PEER_ASSOC_RSP;
//          break;
        case SUBTYPE_REASSOC_REQ:
            *Machine = AP_ASSOC_STATE_MACHINE;
            *MsgType = APMT2_PEER_REASSOC_REQ;
            break;
//      case SUBTYPE_REASSOC_RSP:
//          *Machine = AP_ASSOC_STATE_MACHINE;
//          *MsgType = APMT2_PEER_REASSOC_RSP;
//          break;
        case SUBTYPE_PROBE_REQ:
            *Machine = AP_SYNC_STATE_MACHINE;              
            *MsgType = APMT2_PEER_PROBE_REQ;
            break;
// test for 40Mhz intolerant
#if 0
		case SUBTYPE_PROBE_RSP:
          *Machine = AP_SYNC_STATE_MACHINE;
          *MsgType = APMT2_PEER_PROBE_RSP;
          break;
#endif
        case SUBTYPE_BEACON:
            *Machine = AP_SYNC_STATE_MACHINE;
            *MsgType = APMT2_PEER_BEACON;
            break;
//      case SUBTYPE_ATIM:
//          *Machine = AP_SYNC_STATE_MACHINE;
//          *MsgType = APMT2_PEER_ATIM;
//          break;
        case SUBTYPE_DISASSOC:
            *Machine = AP_ASSOC_STATE_MACHINE;
            *MsgType = APMT2_PEER_DISASSOC_REQ;
            break;
        case SUBTYPE_AUTH:
            // get the sequence number from payload 24 Mac Header + 2 bytes algorithm
            NdisMoveMemory(&Seq, &pFrame->Octet[2], sizeof(USHORT));
            
            if (Seq == 1 || Seq == 3) 
            {
                *Machine = AP_AUTH_RSP_STATE_MACHINE;
                *MsgType = APMT2_PEER_AUTH_ODD;
            } 
//          else if (Seq == 2 || Seq == 4) 
//          {
//              *Machine = AP_AUTH_STATE_MACHINE;
//              *MsgType = APMT2_PEER_AUTH_EVEN;
//          } 
            else 
            {
                DBGPRINT(RT_DEBUG_TRACE,("wrong AUTH seq=%d Octet=%02x %02x %02x %02x %02x %02x %02x %02x\n", Seq,
                    pFrame->Octet[0], pFrame->Octet[1], pFrame->Octet[2], pFrame->Octet[3], 
                    pFrame->Octet[4], pFrame->Octet[5], pFrame->Octet[6], pFrame->Octet[7]));
                return FALSE;
            }
            break;
        case SUBTYPE_DEAUTH:
            *Machine = AP_AUTH_RSP_STATE_MACHINE;
            *MsgType = APMT2_PEER_DEAUTH;
            break;
	case SUBTYPE_ACTION:
		*Machine = ACTION_STATE_MACHINE;
		//  Sometimes Sta will return with category bytes with MSB = 1, if they receive catogory out of their support
		if ((pFrame->Octet[0]&0x7F) > MAX_PEER_CATE_MSG) 
		{
			*MsgType = MT2_ACT_INVALID;
		} 
		else
		{
			*MsgType = (pFrame->Octet[0]&0x7F);
		} 
		break;
        default:
            return FALSE;
            break;
    }

    return TRUE;
}

#if 0 // move to mlme.c 
/*
    ========================================================================
    Routine Description:
        Set/reset MAC registers according to bPiggyBack parameter
        
    Arguments:
        pAd         - Adapter pointer
        bPiggyBack  - Enable / Disable Piggy-Back
        
    Return Value:
        None
        
    ========================================================================
*/
VOID RTMPSetPiggyBack(
    IN PRTMP_ADAPTER    pAd,
    IN BOOLEAN          bPiggyBack)
{
	TX_LINK_CFG_STRUC  TxLinkCfg;
    
	RTMP_IO_READ32(pAd, TX_LINK_CFG, &TxLinkCfg.word);
//	printk("TX_LINK_CFG = %08x\n", TxLinkCfg.word);
	TxLinkCfg.field.TxCFAckEn = bPiggyBack;
	RTMP_IO_WRITE32(pAd, TX_LINK_CFG, TxLinkCfg.word);
}
#endif


/*
    ========================================================================
    Routine Description:
        Periodic evaluate antenna link status
        
    Arguments:
        pAd         - Adapter pointer
        
    Return Value:
        None
        
    ========================================================================
*/
VOID APAsicEvaluateRxAnt(
	IN PRTMP_ADAPTER	pAd)
{
	UCHAR	BBPR3 = 0;
	ULONG	TxTotalCnt;

#ifdef RALINK_ATE
	if (ATE_ON(pAd))
		return;
#endif // RALINK_ATE //

	

	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BBPR3);
	BBPR3 &= (~0x18);
	if(pAd->Antenna.field.RxPath == 3)
	{
		BBPR3 |= (0x10);
	}
	else if(pAd->Antenna.field.RxPath == 2)
	{
		BBPR3 |= (0x8);
	}
	else if(pAd->Antenna.field.RxPath == 1)
	{
		BBPR3 |= (0x0);
	}
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BBPR3);

	TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					pAd->RalinkCounters.OneSecTxRetryOkCount + 
					pAd->RalinkCounters.OneSecTxFailCount;

	if (TxTotalCnt > 50)
	{
		RTMPSetTimer(&pAd->Mlme.RxAntEvalTimer, 20);
		pAd->Mlme.bLowThroughput = FALSE;
	}
	else
	{
		RTMPSetTimer(&pAd->Mlme.RxAntEvalTimer, 300);
		pAd->Mlme.bLowThroughput = TRUE;
	}
}

/*
    ========================================================================
    Routine Description:
        After evaluation, check antenna link status
        
    Arguments:
        pAd         - Adapter pointer
        
    Return Value:
        None
        
    ========================================================================
*/
VOID APAsicRxAntEvalTimeout(
	PRTMP_ADAPTER	pAd) 
{
	UCHAR			BBPR3 = 0;
	CHAR			larger = -127, rssi0, rssi1, rssi2;

#ifdef RALINK_ATE
	if (ATE_ON(pAd))
		return;
#endif // RALINK_ATE //

	// if the traffic is low, use average rssi as the criteria
	if (pAd->Mlme.bLowThroughput == TRUE)
	{
		rssi0 = pAd->ApCfg.RssiSample.LastRssi0;
		rssi1 = pAd->ApCfg.RssiSample.LastRssi1;
		rssi2 = pAd->ApCfg.RssiSample.LastRssi2;
	}
	else
	{
		rssi0 = pAd->ApCfg.RssiSample.AvgRssi0;
		rssi1 = pAd->ApCfg.RssiSample.AvgRssi1;
		rssi2 = pAd->ApCfg.RssiSample.AvgRssi2;
	}

	if(pAd->Antenna.field.RxPath == 3)
	{
		larger = max(rssi0, rssi1);

		if (larger > (rssi2 + 20))
			pAd->Mlme.RealRxPath = 2;
		else
			pAd->Mlme.RealRxPath = 3;
	}
	// Disable the below to fix 1T/2R issue. It's suggested by Rory at 2007/7/11.
#if 0	
	else if(pAd->Antenna.field.RxPath == 2)
	{
		if (rssi0 > (rssi1 + 20))
			pAd->Mlme.RealRxPath = 1;
		else
			pAd->Mlme.RealRxPath = 2;
	}
#endif

	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BBPR3);
	BBPR3 &= (~0x18);
	if(pAd->Mlme.RealRxPath == 3)
	{
		BBPR3 |= (0x10);
	}
	else if(pAd->Mlme.RealRxPath == 2)
	{
		BBPR3 |= (0x8);
	}
	else if(pAd->Mlme.RealRxPath == 1)
	{
		BBPR3 |= (0x0);
	}
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BBPR3);
	
}

/*
	==========================================================================
	Description: BBP TUNING
		dynamic tune BBP R66 to find a balance between sensibility and 
		noise isolation

	IRQL = DISPATCH_LEVEL

	==========================================================================
 */
VOID AsicBbpTuning1(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR	R66, R66UpperBound = 0x30, R66LowerBound = 0x30; /* default bound */
	CHAR	Rssi;

/* Disable BBP Tuning when ATE is running. */
#ifdef RALINK_ATE
	if (ATE_ON(pAd))
		return;
#endif // RALINK_ATE //

	/* This algorithm is trying to compensate the effect of board (system)
		noise issue. */


	/* 2860C did not support Fase CCA, therefore can't tune */
	if (pAd->BbpTuning.bEnable == FALSE)
		return;
	/* End of if */

	/* get current R66 value, BbpWriteLatch[] be will updated in
		RTMP_BBP_IO_WRITE8_BY_REG_ID() */
	R66 = pAd->BbpWriteLatch[66];


	/* find the maximum RSSI from the three receive antennas */
	Rssi = RTMPMaxRssi(pAd, (CHAR)pAd->ApCfg.RssiSample.AvgRssi0,
						(CHAR)pAd->ApCfg.RssiSample.AvgRssi1,
						(CHAR)pAd->ApCfg.RssiSample.AvgRssi2);

	/* get R66LowerBound & R66UpperBound based on RSSI as below figure */
	/* (20 & 40MHz)
		R66
		0x48	|------------------------
				|						|
				|						|
				|						|
		0x38	|------------------------------->
						-70			-80		-90 RSSI
	 */
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		/* we are in connection state */
#if 0
		if (pAd->CommonCfg.BBPCurrentBW == BW_20)
		{
			/* BW=20MHz */
			if (Rssi <= RSSI_FOR_MID_SENSIBILITY) /* <= -90 */
			{
				/* weak RSSI, use largest gain */

				/* GET_LNA_GAIN is BLNAGain (bg channel <=14),
					ALNAGain0 (<=64), ALNAGain1 (<=128), or ALNAGain2 (>128)
					got from EEPROM tunning in Mass Production for different board */
				if (R66 != (0x26 + GET_LNA_GAIN(pAd)))
				{
					R66 = (0x26 + GET_LNA_GAIN(pAd));
					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
				} /* End of if */

				DBGPRINT(RT_DEBUG_TRACE, ("weak RSSI=%d, CCA=%ld, BW=%d, fixed R66 at 0x%x\n", 
						 Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66));
				return;
			}
			else if (Rssi >= RSSI_FOR_MID_LOW_SENSIBILITY) /* >= -80 */
			{
				/* adjustable steps = 0x18 / 4 = 6, one step is 0x4 */
				R66UpperBound = (0x26 + GET_LNA_GAIN(pAd) + 0x18);
				R66LowerBound = (0x26 + GET_LNA_GAIN(pAd));
			}

			else
			{
				/* adjustable steps = 0x8 / 4 = 2, one step is 0x4 */
				R66UpperBound = (0x26 + GET_LNA_GAIN(pAd) + 0x8);
				R66LowerBound = (0x26 + GET_LNA_GAIN(pAd));
			} /* End of if */
		}
		else
#endif
		{
			/* BW=40MHz */
			if (Rssi <= RSSI_FOR_MID_LOW_SENSIBILITY) /* <= -80 */
			{
				/* weak RSSI, use largest gain */

				if (R66 != (0x2E + GET_LNA_GAIN(pAd)))
				{
					R66 = (0x2E + GET_LNA_GAIN(pAd) + 0x08);
					RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
				} /* End of if */

				DBGPRINT(RT_DEBUG_TRACE, ("weak RSSI=%d, CCA=%d, BW=%d, fixed R66 at 0x%x\n", 
						 Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66));
				return;
			}
			else
			{
				/* adjustable steps = 0x10 / 4 = 4, one step is 0x4 */
				R66UpperBound = (0x2E + GET_LNA_GAIN(pAd) + 0x18);
				R66LowerBound = (0x2E + GET_LNA_GAIN(pAd) + 0x08);
			} /* End of if */
		} /* End of if */
	} /* End of if */

	if (pAd->MACVersion == 0x28600100)
	{
		/* no tuning for RT2860C */
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66UpperBound);

		DBGPRINT(RT_DEBUG_TRACE, ("RSSI=%d, CCA=%d, BW=%d, R66= 0x%x, R66LowerBound=0x%x, R66UpperBound=0x%x (Ver.C)\n",
				 Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66, R66LowerBound, R66UpperBound));
	}
	else
	{
		/* do tuning for RT2860D */
		/* FalseCCA UpperBound threshold=512, FalseCCA LowerBound threshold=100 */

		/* R66 is currenly in dynamic tuning range,
			keep dynamic tuning based on False CCA counter */
		if ((pAd->RalinkCounters.OneSecFalseCCACnt > pAd->BbpTuning.FalseCcaUpperThreshold) &&
			(R66 < R66UpperBound))
		{
			/*  1. False CCA Count is too much (>512);
				2. R66 is not largest (larger R66 lower signal strength) */
			R66 += pAd->BbpTuning.R66Delta; /* 4 */

			if (R66 >= R66UpperBound)
				R66 = R66UpperBound;
			/* End of if */
			
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			DBGPRINT(RT_DEBUG_TRACE, ("RSSI=%d, CCA=%d, BW=%d, ++R66= 0x%x\n",
				Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66));
		}
		else if ((pAd->RalinkCounters.OneSecFalseCCACnt < pAd->BbpTuning.FalseCcaLowerThreshold) &&
				 (R66 > R66LowerBound))
		{
			/*  1. False CCA Count is lower (<100);
				2. R66 is not lowest */
			R66 -= pAd->BbpTuning.R66Delta;

			if (R66 <= R66LowerBound)
				R66 = R66LowerBound;
			/* End of if */
			
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			DBGPRINT(RT_DEBUG_TRACE, ("RSSI=%d, CCA=%d, BW=%d, --R66= 0x%x\n",
				Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66));
		}
		else
		{
			/* R66 is largest or lowest or normal False CCA Count */
			if (R66 >= R66UpperBound)
			{
				R66 = R66UpperBound;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			} /* End of if */
			
			if (R66 <= R66LowerBound)
			{
				R66 = R66LowerBound;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			} /* End of if */

			if (pAd->Mlme.OneSecPeriodicRound % 4 == 0)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("RSSI=%d, CCA=%d, BW=%d, R66= 0x%x, R66LowerBound=0x%x, R66UpperBound=0x%x\n",
					Rssi, pAd->RalinkCounters.OneSecFalseCCACnt, pAd->CommonCfg.BBPCurrentBW, R66, R66LowerBound, R66UpperBound));
			} /* End of if */
		} /* End of if */
	} /* End of if */
} /* End of AsicBbpTuning1 */


VOID AsicBbpTuning2(
	IN PRTMP_ADAPTER pAd)
{
	UCHAR	R66;
	CHAR	Rssi;

/* Disable BBP Tuning when ATE is running. */
#ifdef RALINK_ATE
	if (ATE_ON(pAd))
		return;
#endif // RALINK_ATE //

	/* This algorithm is trying to compensate the effect of board (system)
		noise issue. */


	/* 2860C did not support Fase CCA, therefore can't tune */
	if (pAd->BbpTuning.bEnable == FALSE)
		return;
	/* End of if */

	/* get current R66 value, BbpWriteLatch[] be will updated in
		RTMP_BBP_IO_WRITE8_BY_REG_ID() */
	R66 = pAd->BbpWriteLatch[66];


	/* find the maximum RSSI from the three receive antennas */
	Rssi = RTMPMaxRssi(pAd, (CHAR)pAd->ApCfg.RssiSample.AvgRssi0,
						(CHAR)pAd->ApCfg.RssiSample.AvgRssi1,
						(CHAR)pAd->ApCfg.RssiSample.AvgRssi2);


	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		if (Rssi > RSSI_FOR_MID_LOW_SENSIBILITY) /* -80 */
		{
            if (R66 != (0x2E + GET_LNA_GAIN(pAd) + 0x18))
            {
				R66 = (0x2E + GET_LNA_GAIN(pAd)) + 0x18;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
            } /* End of if */

            DBGPRINT(RT_DEBUG_TRACE,
					("strong RSSI=%d, GET_LNA_GAIN=0x%x, CCA=%d, BW=%d, fixed R66 at 0x%x\n",
					Rssi, GET_LNA_GAIN(pAd), pAd->RalinkCounters.OneSecFalseCCACnt,
					pAd->CommonCfg.BBPCurrentBW, R66));
            return;
		}
		else
		{
			/* weak RSSI, use largest gain */
			if (R66 != (0x2E + GET_LNA_GAIN(pAd) + 0x08))
			{
				R66 = (0x2E + GET_LNA_GAIN(pAd)) + 0x08;
				RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, R66);
			} /* End of if */

			DBGPRINT(RT_DEBUG_TRACE,
					("weak RSSI=%d, GET_LNA_GAIN=0x%x, CCA=%d, BW=%d, fixed R66 at 0x%x\n", 
					Rssi, GET_LNA_GAIN(pAd), pAd->RalinkCounters.OneSecFalseCCACnt,
					pAd->CommonCfg.BBPCurrentBW, R66));
			return;
		} /* End of if */
	} /* End of if */
} /* End of AsicBbpTuning2 */

/* End of ap_mlme.c */
