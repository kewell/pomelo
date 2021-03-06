#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

/*
   2014-09-19 13:20:50
   VAL[?] 0-open 1-yester 2-n 3-h 4-l 7-AllTradeSum 8-AllTradeMoney
   var hq_str_sz0=""
   */

/*
0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  8---SUM  9---Money  
"NAME, 16.87, 16.90,      17.13,17.42, 16.70,17.12, 17.13, 9327671, 159983824.30,

B: 10   11    12    13     14    15   16     17    18   19
15523,17.12, 16479,17.11, 39691,17.10,10600,17.09,13400,17.08,

S: 20  21     22    23     24   25    26    27   28    29
200,17.13,  7000,17.14,  29800,17.15,9800,17.16,11924,17.17,

30         31   32
2014-05-22,10:30:00,00"
*/
//#define __DEBUG__
#ifdef __DEBUG__
#define debug_printf(x...) printf(x)
#else
#define debug_printf(x...)
#endif

#define HEADS_LEN           15
#define NAMES_LEN           4
#define ALL_DATA_LEN        31//30 
#define OUT_PUT ".stk.data"
#define EACH_MAX_LEN        2048
int iRunCnt = 1;
int g_showTmpRateFlg = 1;
#define LOOP_TIME           1000
#define FILE_OUTP           "/dev/pts/1"
#define USEFUL_ID           0 
#define RUN_ONCE            "--outa"
#define RUN_NO_STOP         "--d"
#define ASCII_0_VAL         0x30
#define SHOW_GAP_CNTS       142

int g_ucArgDozeSec = 10;
unsigned char g_ucArgGapCnt = 40;

unsigned char g_ucLineCnt = 0;
unsigned char g_ucMaxCnt = 10;
float g_afTradeSum[100] = {0.0};
float g_afLastPrice[100] = {0.0};
float g_afTmpRate[100] = {0.0};
int g_aTradeMoney[100] = {0.0};
unsigned char g_ucIsSZA = 1;
unsigned char g_ucRunOnce = 0;
unsigned char g_ucRunNoStop = 0;

void updateTimeDoze()
{
    FILE *pstTmpFile;
    int readCnt = -1;
    unsigned char aReadData[2] = {0,0};

    pstTmpFile = fopen("/var/www/icons/.dozeTime", "r");
    if (NULL != pstTmpFile)
    {
        readCnt = fread(aReadData, 1, 2, pstTmpFile);

        if (0 < readCnt)
        {
            if (aReadData[0]>0x2F && aReadData[0]<0x3A)
                aReadData[0] -= ASCII_0_VAL;
            else
                aReadData[0] = 0;
        }
        if (1 < readCnt)
        {
            if (aReadData[1]>0x2F && aReadData[1]<0x3A)
                aReadData[1] -= ASCII_0_VAL;
            else
                aReadData[1] = 0;
        }
        g_ucArgDozeSec = (aReadData[0] * ((0 == aReadData[1]) ? ((1 < readCnt)?10:1) : 10) + aReadData[1]);
    }
    printf("DozeSecTime is %d Sec\n", g_ucArgDozeSec);
}


