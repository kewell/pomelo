if [ $# -ne 4 ];then
    echo $0 1, 2, 3, 4
    exit
fi
OUT_PUT_FILE="LOG_$(date +%Y%m%d)_B[$1]_S[$2]_B-Alarm[$3]_S-Alarm[$4]"
#OUT_PUT_FILE="LOG_$(date +%Y%m%d)_B-$1_S-$2-_B-Alarm-$3-_S-Alarm-$4"

echo "--------------------------------------------------------"
for i in `cat list`;do ./a.out $i $1 $2 $3 $4;done

for i in `cat list`;do ./a.out $i $1 $2 $3 $4;done > ${OUT_PUT_FILE}

cat ${OUT_PUT_FILE} | grep ": All"|awk '{print $4}' > rate
rm -rf ${OUT_PUT_FILE}

#echo -ne "--------------------------------------------------------\n"
A=0;for i in `cat rate`;do echo -ne "+$i";A=`echo "$A + $i"|bc`;echo " =$A";done

cat list
N=`cat rate|wc -l`;echo -e "$A \/ $N = ";
FINNAL_RATE=`echo "$A / $N" |bc`
echo -ne "--------------------------------------------------------\n"
echo ${FINNAL_RATE}
touch ${FINNAL_RATE}_B-$1_S-$2-_B-Alarm-$3-_S-Alarm-$4
