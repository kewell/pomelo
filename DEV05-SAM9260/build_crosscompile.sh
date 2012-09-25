#/bin/sh

TARGET="buildroot-2012.08"
TARGET_SRC="buildroot-2012.08.tar.bz2"

if [ ! -f ${TARGET_SRC} ];then

    which wget

    if [ ! $? -eq 0];then
        echo "Sorry cannot find wget command, please install first"
        exit
    fi

    wget -c http://buildroot.uclibc.org/downloads/${TARGET_SRC}

    if [ ! -s ${TARGET_SRC} ];then
        echo "Sorry cannot find file ${TARGET_SRC}, please check"
        exit
    fi

fi

tar xjf ${TARGET_SRC}

if [ ! -d ${TARGET} ];then
    echo "Sorry seems file broken, uncompress failed"
    exit
fi

cp ${TARGET}_4SAM9260.config ${TARGET}/.config -a

cd ${TARGET}

make

cd ../

