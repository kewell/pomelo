/*********************************************************************************
 *      Copyright:  (C) 2012 R&D of San Fran Electronics Co., LTD  
 *                  All rights reserved.
 *
 *       Filename:  lcd.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(02/04/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "02/04/2012 04:31:54 PM"
 *                 
 ********************************************************************************/
#include <time.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lcd.h"
#define EACH_FONT_SIZE  256
#define FONT_COUNT      5
#define ALL_DATA_LEN    EACH_FONT_SIZE * FONT_COUNT

typedef unsigned char U8;

int show_time(unsigned char * pucTimerString)
{
    int fd = -1, ret = 0;
    DIS_FMT *fmt;

    fd = open("/dev/lcd", O_RDWR);

    if (fd < 0)
    {
        perror("Open lcd failure\n");
        return -1;
    }

    fmt = malloc(sizeof(DIS_FMT));
    
    fmt->ucX= 0;
    fmt->ucY= 0;
    fmt->ucHeight = 64;
    fmt->ucWidth = 32;
    fmt->ucGapX= 0;
    fmt->ucGapY = 0;

    fmt->pucData = pucTimerString;

    ret = write(fd, fmt, sizeof(DIS_FMT) + 5 * EACH_FONT_SIZE);

    fmt->ucY= 96;
    ret = write(fd, fmt, sizeof(DIS_FMT) + 5 * EACH_FONT_SIZE);

    if (ret < 0)
    {
        perror("Write falure");
    }

    close(fd);

    return 0;
}

int main (int argc, char **argv)
{
    struct timeb stTb;
    struct tm *pstTm;
    int i, iDataIndex, iHour, iMin;
    unsigned char aucTime[ALL_DATA_LEN];
    unsigned char aucNumber[4];
    
    /* 
     * 1    2   :   0   0   :   0   0
     * |    |   |   |   |   |   |   |
     * 0    1   2   3   4   5   6   7
     *
     */
    int j = 0;

    do
    {
        ftime(&stTb);
        pstTm = localtime(&(stTb.time));

        if (iMin != pstTm->tm_min)
        {
            iMin = pstTm->tm_min;
            aucNumber[2] = iMin / 10;
            aucNumber[3] = iMin % 10;

            iDataIndex = EACH_FONT_SIZE * 2;
        }

        if (iHour != pstTm->tm_hour)
        {
            iHour =  pstTm->tm_hour;
            aucNumber[0] = iHour / 10;
            aucNumber[1] = iHour % 10;

            iDataIndex = ALL_DATA_LEN;
        }

        for (i = 0; i < iDataIndex; i++)
        {
            if (0 == i % EACH_FONT_SIZE)
            {
                j = 0;
            }

            if (i >= 4 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_32_EN[EACH_FONT_SIZE * aucNumber[0] + j];
            }
            else if (i >= 3 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_32_EN[EACH_FONT_SIZE * aucNumber[1] + j];
            }
            else if (i >= 2 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_32_EN[EACH_FONT_SIZE * 10 + j];
            }
            else if (i >= 1 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_32_EN[EACH_FONT_SIZE * aucNumber[2] + j];
            }
            else
            {
                aucTime[i] = width_32_EN[EACH_FONT_SIZE * aucNumber[3] + j];
            }

            j++;
        }

        if (iDataIndex > 0)
        {
            show_time(aucTime);
            iDataIndex = 0;
        }

        sleep(10);
    }while (1);

    return 0;
} /* ----- End of main() ----- */

