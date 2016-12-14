#!/bin/sh

#日期,股票代码,名称,        收盘价,最高价,最低价,前收盘,涨跌幅,换手率,成交量
#2016-03-18,'002007,华兰生物,43.57,44.5,  43.0,   43.0, 1.3256,1.9177,11107700
#    1         2      3        4    5      6       7       8      9     10

if [ $# -ne 1 ];then
    echo "Usage: $0 FILE_NAME"
    exit
fi

INPUT_DATA_FILE=$1

#echo $1
#tail -1 20150708/${INPUT_DATA_FILE}
#tail -1 20160413/${INPUT_DATA_FILE}
#exit


LAST_YEAR_CLS=`tail -1 20150708/${INPUT_DATA_FILE}| awk -F[,] '{print $4}'`
CURR_YEAR_CLS=`tail -1 20160413/${INPUT_DATA_FILE}| awk -F[,] '{print $4}'`

SCALE=3
RET=$(echo "scale=${SCALE};($CURR_YEAR_CLS-$LAST_YEAR_CLS)/$LAST_YEAR_CLS*100"|bc)
echo $1 $RET
