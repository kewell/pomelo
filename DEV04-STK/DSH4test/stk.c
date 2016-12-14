
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
//#define OUT_PUT             "/root/.STK/.stk.data"
#define OUT_PUT             "./.stk.data"
#define BAK_FILE_MSG        "指数  卖5  卖4  卖3  卖2  卖1|  买1  买2  买3  买4  买5| 量    价 涨幅"

#define LOOP_TIME           1000
#define BASE_FOLDER         "/.100G/.KEWELL/stkDailyDetail"
#define FILE_OUTP           "/dev/pts/2"
#define ASCII_0_VAL         0x30

int   g_iArgSlp = 15;
float g_fArgTinyDeal = 10.0;
float g_fTinyPrice = 0.02;
int   g_iArgGapCnt = 15;

#define DEAL_CNT_BIG        400
#define REQT_CNT_BIG        999

#define EACH_REALTIME_DATA_MINIMAL_LEN 100

#define BUF_SIZE_32B        32
#define BUF_SIZE_128B       128
#define BUF_SIZE_256B       256
#define BUF_SIZE_512B       512

int g_ucZhiShuType = 0;
const char g_ucZhiShuCode[4][9] = {"sh000001", "sz399001", "sz399005", "sz399006"};
const char g_ucBakFileMsg[4][150] = {
"上证  卖5  卖4  卖3  卖2  卖1|  买1  买2  买3  买4  买5|成交数/价 涨幅",
"深成  卖5  卖4  卖3  卖2  卖1|  买1  买2  买3  买4  买5|成交数/价 涨幅",
"中小  卖5  卖4  卖3  卖2  卖1|  买1  买2  买3  买4  买5|成交数/价 涨幅",
"创业  卖5  卖4  卖3  卖2  卖1|  买1  买2  买3  买4  买5|成交数/价 涨幅"
};

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
struct tm *g_pstTime = NULL;
/*
0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  8---SUM  9---Money  
"NAME, 16.87, 16.90,      17.13,17.42, 16.70,17.12, 17.13, 9327671, 159983824.30,

B: 10   11    12    13     14    15   16     17    18   19     S: 20  21     22    23     24   25    26    27   28    29    30         31   32
15523,17.12, 16479,17.11, 39691,17.10,10600,17.09,13400,17.08, 200,17.13,  7000,17.14,  29800,17.15,9800,17.16,11924,17.17, 2014-05-22,10:30:00,00"
*/

