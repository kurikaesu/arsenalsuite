
DEPENDPATH+=core view ui

MOC_DIR=.out
OBJECTS_DIR=.out
UI_DIR=.out

include( auto.pri )
include( bach-common.pri )

# FFmpeg support
unix:DEFINES-=USE_FFMPEG
win32:DEFINES-=USE_FFMPEG
contains( DEFINES, USE_FFMPEG ) {
	win32 {
		INCLUDEPATH += c:/msys/1.0/local/include/ffmpeg
		LIBS+=-Lc:/msys/1.0/local/lib -lavcodec -lavformat -lavutil
	}

	unix {
		INCLUDEPATH += /opt/ffmpeg/include/ffmpeg/
		LIBS+=-L/opt/ffmpeg/lib/ -lavcodec -lavutil -lavformat -lswscale
	}
}

unix:DEFINES-=USE_PHONON
contains( DEFINES, USE_PHONON ) {
	QT+=phonon
}

DEFINES-=USE_MEMCACHED
contains( DEFINES, USE_MEMCACHED ) {
	SOURCES+=view/qmemcached.cpp
	HEADERS+=view/qmemcached.h
	INCLUDEPATH+=/opt/memcached/include
	INCLUDEPATH+=/opt/memcached/include/libmemcached
	LIBS+=-L/opt/memcached/lib -lmemcached
}

DEFINES-=USE_IMAGE_MAGICK
contains( DEFINES, USE_IMAGE_MAGICK ) {
	unix:LIBS+=-L/drd/software/ext/imageMagick/lin64/current/lib
	unix:INCLUDEPATH+=/drd/software/ext/imageMagick/lin64/current/include/ImageMagick
	unix:LIBS+=-lMagick++

	macx:INCLUDEPATH+=/usr/local/include
	macx:LIBS+=-lMagick++ -lMagick
}

macx{
	INCLUDEPATH+=/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/include/
	CONFIG-=app_bundle
}

TEMPLATE=app
TARGET=bach
DESTDIR=./

QT+=gui sql network opengl
CONFIG+=qt thread warn_on debug
RESOURCES+=bach.qrc
target.path=./
INSTALLS += target

