#!/bin/sh 
#Loading code
if [ $# "<" 1 ] ; then
	echo "Usage: $0 <module_name>" 
	exit -1 
else 
	module="$1" 
	#echo "Module Device: $1" 
fi 
insmod -f ./${module}.ko || exit 1 
major=`cat /proc/devices | awk "\\$2==\"$module\" {print \\$1}"| head -n 1` 
for i in `seq 0 3`
do
	mknod /dev/${module}$i c $major $i
	chmod a+rw /dev/${module}$i
done
exit 0
