#!/bin/bash

sudo /home/pi/petbot/single_cookie/single_cookie 10
/usr/bin/mutt -s "Atos - This is me? $2"  -a $1 -- mouse9911@gmail.com < /dev/null

