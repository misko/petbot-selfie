#!/bin/bash

DATE=`date +%s`.png
mv /dev/shm/petbot_selfie_gif.png /dev/shm/gif/$DATE

if [ `ls /dev/shm/gif/*.png | wc -l` -gt 10 ]; then 
	killall fswebcam
fi
