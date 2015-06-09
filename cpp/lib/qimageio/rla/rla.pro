
SOURCES+=rla.cpp
HEADERS+=rla.h

TARGET=rla
debug:TARGET=rlad

target.path=$$(QTDIR)/plugins/imageformats/

INSTALLS+=target
TEMPLATE=lib
CONFIG+=plugin
DESTDIR=./
