
#ifndef __BASIC_H__
#define __BASIC_H__

#define MINIMAL_BUF_SIZE        32
#define MIDDLE_BUF_SIZE         128
#define MAXIMAL_BUF_SIZE        256

#define USEFULL_BUF_SIZE        6
#define ARRAY_CNT_S_AND_B       15
#define ARRAY_CNT_TRADE         8

typedef int HI_BOOL;
typedef unsigned char HI_U8;
typedef unsigned short HI_U16;
typedef unsigned int HI_U32;

typedef signed char HI_S8;
typedef short HI_S16;
typedef int HI_S32;
typedef unsigned long long HI_U64;
typedef long long HI_S64;
typedef float HI_F;

/*
0-name 1-open 2-yesterday 3-now 4-High 5-Low 6---B  7---S  8---SUM  9---Money  
"NAME, 16.87, 16.90,      17.13,17.42, 16.70,17.12, 17.13, 9327671, 159983824.30,

B: 10   11    12    13     14    15   16     17    18   19     S: 20  21     22    23     24   25    26    27   28    29    30         31   32
15523,17.12, 16479,17.11, 39691,17.10,10600,17.09,13400,17.08, 200,17.13,  7000,17.14,  29800,17.15,9800,17.16,11924,17.17, 2014-05-22,10:30:00,00"
*/

typedef struct SELL_AND_BUY
{
/*
  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14   0   0   0   0   0   0   0   0   0   0   
  |   |   |   |   |   |   |   |   |   |   |   |   |   |    |-14
  |   |   |   |   |   |   |   |   |   |   |   |   |   |------13
  |   |   |   |   |   |   |   |   |   |   |   |   |----------12
  |   |   |   |   |   |   |   |   |   |   |   |--------------11
  |   |   |   |   |   |   |   |   |   |   |------------------10 TODO
  |   |   |   |   |   |   |   |   |   |---------------------- 9 TODO
  |   |   |   |   |   |   |   |   |-------------------------- 8 TODO
  |   |   |   |   |   |   |   |------------------------------ 7 TODO
  |   |   |   |   |   |   |---------------------------------- 6 TODO
  |   |   |   |   |   |-------------------------------------- 5st req  Price or Count
  |   |   |   |   |------------------------------------------ 4st req  Price or Count
  |   |   |   |---------------------------------------------- 3st req  Price or Count
  |   |   |-------------------------------------------------- 2st req  Price or Count
  |   |------------------------------------------------------ 1st req  Price or Count
  |---------------------------------------------------------- Cur Deal Price [ or Count ??? ]
  
*/
    HI_F   newPrice[ARRAY_CNT_S_AND_B];
    HI_U32 newCount[ARRAY_CNT_S_AND_B];
    
    HI_F   oldPrice[ARRAY_CNT_S_AND_B];
    HI_U32 oldCount[ARRAY_CNT_S_AND_B];

    //HI_F   curBuyPrice;              // 6-------
    //HI_F   curSellPrice;             // 7-------

}st_sell_and_buy;

typedef struct STK_STATIC_DATA
{
    HI_U8  nameByCn[MINIMAL_BUF_SIZE]; // 0
    HI_U8  nameByPinYin[MINIMAL_BUF_SIZE];
    HI_U8  nameByCode[MINIMAL_BUF_SIZE];
    
    HI_F   openPrice;                  // 1
    HI_F   closePrice;                 // 2
    
    HI_U8  useless_date[MINIMAL_BUF_SIZE];     // 30 
    HI_U8  useless_last;               // 32
    
}
stStkStaticData;

typedef struct STK_DYNAMIC_DATA
{
    HI_F   curPrice;                   // 3-------
    HI_F   highPrice;                  // 4-------                
    HI_F   lowPrice;                   // 5-------


    /*
       0   1   2   3   4   5   6   7
       |   |   |   |   |   |------ TODO
       |   |   |   |   |---------- TODO
       |   |   |   |-------------- last_real_del_cnt_BAK
       |   |   |------------------ real_time_del_Cnt = Cur[0] - Last[1]
       |   |---------------------- Last, bak from last Cur
       |-------------------------- Cur  read from WWW
    */
    //HI_U64 tradeCnt[ARRAY_CNT_TRADE];// 8
    HI_U64 tradeCntCur;
    HI_U64 tradeCntLast;
    HI_U64 allTradeCntCur;
    HI_U64 allTradeCntLast;
    
}stStkDynamicData;

typedef struct STK_DATA
{
    HI_U8 flagCodeError = 0;
    HI_U8 flagStkStoped = 0;

    HI_U8 flagInitDone  = 0;
    HI_U8 flagTradeTime = 0;
    
    stStkDynamicData stDymData;
    stStkStaticData  stSatData;
    
    HI_U64 useless_allTradeMoney;      // 9-------
    HI_U8  useless_T[MINIMAL_BUF_SIZE];// 31

}stStkData;

#endif //__BASIC_H__

