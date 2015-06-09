


MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

INCLUDEPATH+=include ../../lib/stone/include

SOURCES += \
	src/main.cpp

 # Stone
LIBS+=-L../../lib/stone -lstone 

win32:LIBS += -lpsapi -lMpr -lws2_32 -lgdi32

CONFIG += qt thread warn_on rtti exceptions
QT+=qt3support network sql xml

TARGET=winmsg

win32 {
	debug {
		CONFIG += console
	}
}

DESTDIR=./
