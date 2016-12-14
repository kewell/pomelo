#!/bin/sh
# useage : $0 $CODE_NAME $MAX_CNT $DATA1 $DATA2 $DATA3

if [ $# -lt 3 ];then
    echo "Args Cnt error, Need 4 but only $#"
    exit
fi
CODE_NAME=$1
NOW_P=$3
PER_P=$4
SCALE=2
RATE=$(echo "scale=${SCALE};${NOW_P}/${PER_P}*100"|bc) 
echo ${RATE} ${CODE_NAME}
exit
