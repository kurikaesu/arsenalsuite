#!/bin/sh
export LD_LIBRARY_PATH=/drd/software/ext/gimp/lin64/2.6.6/lib
/drd/software/ext/gimp/lin64/2.6.6/bin/gimp -i -n -f -d -s -b "(script-fu-thumbnail-kar \"/drd/reference/.thumbnails\" \"$1\" $2)" -b '(gimp-quit 0)'
