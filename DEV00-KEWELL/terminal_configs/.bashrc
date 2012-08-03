# .bashrc

# User specific aliases and functions

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

# Description:  This function used to format all the source code in current forlder
#               and convert source code file format from windows to linux
#      Author:  WenJing(wenjing0101@gmail.com)
#     Version:  1.0.0 (Release by wenjing on 18th Fri. 2010)

function format_src ()
{

  find -iname "*.c" -exec dos2unix {} \;
  find -iname "*.h" -exec dos2unix {} \;
  find -iname "makefile" -exec dos2unix {} \;
  find -iname "Makefile" -exec dos2unix {} \;

  # -npro   不要读取indent的配置文件.indent.pro
  # -kr     使用Kernighan&Ritchie的格式
  # -i4     设置缩排的格数为4
  # -di28   将声明区段的变量置于指定的栏位(28) 
  # -ts4    设置tab的长度为4 
  # -bls    定义结构，"struct"和"{"分行
  # -bl     if(或是else,for等等)与后面执行区段的”{“不同行，且”}”自成一行。
  # -bli0   设置{ }缩排的格数为0 
  # -cli2   使用case时，switch缩排的格数
  # -ss     若for或whiile区段只有一行时，在分号前加上空格
  # -bad    在声明区段后加上空白行
  # -bbb    块注释前加空行
  # -bap    函数结束后加空行
  # -sc     在每行注释左侧加上星号(*)。
  # -bc     在声明区段中，若出现逗号即换行。
  # -sob    删除多余的空白行
  # -l100   非注释行最长100
  # -ncs    不要在类型转换后面加空格
  # -nce    不要将else置于”}”之后
  # -nut    不要使用tab来缩进

  INDET_FORMAT="-npro -kr -i4 -ts4 -bls -bl -bli0 -cli2 -ss -bap -sc -sob -l100 -ncs -nce -nut"

  find -iname "*.c" -exec indent $INDET_FORMAT {} \;
  find -iname "*.h" -exec indent $INDET_FORMAT {} \;

  find -iname "*.h~" | xargs rm -rf {} \;
  find -iname "*.c~" | xargs rm -rf {} \;
}

# Description:  This function used to generate the NUP file
#      Author:  WenJing<wenjing0101@gmail.com>
#     Version:  1.0.0 (Release by wenjing on 24th Apr. 2011)
# 

function nup_gen ()
{
  path=`pwd`
  folder=`basename $path`
  nup_name=${folder}.nup

  #first, compress these folders and name them as ${foldername}.nup
  for i in N??*; do
       cd $i
       7za a -tzip $i.nup *
       mv $i.nup ..
       cd ..
       rm -rf $i
  done

  #second, generate the final NUP file and move it to parent folder
  7za a -tzip $nup_name *
  mv $nup_name ..
  cd ..
  rm -rf $folder
}


# Description:  This function used to uncompress the NUP file
#      Author:  WenJing<wenjing0101@gmail.com>
#     Version:  1.0.0 (Release by wenjing on 24th Apr. 2011)
# 

function nup_uncmp ()
{
  if [ $# -ne 1 ]; then
      echo "This function used to uncompress the NUP file"
      echo "Usage:   $FUNCNAME \"nup_path\"             "
      return;
  fi

  nup_path=$1
  nup_name=`basename ${nup_path}`
  work_path=`echo ${nup_name}|awk -F '.' '{print   $1}'`

  # Uncompress the TOP NUP file
  7za x $nup_path -o$work_path

  if [ ! -d  $work_path ];then
      echo "================================================================="
      echo "*  ERROR: Uncompress failure, make sure the file is zip format  *"
      echo "================================================================="
      echo ""
      return;
  fi

  # Goes into the folder and uncompress the child NUP file one by one.

  cd $work_path
  for i in *.nup; do
      fold_name=`echo $i|awk -F '.' '{print   $1}' `
      echo $fold_name
      7za x $i -o$fold_name
      rm -f $i
  done
}

