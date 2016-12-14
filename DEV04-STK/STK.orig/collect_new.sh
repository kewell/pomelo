#!/bin/sh

BASE_PATH=./
CTK_LIST_FILE=.README.list1
TODAY_DATA=.stk.data
ALL_STK_COLLECT=${BASE_PATH}/_stk.data_
ALL_TMP_COLLECT=${BASE_PATH}/_stk.data2_
ANALYSE_SCRIPT=${BASE_PATH}/progress_new.sh
CALC_MACD=${BASE_PATH}/MACD.sh

STOP_2_T=1501
STOP_3_T=1505
HH_MM_NOW=`date +%H%M`
HH_MM_NOW=`date +%H%M`
SKIP_WAIT=0
echo "------------------$(date)---------------------"

if [ $# -eq 2 ] && [ "SKIP" == $2 ];then
    SKIP_WAIT=1
fi

if [ $# -eq 2 ] && [ "COLLECTCOLLECTCOLLECT" == $1 ] && [ "ALLALLALL" == $2 ];then
    echo "EMPTY HERE"
else
    if [ "ALL" == $1 ];then

        CALC_CNT=4
        for i in `seq 2 30`
        do 
            STK_CODE=`cat ${CTK_LIST_FILE} | awk -F',' '{print $"'$i'"}'`

            if [ "${STK_CODE}" != "" ];then
                if [ ${HH_MM_NOW} -gt ${STOP_2_T} ];then
                    TODAY_TRADE_SUM=""
                else
                    TODAY_TRADE_SUM=`cat ${TODAY_DATA} |grep "${STK_CODE}" | awk -F',' '{print $9}'`
                    #TODAY_TRADE_SUM=`expr ${TODAY_TRADE_SUM} \/ 100`
                fi
                TODAY_TRADE_SUM=`cat ${TODAY_DATA} |grep "${STK_CODE}" | awk -F',' '{print $9}'`

                #NEED_ADD_STR=`cat ${ALL_STK_COLLECT} | grep "^${STK_CODE}"`
                #NEED_ADD_STR=`echo ${NEED_ADD_STR}   | sed -e s/${STK_CODE}//g`
                NEED_ADD_STR=`cat .${STK_CODE}.SUM    | sed -e s/${STK_CODE}//g`   # Change by KEWELL  2016-03-21 17:27:00 
                NEED_ADD_STR=${TODAY_TRADE_SUM}" "${NEED_ADD_STR}

                echo -e "\033[36;40;1m*************************$STK_CODE********************************** \033[0m"
                sh ${CALC_MACD} ${STK_CODE}
                sh ${ANALYSE_SCRIPT} ${STK_CODE} ${CALC_CNT} ${NEED_ADD_STR}

                if [ 0 -eq ${SKIP_WAIT} ];then
                    read
                fi
            fi
        done

    else
        STK_CODE=$1
        CALC_CNT=30

        if [ "${STK_CODE}" != "" ];then
            if [ ! -f ${STK_CODE} ];then
                exit
            fi

            if [ ${HH_MM_NOW} -gt ${STOP_2_T} ];then
                TODAY_TRADE_SUM=""
            else
                TODAY_TRADE_SUM=`cat ${TODAY_DATA} |grep "${STK_CODE}" | awk -F',' '{print $9}'`
                #TODAY_TRADE_SUM=`expr ${TODAY_TRADE_SUM} \/ 100`
            fi
            TODAY_TRADE_SUM=`cat ${TODAY_DATA} |grep "${STK_CODE}" | awk -F',' '{print $9}'`

            #NEED_ADD_STR=`cat ${ALL_STK_COLLECT} | grep "^${STK_CODE}"`
            #NEED_ADD_STR=`echo ${NEED_ADD_STR}   | sed -e s/${STK_CODE}//g`
            NEED_ADD_STR=`cat .${STK_CODE}.SUM    | sed -e s/${STK_CODE}//g`   # Change by KEWELL  2016-03-21 17:27:00 
            NEED_ADD_STR=${TODAY_TRADE_SUM}" "${NEED_ADD_STR}

            sh ${CALC_MACD} ${STK_CODE}
            sh ${ANALYSE_SCRIPT} ${STK_CODE} ${CALC_CNT} ${NEED_ADD_STR}
        fi
    fi
fi
exit
