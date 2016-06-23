#!/bin/sh

#日期,股票代码,名称,        收盘价,最高价,最低价,前收盘,涨跌幅,换手率,成交量
#2016-03-18,'002007,华兰生物,43.57,44.5,  43.0,   43.0, 1.3256,1.9177,11107700
#    1         2      3        4    5      6       7       8      9     10

if [ $# -ne 1 ];then
    echo "Usage: $0 FILE_NAME"
    exit
fi

INPUT_DATA_FILE=$1
if [ ! -s ${INPUT_DATA_FILE} ];then
    echo "FILE ${INPUT_DATA_FILE} ERROR"
    exit
fi

#if [ ${INPUT_DATA_FILE} -nt .${INPUT_DATA_FILE}.SUM ];then
#    rm -rf .${INPUT_DATA_FILE}.SUM
#fi
rm -rf .${INPUT_DATA_FILE}.SUM

echo -e -n "$1 " >> .${INPUT_DATA_FILE}.SUM

ALL_LINE=`cat $INPUT_DATA_FILE | wc -l`
ALL_LINE=`expr $ALL_LINE \- 1`

if [ -f ${INPUT_DATA_FILE} ];then
    tail -$ALL_LINE ${INPUT_DATA_FILE}| awk -F[,] '{printf $10" "}' >> .${INPUT_DATA_FILE}.SUM
fi
exit ###------------------------------------------------------------------------------------------------
