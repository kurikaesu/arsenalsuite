#!/bin/bash
export PATH=/usr/local/Trolltech/Qt-4.5.3/bin:$PATH
export MAKEFLAGS=-j2
export DYLD_LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH=/usr/local/lib

#classmaker -s /path/to/schema.xml -o /path/to/output/dir/
qmake bach.pro && make clean  && make -j8 && make install

qmake bach-lib.pro && make clean  && make -j8 && make install
python2.5 configure.py && make clean && make -j8 && make install
python2.5 configure.py -k && make clean && make -j8 && make install

