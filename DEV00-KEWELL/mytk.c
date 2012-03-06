#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

/*
var hq_str_sz0="||||,9.57,9.54,9.79,9.84,9.43,9.79,9.80,73317197,711198597.82,642866,9.79,339019,9.78,134400,9.77,122800,9.76,129200,9.75,974067,9.80,372836,9.81,566180,9.82,851896,9.83,1095996,9.84,2012-02-24,15:06:00";

0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  9---SUM  10---Money  
       9.57,  9.54,       9.79, 9.84,  9.43, 9.79,  9.80,  73317197,711198597.82,
642866,9.79,339019,9.78,134400,9.77,122800,9.76,129200,9.75,974067,9.80,372836,9.81,566180,9.82,851896,9.83,1095996,9.84,
*/

#define HEADS_LEN           16
#define NAMES_LEN           3
#define ALL_DATA_LEN        9
#define OUT_PUT "/tmp/.data2.list"
#define EACH_MAX_LEN        20
#define WAIT_SEC            30
#define FILE_LENGTH         10

unsigned char g_ucDebug = 0;
unsigned char g_ucLineCnt = 0;
unsigned char g_ucMaxCnt = 10;

void analysia_each_stk (char *pcData, char *pcFileName, float fAlarmRate)
{
    char *pcName, *pcTmp, *pcNext;
    char *aPcVal[ALL_DATA_LEN];
    float val[ALL_DATA_LEN];
    int pts = -1;int ret = -1;
    char * pcSend;
    int i = 0, len = 0;

    pcName = malloc(NAMES_LEN);

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        aPcVal[i] = malloc(EACH_MAX_LEN);
        memset(aPcVal[i], 0, EACH_MAX_LEN);
        val[i] = 0.0;
    }

    strncpy(pcName, pcData + HEADS_LEN, NAMES_LEN);

    i = 0;
    while (i < ALL_DATA_LEN && (pcTmp = strchr(pcData, ',') + 1) != NULL)
    {
        pcNext = strchr(pcTmp, ',');
        len = pcNext - pcTmp;
        
        strncpy(aPcVal[i], pcTmp, len);

        if (8 == i)
        {
            if (NULL != strchr(aPcVal[i], '.'))
            {
                len -= 3;
            }
            if (0 == strcmp(pcName, "001"))
            {
                len -= 4;
            }
            memset(aPcVal[i], 0, EACH_MAX_LEN);
            strncpy(aPcVal[i], pcTmp, len - 4);
        }

        pcData = pcTmp;

        if (i < 5)
            val[i] = strtof(aPcVal[i], NULL);

        i++;
    }

    //0-open 1-yester 2-n 3-h 4-l 8-S
    float fTmpRate = ((val[2] / val[1]) - 1) * 100;
    if(1 == g_ucDebug)
    {
        printf("%s %-5.2f,%-5.2f,%-5.2f %s %s %s %s\n", 
            pcName, 
            fTmpRate,
            ((val[3] / val[1]) - 1) * 100,
            ((val[4] / val[1]) - 1) * 100,
            aPcVal[8],
            aPcVal[2],
            aPcVal[3],
            aPcVal[4]);
    }

    g_ucLineCnt++;

    if((FILE_LENGTH == strlen(pcFileName)) && (fTmpRate > fAlarmRate || fTmpRate < (0 - fAlarmRate)))
    {
        pts = open(pcFileName, O_RDWR);
        if(0 < pts)
        {
            pcSend = (char *)malloc(EACH_MAX_LEN);
            memset(pcSend, 0, EACH_MAX_LEN);
            //sprintf(pcSend, "%s:%.1f%s", pcName, fTmpRate, (g_ucMaxCnt == g_ucLineCnt) ? "\n" : "|");
            sprintf(pcSend, "%s:%.1f|", pcName, fTmpRate);

            ret = write(pts, pcSend, strlen(pcSend));
            
            free(pcSend);
            close(pts);
        }
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
    float fAlarmRate = 3.0;
    int iRunCnt = 1;

    if (3 == argc)
    {
        fAlarmRate = strtof(argv[2], NULL);

        if (fAlarmRate > 6.0)
        {
            fAlarmRate = 3.0;
        }

        if (FILE_LENGTH == strlen(argv[1]))
        {
            iRunCnt= 840; //7hours, 2times/sec
        }
        else
        {
            g_ucDebug = 1;
        }
    }
    else
    {
        printf("hello world\n");
        return 0;
    }

    while(iRunCnt > 0)
    {
        iRunCnt--;

        system("wget -q -O /tmp/.data2.list -i /var/www/icons/.README.list");
        out = fopen(OUT_PUT, "r");

        if(NULL != out)
        {
            while (getline(&eachData, &len, out) != -1)
            {
                analysia_each_stk(eachData, argv[1], fAlarmRate);
            }
        }

        if(out)
          fclose(out);

        if (iRunCnt > 0)
            sleep(WAIT_SEC);
    }
    exit(EXIT_SUCCESS);
}
