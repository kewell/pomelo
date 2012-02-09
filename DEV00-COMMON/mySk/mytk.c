/*********************************************************************************
 *                  All rights reserved.
 *
 *       Filename:  mytk.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(02/08/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "02/08/2012 03:57:59 PM"
 *http://59.175.132.51/list=s_sh000001,s_sz000157,s_sz000893,s_sz000897,s_sh600308,s_sh600362,s_sh600535,s_sh600866,s_sh600884,s_sh601668                 
 *http://hq.sinajs.cn/list=s_sh000001,s_sz000157,s_sz000893,s_sz000897,s_sh600308,s_sh600362,s_sh600535,s_sh600866,s_sh600884,s_sh601668                 
 ********************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#define OUT_PUT "/var/www/html/c-hotkeys.html"
#define FL_TMP "/var/www/redmine/.pmt"
#define STATIC_LEN  9216
#define STK_CNT   10
#define STK_EACH   10
#define ALL_STK_LEN (STK_CNT * STK_EACH)

int main (int argc, char **argv)
{
    FILE *in, *out;
    char *pcBf1, *pcBf2;
    char *pcM, *pcC, *pcN;
    char *line = NULL;
    char *pcOrig;
    size_t len = 0;

    char *p1, *p2, *p3;
    char *pcNa = "0.01 1.57 8.93 8.97 3.08 3.62 5.35 8.66 8.84 6.68<br>";
    float M[STK_CNT], C[STK_CNT], N[STK_CNT];
    int i = 0, write_flag = 0, wait;
    wait = 60 / argc;

    for (i = 0; i < STK_CNT; i++)
    {
        M[i] = -11.0;
        N[i] = 11.0;
        C[i] = 0;
    }

    pcBf2 = malloc(STK_EACH);
    pcM = malloc(ALL_STK_LEN);
    pcC = malloc(ALL_STK_LEN);
    pcN = malloc(ALL_STK_LEN);
    pcOrig = malloc(STATIC_LEN);

    while (1)
    {
        system("wget -q -i /var/www/redmine/.lru -O /var/www/redmine/.pmt");
        in = fopen(FL_TMP, "r");
        if(NULL == in)
        {
            //exit(EXIT_FAILURE);
            sleep(wait);
            continue;
        }

        i = 0;
        write_flag = 0;

        while (getline(&line, &len, in) != -1)
        {
            memset(pcBf2, 0, STK_EACH);
            memset(pcC, 0, ALL_STK_LEN);
            memset(pcM, 0, ALL_STK_LEN);
            memset(pcN, 0, ALL_STK_LEN);

            p1 = strchr(line, ',');p1++;
            p2 = strchr(p1, ',');p2++;
            p3 = strchr(p2, ',');p3++;
            pcBf1 = strchr(p3, ',');

            strncpy(pcBf2, p3, (pcBf1 - p3));
            C[i] = strtof(pcBf2, NULL);

            if (C[i] > M[i])
            {
                //printf("cur[%d]=%.2f  >  Max[%d]=%.2f\n", i, C[i], i, M[i]);
                M[i] = C[i];
                write_flag = 1;
            }
            else if (C[i] < N[i])
            {
                //printf("cur[%d]=%.2f  <  Nin[%d]=%.2f\n", i, C[i], i, N[i]);
                N[i] = C[i];
                write_flag = 1;
            }
            i++;
        }

        if (write_flag)
        {
            sprintf(pcM, "%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f<br>", M[0], M[1], M[2], M[3], M[4], M[5], M[6], M[7], M[8], M[9]);
            sprintf(pcC, "%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f<br>", C[0], C[1], C[2], C[3], C[4], C[5], C[6], C[7], C[8], C[9]);
            sprintf(pcN, "%.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f<br>", N[0], N[1], N[2], N[3], N[4], N[5], N[6], N[7], N[8], N[9]);

            out = fopen(OUT_PUT, "r");
            fread(pcOrig, 1, STATIC_LEN, out);
            if(out)fclose(out);

            out = fopen(OUT_PUT, "w+");

            //fseek(out, 0, SEEK_SET);

            fwrite(pcOrig, 1, STATIC_LEN, out);
            fwrite(pcNa, 1, strlen(pcNa), out);
            fwrite(pcN, 1, strlen(pcN), out);
            fwrite(pcM, 1, strlen(pcM), out);
            fwrite(pcC, 1, strlen(pcC), out);

            if(out)fclose(out);

        }

        if(in)fclose(in);
        sleep(wait);
    }
    
    if(pcBf2)free(pcBf2);
    if(pcM)free(pcM);if(pcC)free(pcC);if(pcN)free(pcN);
    if(pcOrig)free(pcOrig);
    if(line)free(line);

    exit(EXIT_SUCCESS);
}
