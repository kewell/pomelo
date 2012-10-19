#!/bin/sh
# Description:  This shell script used to generate the patch file
#      Author:  WenJing<wenjing0101 at gmail.com>
#    Changlog:
#         1,    Version 1.0.0(2011.10.11), initialize first version 
#         2,    Version 1.1.0(2012.01.19), separate config file from kernel path
#         3,    Version 1.1.1(2012.02.21), select mode
#         4,    Version 1.1.2(2012.03.16), select mode
#               

PWD=`pwd`
CUR_PATH="${PWD}"

KER_VER="linux-3.6-rc4"
DEVICE="4SAM9260"

function disp_banner()
{
   echo ""
   echo "+------------------------------------------+"
   echo "|   Generate ${KER_VER} patch for ${DEVICE}  "
   echo "+------------------------------------------+"
   echo ""
}

# If not define default version, then let user choose a one
if [ -z ${KER_VER} ] ; then
    echo ""
    echo "Current support Linux Kernel version:"
    echo ""

    select KER_VER in "3.6-rc4" "3.0";do
        break;
    done

    KER_VER="linux-${KER_VER}"
fi

# If not define default device, then let user choose a one
if [ -z ${DEVICE} ] ; then
    echo ""
    echo "Current support DEVICE:"
    echo ""

    select DEVICE in "4SAM9260" "sam9260";do
        break;
    done
fi

NEW_KER_VER=${KER_VER}_${DEVICE}
KER_PACKET=${KER_VER}.tar.xz
KER_SRC=${CUR_PATH}/../${KER_VER}

#Display the paraments
#    echo DEVICE=${DEVICE
#    echo src=${KER_SRC
#    echo dest_packet=${KER_PACKET

# Check latest source code exist or not
if [ ! -d ${KER_SRC} ] ; then
    echo "+-------------------------------------------------------------------"
    echo "|  ERROR: Source code \"${KER_SRC}\" doesn't exist!"
    echo "+-------------------------------------------------------------------"
    exit;
fi


echo "+----------------------------------------------------------"
echo "|            Clean up the new source code                  "
echo "+----------------------------------------------------------"

cd ${KER_SRC}
rm -f uImage*.gz
rm -f zImage 
rm -f cscope.*
rm -f tags
rm -f svnrev.h

if [ -s .config ];then
    echo "+----------------------------------------------------------"
    echo "|            Get the current kernel config file            "
    echo "+----------------------------------------------------------"
    mv .config ${CUR_PATH}/${NEW_KER_VER}.config
else
    echo "+----------------------------------------------------------"
    echo "|            Where is the kernel config file ?             "
    echo "+----------------------------------------------------------"
    exit;
fi

make distclean
cd ..

#now we are kernel dir
mv ${KER_VER} ${NEW_KER_VER}

# Check original source code packet exist or not
if [ ! -s ${KER_PACKET} ] ; then
    echo "+-------------------------------------------------------------------"
    echo "| ERROR:  Orignal source code packet doesn't exist!"
    echo "| ${KER_PACKET}"
    echo "+-------------------------------------------------------------------"
    exit;
fi

echo "+------------------------------------------------------------------------"
echo "|           Decrompress orignal source code packet                       "
echo "|           tar -xJf ${KER_PACKET}                                       "
echo "+------------------------------------------------------------------------"
tar -xJf ${KER_PACKET}

echo "+------------------------------------------------------------------------"
echo "|            Generate patch file \"${NEW_SRC}.patch\"                      "
echo "+------------------------------------------------------------------------"

echo "diff -uNr ${KER_VER} ${NEW_KER_VER} > ${NEW_KER_VER}.patch"
diff -uNr ${KER_VER} ${NEW_KER_VER} > ${NEW_KER_VER}.patch

rm -rf ${KER_VER}
mv ${NEW_KER_VER} ${KER_VER}
mv ${CUR_PATH}/../${NEW_KER_VER}.patch ${CUR_PATH}/

