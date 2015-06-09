#!/bin/bash
TMPFILE=`mktemp -t`.png
/drd/software/int/bin/lin64/dcraw -T -c "$1" > $TMPFILE
convert -quality 90 -resize $3x$3 $TMPFILE "$2"
rm $TMPFILE
