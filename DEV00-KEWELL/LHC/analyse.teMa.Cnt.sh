#!/bin/bash
SYS_TMP_FS=/dev/shm/

if [ $# -ne 4 ];then
    echo -e "\n \$#=$#\nError Please input file name to analyse\n\n$0 \$ORIG_DATA_F \$LINE \$BUY_CNT \$REMOVE_LAST_CNT\n"
    exit
fi

ORIG_DATA_F=$1
L=$2
BUY_CNT=$3
REMOVE_LAST_CNT=$4

if [ ! -e $ORIG_DATA_F ];then
    echo "Error File not exsit OR line number Error , \$ORIG_DATA_F=$ORIG_DATA_F, \$L=$L"
    exit
fi

L_RET=`expr $L \+ 1`
L_ALL=`cat ${ORIG_DATA_F}|wc -l`

if [ ${L_RET} -gt ${L_ALL} ];then
    echo "Error \$L=$L, but All File $ORIG_DATA_F line number is $L_ALL"
    exit
fi

FL_RATES=${SYS_TMP_FS}/.${ORIG_DATA_F}_1-49RatesTmp
F_BUY=${SYS_TMP_FS}/.buyNumbers
CURT_BINGO_NUM=`head -$L     $ORIG_DATA_F | tail -1 |awk '{print $1}'`
NEXT_BINGO_NUM=`head -$L_RET $ORIG_DATA_F | tail -1 |awk '{print $1}'`

FIRST_TIME_CALC_RATES_F=${SYS_TMP_FS}/.flag
if [ ! -e ${FIRST_TIME_CALC_RATES_F} ];then echo 1 > ${FIRST_TIME_CALC_RATES_F};fi
FIRST_TIME_CALC_RATES=`cat $FIRST_TIME_CALC_RATES_F`

if [ 1 -eq $FIRST_TIME_CALC_RATES ];then

    #echo "------------- FIRST TIME TO CALC 1---49 NUMS RATES FROM LINE $L------------------------"
    echo 0 > ${FIRST_TIME_CALC_RATES_F}

    for i in {1..49}
    do 
        CNT=0

        for j in `head -$L $ORIG_DATA_F`
        do 
            if [ $i -eq $j ];then
                CNT=`expr $CNT \+ 1`
            fi
        done

        echo $CNT" "$i
    done  \
    > ${FL_RATES}

else

    CUR_NUM_OLD_RATE="`sed -n "${CURT_BINGO_NUM}p" ${FL_RATES} | awk '{print $1}'`"

    #sed -n "${CURT_BINGO_NUM}p" ${FL_RATES}
    #echo CUR_NUM_OLD_RATE=$CUR_NUM_OLD_RATE

    CUR_NUM_NEW_RATE=`expr $CUR_NUM_OLD_RATE \+ 1`
    cat ${FL_RATES} | awk -v Y=$CURT_BINGO_NUM -v X=$CUR_NUM_NEW_RATE '{if ( NR == Y ){print X,Y}else {print $0}}' > ${FL_RATES}.tmps
    'mv' ${FL_RATES}.tmps ${FL_RATES}

fi

BUY_START=`expr $REMOVE_LAST_CNT \+ ${BUY_CNT}`

cat $FL_RATES | sort -nr | tail -${BUY_START} | head -${BUY_CNT} | awk '{printf $NF" "}' > ${F_BUY}
#cat $FL_RATES | sort -nr | tail -${BUY_START} | head -${BUY_CNT} | awk '{printf $NF" "}'
#echo -ne `expr $L \+ 1`" : ---${RET}"

BINGO_F=${SYS_TMP_FS}/.bingo

for B_N in `cat ${F_BUY}`
do
    if [ ${NEXT_BINGO_NUM} -eq ${B_N} ];then
        if [ ! -e $BINGO_F ];then echo 0 > $BINGO_F;fi
        BINGO=`cat $BINGO_F`
        BINGO=`expr $BINGO \+ 1`
        echo $BINGO > ${BINGO_F}
        #echo "`expr $L \+ 1` : ---${NEXT_BINGO_NUM} bg=$BINGO"
        break
    fi
done

#echo

rm -rf ${F_BUY}
#rm -rf ${FL_RATES}
