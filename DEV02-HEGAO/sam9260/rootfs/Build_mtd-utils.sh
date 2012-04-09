#!/bin/sh

#+--------------------------------------------------------------------------------------------
#|Description:  This shell script used to download lzo,zlib,mtd-utils source code
#|              and cross compile it for ARM Linux, all is static cross compile.
#|     Author:  GuoWenxue <guowenxue@gmail.com>
#|  ChangeLog:
#|           1, Initialize 1.0.0 on 2011.04.12
#+--------------------------------------------------------------------------------------------

PRJ_PATH=`pwd`

LZO="lzo-2.04"
ZLIB="zlib-1.2.5"
e2fsprogs_ver=1.42
mtd="mtd-utils-1.4.9"

ARCH=arm926t
INST_PATH=${PRJ_PATH}/../mnt/usr/bin/

sup_arch=("" "arm926t" "arm920t" )

function select_arch()
{
   echo "Current support ARCH: "
   i=1
   len=${#sup_arch[*]}

   while [ $i -lt $len ]; do
     echo "$i: ${sup_arch[$i]}"
     let i++;
   done

   echo "Please select: "
   index=
   read index
   ARCH=${sup_arch[$index]}
}

function decompress_packet()
(
   echo "+---------------------------------------------+"
   echo "|  Decompress $1 now"  
   echo "+---------------------------------------------+"

    ftype=`file "$1"`
    case "$ftype" in
       "$1: Zip archive"*)
           unzip "$1" ;;
       "$1: gzip compressed"*)
           if [ `expr "$1" : ".*.tar.*" ` ] ; then
               tar -xzf $1
           else
               gzip -d "$1"
           fi ;;
       "$1: bzip2 compressed"*)
           if [ `expr "$1" : ".*.tar.*" ` ] ; then
               tar -xjf $1
           else
               bunzip2 "$1"
           fi ;;
       "$1: POSIX tar archive"*)
           tar -xf "$1" ;;
       *)
          echo "$1 is unknow compress format";;
    esac
)

if [ -z $ARCH ] ; then
  select_arch
fi

#Set cross compiler
CROSS=arm-linux-
CROSS_PATH="/opt/buildroot-2011.11/${ARCH}/usr/bin"
#export PATH=/usr/local/bin:/bin:/usr/bin:/usr/local/sbin:/usr/sbin:/sbin:$CROSS_PATH
export CC=${CROSS}gcc
export AR=${CROSS}ar
export LD=${CROSS}ld
export STRIP=${CROSS}strip
export NM=${CROSS}nm
export RANLIB=${CROSS}ranlib
export OBJDUMP=${CROSS}objdump

echo "+----------------------------------------+"
echo "|  Cross compile $LZO now "  
echo "| Crosstool:  $CROSS"
echo "+----------------------------------------+"

# Download lzo source code packet
if [ ! -s $LZO.tar.gz ] ; then
   wget http://www.oberhumer.com/opensource/lzo/download/$LZO.tar.gz
fi

# Decompress lzo source code packet
if [ ! -d $LZO ] ; then
    decompress_packet $LZO.tar.*
fi

# Cross compile lzo

cd  $LZO
if [ ! -s src/.libs/liblzo*.a ] ; then
unset LDFLAGS
./configure  --host=arm-linux --enable-static --disable-shared
make
fi
cd  -


echo "+----------------------------------------+"
echo "|  Cross compile $ZLIB now "  
echo "| Crosstool:  $CROSS"
echo "+----------------------------------------+"

# Download zlib source code packet
if [ ! -s $ZLIB.tar* ] ; then
#wget http://www.zlib.net/$ZLIB.tar.gz
   #wget http://www.imagemagick.org/download/delegates/$ZLIB.tar.bz2
   wget http://down1.chinaunix.net/distfiles/$ZLIB.tar.bz2
fi

# Decompress zlib source code packet
if [ ! -d $ZLIB ] ; then
    decompress_packet $ZLIB.tar.*
fi

#Cross compile zlib

cd  $ZLIB
if [ ! -s libz.a ] ; then
unset LDFLAGS
./configure  --static 
make
fi
cd  -


echo "+----------------------------------------+"
echo "|  Cross compile e2fsprogsV$e2fsprogs_ver now "  
echo "| Crosstool:  $CROSS"
echo "+----------------------------------------+"
#e2fsprogs is for UBIFS, download e2fsprogs source code packet
if [ ! -s e2fsprogs-$e2fsprogs_ver.tar.gz ] ; then
  wget http://nchc.dl.sourceforge.net/project/e2fsprogs/e2fsprogs/$e2fsprogs_ver/e2fsprogs-$e2fsprogs_ver.tar.gz
fi

# Decompress e2fsprogs source code packet
if [ ! -d e2fsprogs-$e2fsprogs_ver ] ; then
    decompress_packet e2fsprogs-$e2fsprogs_ver.tar.*
fi

cd e2fsprogs-$e2fsprogs_ver
if [ ! -s lib/libuuid.a ] ; then
  ./configure --host=arm-linux --build=i686-pc-linux-gnu --enable-elf-shlibs
  make
fi
cd -

echo "+----------------------------------------+"
echo "|  Cross compile mtd-utils now "  
echo "| Crosstool:  $CROSS"
echo "+----------------------------------------+"

if [ ! -s ${mtd}.tar.bz2 ] ; then
   wget ftp://ftp.infradead.org/pub/mtd-utils/${mtd}.tar.bz2
fi 
decompress_packet ${mtd}.tar.bz2

# download mtd-utils source code
#if [ ! -d  mtd-utils* ] ; then
   #git clone git://git.infradead.org/mtd-utils.git

#fi

cd ${mtd}
#Add the CROSS tool in file common.mk

head -1 common.mk | grep "CROSS="
if [ 0 != $? ] ; then 
    echo "Modify file common.mk" 
    sed -i -e 1i"CROSS=$CROSS" common.mk 
fi 

line=`sed -n '/CFLAGS ?= -O2 -g/=' common.mk ` 
if [ ! -z $line ] ; then 
    sed -i -e ${line}s"|.*|CFLAGS ?= -O2 -g --static|" common.mk 
fi

unset LDFLAGS

export CFLAGS="-DWITHOUT_XATTR -I$PRJ_PATH/$ZLIB -I$PRJ_PATH/$LZO/include -I$PRJ_PATH/e2fsprogs-$e2fsprogs_ver/lib"
export ZLIBLDFLAGS=-L$PRJ_PATH/$ZLIB
export LZOLDFLAGS=-L$PRJ_PATH/$LZO/src/.libs/
export LDFLAGS="-static -L $PRJ_PATH/e2fsprogs-$e2fsprogs_ver/lib"
make

set -x
cd arm-linux/
${CROSS}strip nandwrite flash_erase  nanddump 
sudo cp nandwrite $INST_PATH/.nandwrite
sudo cp flash_erase $INST_PATH/.flash_erase
sudo cp nanddump $INST_PATH/.nanddump

