
MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out


SOURCES += \
	mapdialog.cpp \
	main.cpp

HEADERS += \
	mapdialog.h

INTERFACES += \
	mapdialogui.ui



LIBS += -lMpr -lws2_32
 
CONFIG += qt thread warn_on rtti exceptions

TARGET=remap

win32 {
	debug {
		CONFIG += console
	}
}

DESTDIR=./
