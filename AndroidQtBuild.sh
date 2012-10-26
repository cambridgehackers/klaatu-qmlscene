#!/bin/bash
set -e
set -x

OUTFILE=$1
QT_DIR=`dirname $1`

HOST_DIR=$ANDROID_HOST_OUT


cat > $OUTFILE <<EOF
#include <stdio.h>
void main()
{
  printf("This is a simple C file to force the Qt Make process to kick off. rm the .c to force Qt to rebuild. Android HACK!\n");
}
EOF

NUM_PROCS=`cat /proc/cpuinfo  | egrep processor | cat -n | awk {'print $1'} | tail -1`
cd $QT_DIR;


$HOST_DIR/bin/qmake
echo "APO=$ANDROID_PRODUCT_OUT"
echo "NUM_PROCS=$NUM_PROCS"
echo "me=$0"

make -j$NUM_PROCS
make install


#the following makes the qt make system run everytime you build android. If you comment it out 
# you either need to touch the AndroidQtBuild.sh or 
# rm qtbase-dependencyForcer.c
#touch $ANDROID_BUILD_TOP/$0