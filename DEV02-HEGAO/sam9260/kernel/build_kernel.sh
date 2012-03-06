#!/bin/sh
# Descripion:  This shell script used to choose a linux kernel version to cross compile
#     Author:  WenJing <wenjing0101 at gmail.com>
#  ChangeLog:
#       1, Version 1.0.0(2011.10.25), some changes to fit in with new SVN server && Compiling server
#       2, Version 1.0.1(2012.01.19), separate kernel config file from patch package
#

PWD=`pwd`
PACKET_DIR=$PWD/
PATCH_DIR=$PWD/patch

SRC_NAME="linux-3.0"

INST_PATH=$PWD/../../../bin

#===============================================================
#               Functions forward definition                   =
#===============================================================
sup_ver=("" "linux-3.0")
function select_version()
{
   echo "Current support Linux Kernel version:"
   i=1
   len=${#sup_ver[*]}

   while [ $i -lt $len ]; do
       echo "$i: ${sup_ver[$i]}"
       let i++;
   done

   if [ $len -eq 2 ] ; then
        SRC_NAME=${sup_ver[1]}
        return;
   fi

   echo "Please select: "
   index=
   read index 

   SRC_NAME=${sup_ver[$index]}
}


sup_cpu=("" "concentrator" "sam9260")
function select_cpu()
{
   echo "Current support ARCH:"
   i=1
   len=${#sup_cpu[*]}

   while [ $i -lt $len ]; do
       echo "$i: ${sup_cpu[$i]}"
       let i++;
   done

   if [ $len -eq 2 ] ; then
        ARCH=${sup_cpu[1]}
        return;
   fi

   echo "Please select: "
   index=
   read index 

   ARCH=${sup_cpu[$index]}
}


function disp_banner()
{
   echo ""
   echo "****************************************************"
   echo "*     Cross compile $SRC_NAME for $ARCH now...       "
   echo "****************************************************"
   echo ""
}

#===============================================================
#                   Script excute body start                   =
#===============================================================

# If not define default version, then let user choose a one
if [ -z $SRC_NAME ] ; then
    select_version
fi

# If not define default version, then let user choose a one
if [ -z $ARCH ] ; then
    select_cpu
fi

PATCH_SUFFIX=-$ARCH.patch
CONFIG_SUFFIX=-$ARCH.config

disp_banner

# If $SRC_NAME not set, then abort this cross compile
if [ -z $SRC_NAME ] ; then 
    echo "ERROR: Please choose a valid version to cross compile"
    exit 1;
fi

# Check original source code packet exist or not
SRC_ORIG_PACKET=$SRC_NAME.tar.bz2

if [ ! -s $SRC_ORIG_PACKET ] ; then
    cd $PACKET_DIR
    echo "ERROR: Please Download $SRC_ORIG_PACKET to here!"
    #wget http://www.kernel.org/pub/linux/kernel/v2.6/$SRC_NAME.tar.bz2
    cd -
    exit;
fi

if [ ! -s $SRC_ORIG_PACKET ] ; then
    echo ""
    echo "ERROR:$SRC_NAME source code patcket doesn't exist:"
    echo "PATH: \"$SRC_ORIG_PACKET\""
    echo ""
    exit
fi

# Check patche file exist or not
PATCH_FILE_PATH=$PATCH_DIR/$SRC_NAME$PATCH_SUFFIX
CONFIG_FILE_PATH=$PATCH_DIR/$SRC_NAME$CONFIG_SUFFIX

if [ ! -f $PATCH_FILE_PATH -o ! -f $CONFIG_FILE_PATH ] ; then
    echo "ERROR:$SRC_NAME patch or config files doesn't exist:"
    echo ""
    echo "file $PATCH_FILE_PATH or file $CONFIG_FILE_PATH no exist"
    exit
fi

#decompress the source code packet and patch
echo "*  Decompress the source code patcket and patch now...  *"

if [ -d $SRC_NAME ] ; then
    rm -rf $SRC_NAME
fi

#Start to cross compile the source code and install it now
rm svnrev.h -rf;svnrev *
tar -xjf $SRC_ORIG_PACKET
cd ${SRC_NAME}
patch -p1 < $PATCH_FILE_PATH
'cp' $CONFIG_FILE_PATH -a .config
make

'cp' -a arch/arm/boot/zImage .
mkimage -A arm -O linux -T kernel -C none -a 20008000 -e 20008000 -n "Linux Kernel" -d zImage uImage.gz
#rm -f zImage
VERSION=`echo $SRC_NAME | awk -F "-" '{print $2}'`
#set -x
#'cp' -a uImage.gz $INST_PATH/uImage-$ARCH-$VERSION.gz

