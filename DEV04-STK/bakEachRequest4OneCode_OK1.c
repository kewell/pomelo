#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>

#ifdef __DEBUG__
#define debug_printf(x...) printf(x)
#else
#define debug_printf(x...)
#endif

#define HEADS_LEN           11//15
#define NAMES_LEN           8//4
#define ALL_DATA_LEN        31
#define OUT_PUT "/tmp/.data2.list"
#define EACH_MAX_LEN        2048
int iRunCnt = 1;
int g_showTmpRateFlg = 0;
#define LOOP_TIME           1000
#define BASE_FOLDER         "/.100G/.KEWELL/.STK/"
#define FILE_OUTP           "/dev/pts/0"
#define USEFUL_ID           0 
#define RUN_ONCE            "--outa"
#define RUN_NO_STOP         "--d"
#define ASCII_0_VAL         0x30
#define SHOW_GAP_CNTS       142
#define IS_BIG_TRADE_CNT    500
#define IS_BIG_REQUEST_CNT  1000
#define EACH_REALTIME_DATA_MINIMAL_LEN 180

int g_ucArgDozeSec = 15;
unsigned char g_ucArgGapCnt = 50;
unsigned char g_ucLineCnt = 0;
unsigned char g_ucMaxCnt = 10;
float g_afTradeSum[100] = {0.0};
float g_afLastPrice[100] = {0.0};
float g_afTmpRate[100] = {0.0};
int g_aTradeMoney[100] = {0.0};

char g_acBakFileNa[100];
unsigned char g_uc1stCodeFromData = 1;
unsigned char g_ucRunOnce = 0;
char g_acNaList[2][9]; // 0 is STATIC
#define AV_NAME_LEN         (sizeof(g_acNaList) / sizeof(g_acNaList[0]))
#define SPE_DEX             (AV_NAME_LEN - 1)
int g_minLast = 0;
int g_dataError = 0;

int checkCodeName(char *pcStockName)
{
    int i = 0;
    for (i = 0; i < AV_NAME_LEN; i++)
    {
        if (0 == strcmp(pcStockName, g_acNaList[i]))
        {
            return i;
        }
    }
    return -1;
}

int tradeTimeCheck(int h, int m)
{
    if (9 <= h && 16 > h)
    {
        if ((9==h && 28<m) || (11==h && 31>m) || (12==h && 58<m) || (15==h && 1>m))return 0;
    }
    return -1;
}

void analysia_each_stk (char *pcData, unsigned char isFirst, unsigned char index)
{
    //debug_printf("%s", pcData);
    char *pcStockName, *pcTmp, *pcNext;
    char *aPcVal[ALL_DATA_LEN];
    float val[ALL_DATA_LEN];
    int pts = -1;int ret = -1;
    char *pcSend, *pcSendTmp;
    int i = 0, len = 0;
    int isSpecCode = -1; // 成分指数=-1，普通Code=1

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

    isSpecCode = checkCodeName(pcStockName);
    if (-1 == isSpecCode)
    {
        return;
    }

    g_uc1stCodeFromData = 0;
    if (SPE_DEX <= isSpecCode)
    {
        isSpecCode = 1;
    }
    else
    {
        g_uc1stCodeFromData = 1;
        isSpecCode = 0;
    }

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

        //debug_printf("val[%d]=%s\n", i, aPcVal[i]);
        //val[i] = __strtof_internal(aPcVal[i], NULL, 0);
        val[i] = strtof(aPcVal[i], NULL);
        
        pcData = pcTmp;
        i++;
    }

    /* aPcVal[30] is time strings */
    char *pMin = NULL;
    char acHour[3] = {0,0,'\0'}, acMin[3] = {0,0,'\0'};
    
    strncpy(acHour, aPcVal[30], 2);
    pMin = strstr(aPcVal[30], ":");
    if (NULL != pMin)
    {
        pMin++;
        strncpy(acMin, pMin, 2);
    }
    int hour=atoi(acHour), min=atoi(acMin);
    
    if (0 != tradeTimeCheck(hour, min))
    {
        debug_printf("%02d:%02d IS NOT TIME\n", hour, min);
        return;
    }
    
    if (0 != min && g_minLast > min)
        g_dataError = 1;g_dataError = 1;
    g_minLast = min;
    
    float fTmpRate = (val[2] - val[1]) * 100 / val[1];
    float fAllBuy = (val[9] + val[11] + val[13] + val[15] + val[17]);
    float fAllSell = (val[19] + val[21] + val[23] + val[25] + val[27]);
    float fRequestSub = (fAllBuy - fAllSell) / 100;
    unsigned char ucReqIsGood = 0;
    if (fAllBuy > fAllSell)ucReqIsGood = 1;

