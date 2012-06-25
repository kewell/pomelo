/*********************************************************************************
 *      Copyright:  (C) 2012 R&D of San Fran Electronics Co., LTD  
 *                  All rights reserved.
 *
 *       Filename:  sudoku.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(06/21/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "06/21/2012 01:57:53 PM"
 *                 
 ********************************************************************************/
#define EACH_PIX    9
//typedef int u8;
typedef unsigned char u8;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

u8 debug = 0;
#define PRINT(format,args...) if(1 == debug){printf(format, ##args);} 
#define COMPARE_FLAG    7
#define MAX_FAILED    100

u8 data[EACH_PIX][EACH_PIX] = {
#if 1 
    {9,0,5,0,0,0,0,0,8},
    {4,0,0,5,7,0,1,0,6},
    {0,2,7,6,0,0,0,4,0},
    {0,9,6,0,0,3,5,1,2},
    {7,0,4,0,1,0,3,0,0},
    {2,1,0,9,8,0,0,0,4},
    {0,8,1,0,0,4,0,9,0},
    {3,0,0,8,0,0,0,5,1},
    {0,0,2,0,0,7,0,6,0},
#else
    {0,0,6,5,0,0,0,0,0},
    {8,0,9,1,0,4,0,7,0},
    {0,0,3,0,0,0,2,4,0},
    {0,8,7,0,0,3,0,6,5},
    {0,9,0,7,0,0,0,1,0},
    {4,0,1,0,0,9,7,8,0},
    {0,6,0,0,8,0,1,0,7},
    {9,7,2,3,0,1,6,0,0},
    {0,1,8,0,6,0,4,3,0},
#endif
};

u8 data_compare[EACH_PIX][EACH_PIX] = {0};
u8 data_col[EACH_PIX][EACH_PIX] = {0};
int giFailCnt = 0;

u8 aZeroNum[EACH_PIX] = {1};
u8 SUM = 0;

u8 finish();
void reset_val(u8 *data_new, u8 index, u8 isCol);
void clean_global();

u8 less_zero_check(u8 isCol, u8 index, u8 *aucIndex, u8 maxCount, u8 pcDataCol[EACH_PIX][EACH_PIX], u8 pcDataRow[EACH_PIX][EACH_PIX]);
u8 contains_num(u8 *d_data, u8 *d, u8 arraySize);
u8 zero_1(u8 isCol, u8 index, u8 aucIndex[1], u8 pcDataCol[EACH_PIX][EACH_PIX], u8 pcDataRow[EACH_PIX][EACH_PIX]);
u8 sorts(u8);
void init_data(u8);
void print_result(u8);
u8 check();
u8 check_uniq(u8*);
u8 contains_nums(u8 *d_data, u8 *d, u8 arraySize, u8 unSelect);
u8 contains_one_num(u8 *d_data, u8 val);

u8 check_uniq(u8 *check_data)
{
    u8 i,j,k;
    u8 aucCheck[EACH_PIX] = {0};

    for (i = 1; i < EACH_PIX + 1; i++)
    {
        for (j = 0; j < EACH_PIX; j++)
        {
            if (i == check_data[j] || EACH_PIX < check_data[j])
            {
                aucCheck[i - 1]++;
            }
        }
    }
    
    for (i = 0; i < EACH_PIX; i++)
    {
        if (aucCheck[i] > 1)
        {
            return aucCheck[i];
        }
    }

    return 0;
}

u8 check()
{
    u8 i,j,k;
    u8 ret = 100;
    
    for (i = 0; i < EACH_PIX; i++)
    {
        if (check_uniq(data[i]) > 0)
        {
            ret = i;
            break;
        }
        else if (check_uniq(data_col[i]) > 0)
        {
            ret = 10 + i;
            break;
        }
    }
    return ret;
}

void init_data(u8 input)
{
    u8 i, j, k, isCol = 0;
    u8 ret = 0;

    unsigned char *pcInputData;
    pcInputData = malloc(EACH_PIX + 100);

    u8 data_new[EACH_PIX] = {0};

    for (i = 0; i < EACH_PIX; i++)
    {
        for (j = 0; j < EACH_PIX; j++)
        {
            pcInputData[j] = EACH_PIX + 100;
        }

        if (input)
        {
            printf("Line %d :" , i + 1);
            gets(pcInputData);
        }

        for (j = 0; j < EACH_PIX; j++)
        {
            if (input)
            {
                data_compare[i][j] = data[i][j] = data_col[j][i] = pcInputData[j] - 48;
            }
            else
            {
                data_compare[i][j] = data[i][j]; 
                data_col[j][i] = data[i][j]; 
            }
        }
    }

    if (input)
    {
        ret = check();

        while (100 != ret)
        {
            if (ret > 9)
            {
                isCol = 1;
                ret -= 10;
            }

            printf("Please retype %s [%d] :\n", isCol ? "COL" : "ROW", ret + 1);

            gets(pcInputData);
            reset_val(pcInputData, ret, isCol);
            ret = check();
        }
    }

    free(pcInputData);
    printf("\n");

    return;
}

