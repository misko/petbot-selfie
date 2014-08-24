#!/bin/bash


/bin/sh /home/pi/petbot/play_sound.sh "https://petbot.ca/static/sounds/mpu.mp3"
sudo /home/pi/petbot/single_cookie/single_cookie 10
sleep 3
/usr/bin/fswebcam -r 640x480 --skip 5 --no-underlay --no-info --no-banner --no-timestamp --quiet /dev/shm/petbot_selfie_after.jpg
/usr/bin/composite -gravity southeast -quality 100 tiny_logo.png $1 /dev/shm/petbot_selfie.jpg
/usr/bin/composite -gravity southeast -quality 100 tiny_logo.png /dev/shm/petbot_selfie_after.jpg /dev/shm/petbot_selfie_thanks.jpg
#export EMAIL=selfie@petbot.ca
/usr/bin/mutt -s "Atos - This is me? $2"  -a /dev/shm/petbot_selfie.jpg -a /dev/shm/petbot_selfie_thanks.jpg -- mouse9911@gmail.com < /dev/null
rm /dev/shm/petbot_selfie.jpg
rm /dev/shm/petbot_selfie_after.jpg /dev/shm/petbot_selfie_thanks.jpg

