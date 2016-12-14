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

CODE_NAME=`tail -1 ${INPUT_DATA_FILE}|awk -F[,] '{print $3}'`
if [ ${#CODE_NAME} -lt 4 ];then
echo -e -n "\e[1;42m$CODE_NAME  \e[0m "
else
echo -e -n "\e[1;42m$CODE_NAME\e[0m "
fi

START_LINE=2
ALL_FILE_LINE=`cat .${INPUT_DATA_FILE}.nf|wc -l`
MAX_SHOW_IN1LINE=21
if [ ${ALL_FILE_LINE} -gt ${MAX_SHOW_IN1LINE} ];then
    ALL_FILE_LINE=${MAX_SHOW_IN1LINE}
fi


if [ -s ${TODAY_DATA} ];then
    TODAY_N=`cat ${TODAY_DATA} |grep ${INPUT_DATA_FILE} |awk -F',' '{print $4}'`
    YEDAY_C=`cat ${TODAY_DATA} |grep ${INPUT_DATA_FILE} |awk -F',' '{print $3}'`
fi

######################## ADD START 2016-04-08 14:23:37 ######################## 
RET2=$(echo "scale=${SCALE};$TODAY_N-$YEDAY_C"|bc)
if [ 0 = "${RET2}" ];then
    echo -e -n "N_.000 "
else
    RET=$(echo "scale=${SCALE};($TODAY_N-$YEDAY_C)/$YEDAY_C*100"|bc)
    if [ 0 = "${RET}" ];then
        echo -e -n "N_.000 "
    else
        if [ $(echo "$RET2 >0" | bc) = 1 ]; then
            echo -e -n "\e[1;34mN_${RET:0:4} \e[0m"
        else
            echo -e -n "\e[1;35mN_${RET:0:4} \e[0m"
        fi
    fi
fi
######################## ADD ENDED 2016-04-08 14:23:37 ######################## 

for LINE_NUM in `seq ${START_LINE} ${ALL_FILE_LINE}`
do
    SPECIAL_DATE_RATE=`sed -n "${LINE_NUM}p" .${INPUT_DATA_FILE}.Lf`

    if [ "None" = "${SPECIAL_DATE_RATE}" ];then
        echo -e -n "0.00 "
    elif [[ "0.0" = "${SPECIAL_DATE_RATE}" ]];then
        echo -e -n "0.00 "
    elif [ $(echo "$SPECIAL_DATE_RATE > 0" | bc) = 1 ]; then
        echo -e -n "\e[1;34m${SPECIAL_DATE_RATE:0:4} \e[0m"
    else
        echo -e -n "\e[1;35m${SPECIAL_DATE_RATE:0:4} \e[0m"
    fi
done
echo;exit