void reset_val(u8 *data_new, u8 index, u8 isCol)
{
    int i = 0;

    if (isCol)
    {
        for (i = 0; i < EACH_PIX; i++)
        {
            data_col[index][i] = data[i][index] = data_compare[i][index] = data_new[i] - 48;
        }
    }
    else
    {
        for (i = 0; i < EACH_PIX; i++)
        {
            data_compare[index][i] = data[index][i] = data_col[i][index] = data_new[i] - 48;
        }
    }

    return;
}

u8 sorts(u8 isCol)
{
    u8 ret = 1, i, j, k;
    u8 *empty_index;
    empty_index = malloc(sizeof(u8) * EACH_PIX);


    for (i = 0; i < EACH_PIX; i++)
    {
        aZeroNum[i] = 0;

        for (j = 0; j < EACH_PIX; j++)
        {
            if (0 == ((isCol) ? data_col[i][j] : data[i][j]))
            {
                empty_index[aZeroNum[i]] = j;
                aZeroNum[i]++;
            }
        }

        if (1 == aZeroNum[i] && (0 == zero_1(isCol, i, empty_index, data_col, data)))
        {
            ret = 0;
            goto Clean;
        }
        else if (COMPARE_FLAG > aZeroNum[i])
        {
            if (0 == less_zero_check(isCol, i, empty_index, aZeroNum[i], data_col, data))
            {
                ret = 0;
                goto Clean;
            }
        }
    }

Clean:
    free(empty_index);
    return ret;
}

u8 zero_1(u8 isCol, u8 index, u8 aucIndex[1], u8 pcDataCol[EACH_PIX][EACH_PIX], u8 pcDataRow[EACH_PIX][EACH_PIX])
{
    u8 sum = 0, i;

    for (i = 0; i < EACH_PIX; i++)
    {
        sum += (isCol) ? pcDataCol[index][i] : pcDataRow[index][i];
    }

    if (isCol)
        pcDataCol[index][aucIndex[0]] = SUM - sum;
    else
        pcDataRow[index][aucIndex[0]] = SUM - sum;

    PRINT("Set %s [%d][%d] as %d", isCol ? "COL" : "ROW", index + 1, aucIndex[0] + 1,  SUM - sum);

    return 0;
}

u8 contains_one_num (u8 *d_data, u8 val)
{
    u8 j;

    for (j = 0; j < EACH_PIX; j++)
    {
        if (val == d_data[j])
        {
            return 0;
        }
    }

    return 1;
}

u8 contains_nums (u8 *d_data, u8 *d, u8 arraySize, u8 unSelect)
{
    u8 i,j, count = 1;
    u8 ret = 1;

    for (i = 0; i < arraySize; i++)
    {
        if (i != unSelect)
        {
            for (j = 0; j < EACH_PIX; j++)
            {
                if (d[i] == d_data[j])
                {
                    count++;
                    break;
                }
            }
        }
    }

    if (count == arraySize)
    {
        return 0;
    }

    return ret;
}

