#!/bin/sh

CTK_LIST_FILE=.README.list1
#DATA OF CTK_LIST_FILE http://59.175.132.102/list=sh000001,sz399001,sz399005,sz399006,sh601318,sh600050,sh600837,sh600177,sh600704,sh601515,sz002251,sz002081,sh603808,sz000793,sz002007,sz002335,sh603456,sh600478

#echo -e -n "  代 码  "
#echo -e -n "今---0215"

D_TD=`date +%d`
D_TD_1=`expr $D_TD \- 1`
M_TD=`date +%m`
M_TD_1=`expr $M_TD \- 1`
#echo $M_TD$D_TD-0$M_TD_1$D_TD_1

if [ $# -eq 0 ];then
    echo
    echo -e -n "\e[1;36m"
    echo -e -n "$M_TD$D_TD-0$M_TD_1$D_TD_1"
    echo -e -n "\e[0m"

    echo -e -n "\e[1;42m"
    for i in `seq 1 9 `;do
        echo -e -n " $i日 "
    done
    for i in `seq 10 22 `;do
        echo -e -n "$i日 "
    done

    echo -e -n "现价到最高 现价到最低"
    echo
    echo -e -n "\e[0m"
fi

TODAY_START_TIME_FLAG=/root/.STK/.FLAG

#for i in `seq 2 50`
for i in {60..2}
do
    STK_CODE=`cat ${CTK_LIST_FILE} | awk -F',' '{print $"'$i'"}'`
    if [ "${STK_CODE}" != "" ];then
        if [ $# -eq 1 ];then
            if [ $1 = "DOWNLOAD" ];then

                if [ ${TODAY_START_TIME_FLAG} -nt ${STK_CODE} ];then
                    sh .autoWgetFrom163_inputFullCodeName.sh ${STK_CODE}
                    mv ${STK_CODE} .${STK_CODE}.def
                    iconv -f gb2312 -t UTF8 .${STK_CODE}.def -o ${STK_CODE}
                    rm -rf .${STK_CODE}.def
                    dos2unix -q ${STK_CODE}
                    sh .separateFile.sh ${STK_CODE}
                fi
            fi
        fi

        if [ $# -eq 1 ];then
            if [ $1 = "EXT" ];then
                sh .analayDownloadHistoryFile_EXT.sh ${STK_CODE}
            fi

            if [ $1 = "GJD" ];then
                sh .analayDownloadHistoryFile_CMP2FILE.sh ${STK_CODE}
            fi
        else

            sh .analayDownloadHistoryFile.sh ${STK_CODE}
        fi
    #else
    #    echo
    #    exit
    fi
done
