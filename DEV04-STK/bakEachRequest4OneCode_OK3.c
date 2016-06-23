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

//#define __TEST__
#ifdef __DEBUG__
#define debug_printf(x...) printf(x)
#else
#define debug_printf(x...)
#endif

#define HEADS_LEN           11//15
#define NAMES_LEN           8//4
#define ALL_DATA_LEN        31
#define OUT_PUT "/dev/.stk.data"
#define BAK_FILE_MSG " 时间   指数 卖5  卖4  卖3  卖2  卖1 | 买1  买2  买3  买4  买5 | 量    价 涨幅"

#define LOOP_TIME           1000
#define BASE_FOLDER         "/.100G/.KEWELL/.STK/"
#define FILE_OUTP           "/dev/pts/0"
#define ASCII_0_VAL         0x30

#define TINY_DEAL_CNT   25 /******************************/
#define BIG_DEAL_CNT    500
#define BIG_REQ_CNT     999

#define EACH_REALTIME_DATA_MINIMAL_LEN 180

#define MINIMAL_BUF_SIZE        64
#define TINY_BUF_SIZE           128
#define BIG_BUF_SIZE            256
#define HUGE_BUF_SIZE           512
#define MAXIMAL_BUF_SIZE        1024//2048

int g_ucArgDozeSec = 12;
float g_afLastAllTradeSum[100] = {0};
float g_afLastPrice[100] = {0.0};
float g_afTmpRate[100] = {0.0};

char g_acBakFileNa[MINIMAL_BUF_SIZE];
unsigned char g_uc1stCodeFromData = 0;
unsigned char g_ucHiddenMsg = 0;
char g_acNaList[2][9]; // 0 is STATIC
#define AV_NAME_LEN         (sizeof(g_acNaList) / sizeof(g_acNaList[0]))
#define SPE_DEX             (AV_NAME_LEN - 1)
int g_lastMin = 0, g_lastSec = 0;
int g_dateError = 0;
int g_outOfTimeFlag = 0;

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

int tradeTimeCheck(int h, int m, int s)
{
#ifdef __TEST__
    return 0;
#else
    if (9 <= h && 15 >= h)
    {
        if (10==h || 13==h || 14==h 
            || (9==h && 29<=m && 45<s)
            || (11==h && 31>m && 15>s) 
            || (12==h && 59==m && 45<s) || (15==h && 0==m && 15>s))
        {
            return 0;
        }
    }

    if (15<=h)g_outOfTimeFlag = 1;

    return -1;
#endif
}

