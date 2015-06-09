

SOURCES+=tga.cpp
HEADERS+=tga.h

TARGET=tga
debug:TARGET=tgad

target.path=$$(QTDIR)/plugins/imageformats/

INSTALLS+=target
TEMPLATE=lib
CONFIG+=plugin
DESTDIR=./
