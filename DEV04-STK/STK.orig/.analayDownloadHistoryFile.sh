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

N=`head -2 .${INPUT_DATA_FILE}.nf | tail -1`
H=`head -1 .${INPUT_DATA_FILE}.hf`
L=`head -2 .${INPUT_DATA_FILE}.lf | tail -1`
if [ "0.0 = ${L}" ];then
    ALL_STOP_CNT=`cat .${INPUT_DATA_FILE}.lf |grep "0.0"|wc -l`
    ALL_STOP_CNT=`expr ${ALL_STOP_CNT} \+ 2`
    L=`head -${ALL_STOP_CNT} .${INPUT_DATA_FILE}.lf | tail -1`
fi

if [ -s ${TODAY_DATA} ];then
    TODAY_N=`cat ${TODAY_DATA} |grep ${INPUT_DATA_FILE} |awk -F',' '{print $4}'`
    N=${TODAY_N}
    TODAY_H=`cat ${TODAY_DATA} |grep ${INPUT_DATA_FILE} |awk -F',' '{print $5}'`
    TODAY_L=`cat ${TODAY_DATA} |grep ${INPUT_DATA_FILE} |awk -F',' '{print $6}'`
fi

RET_TMP=$(echo "scale=${SCALE};${TODAY_H}-${H}"|bc)
if [ 0 = "${RET_TMP}" ];then
    DONOTHING_HERE=""
else
    if [ $(echo "$RET_TMP > 0" | bc) = 1 ]; then
        H=${TODAY_H}
    fi
fi

RET_TMP=$(echo "scale=${SCALE};${L}-${TODAY_L}"|bc)
if [ 0 = "${RET_TMP}" ];then
    DONOTHING_HERE=""
else
    if [ $(echo "$RET_TMP > 0" | bc) = 1 ]; then
        L=${TODAY_L}
    fi
fi

#:<<BLOCK_BY_KEWELL
#rm -rf .${INPUT_DATA_FILE}.nf .${INPUT_DATA_FILE}.Hf .${INPUT_DATA_FILE}.lf
#cat ${INPUT_DATA_FILE}|grep $N | awk -F[,] '{print $1,$4,$5,$6}'
#cat ${INPUT_DATA_FILE}|grep $H | awk -F[,] '{print $1,$4,$5,$6}'
#cat ${INPUT_DATA_FILE}|grep $L | awk -F[,] '{print $1,$4,$5,$6}'
#echo N=$N H=$H L=$L
#BLOCK_BY_KEWELL

if [ -s .stk.data ];then
    NOW_P=`cat .stk.data |grep "${INPUT_DATA_FILE}" |awk -F[,] '{print $4}'`
    START_LINE=2
else
    NOW_P=`sed -n "2p" .${INPUT_DATA_FILE}.nf`
    START_LINE=3
fi

ALL_FILE_LINE=`cat .${INPUT_DATA_FILE}.nf|wc -l`
MAX_SHOW_IN1LINE=23
if [ ${ALL_FILE_LINE} -gt ${MAX_SHOW_IN1LINE} ];then
    ALL_FILE_LINE=${MAX_SHOW_IN1LINE}
fi

CODE_NAME=`tail -1 ${INPUT_DATA_FILE}|awk -F[,] '{print $3}'`
if [ ${#CODE_NAME} -lt 4 ];then
#echo $CODE_NAME LEN--XiaoYu4 ${#CODE_NAME}
echo -e -n "\e[1;42m$CODE_NAME  \e[0m "
else
#echo $CODE_NAME LEN--DaYuuu4 ${#CODE_NAME}
echo -e -n "\e[1;42m$CODE_NAME\e[0m "
fi
#echo -e -n "\e[1;42m$INPUT_DATA_FILE\e[0m "
#echo -e -n $INPUT_DATA_FILE" "

for LINE_NUM in `seq ${START_LINE} ${ALL_FILE_LINE}`
do
    SPECIAL_DATE_CLOSE=`sed -n "${LINE_NUM}p" .${INPUT_DATA_FILE}.nf`
    if [ "0.0" = "${SPECIAL_DATE_CLOSE}" ];then
        SPECIAL_DATE_CLOSE=`sed -n "${LINE_NUM}p" .${INPUT_DATA_FILE}.QSP`
    fi

    RET2=$(echo "scale=${SCALE};$NOW_P-$SPECIAL_DATE_CLOSE"|bc)
    ### 为了输出更好的排版，故意篡改数据
    if [ 0 = "${RET2}" ];then
        echo -e -n ".000 "
    else
        RET=$(echo "scale=${SCALE};($NOW_P-$SPECIAL_DATE_CLOSE)/$SPECIAL_DATE_CLOSE*100"|bc)
        if [ 0 = "${RET}" ];then
            echo -e -n ".000 "
        else
            if [ $(echo "$RET2 >0" | bc) = 1 ]; then
                echo -e -n "\e[1;34m${RET:0:4} \e[0m"
            else
                echo -e -n "\e[1;35m${RET:0:4} \e[0m"
                #echo -e -n $(echo "scale=${SCALE};($NOW_P-$SPECIAL_DATE_CLOSE)/$SPECIAL_DATE_CLOSE*100"|bc)"\t"
            fi
        fi
    fi
done

#echo;exit

SCALE=3
H_D=`cat ${INPUT_DATA_FILE}|grep "${H}," | awk -F[,] '{print $1}'`
L_D=`cat ${INPUT_DATA_FILE}|grep "${L}," | awk -F[,] '{print $1}'`
H2L=$(echo "scale=${SCALE};($H-$L)/$H*100"|bc) 
L2H=$(echo "scale=${SCALE};($H-$L)/$L*100"|bc) 
N2H=$(echo "scale=${SCALE};($H-$N)/$N*100"|bc) 
N2L=$(echo "scale=${SCALE};($N-$L)/$N*100"|bc) 

#echo -e $INPUT_DATA_FILE H2L=$H2L L2H=$L2H N2H=$N2H"\t"N2L=$N2L"\t"N=${N}"\t"H=${H}\/${H_D}"\t"L=${L}\/${L_D}
#echo -e L2H=${L2H:0:4}" "N2H=${N2H:0:4}" "N2L=${N2L:0:4}
#echo -e "  "${L2H:0:4}%"    "${N2H:0:4}%"    "${N2L:0:4}%
 echo -e "   "${N2H:0:4}%"     "${N2L:0:4}%

