TEMPLATE = app
MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

QT+=sql network

#INCLUDEPATH+=../../lib/assfreezer/include
INCLUDEPATH += ../../lib/snafu/include ../../lib/snafu/.out ../../lib/stone/include ../../lib/classes/autocore ../../lib/classes/autoimp ../../lib/classes/base ../../lib/stone ../../lib/classes
INCLUDEPATH += ../../lib/stonegui/include
INCLUDEPATH +=./include

HEADERS += include/mainwindow.h

SOURCES += src/mainwindow.cpp
SOURCES += src/main.cpp

win32{
LIBS+=-L..\..\lib\snafu -lsnafu
LIBS+=-L..\..\lib\classesui -lclassesui
LIBS+=-L..\..\lib\stonegui -lstonegui
LIBS+=-L..\..\lib\classes -lclasses
LIBS+=-L..\..\lib\stone -lstone
}

unix{
	LIBS+=-L../../lib/snafu -lsnafu
	LIBS+=-L../../lib/classesui -lclassesui
	LIBS+=-L../../lib/stonegui -lstonegui
	LIBS+=-L../../lib/classes -lclasses
	LIBS+=-L../../lib/stone -lstone
}

RC_FILE = snafu.rc
RESOURCES += snafu.qrc

CONFIG += debug warn_on console
#CONFIG += release

TARGET=snafu

