#!/bin/bash
# This file is used to generate dkms.conf file conf path.

Usage()
{
	echo "Usage: ./tools/dkms-conf.sh > ./tools/tmp"
}

C_PATH=`pwd`
DIR_NAME=`echo "$C_PATH" | awk -F '/' '{print $NF}'`
if [ "$DIR_NAME" != "backport-wifi-dkms" ];then
	Usage; exit;
fi

if [ -n $JOBS ];then
        JOBS=`grep -c ^processor /proc/cpuinfo 2>/dev/null`
        if [ ! $JOBS ];then
                JOBS="1"
        fi
fi
export MAKEFLAGS=-j$JOBS

make clean > /dev/null
make > /dev/null

ALL_KO=`find -name *.ko | sort`

#./drivers/staging/r8188eu/r8188eu.ko
COUNT=0
for KO in `echo "$ALL_KO" | sed 's:\./::g'`
do
	BUILT_MODULE_NAME=$(echo $KO | awk -F '[/.]' '{print $(NF-1)}')
	echo BUILT_MODULE_NAME[$COUNT]='"'$BUILT_MODULE_NAME'"'
	DEST_MODULE_NAME=$BUILT_MODULE_NAME
	echo DEST_MODULE_NAME[$COUNT]='"'$DEST_MODULE_NAME'"'
	BUILT_MODULE_LOCATION=$(echo $KO | awk -F "/$BUILT_MODULE_NAME.ko" '{print $1}')
	echo BUILT_MODULE_LOCATION[$COUNT]='"'$BUILT_MODULE_LOCATION'"'
	DEST_MODULE_LOCATION="/updates/$BUILT_MODULE_LOCATION"	
	echo DEST_MODULE_LOCATION[$COUNT]='"'$DEST_MODULE_LOCATION'"'
	COUNT=$(($COUNT+1))
done

make clean > /dev/null
