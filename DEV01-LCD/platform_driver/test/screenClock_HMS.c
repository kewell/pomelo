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

#define MODE_POLL       0x01
#define MODE_NORMAL     0x02
#define EACH_FONT_SIZE  120

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
    fmt->ucY= 100;
    fmt->ucHeight = 40;
    fmt->ucWidth = 20;
    fmt->ucGapX= 0;
    fmt->ucGapY = 0;

    fmt->pucData = pucTimerString;

    ret = write(fd, fmt, sizeof(DIS_FMT) + 120 * 8);

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
    int i, iHour, iMin, iSec, count = 8;
    int loop = EACH_FONT_SIZE * count;
    unsigned char aucTime[loop];
    unsigned char aucNumber[6];
    
    /* 
     * 1    2   :   0   0   :   0   0
     * |    |   |   |   |   |   |   |
     * 0    1   2   3   4   5   6   7
     *
     */
    int j = 0;
    for (i = 0; i < loop; i++)
    {

        if ((i >= 2 * EACH_FONT_SIZE && i < 3 * EACH_FONT_SIZE) || (i >= 5* EACH_FONT_SIZE && i < 6 * EACH_FONT_SIZE))
        {
            if (i == 5 * EACH_FONT_SIZE)
                j = 0;

            aucTime[i] = width_20_EN[EACH_FONT_SIZE * 10 + j];
            j++;
        }
        else
        {
            aucTime[i] = 0;
        }

    }

    do
    {
        ftime(&stTb);
        pstTm = localtime(&(stTb.time));

        iHour =  pstTm->tm_hour;
        iMin = pstTm->tm_min;
        iSec = pstTm->tm_sec;

        aucNumber[0] = iHour / 10;
        aucNumber[1] = iHour % 10;
        aucNumber[2] = iMin / 10;
        aucNumber[3] = iMin % 10;
        aucNumber[4] = iSec / 10;
        aucNumber[5] = iSec % 10;

        j = 0;
        for (i = 0; i < loop; i++)
        {
            if (0 == i % EACH_FONT_SIZE)
            {
                j = 0;
            }

            if ((i >= 2 * EACH_FONT_SIZE && i < 3 * EACH_FONT_SIZE) || (i >= 5* EACH_FONT_SIZE && i < 6 * EACH_FONT_SIZE))
            {
                aucTime[i] = width_20_EN[EACH_FONT_SIZE * 10 + j];
                j++;
            }
            else if (i >= 7 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_20_EN[EACH_FONT_SIZE * aucNumber[0] + j];
                j++;
            }
            else if (i >= 6 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_20_EN[EACH_FONT_SIZE * aucNumber[1] + j];
                j++;
            }
            else if (i >= 4 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_20_EN[EACH_FONT_SIZE * aucNumber[2] + j];
                j++;
            }
            else if (i >= 3 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_20_EN[EACH_FONT_SIZE * aucNumber[3] + j];
                j++;
            }
            else if (i >= 1 * EACH_FONT_SIZE)
            {
                aucTime[i] = width_20_EN[EACH_FONT_SIZE * aucNumber[4] + j];
                j++;
            }
            else
            {
                aucTime[i] = width_20_EN[EACH_FONT_SIZE * aucNumber[5] + j];
                j++;
            }

        }

        sleep(1);

        show_time(aucTime);

    }while (1);

    return 0;
} /* ----- End of main() ----- */

