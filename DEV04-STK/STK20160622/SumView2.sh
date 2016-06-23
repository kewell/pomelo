#!/bin/sh
# useage : $0 $CODE_NAME $MAX_CNT $DATA1 $DATA2 $DATA3

if [ $# -lt 3 ];then
    echo "Args Cnt error, Need 4 but only $#"
    exit
fi
CODE_NAME=$1
NOW_PRICE=$3
PER_PRICE=$4
NOW_SUM=$5
PER_SUM=$6
SCALE=3
RATE_PRICE=$(echo "scale=${SCALE};(${NOW_PRICE}-${PER_PRICE})/${PER_PRICE}*100"|bc) 
SCALE=2
RATE_SUM=$(echo "scale=${SCALE};${NOW_SUM}/${PER_SUM}*100"|bc) 
echo ${RATE_SUM} ${CODE_NAME} ${RATE_PRICE}
#echo ${NOW_PRICE}"___"${PER_PRICE}
exit
