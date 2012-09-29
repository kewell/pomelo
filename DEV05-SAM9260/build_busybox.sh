#!/bin/sh

TARGET="busybox-1.20.2"
TARGET_SRC="busybox-1.20.2.tar.bz2"
CROSS_PATH=/opt/buildroot-2012.08/output/host/usr/bin

if [ ! -d ${CROSS_PATH} ];then
    echo "The Cross Compile not exsit"
    exit 1
fi

if [ ! -d ${TARGET} ];then

    if [ ! -f ${TARGET_SRC} ];then

        which wget

        if [ ! $? -eq 0];then
            echo "Sorry cannot find wget command, please install first"
            exit
        fi

        wget -c http://www.busybox.net/downloads/busybox-1.20.2.tar.bz2/${TARGET_SRC}

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

fi

cd ${TARGET}

if [ ! -s .config ];then
    'cp' ../${TARGET}_4SAM9260.config .config -a
fi

#make ARCH=arm CROSS_COMPILE=${CROSS_PATH}/arm-linux-
make CROSS_COMPILE=${CROSS_PATH}/arm-linux-

cd ../

