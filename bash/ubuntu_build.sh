#!/bin/bash
set -x

aptitude -y update
aptitude -y upgrade
aptitude -y install mercurial

cd /root
hg clone https://barry.robison@code.google.com/p/arsenalsuite/
cd arsenalsuite/
hg update blur_merge_nov_2012

aptitude -y install gcc
aptitude -y install make
aptitude -y install g++
aptitude -y install python-dev
aptitude -y install libqt4-dev libqt4-sql-psql libqt4-webkit
aptitude -y install libxss-dev

export MAKEFLAGS=-j6
export LD_LIBRARY_PATH=/usr/local/lib
export PATH=/usr/local/bin:$PATH
python build.py clean release build verbose install sip sipstatic
python build.py clean release build verbose install sip:skip sipstatic:skip pyqt
python build.py clean release build verbose install sip:skip sipstatic:skip pyqt:skip absubmit pyabsubmit

mkdir cpp/lib/freezer/sipFreezer
python build.py clean release build verbose install sip:skip sipstatic:skip pyqt:skip stone:skip pystone:skip classes:skip pyclasses:skip burner freezer


