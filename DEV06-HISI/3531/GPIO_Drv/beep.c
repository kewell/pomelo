/*********************************************************************************
 *      Copyright:  (C) 2013 KEWELL
 *
 *       Filename:  common_drv_tools.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(06/19/13~)
 *         Author:  WENJING <WJ@HIGHEASYRD.COM WENJING0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "06/19/13 16:09:13"
 *                  2,
 *                 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define GPIOGET_SERIAL422       0x102
#define GPIOGET_ALARM_OUT       0x103 //259     //Set Alarm Out
#define GPIOGET_ALARM_IN        0x104 //260     //Set Alarm In
#define GPIOGET_BUZZER          0x107 //263     
#define GPIOGET_WATCHDOG        0x109
#define GPIOGET_SERIAL485       0x10A   
#define GPIOGET_WATCHDOGEX      0x10B
#define GPIOGET_IPSTATUS        0x10D
#define GPIOGET_LEDSTATUS   0x10E //D270
#define GPIOSET_RESETHDMI  0x10F
#define GPIOSET_DETECTVGA  0x210

int fd = -1;

/**************************************************************************************
 *  Description:
 *   Input Args:
 *  Output Args:
 * Return Value:
 *************************************************************************************/
int usage (char *appName)
{
    printf("usage : \n");
    printf("%s DEV_NAME CMD ARGs\n\n", appName);
    printf("ext   :\n");
    printf("%s /dev/hi_gpio 260 0\n", appName);
/*
    printf("---------------------------command list------------------------------\n");
    printf("GPIOGET_ALARM_OUT=%d\n", GPIOGET_ALARM_OUT);
    printf("GPIOGET_ALARM_IN=%d\n", GPIOGET_ALARM_IN);
    printf("GPIOGET_BUZZER=%d\n", GPIOGET_BUZZER);
    printf("GPIOGET_WATCHDOG=%d\n", GPIOGET_WATCHDOG);
    printf("GPIOGET_SERIAL485=%d\n", GPIOGET_SERIAL485);
    printf("GPIOGET_WATCHDOGEX=%d\n", GPIOGET_WATCHDOGEX);
    printf("GPIOGET_IPSTATUS=%d\n", GPIOGET_IPSTATUS);
    printf("GPIOGET_LEDSTATUS=%d\n", GPIOGET_LEDSTATUS);
    printf("GPIOSET_RESETHDMI=%d\n", GPIOSET_RESETHDMI);
    printf("GPIOSET_DETECTVGA=%d\n", GPIOSET_DETECTVGA);
    printf("---------------------------------------------------------------------\n");
*/
    return 0;
} /* ----- End of usage()  ----- */

void set_Rs422_Mode(int command, int pinLevel)
{
    unsigned char ucArg = pinLevel;    
    ioctl(fd, command, &ucArg);
}

void set_Rs485_Mode(int command, int pinLevel)
{
    unsigned char ucArg = pinLevel;    
    ioctl(fd, command, &ucArg);
}

void beep_testing(int command, int cnt, int msec)
{
    unsigned char ucArg = 1;

    int i = 0, gap = 45000;
    for (i = 0; i < cnt; i++)
    {
        ucArg = 1;
        ioctl(fd, command, &ucArg); // Beep High level
        usleep(gap);
        ucArg = 0;
        ioctl(fd, command, &ucArg); // Beep low level
        usleep(gap);
        ucArg = 1;
        ioctl(fd, command, &ucArg); // Beep High level
        usleep(gap);
        ucArg = 0;
        ioctl(fd, command, &ucArg); // Beep low level
        if (i != (cnt -1))
        {
            usleep(msec * 1000);
        }
    }
}

void led_testing(int command)
{
    unsigned char ucArg = 0, i = 0;

    for (i = 0; i < 2; i++) // WE got 2 sys leds
    {
        ucArg = 0;
        ucArg |= i; // Led index setting

        ioctl(fd, command, &ucArg); // Led_i low level
        sleep(1);
        ucArg |= (1 << 7);
        ioctl(fd, command, &ucArg); // Led_i High level
    }
}

