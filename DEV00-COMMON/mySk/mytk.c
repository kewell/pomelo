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
#include <math.h>
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
    char *pcM, *pcC, *pcN;
    char *line = NULL;
    char *pcOrig;
    size_t len = 0;

    //char *p1, *p2, *p3, *pcBf1, *pcBf2;
    char *pcNa = "0.01 1.57 8.93 8.97 3.08 3.62 5.35 8.66 8.84 6.68<br>";
    float M[STK_CNT], C[STK_CNT], N[STK_CNT], T[STK_CNT];
    int i = 0, write_flag = 0, wait;
    wait = 60 / argc;

    for (i = 0; i < STK_CNT; i++)
    {
        M[i] = -9.9;
        N[i] = 9.9;
        C[i] = 0.0;
        T[i] = 0.0;
    }

    //pcBf2 = malloc(STK_EACH);
    pcM = malloc(ALL_STK_LEN);
    pcC = malloc(ALL_STK_LEN);
    pcN = malloc(ALL_STK_LEN);
    pcOrig = malloc(STATIC_LEN);

    while (1)
    {
        system("wget -q -i /var/www/redmine/.lru -O /var/www/redmine/.tmp");
        system("rm -rf /var/www/redmine/.pmt;for i in `cat /var/www/redmine/.tmp`;do echo $i | awk -F ',' '{ if( $6 )print $4}' >> /var/www/redmine/.pmt;done;rm -rf /var/www/redmine/.tmp");

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
            memset(pcC, 0, ALL_STK_LEN);
            memset(pcM, 0, ALL_STK_LEN);
            memset(pcN, 0, ALL_STK_LEN);
#if 0
            memset(pcBf2, 0, STK_EACH);
            p1 = strchr(line, ',');p1++;
            p2 = strchr(p1, ',');p2++;
            p3 = strchr(p2, ',');p3++;
            pcBf1 = strchr(p3, ',');

            strncpy(pcBf2, p3, (pcBf1 - p3));
            T[i] = strtof(pcBf2, NULL);
#endif
            T[i] = strtof(line, NULL);

            if (fabs(C[i] - T[i]) > 0.0001)
            {
                write_flag = 1;
                C[i] = T[i];

                if (C[i] > M[i])
                {
                    printf("cur[%d]=%-6.2f > Max[%d]=%-6.2f\n", i, C[i], i, M[i]);
                    M[i] = C[i];
                }
                else if (C[i] < N[i])
                {
                    printf("cur[%d]=%-6.2f < Nin[%d]=%-6.2f\n", i, C[i], i, N[i]);
                    N[i] = C[i];
                }
            }
            i++;
        }

        if (write_flag)
        {
            printf("-----------------------------\n");
            sprintf(pcM, "%-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f<br>", M[0], M[1], M[2], M[3], M[4], M[5], M[6], M[7], M[8], M[9]);
            sprintf(pcC, "%-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f<br>", C[0], C[1], C[2], C[3], C[4], C[5], C[6], C[7], C[8], C[9]);
            sprintf(pcN, "%-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f %-4.1f<br>", N[0], N[1], N[2], N[3], N[4], N[5], N[6], N[7], N[8], N[9]);

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
    
    //if(pcBf2)free(pcBf2);
    if(pcM)free(pcM);if(pcC)free(pcC);if(pcN)free(pcN);
    if(pcOrig)free(pcOrig);
    if(line)free(line);

    exit(EXIT_SUCCESS);
}
