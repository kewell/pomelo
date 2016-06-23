#!/bin/sh
STR="$1 $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14} ${15} ${16} ${17} ${18} ${19} ${20}"
STR="$1 $2 $3 $4 $5 $6 $7 $8 $9 ${10} ${11} ${12} ${13} ${14} ${15}"
STR="$1 $2 $3 $4 $5 $6" 
STR="$*"
CNT=$#

if [ $CNT -gt 5 ];then
    SHOW_CNT=5
else
    SHOW_CNT=3
fi
START=1
END=`expr 1 \+ ${SHOW_CNT}`
END=${SHOW_CNT}
MAX=`echo $STR | awk '{print $NF}'`
MAX=`echo $STR | awk '{print $1}'`
MIN=$MAX

:<<BLOCK_BY_WENJING
if [ ${END} -gt ${CNT} ];then
    echo "Warn : Need More data to analyse"
    exit
fi
BLOCK_BY_WENJING

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

for i in `seq $START $END`
do
    DATA=`echo $STR | awk '{print $"'$i'"}'`
    DATA=`expr $DATA \/ ${DIV_DATA}`
    b=''

    if [ $DATA -gt 140 ];then
        printf "%-04d:^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\r\n" ${DATA}
    else
        for ((i=0;"$i"<="$DATA";i+=1))
        do
            b==$b
        done
        printf "%-04d:%-50s\r\n" ${DATA} $b
    fi
done

exit