int checkCodeName(char *pcStockName)
{
    int i = 0;
    //debug_printf("code=[%s]\n", pcStockName);
    for (i = 0; i < AV_NAME_LEN; i++)
    {
        if (0 == strncmp(pcStockName, g_acNaList[i], NAMES_LEN))
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
            || (9==h && 29<=m)
            || (11==h && 30>=m) 
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

    char acStkName[NAMES_LEN], *pcTmp, *pcNext;
    char aacVal[ALL_DATA_LEN][BUF_SIZE_32B];
    
    float val[ALL_DATA_LEN];
    char acBuf[BUF_SIZE_256B], acBufTmp[BUF_SIZE_256B];
    int i = 0, len = 0;
    int isArgCode = 0; // 成分指数=0，普通Code=1

    /* ---------------------------------------initialization, initialize --------------------------------*/
    memset(acStkName, 0, NAMES_LEN);
    if(NULL != (pcData + HEADS_LEN) && strlen(pcData + HEADS_LEN) > NAMES_LEN)
        strncpy(acStkName, pcData + HEADS_LEN, NAMES_LEN);
    else
        goto CleanUp;;

    //debug_printf("code=[%s]\n", acStkName);
    isArgCode = checkCodeName(acStkName);
    if (-1 == isArgCode)
    {
        goto CleanUp;;
    }

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        memset(aacVal[i], 0, BUF_SIZE_32B);
        val[i] = 0.0;
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
            strncpy(aacVal[i], pcTmp, len);
        else
        {
            printf("Line %d error here\n", __LINE__);
            goto CleanUp;;
        }

        //debug_printf("val[%d]=%s\n", i, aacVal[i]);
        //val[i] = __strtof_internal(aacVal[i], NULL, 0);
        val[i] = strtof(aacVal[i], NULL);
        
        pcData = pcTmp;
        i++;
    }

    /* aacVal[30] is time strings */
    char *pMin = NULL, *pSec;
    char acHour[3] = {0,0,'\0'}, acMin[3] = {0,0,'\0'}, acSec[3] = {0,0,'\0'};
    
    strncpy(acHour, aacVal[30], 2);
    pMin = strstr(aacVal[30], ":");
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
        debug_printf("will hidden %d l_sec=%02d sec=%02d for [%s]\n", __LINE__, g_lastSec, sec, acStkName);
        g_ucHiddenMsg = 1;
    }

    /* if (!isArgCode && 0 != min && g_lastMin > min)g_dateError = 1; */
    
    float curAllTradeSum = (val[7]/100), curTradeSum = curAllTradeSum - g_afLastAllTradeSum[index];
    float curPrice = val[2], lastClosePrice = val[1];
    float fTmpRate = (curPrice - lastClosePrice) * 100 / lastClosePrice;
    float subPrice = fabs(curPrice - g_afLastPrice[index]);    

    if (0)//if (isArgCode && (fTmpRate > 9.98 || fTmpRate < -9.98))
    {
        g_ucHiddenMsg = 1;
    }
    
    if (1 == g_ucHiddenMsg)
    {
        goto CleanUp;
    }
    
    memset(acBuf, 0, sizeof(acBuf)*sizeof(char));    
    memset(acBufTmp, 0, sizeof(acBuf)*sizeof(char));
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
        sprintf(acBuf, "\n%s%s%.1f", aacVal[30], (fTmpRate > 0.0) ? "+" : "", fTmpRate);
    }
    else if (isArgCode && !isLargeCnt && g_fTinyPrice  > subPrice && g_fArgTinyDeal > curTradeSum)
    {
        g_ucHiddenMsg = 1;
        debug_printf("will hidden %d l_sec=%02d sec=%02d Sub=%.0f N=%.0f L=%.0f\n", __LINE__, g_lastSec, sec, curTradeSum, curAllTradeSum, g_afLastAllTradeSum[index]);
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
            if (1 == bigReqFlag[i]) sprintf(acBufTmp, "\033[1;33m%5.0f\033[0;39m", val[i]/100);
            else sprintf(acBufTmp, "%5.0f", val[i]/100);

            strcat(acBuf, acBufTmp);
            memset(acBufTmp, 0, BUF_SIZE_256B);
        }
        strcat(acBuf, "|");
        for (i = 9; i <= 17; i += 2)
        {
            if (1 == bigReqFlag[i]) sprintf(acBufTmp, "\033[1;33m%5.0f\033[0;39m", val[i]/100);
            else sprintf(acBufTmp, "%5.0f", val[i]/100);

            strcat(acBuf, acBufTmp);
            memset(acBufTmp, 0, BUF_SIZE_256B);
        }
        strcat(acBuf, "|");
        
        /*---------------------------------SHOW REAL DEAL CNT---------------------------------*/
        /* Big trade sum in this duration, will use colorful putput */
        if (isLargeCnt)
        {
            /* If current price is up use green color, is down use red, other wise use yellow color */
            if (curPrice < g_afLastPrice[index])
            {
                sprintf(acBufTmp, "\033[1;31m%-5.0f\033[0;39m", curTradeSum);
            }
            else if (curPrice > g_afLastPrice[index])
            {
                sprintf(acBufTmp, "\033[1;34m%-5.0f\033[0;39m", curTradeSum);
            }
            else
            {
                sprintf(acBufTmp, "\033[1;33m%-5.0f\033[0;39m", curTradeSum);
            }
        }
        else
        {
            sprintf(acBufTmp, "%-5.0f", curTradeSum);
        }
        strcat(acBuf, acBufTmp);
        memset(acBufTmp, 0, BUF_SIZE_256B);

        /*---------------------------------SHOW Curr Price AND RATE AND TIME ---------------------------------*/
        //float a,c;int b = 0;a = curPrice;b = (int)(int)(a+1.0e-6);//b = (int)a;c = (a - b)*100;

        if (fTmpRate > g_afTmpRate[index]) // Blue
        {
            sprintf(acBufTmp, "%2.2f\033[1;34m%s%.1f\033[0;39m ", curPrice, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        else if (fTmpRate < g_afTmpRate[index]) // Red
        {
            sprintf(acBufTmp, "%2.2f\033[1;31m%s%.1f\033[0;39m ", curPrice, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        else
        {
            sprintf(acBufTmp, "%2.2f%s%.1f ", curPrice, ((fTmpRate >= 0.0) ? "+" : ""), fTmpRate);
        }
        
        strcat(acBuf, acBufTmp);
        memset(acBufTmp, 0, BUF_SIZE_256B);

        g_afTmpRate[index] = fTmpRate;

        strcat(acBuf, acBufTmp);
        memset(acBufTmp, 0, BUF_SIZE_256B);

        if (1 == g_dateError)
        {
            sprintf(acBufTmp, " Time Error");
            strcat(acBuf, acBufTmp);
            memset(acBufTmp, 0, BUF_SIZE_256B);
        }
    }

    if (0 == g_ucHiddenMsg)// && 2 < strlen(acBuf))
    {
        strncpy(pcRetStr, acBuf, strlen(acBuf));
        
        if (isArgCode)
        {
            g_afLastAllTradeSum[index] = curAllTradeSum;
            g_afLastPrice[index] = curPrice;
            g_lastMin = min;
            g_lastSec = sec;
        }
    }
CleanUp:
    return;
}

int initStaticNames()
{
    int ret = 0;
    if ('h'==g_acNaList[SPE_DEX][1] && '6'==g_acNaList[SPE_DEX][2])
    {
        g_ucZhiShuType = 0;//strncpy(g_acNaList[0], "sh000001", 9);
    }
    
    else if ('z'==g_acNaList[SPE_DEX][1])
    {
        if ('3'==g_acNaList[SPE_DEX][2])
            g_ucZhiShuType = 3;//strncpy(g_acNaList[0], "sz399006", 9);
        else if ('0'==g_acNaList[SPE_DEX][2] && '0'==g_acNaList[SPE_DEX][3])
        {
            if ('2'<=g_acNaList[SPE_DEX][4])
                g_ucZhiShuType = 2;//strncpy(g_acNaList[0], "sz399005", 9);
            else
                g_ucZhiShuType = 1;//strncpy(g_acNaList[0], "sz399001", 9);
        }   
    }
    else
    {
        g_ucZhiShuType = 0;//strncpy(g_acNaList[0], "sh000001", 9);        
        printf("Warn Maybe Code is Wrong\n");
        ret = -1;
    }

    strncpy(g_acNaList[0], g_ucZhiShuCode[g_ucZhiShuType], 9);
    return ret;
}

int initOutPutFileName()
{
    struct timeb stTime;
    ftime(&stTime);
    g_pstTime = localtime(&(stTime.time));
#ifdef __TEST__
    return 0;
#else
    if (15 <= g_pstTime->tm_hour || 0 == g_pstTime->tm_wday || 6 == g_pstTime->tm_wday)return -1;
#endif

    //sprintf(g_acBakFileNa, "%s%s_%d%02d%02d", BASE_FOLDER,g_acNaList[SPE_DEX], 1900+g_pstTime->tm_year, g_pstTime->tm_mon+1, g_pstTime->tm_mday);
    sprintf(g_acBakFileNa, "%s%s", BASE_FOLDER,g_acNaList[SPE_DEX]);//, 1900+g_pstTime->tm_year, g_pstTime->tm_mon+1, g_pstTime->tm_mday);
    int fd = open(g_acBakFileNa, O_APPEND | O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IWGRP);
    if (fd > 0)close(fd);

    /* 尽量一天只写一次头信息 */
    if (9 >= g_pstTime->tm_hour && 29 >= g_pstTime->tm_min)
    {
        char *pcTopFileMsg;
        pcTopFileMsg=malloc(BUF_SIZE_256B);
        sprintf(pcTopFileMsg, "\n%02d-%02d-%02d", g_pstTime->tm_year-100, g_pstTime->tm_mon+1, g_pstTime->tm_mday);
        strcat(pcTopFileMsg, g_ucBakFileMsg[g_ucZhiShuType]);//strcat(pcTopFileMsg, BAK_FILE_MSG);
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
        if (6 <= argc)g_iArgGapCnt = atoi(argv[5]);
    }
    else return 1;

    return 0;
}
void printUsage(char *cmds){printf("Usage: %s CODE [SLP_SEC 30] [TinyDealCnt 100] [TinyDealPrice 0.02] [GapCnt 15]\n", cmds);} // TODO

int main (int argc, char **argv)
{
    FILE *out;
    char *eachData = NULL;
    size_t len = 0;
    unsigned char index = 0;
    char outPutMsg[BUF_SIZE_256B], outPutMsgTmp[BUF_SIZE_256B];
    int analysiaCnt = 0;
    
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
    debug_printf("code[0]=[%s], code[1]=[%s]\n", g_acNaList[0], g_acNaList[1]);
    
#ifdef __TEST__
    printf("SleepSec is %d Sec g_fArgTinyDeal=%.0f Code=[%s] g_iArgGapCnt=%d\n", g_iArgSlp, g_fArgTinyDeal, g_acNaList[SPE_DEX], g_iArgGapCnt);
#endif
    
    if (0 != initOutPutFileName())return 0;

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
            if (0 == analysiaCnt % g_iArgGapCnt)
            {
                printf("\n%02d-%02d-%02d%s", g_pstTime->tm_year-100, g_pstTime->tm_mon+1, g_pstTime->tm_mday, g_ucBakFileMsg[g_ucZhiShuType]);
            }
            
            printf("%s", outPutMsg);
#else
            int bakFd = open(g_acBakFileNa, O_RDWR|O_APPEND);//O_APPEND);
            if (0 < bakFd)
            {
                write(bakFd, outPutMsg, strlen(outPutMsg));
                close(bakFd);
            }
#endif
            analysiaCnt++;
        }
        fflush(stdout); /* Add by KEWELL 2015-12-03 17:12:59 */
        sleep(g_iArgSlp);
    }while (0 == g_outOfTimeFlag && 1);

    printf("OUT OF TIME\n");
    exit(EXIT_SUCCESS);return 0;
}

