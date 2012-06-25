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
typedef int u8;
//typedef unsigned int u8;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

u8 debug = 0;
#define PRINT(format,args...) if(1 == debug){printf(format, ##args);} 
#define COMPARE_FLAG    7
#define MAX_TRYS    100

// Diffcult puzzle
#if 1 
u8 data[EACH_PIX][EACH_PIX] = {
#if 0 
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
#else
// Easy puzzle
u8 data[EACH_PIX][EACH_PIX] = {
    {3,1,2,0,9,5,0,7,6},
    {5,0,9,1,0,7,0,8,2},
    {4,0,7,2,6,3,5,0,0},
    {9,0,0,7,0,0,2,4,0},
    {0,2,8,0,1,0,0,9,3},
    {0,3,0,9,8,2,0,5,7},
    {0,4,5,6,0,0,0,3,1},
    {1,7,0,3,5,8,9,0,4},
    {8,0,3,4,2,0,7,0,5}
};
#endif

u8 data_compare[EACH_PIX][EACH_PIX] = {0};
u8 data_col[EACH_PIX][EACH_PIX] = {0};
int total_count[6] = {0,0,0,0,0,0};
u8 gucColCnt = 0, gucRowCnt = 0;

u8 aZeroNum[EACH_PIX] = {1};
u8 SUM = 0;

u8 finish();
void reset_val(u8 *data_new, u8 index, u8 isCol);
void clean_global();

u8 less_zero_check(u8 isCol, u8 index, u8 *aucIndex, u8 maxCount);
u8 contains_num(u8 *d_data, u8 *d, u8 arraySize);
u8 zero_1(u8 isCol, u8 index, u8 aucIndex[1]);
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

        //printf("\n");
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
    if(isCol)
        gucColCnt++;
    else
        gucRowCnt++;

    u8 ret = 1, i, j, k;
    int *empty_index;
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

        if (1 == aZeroNum[i] && (0 == zero_1(isCol, i, empty_index)))
        {
            ret = 0;
            goto Clean;
        }
        else if (COMPARE_FLAG > aZeroNum[i])
        {
            if (0 == less_zero_check(isCol, i, empty_index, aZeroNum[i]))
            {
                //PRINT("\n%s by %s : %03d Success\n", __func__, ((isCol) ? "COL" : "ROW"),  __LINE__);
                ret = 0;
                goto Clean;
            }
        }
    }

Clean:
    free(empty_index);
    return ret;
}

u8 zero_1(u8 isCol, u8 index, u8 aucIndex[1])
{
    u8 sum = 0, i;

    //PRINT("Go into %10s : empty index is %s[%d], PIX [%d]\n", __func__, ((1 == isCol) ? "COL" : "ROW"), index + 1, aucIndex[0] + 1);

    if (isCol)
    {
        for (i = 0; i < EACH_PIX; i++)
        {
            sum += data_col[index][i];
        }

        data_col[index][aucIndex[0]] = SUM - sum;
        PRINT("Set Col[%d][%d] as %d", index + 1, aucIndex[0] + 1,  SUM - sum);
    }
    else
    {
        for (i = 0; i < EACH_PIX; i++)
        {
            sum += data[index][i];
        }

        data[index][aucIndex[0]] = SUM - sum;
        PRINT("Set Row[%d][%d] as %d", index + 1, aucIndex[0] + 1,  SUM - sum);
    }

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

    for (j = 0; j < EACH_PIX; j++)
    {
        PRINT("%d ", d_data[j]);
    }

    PRINT("   ---- include  ");
    for (i = 0; i < arraySize; i++)
    {
        if (i != unSelect)
            PRINT("[%d] ", d[i]);
    }

    PRINT("   ???");

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
        PRINT("   Yes\n");
        return 0;
    }
    PRINT("   NO\n");

    return ret;
}

u8 less_zero_check(u8 isCol, u8 index, u8 *aucIndex, u8 maxCount)
{
    u8 sum = 0, i, j, k, ret = 1;
    u8 flag = 0, count = 0, cnt = 0, printf_flag = 1;

    u8 aucNum[EACH_PIX + 100] = {0};

    for (k = 1; k < EACH_PIX + 1; k++)
    {
        flag = 0;
        for (j = 0; j < EACH_PIX; j++)
        {
            if (k == (isCol ? data_col[index][j] : data[index][j]))
            {
                flag = 1;
                break;
            }
        }

        if (!flag)
        {
            if (printf_flag)
            {
                PRINT("%s [%d] LOST ", (1 == isCol) ? "COL" : "ROW", index + 1);
                printf_flag = 0;
            }

            aucNum[count] = k;
            count++;
            PRINT("[%d] ", k);
        }

        if (EACH_PIX == count)
        {
            break;
        }
    }

    if (count > 0)
        PRINT("\n");

    for(i = 0; i < maxCount; i++)
    {
        cnt = 0;

        for(j = 0; j < maxCount; j++)
        {
            if (0 == contains_one_num((isCol ? data[aucIndex[j]] : data_col[aucIndex[j]]), aucNum[i]))
            {
                cnt++;
            }
        }

        if (cnt == (maxCount -1))
        {
            for(j = 0; j < maxCount; j++)
            {
                if (1 == contains_one_num((isCol ? data[aucIndex[j]] : data_col[aucIndex[j]]), aucNum[i]))
                {
                    if (isCol)
                        data_col[index][aucIndex[j]] = aucNum[i]; /////
                    else
                        data[index][aucIndex[j]] = aucNum[i]; /////

                    PRINT("METHOD_1 : Set %s [%d][%d] as %d", isCol ? "COL" : "ROW", index + 1, aucIndex[j] + 1, aucNum[i]);
                    total_count[4]++;
                    return 0;
                }
            }
        }
    }

    for(i = 0; i < maxCount; i++)
    {
        for(j = 0; j < maxCount; j++)
        {
            if (0 == contains_nums((isCol ? data[aucIndex[j]] : data_col[aucIndex[j]]), aucNum, maxCount, i))
            {
                if (isCol)
                    data_col[index][aucIndex[j]] = aucNum[i]; /////
                else
                    data[index][aucIndex[j]] = aucNum[i]; /////

                PRINT("METHOD_2 : Set %s [%d][%d] as %d", isCol ? "COL" : "ROW", index + 1, aucIndex[j] + 1, aucNum[i]);
                total_count[5]++;

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

    for (i = 0; i < EACH_PIX; i++)
    {
        SUM += (i + 1);
    }

    if (argc > 1)
        init_data(1);
    else
        init_data(0);


    print_result(ucFinish);

    PRINT("\n-------------------------------------------------------------------\n");
    
    while (1)
    {
        if (0 == finish())
        {
            ucFinish = 1;
            break;
        }

        if (total_count[3] > MAX_TRYS)
        {
            break;
        }

        clean_global();

        //for (order = 0 ; order < 2; order++) // 0 --- sort by rows; 1 --- sort by cols
        for (order = 1 ; order > -1; order--) // 0 --- sort by rows; 1 --- sort by cols
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
                PRINT("\n================= Sort by %s Success =================\n\n", (order) ? "COL" : "ROW");

                total_count[order]++;
                total_count[2]++;

                break;
            }
            else
            {
                total_count[3]++;
            }
        }

    }

    PRINT("\n-------------------------------------------------------------------\n");
    
    print_result(ucFinish);

    printf("%d = Row[%d] + Col[%d], Method[1]=%d, Method[2]=%d\n", total_count[2], total_count[0], total_count[1], total_count[4], total_count[5]);
    printf("all COL Cnt = %d, all ROW Cnt = %d\n", gucColCnt, gucRowCnt);
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

