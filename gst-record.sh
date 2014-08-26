#!/bin/bash


if [ $# -ne 4 ] ; then
	echo $0 resolution bitrate seconds fileout
	exit
fi


resolution=$1
bitrate=$2
seconds=$3
filename=$4


x=0
y=0
if [ "$resolution" = "320x240" ]; then
	x=320
	y=240
#elif [ "$resolution" = "640x480" ]; then
#	x=640
#	y=480
else
	echo Unsupported resolution
	exit
fi

rm -f $filename
touch $filename
retries=0
while [ "`du -sm $filename | awk '{print $1}'`" -lt 1 ]; do
	/usr/bin/gst-launch-1.0 v4l2src do-timestamp=true num-buffers=`expr 30 \* $3` io-mode=1 ! queue ! videorate ! video/x-raw, width=$x, height=$y, framerate=30/1 ! gdkpixbufoverlay location=/home/pi/petbot-selfie/petbot_video_watermark.png offset-x=180 offset-y=180 ! omxh264enc target-bitrate=$bitrate control-rate=variable ! h264parse ! qtmux dts-method=asc presentation-time=1 ! filesink location=$filename
	#echo /usr/bin/gst-launch-1.0 v4l2src do-timestamp=true num-buffers=`expr 30 \* $3` ! video/x-raw, width=$x, height=$y, framerate=30/1 ! queue !  videorate ! video/x-raw, width=640, height=480, framerate=30/1 ! gdkpixbufoverlay location=/home/pi/petbot-selfie/petbot_video_watermark.png offset-x=180 offset-y=180 ! omxh264enc target-bitrate=$bitrate control-rate=variable ! h264parse ! qtmux dts-method=asc presentation-time=1 ! filesink location=$filename # 2> /dev/null > /dev/null
	#exit
	#exit
	if [ "`du -sm $filename | awk '{print $1}'`" -gt 0 ]; then
		echo Captured ok!
		exit
	fi
	echo Reloading ...
	sudo /home/pi/petbot-selfie/reload.sh
	retries=`expr $retries + 1`
	if [ "$retries" -gt 8 ]; then
		echo Failed to capture
		exit
	fi
done