void analysia_each_stk (char *pcData, float fAlarmRate, unsigned char isFirst, unsigned char index)
{
    debug_printf("%s", pcData);
    char *pcStockName, *pcTmp, *pcNext;
    char *aPcVal[ALL_DATA_LEN];
    float val[ALL_DATA_LEN];
    int pts = -1;int ret = -1;
    char *pcSend;
    int i = 0, len = 0;

    /* ---------------------------------------initialization, initialize --------------------------------*/
    pcStockName = malloc(NAMES_LEN);

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        aPcVal[i] = malloc(EACH_MAX_LEN);
        memset(aPcVal[i], 0, EACH_MAX_LEN);
        val[i] = 0.0;
    }

    if(NULL != (pcData + HEADS_LEN) && strlen(pcData + HEADS_LEN) > NAMES_LEN)
        strncpy(pcStockName, pcData + HEADS_LEN, NAMES_LEN);
    else
        return;
    /* ---------------------------------------initialization, initialize --------------------------------*/

    /* ---------------------------------------Got one line each time ------------------------------------*/
    i = 0;
    while (i < ALL_DATA_LEN && (pcTmp = strchr(pcData, ',') + 1) != NULL)
    {
        pcNext = strchr(pcTmp, ',');

        if (NULL == pcNext)
        {
            printf("Line %d error here\n", __LINE__);
            continue;
        }

        len = pcNext - pcTmp;

        if (NULL != pcTmp && strlen(pcTmp) > len)
            strncpy(aPcVal[i], pcTmp, len);
        else
        {
            printf("Line %d error here\n", __LINE__);
            return;
        }

        debug_printf("val[%d]=%s   ||| ", i, aPcVal[i]);
        //val[i] = __strtof_internal(aPcVal[i], NULL, 0);
        val[i] = strtof(aPcVal[i], NULL);
/*
        if (g_ucIsSZA && 8 == i)
        {
            memset(aPcVal[i], 0, EACH_MAX_LEN);

            if (NULL != pcTmp && strlen(pcTmp) > (len -7))
                strncpy(aPcVal[i], pcTmp, len - 7);
            else
                return;
        }
*/

        pcData = pcTmp;
        i++;
    }
    debug_printf("\n");
    debug_printf("%s =%f, =%f, =%f ,=%f, =%f\n\n", pcStockName, val[2], val[1],  (val[2] - val[1]), (val[2] - val[1]) * 100, (val[2] - val[1]) * 100 / val[1]);
    /* ---------------------------------------Got one line each time ------------------------------------*/

    int hour,min, sec;
    if (1 == g_ucIsSZA)
    {
        /*  aacVal[30] is time strings */
        char *pMin = NULL, *pSec;
        char acHour[3] = {0,0,'\0'}, acMin[3] = {0,0,'\0'}, acSec[3] = {0,0,'\0'};

        strncpy(acHour, aPcVal[30], 2);
        pMin = strstr(aPcVal[30], ":");
        if (NULL != pMin)
        {
            pMin++;strncpy(acMin, pMin, 2);        
            pSec = strstr(pMin, ":");pSec++;strncpy(acSec, pSec, 2);
        }
        hour=atoi(acHour), min=atoi(acMin), sec=atoi(acSec);
    }

    //0-open 1-yester 2-n 3-h 4-l 7-AllTradeSum 8-AllTradeMoney
    float fTmpRate = (val[2] - val[1]) * 100 / val[1];
    float fAllBuy = (val[9] + val[11] + val[13] + val[15] + val[17]);
    float fAllSell = (val[19] + val[21] + val[23] + val[25] + val[27]);
    float fAllMoney = val[8] / 10000000;
    float fRequestSub = (fAllBuy - fAllSell) / 100;

    if(1 == g_ucRunOnce)
    {
        printf("%s %5.2f,%5.2f,%5.2f %-5.5s %-5.5s %-5.5s__%.0f/%.0f/%.0f/%.0f/%.0f__%.0f/%.0f/%.0f/%.0f/%.0f__%-2.0f___%.1f[KW]\n", 
                pcStockName, 
                fTmpRate,
                ((val[3] - val[1]) * 100 / val[1]),
                ((val[4] - val[1]) * 100 / val[1]),
                aPcVal[2],
                aPcVal[3],
                aPcVal[4],
                val[27]/100, val[25]/100, val[23]/100, val[21]/100, val[19]/100,  
                val[9]/100, val[11]/100, val[13]/100, val[15]/100, val[17]/100,
                (g_ucIsSZA) ? val[8] : ((fAllBuy - fAllSell)/100),
                fAllMoney
              );
    }

    g_ucLineCnt++;

    if(g_ucRunNoStop) 
    {
        pts = open(FILE_OUTP, O_RDWR);

        if(0 < pts)
        {
            pcSend = (char *)malloc(EACH_MAX_LEN);
            memset(pcSend, 0, EACH_MAX_LEN);
            unsigned char useLargeFlag = 0;

            if (g_ucIsSZA)
            {
                if (fTmpRate > 0)
                {
                    sprintf(pcSend, "\n%02d%02d\033[1;34m%02d\033[0;39m", min,sec, (int)(fTmpRate*10));
                }
                else
                {
                    sprintf(pcSend, "\n%02d%02d\033[1;31m%02d\033[0;39m", min,sec, abs((int)(fTmpRate*10)));
                }
            }
            else
            {
                if (abs((int)fRequestSub >= 10000))
                {
                    //fRequestSub=9999.0;
                    fRequestSub = fRequestSub / 1000;
                    useLargeFlag = 1;
                }

                if (isFirst)
                {
                    if (0 == strcmp(pcStockName, "0001") || 0 == strcmp(pcStockName, "9001") || 0 == strcmp(pcStockName, "9005") || 0 == strcmp(pcStockName, "9006"))
                    {
                        //sprintf(pcSend, "%s", "    ");// Original
                          sprintf(pcSend, "  ");
                    }
                    else
                    {
                        sprintf(pcSend, " %s", pcStockName);
                    }
                }
                else if (1 == g_showTmpRateFlg)
                {
                    if (0 == strcmp(pcStockName, "0001") || 0 == strcmp(pcStockName, "9001") || 0 == strcmp(pcStockName, "9005") || 0 == strcmp(pcStockName, "9006"))
                    {
                        if (fTmpRate > 0)
                        {
                            sprintf(pcSend, "\033[1;34m%02d\033[0;39m", (int)(fTmpRate*10));  
                        }
                        else
                        {
                            sprintf(pcSend, "\033[1;31m%02d\033[0;39m", abs((int)(fTmpRate*10)));
                        }
                    }
                    else if (fTmpRate > g_afTmpRate[index])
                    {
                        sprintf(pcSend, " \033[1;34m%s%.1f\033[0;39m", ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
                    }
                    else if (fTmpRate < g_afTmpRate[index])
                    {
                        sprintf(pcSend, " \033[1;31m%s%.1f\033[0;39m", ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
                    }
                    else
                    {
                        sprintf(pcSend, " %s%.1f", ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
                    }

                    g_afTmpRate[index] = fTmpRate;
                }
                else
                {
                    if (0 == strcmp(pcStockName, "0001") || 0 == strcmp(pcStockName, "9001") || 0 == strcmp(pcStockName, "9005") || 0 == strcmp(pcStockName, "9006"))
                    {
                        if (fTmpRate > 0)
                        {
                            sprintf(pcSend, "\033[1;34m%02d\033[0;39m", (int)(fTmpRate*10));  
                        }
                        else
                        {
                            sprintf(pcSend, "\033[1;31m%02d\033[0;39m", abs((int)(fTmpRate*10)));
                        }
                    }
                    else
                    {
                        /* Big trade sum in this duration, will use colorful putput */
                        if (1 || val[7] - g_afTradeSum[index] > 1000)
                        {
                            /* If current price is up use green color, other wise use red color */
                            if (val[2] < g_afLastPrice[index])
                            {
                                sprintf(pcSend, " \033[1;31m%-4.0f\033[0;39m", (val[7] - g_afTradeSum[index]) / 100);
                            }
                            else if (val[2] > g_afLastPrice[index])
                            {
                                sprintf(pcSend, " \033[1;34m%-4.0f\033[0;39m", (val[7] - g_afTradeSum[index]) / 100);
                            }
                            else
                            {
                                sprintf(pcSend, " %-4.0f", (val[7] - g_afTradeSum[index]) / 100);
                            }
                        }
                        else
                        {
                            sprintf(pcSend, " %-4.0f", (val[7] - g_afTradeSum[index]) / 100);
                        }
                    }
                }
            }

            ret = write(pts, pcSend, strlen(pcSend));

            free(pcSend);
            close(pts);
        }
    }
    else if(!g_ucRunOnce && (g_ucIsSZA || fTmpRate > fAlarmRate || fTmpRate < (0 - fAlarmRate)))
    {
        pts = open(FILE_OUTP, O_RDWR);

        if(0 < pts)
        {
            pcSend = (char *)malloc(EACH_MAX_LEN);
            memset(pcSend, 0, EACH_MAX_LEN);
            sprintf(pcSend, "%s:%s%.1f|", pcStockName, (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
            ret = write(pts, pcSend, strlen(pcSend));

            free(pcSend);
            close(pts);
        }
    }

    g_afTradeSum[index] = val[7];
    g_afLastPrice[index] = val[2];
    g_aTradeMoney[index] = abs((int)fRequestSub);

    if (g_ucMaxCnt == g_ucLineCnt)
    {
        g_ucLineCnt = 0;
    }

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        free(aPcVal[i]);
    }
    free(pcStockName);
}

int main (int argc, char **argv)
{
    FILE *out;
    char *eachData = NULL;
    size_t len = 0;
    float fAlarmRate = 12.0;
    unsigned char isFirst = 1, index = 0;

    //updateTimeDoze();
    if (0)//(2 != argc || 0 != strcmp(FILE_OUTP, ttyname(0)))
    {
        printf ("---------------KEWELL--------------- : %s %s() +%d\n", __FILE__, __func__, __LINE__);
        goto CleanUp;
    }

    if (2 <= argc)
    {
        if (1 == strlen(argv[1]))
        {
            //fAlarmRate = __strtof_internal(argv[1], NULL, 0);
            fAlarmRate = strtof(argv[1], NULL);
        }

        if (fAlarmRate > 0.9 && fAlarmRate < 9.1)
        {
            iRunCnt= LOOP_TIME;
        }
        else if (0 == strcmp(argv[1], RUN_ONCE))
        {
            g_ucRunOnce = 1;
        }
        else if (0 == strcmp(argv[1], RUN_NO_STOP))
        {
            g_ucRunNoStop = 1;
            iRunCnt= LOOP_TIME;
        }
        else
        {
            printf ("---------------KEWELL--------------- : %s %s() +%d\n", __FILE__, __func__, __LINE__);
            goto CleanUp;
        }

        if (3 <= argc)
        {
            g_ucArgDozeSec = strtof(argv[2], NULL);
        }

        if (4 <= argc)
        {
            g_ucArgGapCnt = (unsigned char)atoi(argv[3]);
        }
    }
    printf("DozeSecTime is %d Sec GapCnt=%d\n", g_ucArgDozeSec, g_ucArgGapCnt);

    while (iRunCnt > 0)
    {
        iRunCnt--;

        if (iRunCnt > 0)
        {
            if (0 == isFirst && 0 == g_showTmpRateFlg)
            {
                sleep(g_ucArgDozeSec);
            }

            if (1 && 1 == isFirst)
            {
                int pts = open(FILE_OUTP, O_RDWR);

                if(0 < pts)
                {
                write(pts, "\n-------------------------------------------------------------------------------------------------------------------------------------------------\n", SHOW_GAP_CNTS);
                    close(pts);
                }
            }
            g_ucIsSZA = 1;
        }

        out = fopen(OUT_PUT, "r");

        if (NULL != out)
        {
            index = 0;
            while (getline(&eachData, &len, out) != -1)
            {
                if (g_ucIsSZA || strlen(eachData) > 180)
                {
                    if (NULL != eachData)
                    {
                        analysia_each_stk(eachData, fAlarmRate, isFirst, index);
                        g_ucIsSZA = 0;
                    }
                }
                index++;
            }
        }

        if (0 && 1 == g_showTmpRateFlg)
        {
            int pts = open(FILE_OUTP, O_RDWR);

            if(0 < pts)
            {
                write(pts, "\n-------------------------------------------------------------------------------------------------------------------------------------------------\n", SHOW_GAP_CNTS);
                close(pts);
            }
        }

        if (out)
        {
            fclose(out);
        }

        if (0 == iRunCnt % g_ucArgGapCnt)
        {
            isFirst = 1;
            g_showTmpRateFlg = 1;
        }
        else if (0 == (iRunCnt + 1) % g_ucArgGapCnt)
        {
            g_showTmpRateFlg = 1;
            isFirst = 0;
        }
        else
        {
            g_showTmpRateFlg = 0;
            isFirst = 0;
        }

    }
    exit(EXIT_SUCCESS);

    printf ("---------------KEWELL--------------- : %s %s() +%d\n", __FILE__, __func__, __LINE__);
CleanUp:
    printf("hello world\n");
    return 0;
}
