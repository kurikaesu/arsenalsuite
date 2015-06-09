

SOURCES+=exr.cpp
HEADERS+=exr.h

DEFINES+=HAVE_EXR

TARGET=exr

unix {
	INCLUDEPATH+=/usr/include/OpenEXR/
	LIBS+=-lIex -lImath -lHalf -lIlmImf
} else {
	DEFINES+=OPENEXR_DLL
	CONFIG+=exceptions
	#INCLUDEPATH+=C:\openexr-1.4.0-vs2005\openexr-1.4.0-vs2005\include\OpenEXR
	INCLUDEPATH+=C:\source\openexr_stuff\Deploy\include
	#LIBS+="-LC:\openexr-1.4.0-vsnet2003\openexr-1.4.0-vsnet2003\lib"
	#LIBS+="-LC:\openexr-1.4.0a\openexr-1.4.0\bin"
	LIBS+=-LC:\source\openexr_stuff\Deploy\lib\x64\Release
	LIBS+=-lIlmImf -lIex -lImath -lHalf -lIlmThread
}

target.path=$$(QTDIR)/plugins/imageformats/

INSTALLS+=target
TEMPLATE=lib
CONFIG+=plugin
QT+=QtGui
DESTDIR=./
