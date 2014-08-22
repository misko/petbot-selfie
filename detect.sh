#!/bin/bash

a=0

while [ 1 -lt 2 ] ; do
	fswebcam -r 640x480 --skip 10 --no-underlay --no-info --no-banner --no-timestamp --quiet /dev/shm/out.jpg
	prob=`sudo ./load ccv2012.ntwk -2 ./model_2012_l2_p5p50_n4 /dev/shm/out.jpg | awk '{print int($1*10000)}'`
	echo PROB is $prob
	if [ $prob -gt 4000 ]; then 
		cp out.jpg ${a}.jpg
		date
		echo HEY ATOS! 
		a=`expr $a + 1`
	fi
	exit
done;
