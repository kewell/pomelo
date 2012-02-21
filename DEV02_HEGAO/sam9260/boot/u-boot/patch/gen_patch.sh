#!/bin/sh
# Description:  This shell script used to generate the patch file
#      Author:  GuoWenxue<wenjing at gmail.com>
#    Changlog:
#         1,    Version 1.0.0(2011.04.01), initialize first version 
#         2,    Version 1.0.1(2012.02.21), add mutil original version support 
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
   echo "|  Generate $SRC_VER patch for $PLATFORM    "
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

SRC_ARCHIVE=$CUR_PATH/../$SRC_VER.tar.bz2
SRC_PATH=$CUR_PATH/../$SRC_VER

# Check latest source code exist or not
if [ ! -d $SRC_PATH ] ; then
    echo "+-------------------------------------------------------------------"
    echo "|  ERROR: Source code \"$SRC_PATH\" doesn't exist!"
    echo "+-------------------------------------------------------------------"
    exit;
fi

# Check original source code packet exist or not
if [ ! -s $SRC_ARCHIVE ] ; then
    echo "+-------------------------------------------------------------------"
    echo "| ERROR:  Orignal source code packet doesn't exist!"
    echo "| $SRC_ARCHIVE"
    echo "+-------------------------------------------------------------------"
    exit;
fi

# Clean up the source code
echo "+----------------------------------------------------------"
echo "|            Clean up the new source code                  "
echo "+----------------------------------------------------------"
NEW_SRC_PATH=$SRC_PATH-$PALTFORM
cd $SRC_PATH
rm -f tags cscope*
make distclean
cd ..
mv $SRC_PATH $NEW_SRC_PATH

echo "+----------------------------------------------------------"
echo "|         Decompress original source code packet           "
echo "+----------------------------------------------------------"
tar -xjf $SRC_ARCHIVE

# Generate the patch file
echo "+------------------------------------------------------------------------"
echo "| Generate patch file \"$NEW_SRC.patch\"                                 "
echo "+------------------------------------------------------------------------"
diff -Nuar $SRC_PATH $NEW_SRC_PATH > $SRC_VER-$PALTFORM.patch

# Rollback to the original status
rm -rf $SRC_PATH
mv $NEW_SRC_PATH $SRC_PATH
mv $CUR_PATH/../$SRC_VER-$PALTFORM.patch $CUR_PATH/

