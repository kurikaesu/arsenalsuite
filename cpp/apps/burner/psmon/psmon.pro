
MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out


INCLUDEPATH+=../../../lib/stone/include

LIBS+=-L../../../lib/stone -lstone
!win32:LIBS+=-lutil

SOURCES += \
	psmon.cpp

win32:LIBS += -lpsapi -lMpr -lws2_32 -lgdi32

CONFIG += qt warn_on
QT-= gui
QT+= network

TARGET=abpsmon

win32 {
	debug {
		CONFIG += console
	}
}

DESTDIR=./

unix {
	target.path=$$(DESTDIR)/usr/bin/
	INSTALLS += target
}
macx:CONFIG-=app_bundle
