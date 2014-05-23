#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>
/*
var hq_str_sz0=""

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

#define HEADS_LEN           16
#define NAMES_LEN           3
#define ALL_DATA_LEN        30 
#define OUT_PUT "/tmp/.data2.list"
#define EACH_MAX_LEN        20
#define WAIT_SEC            8
#define LOOP_TIME           240
#define FILE_OUTP           "/dev/pts/1"
#define USEFUL_ID           0 
#define DEBUG_FLAG          "--outa"
#define DEBUG_ALL           "--d"

unsigned char g_ucDebug = 0;
unsigned char g_ucLineCnt = 0;
unsigned char g_ucMaxCnt = 10;
float g_fLastDeal = 0;
unsigned char g_ucIsSZA = 1;
unsigned char g_ucDebugAll = 0;

void analysia_each_stk (char *pcData, float fAlarmRate, unsigned char isFirst)
{
#ifdef __DEBUG__
    printf("%s", pcData);
#endif

    char *pcName, *pcTmp, *pcNext;
    char *aPcVal[ALL_DATA_LEN];
    float val[ALL_DATA_LEN];
    int pts = -1;int ret = -1;
    char *pcSend;
    int i = 0, len = 0;

    pcName = malloc(NAMES_LEN);

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        aPcVal[i] = malloc(EACH_MAX_LEN);
        memset(aPcVal[i], 0, EACH_MAX_LEN);
        val[i] = 0.0;
    }

    if(NULL != (pcData + HEADS_LEN) && strlen(pcData + HEADS_LEN) > NAMES_LEN)
        strncpy(pcName, pcData + HEADS_LEN, NAMES_LEN);
    else
        return;

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

#ifdef __DEBUG__
        printf("val[%d]=%s   ||| ", i, aPcVal[i]);
#endif
        if (5 > i || (i >= 9 && 1 == i % 2))
        {
            //val[i] = __strtof_internal(aPcVal[i], NULL, 0);
            val[i] = strtof(aPcVal[i], NULL);
        }

        if (g_ucIsSZA && 8 == i)
        {
            memset(aPcVal[i], 0, EACH_MAX_LEN);

            if (NULL != pcTmp && strlen(pcTmp) > (len -7))
                strncpy(aPcVal[i], pcTmp, len - 7);
            else
                return;

            //val[i] = __strtof_internal(aPcVal[i], NULL, 0);
            val[i] = strtof(aPcVal[i], NULL);
        }

        pcData = pcTmp;
        i++;
    }
#ifdef __DEBUG__
    printf("\n");

    printf("%s =%f, =%f, =%f ,=%f, =%f\n\n", pcName, val[2], val[1],  (val[2] - val[1]), (val[2] - val[1]) * 100, (val[2] - val[1]) * 100 / val[1]);
#endif

    //0-open 1-yester 2-n 3-h 4-l 8-S
    float fTmpRate = (val[2] - val[1]) * 100 / val[1];
    float fAllBuy = (val[9] + val[11] + val[13] + val[15] + val[17]);
    float fAllSell = (val[19] + val[21] + val[23] + val[25] + val[27]);

    if(1 == g_ucDebug)
    {
        printf("%s %5.2f,%5.2f,%5.2f %-5.5s %-5.5s %-5.5s__%.0f/%.0f/%.0f/%.0f/%.0f__%.0f/%.0f/%.0f/%.0f/%.0f__%-2.0f\n", 
                pcName, 
                fTmpRate,
                ((val[3] - val[1]) * 100 / val[1]),
                ((val[4] - val[1]) * 100 / val[1]),
                aPcVal[2],
                aPcVal[3],
                aPcVal[4],
                val[27]/100, val[25]/100, val[23]/100, val[21]/100, val[19]/100,  
                val[9]/100, val[11]/100, val[13]/100, val[15]/100, val[17]/100,
                (g_ucIsSZA) ? val[8] : ((fAllBuy - fAllSell)/100)
              );
    }

    g_ucLineCnt++;

    if(g_ucDebugAll) 
    {
        pts = open(FILE_OUTP, O_RDWR);

        if(0 < pts)
        {
            pcSend = (char *)malloc(EACH_MAX_LEN);
            memset(pcSend, 0, EACH_MAX_LEN);

            if (g_ucIsSZA)
            {
                sprintf(pcSend, "\n%2.0f%s%.1f", val[8] - (isFirst) ? val[8] : g_fLastDeal, (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
            }
            else
            {
                float fRequestSub = (fAllBuy - fAllSell) / 100;

                if (abs((int)fRequestSub >= 10000))
                {
                    fRequestSub=9999.0;
                }

                if(isFirst)
                {
                    sprintf(pcSend, " %s", pcName);
                }
                else
                {
                    if (abs((int)fRequestSub) > 0)
                    {
                        sprintf(pcSend, "%s%.1f[%s%-4d]", (fTmpRate >= 0.0) ? "+" : "", fTmpRate, (fAllBuy > fAllSell) ? "+" : "-", abs((int)fRequestSub));
                    }
                    else
                    {
                        sprintf(pcSend, "%s%.1f", (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
                    }
                }
            }

            ret = write(pts, pcSend, strlen(pcSend));

            free(pcSend);
            close(pts);
        }
    }
    else if(!g_ucDebug && (g_ucIsSZA || fTmpRate > fAlarmRate || fTmpRate < (0 - fAlarmRate)))
    {
        pts = open(FILE_OUTP, O_RDWR);

        if(0 < pts)
        {
            pcSend = (char *)malloc(EACH_MAX_LEN);
            memset(pcSend, 0, EACH_MAX_LEN);
#if 0
            if (g_ucIsSZA)
            {
                sprintf(pcSend, "\n%.0f:%.1f|", val[8] - g_fLastDeal, fTmpRate);
            }
            else
            {
                sprintf(pcSend, "%s:%.1f|", pcName, fTmpRate);
            }
#endif
            if (g_ucIsSZA)
            {
                sprintf(pcSend, "\n%2.0f:%s%.1f|", val[8] - g_fLastDeal, (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
            }
            else
            {
                sprintf(pcSend, "%s:%s%.1f|", pcName, (fTmpRate >= 0.0) ? "+" : "", fTmpRate);
            }
            ret = write(pts, pcSend, strlen(pcSend));

            free(pcSend);
            close(pts);
        }
    }

    if (g_ucIsSZA)
    {
        g_fLastDeal = val[8];
    }

    if (g_ucMaxCnt == g_ucLineCnt)
    {
        g_ucLineCnt = 0;
    }

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        free(aPcVal[i]);
    }
    free(pcName);
}

int main (int argc, char **argv)
{
    FILE *out;
    char *eachData = NULL;
    size_t len = 0;
    float fAlarmRate = 12.0;
    int iRunCnt = 1;
    unsigned char isFirst = 1;

    //if (2 != argc || USEFUL_ID != getuid() || USEFUL_ID != geteuid() || 0 != strcmp(FILE_OUTP, ttyname(0)))
    if (2 != argc || 0 != strcmp(FILE_OUTP, ttyname(0)))
    {
        printf ("---------------KEWELL--------------- : %s %s() +%d\n", __FILE__, __func__, __LINE__);
        goto CleanUp;
    }

    if (2 == argc)
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
        else if (0 == strcmp(argv[1], DEBUG_FLAG))
        {
            g_ucDebug = 1;
        }
        else if (0 == strcmp(argv[1], DEBUG_ALL))
        {
            g_ucDebugAll = 1;
            iRunCnt= LOOP_TIME;
        }
        else
        {
            printf ("---------------KEWELL--------------- : %s %s() +%d\n", __FILE__, __func__, __LINE__);
            goto CleanUp;
        }
    }

    while (iRunCnt > 0)
    {
        iRunCnt--;

        system("wget -q -O /tmp/.data2.list -i /var/www/icons/.README.list");
        out = fopen(OUT_PUT, "r");

        if (NULL != out)
        {
            while (getline(&eachData, &len, out) != -1)
            {
                if (g_ucIsSZA || strlen(eachData) > 180)
                {
                    if (NULL != eachData)
                    {
                        analysia_each_stk(eachData, fAlarmRate, isFirst);
                        g_ucIsSZA = 0;
                    }
                }
            }    
        }

        if (1 == isFirst)
        {
            isFirst = 0;
        }

        if (out)
        {
            fclose(out);
            //system("rm -rf /tmp/.data2.list");
        }

        if (iRunCnt > 0)
        {
            sleep(WAIT_SEC);
            g_ucIsSZA = 1;
        }
    }
    exit(EXIT_SUCCESS);

    printf ("---------------KEWELL--------------- : %s %s() +%d\n", __FILE__, __func__, __LINE__);
CleanUp:
    system("rm -rf /tmp/.data2.list");
    printf("hello world\n");
    return 0;
}
