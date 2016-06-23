#include <stdlib.h>
#include <stdio.h>
//#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>

#include <ctype.h>
#include<fstream.h>
//#include <iostream>
//using namespace std;

//#include <stdio.h> 
#include <afxinet.h>

//#include "stdafx.h"
 
/*
var hq_str_sz0="||||,9.57,9.54,9.79,9.84,9.43,9.79,9.80,73317197,711198597.82,642866,9.79,339019,9.78,134400,9.77,122800,9.76,129200,9.75,974067,9.80,372836,9.81,566180,9.82,851896,9.83,1095996,9.84,2012-02-24,15:06:00";

0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  9---SUM  10---Money  
       9.57,  9.54,       9.79, 9.84,  9.43, 9.79,  9.80,  73317197,711198597.82,
642866,9.79,339019,9.78,134400,9.77,122800,9.76,129200,9.75,974067,9.80,372836,9.81,566180,9.82,851896,9.83,1095996,9.84,

var hq_str_sh*="||||,6.92,6.98,6.99,7.00,6.75,6.97,6.98,14783140,101631270,2800,6.97,21400,6.96,43500,6.95,12300,6.94,23100,6.93,500,6.98,34000,6.99,171480,7.00,78300,7.01,60100,7.02,2015-10-30,13:59:01,00";

0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  9---SUM  10---Money  
||||,  6.92,  6.98,       6.99, 7.00,  6.75, 6.97,  6.98,  14783140,101631270,
11-B1 12-B1 13-B2 14-B2  15-B3 16-B3  17-B4 18-B4 19-B5 20-B5  21-S1 22-S1 23-S2 24-S2 25-S3  26-S3 27-S4 28-S4 29-S5 30-S5 31-Time 
2800, 6.97, 21400, 6.96, 43500, 6.95, 12300, 6.94,23100, 6.93, 500,  6.98, 34000,6.99, 171480,7.00, 78300,7.01, 60100,7.02, 2015-10-30,13:59:01,00
*/
#define URL_PATH			"c:\\windows\\.txt.url.txt"
#define HEADS_LEN           16
#define NAMES_LEN           3
#define ALL_DATA_LEN        9
#define OUT_PUT "/tmp/.data2.list"
#define EACH_MAX_LEN        20
#define WAIT_SEC            60
#define LOOP_TIME           120
#define FILE_OUTP           "/dev/pts/1"
#define USEFUL_ID           502
#define DEBUG_FLAG          "--outa"

#define MIN_BUFF_LEN		64
#define MID_BUFF_LEN		256
#define MAX_BUFF_LEN		2048

unsigned char g_ucDebug = 1;/////////////////////////////
unsigned char g_ucLineCnt = 0;
unsigned char g_ucMaxCnt = 10;
float g_fLastDeal = 0;
unsigned char g_ucIsSZA = 1;

void analysia_each_stk (char *pcData, float fAlarmRate)
{
    char *pcName, *pcTmp, *pcNext;
    char *aPcVal[ALL_DATA_LEN];
    float val[ALL_DATA_LEN];
    int pts = -1;int ret = -1;
    //char *pcSend;
    unsigned int i = 0, len = 0;

#ifdef DEBUG
	printf("%s\n", pcData);
	printf("%d\n", __LINE__);
#endif

    pcName = (char *)malloc(NAMES_LEN);

    for (i = 0; i < ALL_DATA_LEN; i++)
    {
        aPcVal[i] = (char *)malloc(EACH_MAX_LEN);
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
            continue;
        }

        len = pcNext - pcTmp;
        
        if (NULL != pcTmp && strlen(pcTmp) > len)
            strncpy(aPcVal[i], pcTmp, len);
        else
            return;

        if (5 > i)
        {
            val[i] = (float)strtod(aPcVal[i], NULL);
        }

        if (g_ucIsSZA && 8 == i)
        {
            memset(aPcVal[i], 0, EACH_MAX_LEN);

            if (NULL != pcTmp && strlen(pcTmp) > (len -7))
                strncpy(aPcVal[i], pcTmp, len - 7);
            else
                return;

            val[i] = (float)strtod(aPcVal[i], NULL);
        }

        pcData = pcTmp;
        i++;
    }

    //0-open 1-yester 2-n 3-h 4-l 8-S
    float fTmpRate = ((val[2] / val[1]) - 1) * 100;
    if(1 == g_ucDebug)
    {
        printf("%s %5.2f,%5.2f,%5.2f %5s %5s %5s %-2.0f\n", 
            pcName, 
            fTmpRate,
            ((val[3] / val[1]) - 1) * 100,
            ((val[4] / val[1]) - 1) * 100,
            aPcVal[2],
            aPcVal[3],
            aPcVal[4],
            (g_ucIsSZA) ? val[8] : 0);
    }

	g_ucLineCnt++;

    if (g_ucIsSZA)
    {
        g_fLastDeal = val[8];
		g_ucIsSZA = 0;
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

int main(int argc, char* argv[]) 
{
    float fAlarmRate = 12.0;
	int sleep = 0;
	
	CInternetSession session("HttpClient");    
	//char *url = "http://hq.sinajs.cn/list=sh000001";
	char url[MID_BUFF_LEN];
    
	fstream f(URL_PATH, ios::in);
	f.getline(url, MID_BUFF_LEN);

	system("time /\T");
	
	CHttpFile* pfile = (CHttpFile *)session.OpenURL(url);
    DWORD dwStatusCode;
    pfile->QueryInfoStatusCode(dwStatusCode); 
    if(dwStatusCode == HTTP_STATUS_OK)
    { 
        CString content;
        CString data;
		char *pcData;
        while (pfile->ReadString(data))
        {
            content += data + "\r\n";
#ifdef DEBUG
			printf(" %s\n " ,(LPCTSTR)data);
#endif			
			pcData = (LPSTR)(LPCTSTR)data;
            analysia_each_stk(pcData, fAlarmRate);
        }
        content.TrimRight(); 
    }

	if (2 == argc && 0 == strcmp("S", argv[1]))
	{
		scanf("%d", &sleep);
	}

    pfile->Close();
    delete pfile; 
    session.Close();	
    return 0 ; 
}
