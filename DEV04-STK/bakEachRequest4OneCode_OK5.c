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
#define OUT_PUT             "/dev/.stk.data"
#define BAK_FILE_MSG        "指数  卖5  卖4  卖3  卖2  卖1|  买1  买2  买3  买4  买5| 量    价 涨幅"

#define LOOP_TIME           1000
#define BASE_FOLDER         "/.100G/.KEWELL/.STK/"
#define FILE_OUTP           "/dev/pts/2"
#define ASCII_0_VAL         0x30

int   g_iArgSlp = 30;
float g_fArgTinyDeal = 100.0;
float g_fTinyPrice = 0.02;

#define DEAL_CNT_BIG        500
#define REQT_CNT_BIG        999

#define EACH_REALTIME_DATA_MINIMAL_LEN 100

#define BUF_SIZE_32B        32
#define BUF_SIZE_128B       128
#define BUF_SIZE_256B       256
#define BUF_SIZE_512B       512

float g_afLastAllTradeSum[100] = {0};
float g_afLastPrice[100] = {0.0};
float g_afTmpRate[100] = {0.0};

char g_acBakFileNa[BUF_SIZE_32B];
unsigned char g_ucHiddenMsg = 0;
char g_acNaList[2][9]; // 0 is STATIC
#define AV_NAME_LEN         (sizeof(g_acNaList) / sizeof(g_acNaList[0]))
#define SPE_DEX             (AV_NAME_LEN - 1)
int g_lastMin = 0, g_lastSec = 0;
int g_dateError = 0;
int g_outOfTimeFlag = 0;

