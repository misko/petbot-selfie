#!/bin/bash


rm -rf /dev/shm/gif
mkdir -p /dev/shm/gif
/usr/bin/fswebcam -r 1700x120 --skip 5 --no-underlay --no-info --no-banner --no-timestamp --quiet --loop 2 /dev/shm/petbot_selfie_gif.png --exec /home/pi/petselfie/gif_capt.sh  



