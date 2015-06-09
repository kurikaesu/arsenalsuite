

SOURCES+=tif.cpp
HEADERS+=tif.h

TARGET=tif

win32 {
	INCLUDEPATH += C:/GnuWin32/include
	LIBS+=-LC:/GnuWin32/bin -LC:/GnuWin32/lib
}

LIBS+=-ltiff

target.path=$$(QTDIR)/plugins/imageformats/

INSTALLS+=target
TEMPLATE=lib
CONFIG+=plugin
QT+=QtGui
DESTDIR=./
