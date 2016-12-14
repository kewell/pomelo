#!/bin/sh

#日期,股票代码,名称,        收盘价,最高价,最低价,前收盘,涨跌幅,换手率,成交量
#2016-03-18,'002007,华兰生物,43.57,44.5,  43.0,   43.0, 1.3256,1.9177,11107700
#    1         2      3        4    5      6       7       8      9     10
H=1.1
H_I=1.2
L=1.3
L_I=1.4
SCALE=3 # default is 4
#awk -F[,] '{print H=$4}'
TODAY_DATA=.stk.data

if [ $# -ne 1 ];then
    echo "Usage: $0 FILE_NAME"
    exit
fi

INPUT_DATA_FILE=$1
if [ ! -s ${INPUT_DATA_FILE} ];then
    echo "FILE ${INPUT_DATA_FILE} ERROR"
    exit
fi

if [ -s .stk.data ];then
    NOW_P=`cat .stk.data |grep "${INPUT_DATA_FILE}" |awk -F[,] '{print $4}'`
    START_LINE=2
else
    NOW_P=`sed -n "2p" .${INPUT_DATA_FILE}.nf`
    START_LINE=3
fi

ALL_FILE_LINE=`cat .${INPUT_DATA_FILE}.nf|wc -l`
MAX_SHOW_IN1LINE=31
if [ ${ALL_FILE_LINE} -gt ${MAX_SHOW_IN1LINE} ];then
    ALL_FILE_LINE=${MAX_SHOW_IN1LINE}
fi

#-----------------------------------------------------------
#echo "                DAY_5   DAY_10  DAY_20  DAY_30"

TOTAL=${NOW_P}
for LINE_NUM in `seq ${START_LINE} ${ALL_FILE_LINE}`
do
    SPECIAL_DATE_CLOSE=`sed -n "${LINE_NUM}p" .${INPUT_DATA_FILE}.nf`
    if [ "0.0" = "${SPECIAL_DATE_CLOSE}" ];then
        SPECIAL_DATE_CLOSE=`sed -n "${LINE_NUM}p" .${INPUT_DATA_FILE}.QSP`
    fi

    TOTAL=$(echo "scale=${SCALE};$TOTAL+$SPECIAL_DATE_CLOSE"|bc)
    if [ 5 -eq ${LINE_NUM} ];then
        MACD_PRICE=$(echo "scale=${SCALE};${TOTAL}/${LINE_NUM}"|bc)
    #    echo -e -n "                ${MACD_PRICE}\t"
        echo -e -n "DAY_5_10_20_30  ${MACD_PRICE}\t"
    elif [ 10 -eq ${LINE_NUM} ] || [ 20 -eq ${LINE_NUM} ] || [ 30 -eq ${LINE_NUM} ];then
        MACD_PRICE=$(echo "scale=${SCALE};${TOTAL}/${LINE_NUM}"|bc)
        echo -e -n "${MACD_PRICE}\t"
    fi
done
echo
