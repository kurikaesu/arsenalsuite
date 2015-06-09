

SOURCES+=sgi.cpp
HEADERS+=sgi.h

TARGET=sgi

target.path=$$(QTDIR)/plugins/imageformats/

INSTALLS+=target
TEMPLATE=lib
CONFIG+=plugin
QT+=QtGui
DESTDIR=./