/*
0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  8---SUM  9---Money  
"NAME, 16.87, 16.90,      17.13,17.42, 16.70,17.12, 17.13, 9327671, 159983824.30,

B: 10   11    12    13     14    15   16     17    18   19     S: 20  21     22    23     24   25    26    27   28    29    30         31   32
15523,17.12, 16479,17.11, 39691,17.10,10600,17.09,13400,17.08, 200,17.13,  7000,17.14,  29800,17.15,9800,17.16,11924,17.17, 2014-05-22,10:30:00,00"
*/

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
    if (1==g_ucHiddenMsg)return;

    char *pcStockName, *pcTmp, *pcNext;
    char *aPcVal[ALL_DATA_LEN];
    
    float val[ALL_DATA_LEN];
    char *pcSend, *pcSendTmp;
    int i = 0, len = 0;
    int isArgCode = 0; // 成分指数=0，普通Code=1

    /* ---------------------------------------initialization, initialize --------------------------------*/
    pcStockName = malloc(NAMES_LEN);

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        aPcVal[i] = malloc(BUF_SIZE_32B);
        memset(aPcVal[i], 0, BUF_SIZE_32B);
        val[i] = 0.0;
    }

    if(NULL != (pcData + HEADS_LEN) && strlen(pcData + HEADS_LEN) > NAMES_LEN)
        strncpy(pcStockName, pcData + HEADS_LEN, NAMES_LEN);
    else
        goto CleanUp;;

    isArgCode = checkCodeName(pcStockName);
    if (-1 == isArgCode)
    {
        goto CleanUp;;
    }

    if (SPE_DEX <= isArgCode)
    {
        isArgCode = 1;
    }
    else
    {
        isArgCode = 0;
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
            goto CleanUp;;
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
        goto CleanUp;
    }

    if (!isArgCode && g_lastSec == sec)
    {
        debug_printf("will hidden %d l_sec=%02d sec=%02d for [%s]\n", __LINE__, g_lastSec, sec, pcStockName);
        g_ucHiddenMsg = 1;
    }

    if (!isArgCode && 0 != min && g_lastMin > min)
    {
        g_dateError = 1;
    }
    
    float curAllTradeSum = (val[7]/100), curTradeSum = curAllTradeSum - g_afLastAllTradeSum[index];
    float curPrice = val[2], lastClosePrice = val[1];
    float fTmpRate = (curPrice - lastClosePrice) * 100 / lastClosePrice;
    float subPrice = fabs(curPrice - g_afLastPrice[index]);    

    if (isArgCode && (fTmpRate > 9.98 || fTmpRate < -9.98))
    {
    debug_printf("myCode Rate=%.2f\n",fTmpRate);
        g_ucHiddenMsg = 1;
    }
    
    if (1 == g_ucHiddenMsg)
    {
        goto CleanUp;
    }
    
    pcSend = (char *)malloc(BUF_SIZE_256B);
    memset(pcSend, 0, BUF_SIZE_256B);
    pcSendTmp = (char *)malloc(BUF_SIZE_256B);
    memset(pcSendTmp, 0, BUF_SIZE_256B);
    unsigned char isLargeCnt = 0;
    unsigned char bigReqFlag[ALL_DATA_LEN] = {0};
    
    if (isArgCode)
    {
        for (i = 27; i >= 9; i -= 2)
        {
            if (val[i]/100 > REQT_CNT_BIG)
                bigReqFlag[i] = 1;
        }
        
        if (curTradeSum > DEAL_CNT_BIG)
            isLargeCnt = 1;
    }
    
    if (!isArgCode)
    {
        sprintf(pcSend, "\n%s%s%.1f", aPcVal[30], (fTmpRate > 0.0) ? "+" : "", fTmpRate);
    }
    else if (isArgCode && !isLargeCnt && g_fTinyPrice  > subPrice && g_fArgTinyDeal > curTradeSum)
    {
        g_ucHiddenMsg = 1;
        debug_printf("will hidden at l_sec=%02d sec=%02d\n", g_lastSec, sec);
    }
    else
    {
/*  9 #define YELLOW               "\e[1;33m"
    10 #define BLUE                 "\e[0;34m"
    11 #define L_BLUE               "\e[1;34m"
    12 #define PURPLE               "\e[0;35m"
    13 #define L_PURPLE             "\e[1;35m"
    14 #define CYAN                 "\e[0;36m"
    15 #define L_CYAN               "\e[1;36m"
    16 #define GRAY                 "\e[0;37m"
    17 #define WHITE                "\e[1;37m"
    18 
    19 #define BOLD                 "\e[1m"    */

        // 普通的CODE只输出B/S 请求数量和成交的数量
        /*---------------------------------SHOW REAL REQEST S/B CNT---------------------------------*/
        for (i = 27; i >= 19; i -= 2)
        {
            if (1 == bigReqFlag[i]) sprintf(pcSendTmp, "\033[1;33m%5.0f\033[0;39m", val[i]/100);
            else sprintf(pcSendTmp, "%5.0f", val[i]/100);

            strcat(pcSend, pcSendTmp);
            memset(pcSendTmp, 0, BUF_SIZE_256B);
        }
        strcat(pcSend, "|");
        for (i = 9; i <= 17; i += 2)
        {
            if (1 == bigReqFlag[i]) sprintf(pcSendTmp, "\033[1;33m%5.0f\033[0;39m", val[i]/100);
            else sprintf(pcSendTmp, "%5.0f", val[i]/100);

            strcat(pcSend, pcSendTmp);
            memset(pcSendTmp, 0, BUF_SIZE_256B);
        }
        strcat(pcSend, "|");
        
        /*---------------------------------SHOW REAL DEAL CNT---------------------------------*/
        /* Big trade sum in this duration, will use colorful putput */
        if (isLargeCnt)
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
        memset(pcSendTmp, 0, BUF_SIZE_256B);

        /*---------------------------------SHOW Curr Price AND RATE AND TIME ---------------------------------*/
        //float a,c;int b = 0;a = curPrice;b = (int)(int)(a+1.0e-6);//b = (int)a;c = (a - b)*100;

        if (fTmpRate > g_afTmpRate[index]) // Blue
        {
            sprintf(pcSendTmp, "%2.2f\033[1;34m%s%.1f\033[0;39m ", curPrice, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        else if (fTmpRate < g_afTmpRate[index]) // Red
        {
            sprintf(pcSendTmp, "%2.2f\033[1;31m%s%.1f\033[0;39m ", curPrice, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        else
        {
            sprintf(pcSendTmp, "%2.2f%s%.1f ", curPrice, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        
        strcat(pcSend, pcSendTmp);
        memset(pcSendTmp, 0, BUF_SIZE_256B);

        g_afTmpRate[index] = fTmpRate;

        strcat(pcSend, pcSendTmp);
        memset(pcSendTmp, 0, BUF_SIZE_256B);

        if (1 == g_dateError)
        {
            sprintf(pcSendTmp, " Time Error");
            strcat(pcSend, pcSendTmp);
            memset(pcSendTmp, 0, BUF_SIZE_256B);
        }
    }

    if (0 == g_ucHiddenMsg)// && 2 < strlen(pcSend))
    {
        strncpy(pcRetStr, pcSend, strlen(pcSend));
        g_afLastAllTradeSum[index] = curAllTradeSum;
        g_afLastPrice[index] = curPrice;

        g_lastMin = min;
        g_lastSec = sec;
        debug_printf("will hidden %d l_sec=%02d sec=%02d for [%s]\n", __LINE__, g_lastSec, sec, pcStockName);
    }
    
    free(pcSend);
    free(pcSendTmp);

CleanUp:
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

int initOutPutFileName()
{
    struct timeb stTime;
    ftime(&stTime);
    struct tm *pstTime = localtime(&(stTime.time));
#ifdef __TEST__
    return 0;
#else
    if (15 <= pstTime->tm_hour || 0 == pstTime->tm_wday || 6 == pstTime->tm_wday)return -1;
#endif

    //sprintf(g_acBakFileNa, "%s%s_%d%02d%02d", BASE_FOLDER,g_acNaList[SPE_DEX], 1900+pstTime->tm_year, pstTime->tm_mon+1, pstTime->tm_mday);
    sprintf(g_acBakFileNa, "%s%s", BASE_FOLDER,g_acNaList[SPE_DEX]);//, 1900+pstTime->tm_year, pstTime->tm_mon+1, pstTime->tm_mday);
    int fd = open(g_acBakFileNa, O_APPEND | O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP);
    if (fd > 0)close(fd);

    /* 尽量一天只写一次头信息 */
    if (9 >= pstTime->tm_hour && 29 >= pstTime->tm_min)
    {
        char *pcTopFileMsg;
        pcTopFileMsg=malloc(BUF_SIZE_256B);
        sprintf(pcTopFileMsg, "\n%02d-%02d-%02d", pstTime->tm_year-100, pstTime->tm_mon+1, pstTime->tm_mday);
        strcat(pcTopFileMsg, BAK_FILE_MSG);

        fd = open(g_acBakFileNa, O_RDWR|O_APPEND);
        write(fd, pcTopFileMsg, strlen(pcTopFileMsg));
        close(fd);
        free(pcTopFileMsg);
    }
    return 0;
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
        
        if (3 <= argc)g_iArgSlp = atoi(argv[2]);
        if (4 <= argc)g_fArgTinyDeal = strtof(argv[3], NULL);
        if (5 <= argc)g_fTinyPrice = strtof(argv[4], NULL);
    }
    else return 1;

    return 0;
}
void printUsage(char *cmds){printf("Usage: %s CODE [SLP_SEC 30] [TinyDealCnt 100] [TinyDealPrice 0.02]\n", cmds);} // TODO

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

    if (0 != initOutPutFileName())return 0;
    
#ifdef __TEST__
    printf("SleepSec is %d Sec g_fArgTinyDeal=%.0f Code=[%s]\n", g_iArgSlp, g_fArgTinyDeal, g_acNaList[SPE_DEX]);
#endif
    
    outPutMsg = (char *)malloc(BUF_SIZE_256B);
    outPutMsgTmp = (char *)malloc(BUF_SIZE_256B);
    
    do
    {
        g_ucHiddenMsg = 0;
        g_dateError = 0;
        memset(outPutMsg, 0, BUF_SIZE_256B);

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
                        memset(outPutMsgTmp, 0, BUF_SIZE_256B);
                        analysia_each_stk(eachData, index, outPutMsgTmp);

                        if (0 == g_ucHiddenMsg && NULL != outPutMsgTmp && 2 < strlen(outPutMsgTmp))
                        {
                            strcat(outPutMsg, outPutMsgTmp);
                        }                        
                    }
                }
                index++;
            }
            fclose(out);
        }
        
        if (0 == g_ucHiddenMsg && NULL != outPutMsg && 20 < strlen(outPutMsg)) // TODO //11:20:42+4.0
        {
#ifdef __TEST__
            int ptsFd = open(FILE_OUTP, O_RDWR);
            if (0<ptsFd){write(ptsFd, outPutMsg, strlen(outPutMsg));close(ptsFd);}
#else
            int bakFd = open(g_acBakFileNa, O_RDWR|O_APPEND);//O_APPEND);
            if (0 < bakFd)write(bakFd, outPutMsg, strlen(outPutMsg));close(bakFd);
#endif
        }
        sleep(g_iArgSlp);
    }while (0 == g_outOfTimeFlag && 1);

    printf("OUT OF TIME\n");
    free(outPutMsg);free(outPutMsgTmp);
    exit(EXIT_SUCCESS);return 0;
}

