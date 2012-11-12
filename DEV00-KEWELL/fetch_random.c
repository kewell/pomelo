/*********************************************************************************
 *      Copyright:  (C) 2012 KEWELL
 *
 *       Filename:  fetch_random.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(09/28/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "09/28/2012 02:21:00 PM"
 *                  2,
 *                 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 ********************************************************************************/
#ifndef WIN32
#define LINUX
// gcc fetch_random.c  -I/usr/include/mysql/ -L/usr/lib/mysql -l mysqlclient

#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include <mysql.h>

#ifdef LINUX
#include <unistd.h>
#endif

#define SEED            100
#define MAX_LOOP        100

#define BUF_S_SIZE      32
#define BUF_M_SIZE      128
#define BUF_L_SIZE      256

#define RE_SIZE_RATE    100

#ifdef WIN32
#pragma comment(lib, "libmysql")
#pragma comment(lib, "mysqlclient")
#endif

MYSQL *conn_ptr;

int random_number(int min, int max);
int get_time_list(time_t startTime, time_t endTime, int gapSec);
int setRangeByHour(int *min, int *max, int hour);
int generate_random(int min, int max, int seed);
int fetch_demand_random(struct tm *stExactTime);
int mysql_term();
int mysql_exec(char *sqlCmd);
int mysql_check_status(char *sqlCmd);
int mysql_connect();

#if defined (WIN32)
/* 从min和max中返回一个随机值 */
int random_number(int min, int max)
{
     int res = 0, range = 0;

     range = max - min + 1;
     srand((unsigned)time(NULL) + rand());
     //srand(range);
     res = rand() % (max - min + 1) + min;

     return res;
}

#elif defined (LINUX)

int random_number(int min, int max)
{
    static int dev_random_fd = -1;
    char *next_random_byte;
    int bytes_to_read;
    unsigned random_value;
    
    assert(max > min);
    
    if (dev_random_fd == -1)
    {
        dev_random_fd = open("/dev/urandom", O_RDONLY);
        assert(dev_random_fd != -1);
    }
    
    next_random_byte = (char *)&random_value;
    bytes_to_read = sizeof(random_value);
    
    /* 因为是从/dev/random中读取，read可能会被阻塞，一次读取可能只能得到一个字节，
     * 循环是为了让我们读取足够的字节数来填充random_value.
     */
    do
    {
        int bytes_read;
        bytes_read = read(dev_random_fd, next_random_byte, bytes_to_read);
        bytes_to_read -= bytes_read;
        next_random_byte += bytes_read;
    }while(bytes_to_read > 0);
    
    return min + (random_value % (max - min + 1));
}

#endif

int get_time_list(time_t startTime, time_t endTime, int gapSec)
{
    struct tm *stTime;
    char *pcSqlCmd;
    float fRandomVal;
    int i;

    pcSqlCmd = malloc(BUF_M_SIZE);

    if (0 >= startTime)
    {
        time(&startTime);
        endTime = startTime + gapSec * 10;
    }

    if (endTime < startTime)
    {
        return -1;
    }

#if 0
    struct tm {
        int tm_sec;         /* seconds */
        int tm_min;         /* minutes */
        int tm_hour;        /* hours */
        int tm_mday;        /* day of the month */
        int tm_mon;         /* month */
        int tm_year;        /* year */
        int tm_wday;        /* day of the week */
        int tm_yday;        /* day in the year */
        int tm_isdst;       /* daylight saving time */
    };

    time_t mktime(struct tm *tm);

#endif

    stTime = localtime(&startTime);

    mysql_connect();
    mysql_check_status("select * from meter_history");

    for (i = 0; endTime >= startTime; i++)
    {
        stTime = localtime(&startTime);
        startTime += gapSec;

        // str: insert into meter_history values(002, '2012-11-11 11:11:11', 2)
        // (002, '2012-11-11 11:11:11', 2)
        fRandomVal += ((float)fetch_demand_random(stTime) / RE_SIZE_RATE);

        memset(pcSqlCmd, 0 ,BUF_M_SIZE);

        sprintf(pcSqlCmd, "insert into meter_history values(%03d, '%04d-%02d-%02d %02d:%02d:%02d', %.2f)",
                i,
                stTime->tm_year + ((stTime->tm_year >= 70) ? 1900 : 2000),
                stTime->tm_mon + 1, stTime->tm_mday, 
                stTime->tm_hour, stTime->tm_min, stTime->tm_sec,
                fRandomVal);

        printf("%s\n", pcSqlCmd);
        mysql_exec(pcSqlCmd);
    }

    mysql_term();

    if(pcSqlCmd)
    {
        free(pcSqlCmd);
    }

    return 0;
}