/*
0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  8---SUM  9---Money  
"NAME, 16.87, 16.90,      17.13,17.42, 16.70,17.12, 17.13, 9327671, 159983824.30,

B: 10   11    12    13     14    15   16     17    18   19     S: 20  21     22    23     24   25    26    27   28    29    30         31   32
15523,17.12, 16479,17.11, 39691,17.10,10600,17.09,13400,17.08, 200,17.13,  7000,17.14,  29800,17.15,9800,17.16,11924,17.17, 2014-05-22,10:30:00,00"
*/

    g_ucLineCnt++;
    pts = open(FILE_OUTP, O_RDWR);
    //if (0 < pts)
    {
        pcSend = (char *)malloc(EACH_MAX_LEN);
        memset(pcSend, 0, EACH_MAX_LEN);
        pcSendTmp = (char *)malloc(EACH_MAX_LEN);
        memset(pcSendTmp, 0, EACH_MAX_LEN);
        unsigned char useLargeFlag = 0;

        if (g_uc1stCodeFromData)
        {
            sprintf(pcSend, "\n%s%s%.1f", aPcVal[30], (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
        }
        else if (1 != isSpecCode)
        {
            sprintf(pcSend, "%s%.1f", (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
        }
        else
        {
            if (abs((int)fRequestSub >= IS_BIG_REQUEST_CNT))
            {
                useLargeFlag = 1;
            }

            if (0 && isFirst) /* Ignore Here*/
            {
                if (0 == isSpecCode)
                {
                    sprintf(pcSend, "%s", "    ");
                }
                else
                {
                    sprintf(pcSend, " %s", pcStockName);
                }
            }
            else if (0 && 1 == g_showTmpRateFlg) /* Ignore Here 每到指定统计时间点，只输出所有CODE 的当前RATE */
            {
                if (0 == isSpecCode)
                {
                    sprintf(pcSend, "%s%.1f", ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
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

                // 普通的CODE只输出B/S 请求数量和成交的数量
                /*---------------------------------SHOW REAL REQEST S/B CNT---------------------------------*/
                sprintf(pcSendTmp, " %-4.0f %-4.0f %-4.0f %-4.0f %-4.0f|%-4.0f %-4.0f %-4.0f %-4.0f %-4.0f|", 
                        val[27]/100,val[25]/100,val[23]/100,val[21]/100,val[19]/100, /* S5 S4 S3 S2 S1 */
                        val[9]/100,val[11]/100,val[13]/100,val[15]/100,val[17]/100); /* B1 B2 B3 B4 B5 */
                strcat(pcSend, pcSendTmp);
                memset(pcSendTmp, 0, EACH_MAX_LEN);

                /*---------------------------------SHOW REAL REQUEST CNT RET---------------------------------*/                                            
                #if 0
                if (1 == useLargeFlag) // large Cnt use yellow output
                {
                    sprintf(pcSendTmp, "\033[1;33m%s%-4.0f\033[0;39m", (1==ucReqIsGood)?"+":"-", (float)abs(fRequestSub));
                }
                else
                {
                    sprintf(pcSendTmp, "%s%-4.0f", (1==ucReqIsGood)?"+":"-", (float)abs(fRequestSub));
                }
                strcat(pcSend, pcSendTmp);
                memset(pcSendTmp, 0, EACH_MAX_LEN);
                #endif
                /*---------------------------------SHOW REAL DEAL CNT---------------------------------*/
                /* Big trade sum in this duration, will use colorful putput */
                /* val[7] is Cur TradeSum */
                if (val[7] - g_afTradeSum[index] >= IS_BIG_TRADE_CNT || val[7] > 10 * g_afTradeSum[index])
                {
                    /* If current price is up use green color, other wise use red color */
                    if (val[2] < g_afLastPrice[index])
                    {
                        sprintf(pcSendTmp, "\033[1;31m%-4.0f\033[0;39m", (val[7] - g_afTradeSum[index]) / 100);
                    }
                    else if (val[2] > g_afLastPrice[index])
                    {
                        sprintf(pcSendTmp, "\033[1;34m%-4.0f\033[0;39m", (val[7] - g_afTradeSum[index]) / 100);
                    }
                    else
                    {
                        sprintf(pcSendTmp, "%-4.0f", (val[7] - g_afTradeSum[index]) / 100);
                    }
                }
                else
                {
                    sprintf(pcSendTmp, "%-4.0f", (val[7] - g_afTradeSum[index]) / 100);
                }
                strcat(pcSend, pcSendTmp);
                memset(pcSendTmp, 0, EACH_MAX_LEN);

                /*---------------------------------SHOW Curr Price AND RATE AND TIME ---------------------------------*/
                /* val[2] is Current deal price, val[20] is Sell_1 price, val[10] is Buy_1 Price */
                float a,c;
                int b = 0;;
                a = val[2];
                b = (int)(int)(a+1.0e-6);//b = (int)a;
                c = (a - b)*100;

                if (0 == isSpecCode)
                {
                    sprintf(pcSendTmp, "%s%.1f", ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
                }
                else if (fTmpRate > g_afTmpRate[index])
                {
                    sprintf(pcSendTmp, "%2.0f\033[1;34m%s%.1f\033[0;39m ", c, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
                }
                else if (fTmpRate < g_afTmpRate[index])
                {
                    sprintf(pcSendTmp, "%2.0f\033[1;31m%s%.1f\033[0;39m ", c, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
                }
                else
                {
                    sprintf(pcSendTmp, "%2.0f%s%.1f ", c, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
                }
                
                strcat(pcSend, pcSendTmp);
                memset(pcSendTmp, 0, EACH_MAX_LEN);

                g_afTmpRate[index] = fTmpRate;

                strcat(pcSend, pcSendTmp);
                memset(pcSendTmp, 0, EACH_MAX_LEN);

                if (1 == g_dataError)
                {
                    sprintf(pcSendTmp, " Error");
                    strcat(pcSend, pcSendTmp);
                    memset(pcSendTmp, 0, EACH_MAX_LEN);
                }
            }
        }
        ret = write(pts, pcSend, strlen(pcSend));
        
        int fileBakFd = open(g_acBakFileNa, O_RDWR|O_APPEND);//O_APPEND);
        if (0 < fileBakFd)
        {
            ret = write(fileBakFd, pcSend, strlen(pcSend));
        }
        free(pcSend);
        free(pcSendTmp);
        close(pts);
        close(fileBakFd);
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

void initStaticNames()
{
    if ('h'==g_acNaList[SPE_DEX][1] && '6'==g_acNaList[SPE_DEX][2])
        strncpy(g_acNaList[0], "sh000001", 9);
    else if ('z'==g_acNaList[SPE_DEX][1])
    {
        if ('3'==g_acNaList[SPE_DEX][2])
            strncpy(g_acNaList[0], "sz399006", 9);
        else if ('0'==g_acNaList[SPE_DEX][2] && '0'==g_acNaList[SPE_DEX][3])
        {
            if ('2'<=g_acNaList[SPE_DEX][4])
                strncpy(g_acNaList[0], "sz399005", 9);
            else
                strncpy(g_acNaList[0], "sz399001", 9);
        }   
    }
    else
    {
        strncpy(g_acNaList[0], "sh000001", 9);
        printf("Error Maybe Code is Wrong\n");
    }
}

void initOutPutFileName()
{
    struct timeb stTime;
    ftime(&stTime);
    struct tm *pstTime = localtime(&(stTime.time)); 
    sprintf(g_acBakFileNa, "%s%s_%d%02d%02d", 
        BASE_FOLDER,g_acNaList[SPE_DEX], 1900+pstTime->tm_year, pstTime->tm_mon+1, pstTime->tm_mday);
    open(g_acBakFileNa, O_APPEND | O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP);
}

int argsCheck (int argc, char **argv)
{
    char aucSpecArgCode[9] = {0};
    if (2 <= argc)
    {
        if (0 == strcmp(argv[1], RUN_ONCE))
        {
            g_ucRunOnce = 1;
        }
        else if (0 == strcmp(argv[1], RUN_NO_STOP))
        {
            iRunCnt = LOOP_TIME;
        }

        if (3 <= argc)
        {
            g_ucArgDozeSec = strtof(argv[2], NULL);
        }
        if (4 <= argc)
        {
            g_ucArgGapCnt = strtof(argv[3], NULL);
        }
        if (5 <= argc)
        {
            strncpy(aucSpecArgCode, argv[4], 9);
            if (aucSpecArgCode[0] == 's' && aucSpecArgCode[2] >= '0' && aucSpecArgCode[2] <= '9'
                && aucSpecArgCode[7] >= '0' && aucSpecArgCode[7] <= '9')
            {
                strncpy(g_acNaList[SPE_DEX], aucSpecArgCode, 9);
            }
            else
            {
                return 1;
            }
        }
    }
    return 0;
}
void printUsage(char *cmds){printf("Usage: %s %s SleepSec GapCnt Code\n", cmds, RUN_NO_STOP);} // TODO

int main (int argc, char **argv)
{
    FILE *out;
    char *eachData = NULL;
    size_t len = 0;
    unsigned char isFirst = 1, index = 0;
    
    if (0 == argsCheck(argc, argv))
    {
        initStaticNames();
    }
    else
    {
        printf("Code Error\n");
        printUsage(argv[0]);
        return 0;
    }

    initOutPutFileName();
    printf("DozeSecTime is %d Sec GapCnt=%d Code=[%s]\n", g_ucArgDozeSec, g_ucArgGapCnt, g_acNaList[SPE_DEX]);

    while (iRunCnt > 0)
    {
        iRunCnt--;

        if (iRunCnt > 0)
        {
            if (0 == isFirst && 0 == g_showTmpRateFlg)
            {
                sleep(g_ucArgDozeSec);
            }
            g_uc1stCodeFromData = 1;
        }

        out = fopen(OUT_PUT, "r");
        if (NULL != out)
        {
            index = 0;
            g_dataError = 0;
            while (getline(&eachData, &len, out) != -1)
            {
                if (g_uc1stCodeFromData || strlen(eachData) > EACH_REALTIME_DATA_MINIMAL_LEN)
                {
                    if (NULL != eachData)
                    {
                        analysia_each_stk(eachData, isFirst, index);
                        g_uc1stCodeFromData = 0;
                    }
                }
                index++;
            }    
        }

        if (out)
        {
            fclose(out);
        }

        #if 0
        if (0 == iRunCnt % g_ucArgGapCnt)
        {
            isFirst = 1;
            g_showTmpRateFlg = 0;
        }
        else if (0 == (iRunCnt + 1) % g_ucArgGapCnt)
        {
            g_showTmpRateFlg = 1;
            isFirst = 0;
        }
        else
        #endif
        {
            g_showTmpRateFlg = 0;
            isFirst = 0;
        }

    }
    exit(EXIT_SUCCESS); 
    return 0;
}