void analysia_each_stk (char *pcData, unsigned char index, char *pcRetStr)
{
    //debug_printf("%s", pcData);
    char *pcStockName, *pcTmp, *pcNext;
    char *aPcVal[ALL_DATA_LEN];
    
    float val[ALL_DATA_LEN];
    char *pcSend, *pcSendTmp;
    int i = 0, len = 0;
    int isSpecCode = -1; // 成分指数=0，普通Code=1

    /* ---------------------------------------initialization, initialize --------------------------------*/
    //char pcRetStr[BIG_BUF_SIZE];memset(pcRetStr, 0, BIG_BUF_SIZE);

    pcStockName = malloc(NAMES_LEN);

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        aPcVal[i] = malloc(MAXIMAL_BUF_SIZE);
        memset(aPcVal[i], 0, MAXIMAL_BUF_SIZE);
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

    if (SPE_DEX <= isSpecCode)
    {
        isSpecCode = 1;
    }
    else
    {
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
    char *pMin = NULL, *pSec;
    char acHour[3] = {0,0,'\0'}, acMin[3] = {0,0,'\0'}, acSec[3] = {0,0,'\0'};
    
    strncpy(acHour, aPcVal[30], 2);
    pMin = strstr(aPcVal[30], ":");
    if (NULL != pMin)
    {
        pMin++;strncpy(acMin, pMin, 2);        
        pSec = strstr(pMin, ":");pSec++;strncpy(acSec, pSec, 2);
    }
    int hour=atoi(acHour), min=atoi(acMin), sec=atoi(acSec);
    
    if (0 != tradeTimeCheck(hour, min, sec))
    {
        debug_printf("%02d:%02d IS NOT TIME\n", hour, min);
        return;
    }

/*
0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  8---SUM  9---Money  
"NAME, 16.87, 16.90,      17.13,17.42, 16.70,17.12, 17.13, 9327671, 159983824.30,

B: 10   11    12    13     14    15   16     17    18   19     S: 20  21     22    23     24   25    26    27   28    29    30         31   32
15523,17.12, 16479,17.11, 39691,17.10,10600,17.09,13400,17.08, 200,17.13,  7000,17.14,  29800,17.15,9800,17.16,11924,17.17, 2014-05-22,10:30:00,00"
*/
    if (0 == index && g_lastSec == sec)
    {
        g_ucHiddenMsg = 1;
    }
    if (0 == index && 0 != min && g_lastMin > min)
    {
        g_dateError = 1;
    }      
    g_lastMin = min;
    g_lastSec = sec;
    
    float curAllTradeSum = (val[7]/100), curTradeSum = curAllTradeSum - g_afLastAllTradeSum[index];
    float curPrice = val[2], lastClosePrice = val[1];
    float fTmpRate = (curPrice - lastClosePrice) * 100 / lastClosePrice;
    float fAllReqBuy = (val[9] + val[11] + val[13] + val[15] + val[17]);
    float fAllReqSell = (val[19] + val[21] + val[23] + val[25] + val[27]);
    float fReqSub = (fAllReqBuy - fAllReqSell) / 100;
    unsigned char ucReqIsGood = 0;
    if (fAllReqBuy > fAllReqSell)ucReqIsGood = 1;
    
    pcSend = (char *)malloc(MAXIMAL_BUF_SIZE);
    memset(pcSend, 0, MAXIMAL_BUF_SIZE);
    pcSendTmp = (char *)malloc(MAXIMAL_BUF_SIZE);
    memset(pcSendTmp, 0, MAXIMAL_BUF_SIZE);
    unsigned char useLargeFlag = 0;
    unsigned char bigReqFlag[ALL_DATA_LEN] = {0};

    if (1 == isSpecCode)
    {
        for (i = 27; i >= 9; i -= 2)
        {
            if (val[i]/100 > BIG_REQ_CNT)
                bigReqFlag[i] = 1;
        }
        
        if (abs((int)fReqSub) > BIG_REQ_CNT || curTradeSum > BIG_DEAL_CNT)
            useLargeFlag = 1;
    }
    
    if (g_uc1stCodeFromData)
    {
        sprintf(pcSend, "\n%s%s%.1f", aPcVal[30], (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
    }
    else if (1 != isSpecCode)
    {
        sprintf(pcSend, "%s%.1f", (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
    }
    else if (1 == isSpecCode && 0 == useLargeFlag && curPrice == g_afLastPrice[index] 
            && TINY_DEAL_CNT > curTradeSum && BIG_REQ_CNT > abs((int)fReqSub))
    {
        g_ucHiddenMsg = 1;
    }
    else
    {            
        // 普通的CODE只输出B/S 请求数量和成交的数量
        /*---------------------------------SHOW REAL REQEST S/B CNT---------------------------------*/
#if 1
/*
 9 #define YELLOW               "\e[1;33m"
10 #define BLUE                 "\e[0;34m"
11 #define L_BLUE               "\e[1;34m"
12 #define PURPLE               "\e[0;35m"
13 #define L_PURPLE             "\e[1;35m"
14 #define CYAN                 "\e[0;36m"
15 #define L_CYAN               "\e[1;36m"
16 #define GRAY                 "\e[0;37m"
17 #define WHITE                "\e[1;37m"
18 
19 #define BOLD                 "\e[1m"
*/
        for (i = 27; i >= 19; i -= 2)
        {
            if (1 == bigReqFlag[i]) sprintf(pcSendTmp, "\033[1;33m%-5.0f\033[0;39m", val[i]/100);
            else sprintf(pcSendTmp, "%-5.0f", val[i]/100);

            strcat(pcSend, pcSendTmp);
            memset(pcSendTmp, 0, MAXIMAL_BUF_SIZE);
        }
        strcat(pcSend, "|");
        for (i = 9; i <= 17; i += 2)
        {
            if (1 == bigReqFlag[i]) sprintf(pcSendTmp, "\033[1;33m%-5.0f\033[0;39m", val[i]/100);
            else sprintf(pcSendTmp, "%-5.0f", val[i]/100);

            strcat(pcSend, pcSendTmp);
            memset(pcSendTmp, 0, MAXIMAL_BUF_SIZE);
        }
        strcat(pcSend, "|");
#else        
        sprintf(pcSendTmp, "%-5.0f%-5.0f%-5.0f%-5.0f%-5.0f|%-5.0f%-5.0f%-5.0f%-5.0f%-5.0f|", 
                val[27]/100,val[25]/100,val[23]/100,val[21]/100,val[19]/100, /* S5 S4 S3 S2 S1 */
                val[9]/100,val[11]/100,val[13]/100,val[15]/100,val[17]/100); /* B1 B2 B3 B4 B5 */
        strcat(pcSend, pcSendTmp);
        memset(pcSendTmp, 0, MAXIMAL_BUF_SIZE);
#endif
        /*---------------------------------SHOW REAL DEAL CNT---------------------------------*/
        /* Big trade sum in this duration, will use colorful putput */
        if (1 == useLargeFlag)
        {
            /* If current price is up use green color, is down use red, other wise use yellow color */
            if (curPrice < g_afLastPrice[index])
            {
                sprintf(pcSendTmp, "\033[1;31m%-5.0f\033[0;39m", curTradeSum);
            }
            else if (curPrice > g_afLastPrice[index])
            {
                sprintf(pcSendTmp, "\033[1;34m%-5.0f\033[0;39m", curTradeSum);
            }
            else
            {
                sprintf(pcSendTmp, "\033[1;33m%-5.0f\033[0;39m", curTradeSum);
            }
        }
        else
        {
            sprintf(pcSendTmp, "%-5.0f", curTradeSum);
        }
        strcat(pcSend, pcSendTmp);
        memset(pcSendTmp, 0, MAXIMAL_BUF_SIZE);

        /*---------------------------------SHOW Curr Price AND RATE AND TIME ---------------------------------*/
        /* val[2] is Current deal price, val[20] is Sell_1 price, val[10] is Buy_1 Price */
        float a,c;
        int b = 0;;
        a = curPrice;
        b = (int)(int)(a+1.0e-6);//b = (int)a;
        c = (a - b)*100;

        if (fTmpRate > g_afTmpRate[index]) // Blue
        {
            sprintf(pcSendTmp, "%2.2f\033[1;34m%s%.1f\033[0;39m ", a, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        else if (fTmpRate < g_afTmpRate[index]) // Red
        {
            sprintf(pcSendTmp, "%2.2f\033[1;31m%s%.1f\033[0;39m ", a, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        else
        {
            sprintf(pcSendTmp, "%2.2f%s%.1f ", a, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        
        strcat(pcSend, pcSendTmp);
        memset(pcSendTmp, 0, MAXIMAL_BUF_SIZE);

        g_afTmpRate[index] = fTmpRate;

        strcat(pcSend, pcSendTmp);
        memset(pcSendTmp, 0, MAXIMAL_BUF_SIZE);

        if (1 == g_dateError)
        {
            sprintf(pcSendTmp, " Time Error");
            strcat(pcSend, pcSendTmp);
            memset(pcSendTmp, 0, MAXIMAL_BUF_SIZE);
        }
    }
    
    if (0 == g_ucHiddenMsg && 2 < strlen(pcSend))
    {
        strncpy(pcRetStr, pcSend, strlen(pcSend));
        g_afLastAllTradeSum[index] = curAllTradeSum;
        g_afLastPrice[index] = curPrice;
    }
    
    free(pcSend);
    free(pcSendTmp);
    free(pcStockName);

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        free(aPcVal[i]);
    }
}

int initStaticNames()
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
        printf("Warn Maybe Code is Wrong\n");
        return -1;
    }
    return 0;
}

void initOutPutFileName()
{
    struct timeb stTime;
    ftime(&stTime);
    struct tm *pstTime = localtime(&(stTime.time)); 
    sprintf(g_acBakFileNa, "%s%s_%d%02d%02d", 
        BASE_FOLDER,g_acNaList[SPE_DEX], 1900+pstTime->tm_year, pstTime->tm_mon+1, pstTime->tm_mday);
    open(g_acBakFileNa, O_APPEND | O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP);
    
    int fileBakFd = open(g_acBakFileNa, O_RDWR|O_APPEND);//O_APPEND);
    write(fileBakFd, BAK_FILE_MSG, strlen(BAK_FILE_MSG));
    close(fileBakFd);
            
}

int argsCheck (int argc, char **argv)
{
    char aucSpecArgCode[9] = {0};
    
    if (2 <= argc)
    {
        strncpy(aucSpecArgCode, argv[1], 9);
        if (aucSpecArgCode[0] == 's' && aucSpecArgCode[2] >= '0' && aucSpecArgCode[2] <= '9'
            && aucSpecArgCode[7] >= '0' && aucSpecArgCode[7] <= '9')
        {
            strncpy(g_acNaList[SPE_DEX], aucSpecArgCode, 9);
        }
        else
        {
            return 1;
        }
        
        if (3 <= argc)g_ucArgDozeSec = strtof(argv[2], NULL);
    }
    else return 1;

    return 0;
}
void printUsage(char *cmds){printf("Usage: %s CODE [SLP_SEC]\n", cmds);} // TODO

int main (int argc, char **argv)
{
    FILE *out;
    char *eachData = NULL;
    size_t len = 0;
    unsigned char index = 0;
    char *outPutMsg, *outPutMsgTmp;
    
    if (0 == argsCheck(argc, argv))
    {
        if (0 != initStaticNames())
        {
            printUsage(argv[0]);
            return 0;
        }
    }
    else
    {
        printUsage(argv[0]);
        return 0;
    }

    initOutPutFileName();
    debug_printf("SleepSec is %d Sec Code=[%s]\n", g_ucArgDozeSec, g_acNaList[SPE_DEX]);
    
    outPutMsg = (char *)malloc(HUGE_BUF_SIZE);
    outPutMsgTmp = (char *)malloc(BIG_BUF_SIZE);

    while (0 == g_outOfTimeFlag)
    {
        g_ucHiddenMsg = 0;
        g_dateError = 0;
        memset(outPutMsg, 0, HUGE_BUF_SIZE);

        g_uc1stCodeFromData = 1;

        out = fopen(OUT_PUT, "r");
        if (NULL != out)
        {
            index = 0;            
            while (getline(&eachData, &len, out) != -1)
            {
                if (strlen(eachData) > EACH_REALTIME_DATA_MINIMAL_LEN)
                {
                    if (NULL != eachData)
                    {
                        memset(outPutMsgTmp, 0, BIG_BUF_SIZE);
                        analysia_each_stk(eachData, index, outPutMsgTmp);
                        g_uc1stCodeFromData = 0;

                        if (0 == g_ucHiddenMsg && NULL != outPutMsgTmp && 2 < strlen(outPutMsgTmp))
                        {
                            strcat(outPutMsg, outPutMsgTmp);
                        }                        
                    }
                }
                index++;
            }    
        }
        
        if (out)
        {
            fclose(out);
        }
        
        if (0 == g_ucHiddenMsg && NULL != outPutMsg && 2 < strlen(outPutMsg))
        {        
            int ptsFd = open(FILE_OUTP, O_RDWR);
            write(ptsFd, outPutMsg, strlen(outPutMsg));
            
            int fileBakFd = open(g_acBakFileNa, O_RDWR|O_APPEND);//O_APPEND);
            if (0 < fileBakFd)
            {
                write(fileBakFd, outPutMsg, strlen(outPutMsg));
            }
            close(ptsFd);
            close(fileBakFd);
        }
    }
    printf("OUT OF TIME\n");
    free(outPutMsg);free(outPutMsgTmp);
    exit(EXIT_SUCCESS); 
    return 0;
}

