#!/bin/bash
qmake
make -j24 CC="distcc gcc" CXX="distcc g++"
