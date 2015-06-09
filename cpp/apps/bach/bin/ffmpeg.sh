#!/bin/bash
export LD_LIBRARY_PATH=/drd/software/ext/ffmpeg/lin64/lib
TMPFILE=`mktemp -t`
/drd/software/ext/ffmpeg/lin64/bin/ffmpeg -i "$1" -an -ss 00:00:02 -an -r 1 -vframes 1 -y $TMPFILE.png
convert -quality 90 -resize $3x$3 $TMPFILE.png "$2"
rm $TMPFILE.png
