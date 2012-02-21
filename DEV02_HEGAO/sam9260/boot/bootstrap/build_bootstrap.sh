#********************************************************************************
#      Copyright:  (C) 2011 R&D of San Fran Electronics Co., LTD  
#                  All rights reserved.
#
#        Filename:  build.sh
#    Description:  This file used to auto uncompress the bootstrap and cross 
#                  compile it for AT91SAM9260 BOARD.
#                 
#        Version:  1.0.0(10/12/2011~10/12/2011)
#         Author:  Guo Wenxue <guowenxue@gmail.com>
#      ChangeLog:  1, Release initial version on "10/12/2011 02:36:42 PM"
#
#  AT91SAM Bootstrap Binary download adderss:
#          http://www.at91.com/linux4sam/bin/view/Linux4SAM/AT91Bootstrap
#
#  AT91SAM Bootstrap source code download adderss:
#          http://www.atmel.com/dyn/products/tools_card.asp?tool_id=4093
#                 
#********************************************************************************

TOP_SRC=`pwd`
INST_PATH=$TOP_SRC/../../bin
VERSION=1.16
SRC_NAME=AT91Bootstrap${VERSION}
BOARD=at91sam9260ek
BOOT_ROM=nandflash
CROSS_TOOL=/opt/buildroot-2011.02/arm926t/usr/bin/arm-linux-


#===============================================================
#               Select BOARD type function                       =
#===============================================================
sup_BOARD=("" "at91sam9260ek" "at91sam9g20ek")
function select_BOARD()
{
   echo "Current support BOARD:" 
   i=1 
   len=${#sup_BOARD[*]} 
   while [ $i -lt $len ]; do 
       echo "$i: ${sup_BOARD[$i]}" 
       let i++; 
   done 
   
   if [ $len -eq 2 ] ; then 
       BOARD=${sup_BOARD[1]} 
       return;
   fi 
   
   echo "Please select: " 
   index= 
   read index 
   BOARD=${sup_BOARD[$index]} 
}

#===============================================================
#               Select boot from storage                       =
#===============================================================
sup_boot=("" "dataflash" "nandflash")
function select_boot()
{
   echo "Current support boot storage:" 
   i=1 
   len=${#sup_boot[*]} 
   while [ $i -lt $len ]; do 
       echo "$i: ${sup_boot[$i]}" 
       let i++; 
   done 
   
   if [ $len -eq 2 ] ; then 
       BOARD=${sup_boot[1]} 
       return;
   fi 
   
   echo "Please select: " 
   index= 
   read index 
   BOOT_ROM=${sup_boot[$index]} 
}


function disp_banner()
{
   echo "" 
   echo "+----------------------------------------------------------------+" 
   echo "|  Build $SRC_NAME for $BOARD with $BOOT_ROM                        " 
   echo "+----------------------------------------------------------------+" 
   echo ""
}

if [ -z $BOARD ] ; then
    select_BOARD
fi

if [ -z $BOOT_ROM ] ; then
    select_boot
fi

if [ ! -f ${SRC_NAME}.zip ] ; then
    wget http://www.atmel.com/dyn/resources/prod_documents/AT91Bootstrap1.16.zip
fi
disp_banner    #Display this shell script banner

#Remove the exist source code
if [ -d Bootstrap-v${VERSION} ] ; then
    rm -rf Bootstrap-v${VERSION}
fi

#Decompress the source code
unzip ${SRC_NAME}.zip 1>/dev/null
cd Bootstrap-v${VERSION}/board/${BOARD}/${BOOT_ROM}

#Modify the cross compiler
line=`sed -n '/^CROSS_COMPILE=/=' Makefile`
sed -i -e ${line}s"|.*|CROSS_COMPILE=${CROSS_TOOL}|" Makefile

#Cross compile the bootstrap and install it
make
cp ${BOOT_ROM}_${BOARD}.bin ${TOP_SRC}/bootstrapV${VERSION}_${BOOT_ROM}_${BOARD}.bin
cp ${BOOT_ROM}_${BOARD}.bin ${INST_PATH}/bootstrapV${VERSION}_${BOOT_ROM}_${BOARD}.bin

#Remove the source code directory
cd ${TOP_SRC} && rm -rf Bootstrap-v${VERSION}
