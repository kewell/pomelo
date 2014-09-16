/*********************************************************************************
 *      Copyright:  (C) 2014 KEWELL
 *
 *       Filename:  how2buy.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(08/29/2014~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "08/29/2014 02:04:57 PM"
 *                  2,
 ********************************************************************************/
//#define __DEBUG__
#include <stdlib.h>                                                                                                         
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

#define ALL_DATA_LEN        7
#define OUT_PUT "list3"
#define EACH_MAX_LEN        64
#define ASCII_0_VAL         0x30
#define U8 unsigned char
#define U16 unsigned int

#ifdef __DEBUG__
#define logs(x...) printf(x)
#else
#define logs(x...)
#endif

U16 g_u16Days, g_tradeCnt = 0, g_1stFlag = 1;
float g_buyVal, g_sellVal, g_lastVal, g_fAllEarn = 0;
float g_fBuyRate = 3.0, g_fSellRate = 3.0, g_winAlarm = 7.0, g_lossAlarm = 7.0;
float g_fAvg5, g_fAvg10, g_fAvg20, g_fAvg30, g_fAvg60;
float g_afAllOff[365];
U8 canBuy = 1, canSell = 0, sellEnd = 0;

float calc_avgX_price(int avgX, int index);

void analysia_each_stk (char *pcData)
{
    //logs("%s", pcData);
    char *pcNext, *aPcVal[ALL_DATA_LEN], *date;
    float val[ALL_DATA_LEN], highRate, highVal, lowRate, lowVal, offVal;
    int i = 0, len = 0;
    U8 forceClean = 0;    

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        aPcVal[i] = malloc(EACH_MAX_LEN);
        memset(aPcVal[i], 0, EACH_MAX_LEN);
    }
    i = 0;

    /* symbol   date    open    high    low  close   volume */
    while (i < ALL_DATA_LEN && pcData != NULL)
    {
        if ('\t' == pcData[0])
        {
            pcData++;
            continue;
        }

        pcNext = strchr(pcData, '\t');

        if (NULL == pcNext)
        {
            strncpy(aPcVal[i], pcData, strlen(pcData) - 1);
        }
        else
        {
            len = pcNext - pcData;
            strncpy(aPcVal[i], pcData, len);
        }

        if (i > 2 && i < 6)
        {
            val[i] = strtof(aPcVal[i], NULL);
        }

        pcData = pcData + len;

        i++;
    }

    date = aPcVal[1];
    highVal = val[3];
    lowVal = val[4];
    offVal = val[5];

    if (1 == g_1stFlag)
    {
        g_1stFlag = 0;
        g_lastVal = offVal;
    }
    
    highRate = ((highVal - g_lastVal) / g_lastVal) * 100;
    lowRate = ((g_lastVal - lowVal) / g_lastVal) * 100;

    if (highRate > 10.9 || lowRate > 10.9)
    {
        //printf("Data error : H=%.2f L=%.2f\n", highRate, lowRate);
        for (i = 0; i < ALL_DATA_LEN; i++)
	    {
	        free(aPcVal[i]);
	    }
	    g_1stFlag = 1;
        canBuy = 1, canSell = 0, sellEnd = 0;
        return;
    }

    if (1 == canSell)
    {
        if (((g_buyVal - lowVal) / g_buyVal * 100 > g_lossAlarm)) //compare total lose Rate        
        {
            sellEnd = 1;
            g_sellVal = g_buyVal * (1 - ((g_lossAlarm)/100));
            forceClean = 1;
        }
        else if (0 && ((highVal - g_lastVal) / g_lastVal * 100 > g_fSellRate))  //compare everyDay Rate
        {
            sellEnd = 1;
            g_sellVal = g_lastVal * (1 + ((g_fSellRate)/100));
        }
        else if (((highVal - g_buyVal) / g_buyVal * 100 > g_winAlarm))  //compare total win  Rate
        {
            sellEnd = 1;
            g_sellVal = g_buyVal * (1 + ((g_winAlarm)/100));
        }
        
        if (1 == sellEnd)
        {
            canBuy = 1;
	        canSell = 0;
	        sellEnd = 0;
	        printf("%-10s S=%-6.2f OFF=%-6.2f  H=%-6.2f L=%-6.2f", 
	            date, g_sellVal, offVal, highRate, lowRate);

	        if (1 > forceClean)
	        {
	            printf("\033[0;31m%.2f\033[0;39m\n", (g_sellVal - g_buyVal) / g_buyVal * 100);
	        }
	        else
	        {
	            printf("\033[0;32m%.2f\033[0;39m\n", (g_sellVal - g_buyVal) / g_buyVal * 100);
	        }

	        g_tradeCnt++;
	        g_fAllEarn += (g_sellVal - g_buyVal) / g_buyVal * 100;
        }     
    }
    else if (1 == canBuy)
    {
        if ((g_lastVal - lowVal) / g_lastVal * 100 > g_fBuyRate)
        {
	        canBuy = 0;
	        forceClean = 0;	        
	        canSell = 1;
	        g_buyVal = g_lastVal * (1 - ((g_fBuyRate)/100));
	        printf("%-10s B=%-6.2f OFF=%-6.2f  H=%-6.2f L=%-6.2f\n", date, g_buyVal, offVal, highRate ,lowRate);
        }
    }

    g_lastVal = offVal;    
    g_afAllOff[g_u16Days] = g_lastVal;

    g_u16Days++;
    g_fAvg5  = calc_avgX_price(5, g_u16Days);
    g_fAvg10 = calc_avgX_price(10, g_u16Days);
    g_fAvg20 = calc_avgX_price(20, g_u16Days);
    g_fAvg30 = calc_avgX_price(30, g_u16Days);
    g_fAvg60 = calc_avgX_price(60, g_u16Days);
    logs("%3d A5=%.2f A10=%.2f A20=%.2f A30=%.2f A60=%.2f\n", g_u16Days, g_fAvg5, g_fAvg10, g_fAvg20, g_fAvg30, g_fAvg60);

	for (i = 0; i < ALL_DATA_LEN; i++)
	{
		free(aPcVal[i]);
	}
}

