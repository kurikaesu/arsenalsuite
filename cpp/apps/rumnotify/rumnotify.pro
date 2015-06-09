

SOURCES	+= \
	main.cpp \
	notifier.cpp

HEADERS += \
	notifier.h


MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

INCLUDEPATH+=../../lib/stone ../../lib/stone/include ../../lib/classes/autocore ../../lib/classes/autoimp ../../lib/classes/base

win32{
	LIBS+=-L..\..\lib\stone -lstone -L..\..\lib\classes -lclasses
	LIBS+=-lpsapi
}

unix{
	LIBS+=-L../../lib/stone -L../../lib/classes -Wl,-dn -lstone -lclasses -Wl,-dy
}

TEMPLATE=app
CONFIG+=qt thread opengl
QT+=qt3support xml network sql
#CONFIG+=qt thread release
TARGET=rumnotify

