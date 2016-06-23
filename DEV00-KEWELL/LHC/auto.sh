#!/bin/bash

BUY_NUM_CNT=4
START_SN=199
STOP_SN=426

SKIP_LAST_NUMS=0
STEP=4
MAX_SKIP_NUM=`expr 49 \- ${SKIP_LAST_NUMS} \- $STEP \+ 1`

EACH_NUM_BUY_MONRY=10
ALL_CN_CNT=`expr $STOP_SN \- $START_SN \- 1`
NEED_CASH=`expr $BUY_NUM_CNT \* $EACH_NUM_BUY_MONRY \* $ALL_CN_CNT`
EARN_CASH=0

while [ $SKIP_LAST_NUMS -lt ${MAX_SKIP_NUM} ];
do
    #for t in {$START_SN..$STOP_SN}
    for t in `seq $START_SN $STOP_SN`
    #for t in {199..426}
    do
        sh analyse.teMa.Cnt.sh data.From2013N $t ${BUY_NUM_CNT} ${SKIP_LAST_NUMS}
    done

    WIN_CNT=`cat /dev/shm/.bingo`
    EARN_CASH=`expr $WIN_CNT \* 40 \* $EACH_NUM_BUY_MONRY`
    PROFIT=`expr $EARN_CASH \- $NEED_CASH`
    echo "---------------- DROP LAST ${SKIP_LAST_NUMS} buy $BUY_NUM_CNT each time then -------bingo=${WIN_CNT} profit=${PROFIT}"

    EARN_CASH=0
    PROFIT=0
    WIN_CNT=0

    SKIP_LAST_NUMS=`expr ${SKIP_LAST_NUMS} \+ ${STEP}`
    'rm' -rf /dev/shm/.[a-Z]*
done
