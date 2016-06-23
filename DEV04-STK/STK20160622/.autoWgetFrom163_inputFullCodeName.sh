#!/bin/sh

SH_EX_FLAG=0
SZ_EX_FLAG=1
START_DATE=20151231
START_DATE=20150707
START_DATE=20160503

if [ $# -le 0 ];then
    echo "Usage: $0 FULL_CODE_NAME"
    exit
fi

if [ $# -gt 1 ];then
    START_DATE=$2

    if [ 8 -ne ${#START_DATE} ];then # Error start Date input
        echo "Error start Date input"
        exit
    fi
fi

CODE_NUM=$1
OUT_FILE_TMP=$1
CODE_NUM_OF_2ST_CHAR=`expr substr "$CODE_NUM" 2 1`
CODE_NUM_OF_DIGIT_ONLY=`expr substr "$CODE_NUM" 3 6`
CODE_NUM=$CODE_NUM_OF_DIGIT_ONLY

if [ "z" = "$CODE_NUM_OF_2ST_CHAR" ];then
    EX_FLAG=$SZ_EX_FLAG
else
    EX_FLAG=$SH_EX_FLAG
fi

#echo ${EX_FLAG}${CODE_NUM};exit

#END_DATE=20160413
#START_DATE=${END_DATE}
END_DATE=${START_DATE}
END_DATE=`date +%Y%m%d`
#quotes.money.163.com=220.181.8.27
wget -q --tries=2 --timeout=2 --wait=2 -O ${OUT_FILE_TMP} http://quotes.money.163.com/service/chddata.html?code=${EX_FLAG}${CODE_NUM}\&start=${START_DATE}\&end=${END_DATE}\&fields=TCLOSE\;HIGH\;LOW\;LCLOSE\;PCHG\;TURNOVER\;VOTURNOVER

if [ ! -s ${OUT_FILE_TMP} ];then
    sleep 1
    wget -q --tries=3 --timeout=2 --wait=2 -O ${OUT_FILE_TMP} http://quotes.money.163.com/service/chddata.html?code=${EX_FLAG}${CODE_NUM}\&start=${START_DATE}\&end=${END_DATE}\&fields=TCLOSE\;HIGH\;LOW\;LCLOSE\;PCHG\;TURNOVER\;VOTURNOVER
fi

if [ ! -s ${OUT_FILE_TMP} ];then
    sleep 3
    wget -q --tries=3 --timeout=2 --wait=2 -O ${OUT_FILE_TMP} http://quotes.money.163.com/service/chddata.html?code=${EX_FLAG}${CODE_NUM}\&start=${START_DATE}\&end=${END_DATE}\&fields=TCLOSE\;HIGH\;LOW\;LCLOSE\;PCHG\;TURNOVER\;VOTURNOVER
fi
