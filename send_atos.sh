#!/bin/bash

sudo /home/pi/petbot/single_cookie/single_cookie 10
/usr/bin/composite -gravity southeast -quality 100 tiny_logo.png $1 /dev/shm/petbot_selfie.jpg
/usr/bin/mutt -s "Atos - This is me? $2"  -a /dev/shm/petbot_selfie.jpg -- mouse9911@gmail.com < /dev/null
rm /dev/shm/petbot_selfie.jpg

