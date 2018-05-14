#!/bin/sh
echo -en '\14' > /dev/lcd
while true;
do
#	echo -en "\033[f`date +\"%A %e %b\"`\n`date +%r`" > /dev/lcd
	echo -en "\14`date +\"%A %e %b\"`\n`date +%r`" > /dev/lcd
	sleep 1s
done
