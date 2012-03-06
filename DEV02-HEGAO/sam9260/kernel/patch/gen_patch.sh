#!/bin/sh
# Description:  This shell script used to generate the patch file
#      Author:  WenJing<wenjing0101 at gmail.com>
#    Changlog:
#         1,    Version 1.0.0(2011.10.11), initialize first version 
#         2,    Version 1.1.0(2012.01.19), separate config file from kernel path
#         3,    Version 1.1.1(2012.02.21), select mode
#               

PWD=`pwd`
CUR_PATH="$PWD"

KER_NAME="linux-3.0"

#===============================================================
#               Functions forward definition                   =
#===============================================================
sup_plat=("" "concentrator" "sam9260")
function select_plat()
{
   echo "Current support platform:"
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


function disp_banner()
{
   echo ""
   echo "+------------------------------------------+"
   echo "|   Generate $KER_NAME patch for $PLATFORM  "
   echo "+------------------------------------------+"
   echo ""
}

# If not define default platform, then let user choose a one
if [ -z $PLATFORM ] ; then
    select_plat
fi

NEW_KER_NAME=$KER_NAME-$PLATFORM
KER_PACKET=$KER_NAME.tar.bz2
KER_SRC=$CUR_PATH/../$KER_NAME

#Display the paraments
#    echo PLATFORM=$PLATFORM
#    echo src=$KER_SRC
#    echo dest_packet=$KER_PACKET

# Check latest source code exist or not
if [ ! -d $KER_SRC ] ; then
    echo "+-------------------------------------------------------------------"
    echo "|  ERROR: Source code \"$KER_SRC\" doesn't exist!"
    echo "+-------------------------------------------------------------------"
    exit;
fi


echo "+----------------------------------------------------------"
echo "|            Clean up the new source code                  "
echo "+----------------------------------------------------------"

cd $KER_SRC
rm -f uImage*.gz
rm -f cscope.*
rm -f tags
rm -f svnrev.h

if [ -s .config ];then
    echo "+----------------------------------------------------------"
    echo "|            Get the current kernel config file            "
    echo "+----------------------------------------------------------"
    mv .config $CUR_PATH/$NEW_KER_NAME.config
else
    echo "+----------------------------------------------------------"
    echo "|            Where is the kernel config file ?             "
    echo "+----------------------------------------------------------"
    exit;
fi

make distclean
cd ..

#now we are kernel dir
mv $KER_NAME $NEW_KER_NAME 

# Check original source code packet exist or not
if [ ! -s $KER_PACKET ] ; then
    echo "+-------------------------------------------------------------------"
    echo "| ERROR:  Orignal source code packet doesn't exist!"
    echo "| $KER_PACKET"
    echo "+-------------------------------------------------------------------"
    exit;
fi

echo "+------------------------------------------------------------------------"
echo "|           Decrompress orignal source code packet                       "
echo "|           tar -xjf $KER_PACKET																				 "
echo "+------------------------------------------------------------------------"
tar -xjf $KER_PACKET

echo "+------------------------------------------------------------------------"
echo "|            Generate patch file \"$NEW_SRC.patch\"                      "
echo "+------------------------------------------------------------------------"

echo "diff -uNr $KER_NAME $NEW_KER_NAME > $NEW_KER_NAME.patch"
diff -uNr $KER_NAME $NEW_KER_NAME > $NEW_KER_NAME.patch

rm -rf $KER_NAME
mv $NEW_KER_NAME $KER_NAME
mv $CUR_PATH/../$NEW_KER_NAME.patch $CUR_PATH/