u8 less_zero_check(u8 isCol, u8 index, u8 *aucIndex, u8 maxCount, u8 pcDataCol[EACH_PIX][EACH_PIX], u8 pcDataRow[EACH_PIX][EACH_PIX])
{
    u8 sum = 0, i, j, k, ret = 1;
    u8 flag = 0, count = 0, cnt = 0;

    u8 aucNum[EACH_PIX + 100] = {0};

    for (k = 1; k < EACH_PIX + 1; k++)
    {
        flag = 0;
        for (j = 0; j < EACH_PIX; j++)
        {
            if (k == (isCol ? pcDataCol[index][j] : pcDataRow[index][j]))
            {
                flag = 1;
                break;
            }
        }

        if (!flag)
        {
            aucNum[count] = k;
            count++;
        }

        if (EACH_PIX == count)
        {
            break;
        }
    }

    for(i = 0; i < maxCount; i++)
    {
        cnt = 0;

        for(j = 0; j < maxCount; j++)
        {
            if (0 == contains_one_num((isCol ? pcDataRow[aucIndex[j]] : pcDataCol[aucIndex[j]]), aucNum[i]))
            {
                cnt++;
            }
        }

        if (cnt == (maxCount -1))
        {
            for(j = 0; j < maxCount; j++)
            {
                if (1 == contains_one_num((isCol ? pcDataRow[aucIndex[j]] : pcDataCol[aucIndex[j]]), aucNum[i]))
                {
                    if (isCol)
                        pcDataCol[index][aucIndex[j]] = aucNum[i]; /////
                    else
                        pcDataRow[index][aucIndex[j]] = aucNum[i]; /////

                    PRINT("METHOD_1 : Set %s [%d][%d] as %d", isCol ? "COL" : "ROW", index + 1, aucIndex[j] + 1, aucNum[i]);
                    return 0;
                }
            }
        }
    }

    for(i = 0; i < maxCount; i++)
    {
        for(j = 0; j < maxCount; j++)
        {
            if (0 == contains_nums((isCol ? pcDataRow[aucIndex[j]] : pcDataCol[aucIndex[j]]), aucNum, maxCount, i))
            {
                if (isCol)
                    pcDataCol[index][aucIndex[j]] = aucNum[i]; /////
                else
                    pcDataRow[index][aucIndex[j]] = aucNum[i]; /////

                PRINT("METHOD_2 : Set %s [%d][%d] as %d", isCol ? "COL" : "ROW", index + 1, aucIndex[j] + 1, aucNum[i]);
                return 0;
            }
        }
    }
    return ret;
}

int main (int argc, int **argv)
{
    u8 i, j, k, ucFinish = 0;
    int order;

    if (argc > 1)
        init_data(1);
    else
        init_data(0);

    for (i = 0; i < EACH_PIX; i++)
    {
        SUM += (i + 1);
    }

    print_result(ucFinish);

    PRINT("\n-------------------------------------------------------------------\n");
    
    while (1)
    {
        if (0 == finish())
        {
            ucFinish = 1;
            break;
        }
        else if (giFailCnt > MAX_FAILED)
        {
            break;
        }

        clean_global();

        for (order = 0 ; order < 2; order++) // 0 --- sort by rows; 1 --- sort by cols
        {
            if(0 == sorts(order))
            {
                for (i = 0; i < EACH_PIX; i++)
                {
                    for (j = 0; j < EACH_PIX; j++)
                    {
                        if(!order)
                            data_col[i][j] = data[j][i];
                        else
                            data[i][j] = data_col[j][i];
                    }
                }

                break;
            }
            else
            {
                giFailCnt++;
            }
        }
    }

    PRINT("\n-------------------------------------------------------------------\n");
    print_result(ucFinish);
    return 0;
}

void print_result(u8 ucFinish)
{
    u8 i, j, ucZeroCntRow = 0, ucZeroCntCol[EACH_PIX] = {0};
    printf("\n\n\n");

    printf("    |  1 2 3 4 5 6 7 8 9\n");
    printf("__________________________\n");

    for (i = 0; i < EACH_PIX; i++)
    {
        printf("%d : | ", i + 1);

        for (j = 0; j < EACH_PIX; j++)
        {
            if (ucFinish)
            {
                if (0 == data_compare[i][j])
                    printf(" %d", (data[i][j]));
                else
                    printf("  ");
            }
            else
            {
                printf(" %d", (data[i][j]));
            }
        }

        printf("\n");

        if (!ucFinish)
        {
            printf("    | ");

            for (j = 0; j < EACH_PIX; j++)
            {
                printf(" %s", data_compare[i][j] == data[i][j] ? " " : "-");
            }

            ucZeroCntRow = 0;
            for (j = 0; j < EACH_PIX; j++)
            {
                if(!data[i][j])
                    ucZeroCntRow++;
            }

            for (j = 0; j < EACH_PIX; j++)
            {
                if(!data_col[i][j])
                    ucZeroCntCol[i]++;
            }

            if (ucZeroCntRow)
                printf(" ----%d", ucZeroCntRow);

            printf("\n");
        }
    }

    printf("    |  | | | | | | | | |\n");
    printf("    |  | | | | | | | | |\n");
    printf("    |  ");

    for (i = 0; i < EACH_PIX; i++)
    {
        printf("%d ", ucZeroCntCol[i]);
    }

    printf("\n__________________________\n\n");
    printf("giFailCnt = %d\n", giFailCnt);
}

void clean_global()
{
    u8 i = 0;

    for (i = 0; i < EACH_PIX; i++)
    {
        aZeroNum[i] = 0;
    }
    return;
}

u8 finish()
{
    u8 i = 0, ret = 0;

    for (i = 0; i < EACH_PIX; i++)
    {
        if (aZeroNum[i] != 0)
        {
            ret = 1;
            break;
        }
    }
    return ret;
}

