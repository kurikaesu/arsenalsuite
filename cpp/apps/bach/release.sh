#!/bin/bash

V=$1

if [ `uname -s` == "Linux" ]; then
ARCH="lin64"
rsync -acv libbach.so* /drd/software/int/apps/bach/$ARCH/$V/
else
ARCH="osx"
rsync -acv libbach.*dylib /drd/software/int/apps/bach/$ARCH/$V/
fi

rsync --exclude=.svn -acv images /drd/software/int/apps/bach/$ARCH/$V/
rsync --exclude=.svn -acv data_export /drd/software/int/apps/bach/$ARCH/$V/
rsync --exclude=.svn -acv plugins /drd/software/int/apps/bach/$ARCH/$V/
cp sipBach/Bach.so /drd/software/int/apps/bach/$ARCH/$V/
cp bach /drd/software/int/apps/bach/$ARCH/$V/
cp bach*.ini /drd/software/int/apps/bach/$ARCH/$V/
cp bachutil.py /drd/software/int/apps/bach/$ARCH/$V/
cp initbach.py /drd/software/int/apps/bach/$ARCH/$V/
cp lsq /drd/software/int/apps/bach/$ARCH/$V/

