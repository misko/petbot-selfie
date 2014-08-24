gcc `pkg-config gstreamer-1.0 --cflags` -I/opt/vc/include/IL/ gst-record.c -o gst-record `pkg-config gstreamer-1.0 --libs` -O3 -g -Wall
