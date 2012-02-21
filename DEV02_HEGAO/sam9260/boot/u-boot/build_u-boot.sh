#!/bin/sh
# Descripion:  This shell script used to choose a u-boot version to cross compile
#     Author:  WenJing <wenjing0101 at gmail.com>
#  ChangeLog:
#       1, Version 1.0.0(2011.04.01), initialize first version 
#       2, Version 1.0.1(2011.04.03), modify it to compatible with Linux kernel build script
#

PWD=`pwd`
CUR_PATH=$PWD

#===============================================================
#               Functions forward definition                   =
#===============================================================
sup_plat=("" "concentrator" "sam9260")
function select_plat()
{
   echo "Current support PLATFORM:"
   i=1
   len=${#sup_plat[*]}

   while [ $i -lt $len ]; do
       echo "$i: ${sup_plat[$i]}"
       let i++;
   done

   if [ $len -eq 2 ] ; then
       PLATFORM=${sup_plat[1]}
       return;
   fi

   echo "Please select: "
   index=
   read index 

   PLATFORM=${sup_plat[$index]}
}

sup_ver=("" "u-boot-2011.09" "u-boot-2010.09")
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
        SRC_VER=${sup_ver[1]}
        return;
   fi

   echo "Please select: "
   index=
   read index

   SRC_VER=${sup_ver[$index]}
}

function disp_banner()
{
   echo ""
   echo "+------------------------------------------+"
   echo "|      Build $SRC_VER for $PLATFORM             "
   echo "+------------------------------------------+"
   echo ""
}

#===============================================================
#                   Script excute body start                   =
#===============================================================

# If not define default version, then let user choose a one
if [ -z $SRC_VER ] ; then
    select_version
fi

# If not define default PLATFORM, then let user choose a one
if [ -z $PLATFORM ] ; then
    select_plat
fi

disp_banner    #Display this shell script banner

# If $SRC_VER not set, then abort this cross compile
if [ -z $SRC_VER ] ; then 
    echo "ERROR: Please choose a valid version to cross compile"
    exit 1;
fi

# Check original source code packet exist or not
SRC_ORIG_PACKET=$CUR_PATH/$SRC_VER.tar.bz2

if [ ! -s $SRC_ORIG_PACKET ] ; then
    echo ""
    echo "ERROR:$SRC_VER source code patcket doesn't exist:"
    echo "Please download U-boot source code to packet path:"
    echo "PATH: \"$SRC_ORIG_PACKET\""
    echo ""
    exit
fi

# Check patche file exist or not
PATCH_FILE=$CUR_PATH/patch/$SRC_VER-$PLATFORM.patch

if [ ! -f $PATCH_FILE ] ; then
    echo "ERROR:$SRC_VER patch file doesn't exist:"
    echo "PATH: \"$PATCH_FILE\""
    echo ""
    exit
fi

#decompress the source code packet and patch
echo "*  Decompress the source code patcket and patch now...  *"

if [ -d $SRC_VER ] ; then
    rm -rf $SRC_VER
fi

#Remove old source code
tar -xjf $SRC_ORIG_PACKET
patch -p0 < $PATCH_FILE

#Start to cross compile the source code and install it now
cd $SRC_VER

make at91sam9260ek_nandflash_config && make
make tools && make env
set -x
find -name fw_printenv -exec mv '{}' $CUR_PATH \; 
find -name mkimage -exec mv '{}' $CUR_PATH \; 
'cp' -af u-boot.bin $SRC_VER-${PLATFORM}.bin
'cp' -af u-boot.bin $SRC_VER-${PLATFORM}.bin

