#!/bin/sh
# useage : $0 $CODE_NAME $MAX_CNT $DATA1 $DATA2 $DATA3

STR="$*"
CNT=$#

if [ $CNT -gt 30 ];then
    SHOW_CNT=30
else
    SHOW_CNT=$CNT
fi

CODE_INDEX=$1
SHOW_CNT=$2
START=3
END=`expr 3 \+ ${SHOW_CNT}`
MAX=`echo $STR | awk '{print $NF}'`
MAX=`echo $STR | awk '{print $3}'`
MIN=$MAX

for i in `seq $START $END`
do
    DATA=`echo $STR | awk '{print $"'$i'"}'`
    if [ 0 -ne $DATA ];then
        if [ $DATA -lt $MIN ];then
            MIN=$DATA
        fi
        if [ $DATA -gt $MAX ];then
            MAX=$DATA
        fi
    fi
done

DIV_DATA=100
DIV_DATA=`expr $MIN \/ 10`
:<<BLOCK_BY_KEWELL
for ((i=0;$i<=10;i+=1))
do
    if [ `expr $MIN \/ ${DIV_DATA}` -ge 100 ];then
        DIV_DATA=`expr ${DIV_DATA} \* 10`
    else
        break
    fi
done
BLOCK_BY_KEWELL

MIN=`expr $MIN \/ ${DIV_DATA}`
MAX=`expr $MAX \/ ${DIV_DATA}`
#echo $MIN------------- $MAX-------- $DIV_DATA------------;exit
#echo MAX=$MAX MIN=$MIN ST=$START DIV_DATA=$DIV_DATA

echo "                NOW     HIGH    LOW     PER_VAL SUM SUM"
for i in `seq $START $END`
do
    DATA=`echo $STR | awk '{print $"'$i'"}'`
    DATA=`expr $DATA \/ ${DIV_DATA}`
    b=''

    if [ 3 == ${i} ];then
        echo -e -n $(date +%Y"-"%m"-"%d)
        PER_P=`cat .stk.data |grep "${CODE_INDEX}"|awk -F[,] '{print $3}'`
        NOW_P=`cat .stk.data |grep "${CODE_INDEX}"|awk -F[,] '{print $4}'`

        SCALE=3
        RATE=$(echo "scale=${SCALE};(${NOW_P}-${PER_P})/${PER_P}*100"|bc) 

        cat .stk.data |grep "${CODE_INDEX}"|awk -F[,] '{printf "\t"$4"\t"$5"\t"$6"\t"}';
        echo -e -n "${RATE}\t"
    else
        HEAD_CNT=${i}
        HEAD_CNT=`expr ${i} \- 2`
        #HEAD_CNT=`expr ${HEAD_CNT} + 1`
        head -${HEAD_CNT} ${CODE_INDEX}|tail -1|awk -F[,] '{printf $1"\t"$4"\t"$5"\t"$6"\t"$8"\t"}'
    fi

    if [ $DATA -gt 140 ];then
        printf "%-04d:^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n" ${DATA}
    else
        for ((i=0;"$i"<="$DATA";i+=2))
        do
            b==$b
        done
        printf "%-04d:%-50s\r\n" ${DATA} $b
    fi
done

exit