float calc_avgX_price(int avgX, int index)
{
    float fAllPrice = 0;
    int i = 0, j = 0, stopIndex = index;

    if (index > avgX)
    {
        stopIndex = avgX;
    }

    for (i = index - 1; j < stopIndex; i--, j++)
    {
        fAllPrice += g_afAllOff[i];
    }

    return fAllPrice / (index > avgX ? avgX : index);
}

int main (int argc, char **argv)
{
    FILE *out;
    char *eachData = NULL;
    size_t len = 0;

    if (1 == argc || 0 == strcmp("-h", argv[1]) || 0 == strcmp("-help", argv[1]) || 0 == strcmp("-H", argv[1]))
    {
        printf("Howto : %s _FILE_ [buyRate] [sellRate] [lossAlarmRate] [winAlarmRate]\n\n", argv[0]);
        return 0;
    }
    
    out = fopen(argv[1], "r");
    int tmp = 0;

    if (2 < argc)
    {
        g_fBuyRate = strtof(argv[2], NULL);
    }

    if (3 < argc)
    {
        g_fSellRate = strtof(argv[3], NULL);
    }

    if (4 < argc)
    {
        g_lossAlarm = strtof(argv[4], NULL);
    }
    
    if (5 < argc)
    {
        g_winAlarm = strtof(argv[5], NULL);
    }

    if (NULL != out)
    {
        printf("-----BuyRate=%.0f sellRate=%.0f lossAlarmRate=%.0f winAlarmRate=%.0f-----\n", 
            g_fBuyRate, g_fSellRate, g_lossAlarm, g_winAlarm);
        
        while (getline(&eachData, &len, out) != -1)
        {
            if (NULL != eachData && tmp < 256)
            {
                analysia_each_stk(eachData);tmp++;
            }

            eachData = NULL;
        }    
    }
    else
    {
        printf("Cannot open file %s\n", argv[1]);
    }

    if (out)
    {
        fclose(out);
    }

    if (g_tradeCnt > 0)
    {
        if (g_fAllEarn > 0)
	    {
	        printf("\033[0;31m%-6s : All %.2f TradeCnt %d\033[0;39m\n", argv[1], g_fAllEarn, g_tradeCnt);
	    }
	    else
	    {
	        printf("\033[0;32m%-6s : All %.2f TradeCnt %d\033[0;39m\n", argv[1], g_fAllEarn, g_tradeCnt);
	    }
    }
    
    
    return 0;

} /* ----- End of main() ----- */
