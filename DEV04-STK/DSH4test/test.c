/*********************************************************************************
 *      Copyright:  (C) 2015 KEWELL
 *
 *       Filename:  test.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/02/2015~)
 *         Author:  KEWELL <EMBLINUXEDU@163.COM>
 *      ChangeLog:  1, Release initial version on "11/02/2015 02:23:45 PM"
 *                  2,
 *                 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 ********************************************************************************/

unsigned char avaliableNames[10][9]; // 0 1 2 is STATIC
#define  SPEC_CODE_INDEX     3

void initStaticNames()
{
    strncpy(avaliableNames[0], "sh000001", 9);
    strncpy(avaliableNames[1], "sz399001", 9);
    strncpy(avaliableNames[2], "sz399006", 9);
}


unsigned char checkCodeName(char *pcStockName)
{
    int i = 0;
    int arraryLen = sizeof(avaliableNames) / sizeof(avaliableNames[0]);
printf("%d\n", arraryLen);
    for (i = 0; i < arraryLen; i++)
    {
        printf("%s, %s\n", pcStockName, avaliableNames[i]);
        if (0 == strcmp(pcStockName, avaliableNames[i]))
        {
printf("OK\n");
            return 0;
        }
    }
    return 1;
}

/********************************************************************************
 *  Description:
 *   Input Args:
 *  Output Args:
 * Return Value:
 ********************************************************************************/
int main (int argc, char **argv)
{
    char acArrTest[124];
    printf("len=%d sizeof=%d\n", strlen(argv[1]), sizeof(argv[1]));
    printf("len=%d sizeof=%d\n", strlen(acArrTest), sizeof(acArrTest));
initStaticNames();
float f1 = 1.2345, f2=1.2345;
if (f1 == f2)printf("error\n");
if (f1 == f2)printf("error\n");
if (f1 == f2)printf("error\n");
if (f1 == f2)printf("error\n");
    checkCodeName(argv[1]);
    return 0;
} /* ----- End of main() ----- */