void alarm_out_setting(int command, unsigned char alarmOutStatus)
{
    int iRet = -1, i = 0;
    unsigned char ucArg = 0;

    for (i = 0; i < 8; i++)
    {
        ucArg = 0;
        ucArg |= (i);
        ucArg |= (alarmOutStatus << 7);
        
        iRet = ioctl(fd, command, &ucArg);

        if (0 != iRet)
        {
            printf("ioctl command %d got error\n", command);
            return;
        }
        else
        {
            printf("alarm_%02d setted as %s\n", i + 1, (alarmOutStatus == 1) ? "High" : "Low");
        }
    }
}

void alarm_in_detect(int command, unsigned char loopFlag)
{
    int iRet = -1;
    unsigned long ulArg = 0;
    char flag[16] = {0};
    int i = 0, j = 0;
    
    do
    {
        iRet = ioctl(fd, command, &ulArg);

        if (0 != iRet)
        {
            printf("ioctl command %d got error\n", command);
            return;
        }

        for (i = 0; i < 16; i++)
        {
            if (0x01 == ((ulArg >> i) & 0x1))
            {
                flag[i] = 1;
            }
        }

        for (i = 0; i < 16; i++)
        {
            if (flag[i] == 1)
            {
                printf("alarm_%02d detected\n", i + 1);
                flag[i] = 0;
            }
        }
        
        sleep(1);     
    }while(loopFlag);
}

int main (int argc, char **argv)
{
    int iCmd = -1;
    unsigned long ulArg = 0, ucSelect = 0;
    int iRet  = -1;
    unsigned char *pucFileName = "/dev/hi_gpio";

#if 0
    if (3 >= argc)
    {
        usage(argv[0]);
        return -1;
    }

    iCmd = atoi(argv[2]);
    ulArg = (unsigned long)atoi(argv[3]);
    pucFileName = argv[1];
#endif

    fd = open (pucFileName, O_RDWR);

    if (0 > fd)
    {
        printf("open %s failed\n", pucFileName);
        return -1;
    }
#if 0
    printf("---------------------------Test list------------------------------\n");
    printf("0 : led\n");
    printf("1 : Beep\n");
    printf("2 : Set    All Alarm Out as High\n");
    printf("3 : Set    All Alarm Out as Low\n");
    printf("4 : Detect All Alarm In once\n");
    printf("5 : Detect All Alarm In always\n\n");
    printf("6 : Set RS485 Recv mode------r\n");
    printf("7 : Set RS485 Send mode\n");
    printf("8 : Set RS422 Send enable----s\n");
    printf("9 : Set RS422 Send disable\n");
    
    printf("a : Quit\n\n");

Test_again:    
    printf("Please select one number : \n\n\n");
    ucSelect = getchar();
    getchar();
    fflush(0);
    ucSelect -=  0x30; // 0x30 = 0
#endif
    ucSelect = 1;
    switch (ucSelect)
    {
        case 0:
            led_testing(GPIOGET_LEDSTATUS);
            break;
        case 1:
            beep_testing(GPIOGET_BUZZER, atoi(argv[1]), atoi(argv[2]));
            break;
        case 2:
            alarm_out_setting(GPIOGET_ALARM_OUT, 1);
            break;
        case 3:
            alarm_out_setting(GPIOGET_ALARM_OUT, 0);
            break;
        case 4:
            alarm_in_detect(GPIOGET_ALARM_IN, 0);
            break;
        case 5:
            alarm_in_detect(GPIOGET_ALARM_IN, 1);
            break;
        case 6:
            set_Rs485_Mode(GPIOGET_SERIAL485, 0);
            break;
        case 7:
            set_Rs485_Mode(GPIOGET_SERIAL485, 1);
            break;
        case 8:
            set_Rs422_Mode(GPIOGET_SERIAL422, 1);
            break;
        case 9:
            set_Rs422_Mode(GPIOGET_SERIAL422, 0);
            break;
        case 'a':
            return -1;
        default:
            printf("select error\n\n\n");
            break;
    }
    //goto Test_again;
    
    close(fd);
    
    return iRet;
} /* ----- End of main() ----- */

