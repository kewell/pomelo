#!/bin/sh

TARGET="u-boot-2012.07"
TARGET_SRC="u-boot-2012.07.tar.bz2"
CROSS_PATH=/opt/buildroot-2012.08/output/host/usr/bin

if [ ! -d ${CROSS_PATH} ];then
    echo "The Cross Compile not exsit"
    exit 1
fi

if [ ! -d ${TARGET} ];then

    if [ ! -f ${TARGET_SRC} ];then

        which wget

        if [ ! $? -eq 0 ];then
            echo "Sorry cannot find wget command, please install first"
            exit
        fi

        wget -c ftp://ftp.denx.de/pub/u-boot/${TARGET_SRC}

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

patch -p1 < ../${TARGET}_4SAM9260.patch
make at91sam9260ek_nandflash_config
make CROSS_COMPILE=${CROSS_PATH}/arm-linux-

cd ../

