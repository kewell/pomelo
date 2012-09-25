#/bin/sh

TARGET="linux-3.6-rc4"
TARGET_SRC="linux-3.6-rc4.tar.xz"
MKIMAGE="uboot/u-boot-2012.07/tools/mkimage"
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

        wget -c http://www.kernel.org/pub/linux/kernel/v3.0/${TARGET_SRC}

        if [ ! -s ${TARGET_SRC} ];then
            echo "Sorry cannot find file ${TARGET_SRC}, please check"
            exit
        fi

    fi

    tar xJf ${TARGET_SRC}

    if [ ! -d ${TARGET} ];then
        echo "Sorry seems file broken, uncompress failed"
        exit
    fi

fi

cd ${TARGET}

if [ ! -s .config ];then
    cp ../${TARGET}_4SAM9260.config .config -a
fi

make ARCH=arm CROSS_COMPILE=${CROSS_PATH}/arm-linux-

'cp' arch/arm/boot/zImage . -f -a 

./../${MKIMAGE} -A arm -O linux -n AT91SAM9260 -C NONE -a 0x20008000 -e 0x20008000 -d zImage uImage.gz 

cd ../