int setRangeByHour(int *min, int *max, int hour)
{
    if (6 >= hour) // XL
    {
        *min = 5;
        *max = 15;
    }
    else if (8 >= hour || 21 <= hour) // L
    {
        *min = 15;
        *max = 30;
    }
    else if (12 >= hour || (14 <= hour && 18 >= hour)) // XH
    {
        *min = 80;
        *max = 100;
    }
    else if (14 >= hour || (18 <= hour && 21 >= hour)) // H
    {
        *min = 50;
        *max = 80;
    }

    return 0;
}

int generate_random(int min, int max, int seed)
{
    int range = 0;

    if (min > max)
    {
        return min;
    }

    range = max - min + 1;
    srand(seed);
    return (rand() % range + min);
}

/*
 * Su Mo Tu We Th Fr Sa
 * 1  2  3  4  5  6  7
 * 
 *      MIN  MAX  SEED
 * XH   80   100
 * H    50   80
 * M    30   50
 * L    15   30
 * XL   5    15
 *
 * 00-06 06-08 08-12 12-14 14-18 18-21 21-24
 *   XL    L     XH    H     XH    H     L   (WEEKDAY)
 *   XL    XL    L     L     L     XL    XL  (WEEKEND)
 */
int fetch_demand_random(struct tm *stExactTime)
{
    //int wday = stExactTime->tm_wday; /* 0-6  */
    //unsigned char ucWeekendFlag = 0;
    //unsigned char ucRushTie = 0;
    int min, max;
    int hour = stExactTime->tm_hour; /* 0-23 */

    //if (wday > 4) /* wday =[0|6], means weekend */

    setRangeByHour(&min, &max, hour);
    //return generate_random(min, max, time(NULL));
    return random_number(min, max);
}

int mysql_term()
{
    mysql_close(conn_ptr);
    return EXIT_SUCCESS;
}

int mysql_check_status(char *sqlCmd)
{
    MYSQL_RES *res_ptr;
    MYSQL_ROW sqlrow;
    //MYSQL_FIELD *fd;
    int res = 0, i = 0, j = 0;

    if (conn_ptr) 
    {
        res = mysql_query(conn_ptr, sqlCmd); //查询语句
        if (res) 
        {       
            printf("SELECT error:%s\n",mysql_error(conn_ptr));   
        } 
        else 
        {      
            res_ptr = mysql_store_result(conn_ptr);             //取出结果集
            if(res_ptr) 
            {             
                printf("%lu Rows\n",(unsigned long)mysql_num_rows(res_ptr)); 
                j = mysql_num_fields(res_ptr);        
                while((sqlrow = mysql_fetch_row(res_ptr)))  
                {   //依次取出记录
                    for(i = 0; i < j; i++)       
                        printf("%s\t", sqlrow[i]);              //输出
                    printf("\n");        
                }            
                if (mysql_errno(conn_ptr)) 
                {                    
                    fprintf(stderr, "Retrive error:%s\n", mysql_error(conn_ptr));             
                }      
            }      
            mysql_free_result(res_ptr);      
        }
    } 
    else 
    {
        printf("Connection failed\n");
    }

    return EXIT_SUCCESS;
}

int mysql_exec(char *sqlCmd)
{
    int res;

    if (conn_ptr) 
    {
        //可以把insert语句替换成delete或者update语句，都一样的
        //res = mysql_query(conn_ptr, "delete from user where name = 'Ann' and age < 10");
        //res = mysql_query(conn_ptr, "insert into meter_history values(002, '2012-11-11 11:11:11', 2)");
        res = mysql_query(conn_ptr, sqlCmd);

        if (!res) {     //输出受影响的行数
            printf("Inserted %lu rows\n",(unsigned long)mysql_affected_rows(conn_ptr)); 
        }  else {       //打印出错误代码及详细信息
            fprintf(stderr, "Insert error %d: %sn", mysql_errno(conn_ptr), mysql_error(conn_ptr));
        }
    } else {
        printf("Connection failed\n");
    }

    return EXIT_SUCCESS;
}

int mysql_connect()
{
    //MYSQL_RES *res_ptr;
    //MYSQL_ROW sqlrow;
    //MYSQL_FIELD *fd;
    //int res, i, j;

    conn_ptr = mysql_init(NULL);
    if (!conn_ptr) {
        printf("mysql_init failed\n");
        return EXIT_FAILURE;
    }

    //MYSQL *STDCALL mysql_real_connect(MYSQL *mysql, const char *host, 
    // const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag); 
    conn_ptr = mysql_real_connect(conn_ptr, "192.168.200.188", "mysql", "", "test", 0, NULL, 0);


    if (!conn_ptr) {
        printf("Connection failed\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main (int argc, char **argv)
{
    //get_time_list(1331111222, 1382111222, 2000000);
    printf("------------------------------------------\n");
    get_time_list(0, 0, 3600);

    return 0;
}

