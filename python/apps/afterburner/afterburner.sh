#!/bin/bash

HOST=$1

RESULT=`ps aux | grep afterburner.py | grep -v grep`
if [ "$?" -eq "0" ]; then
    exit 0
fi

while read e; do export "$e"; done < <(/drd/software/int/bin/launcher.sh --project hf2 --dept rnd --printEnv farm)

cd /drd/software/int/sys/afterburner/source/
/usr/bin/python2.5 afterburner.py $HOST
