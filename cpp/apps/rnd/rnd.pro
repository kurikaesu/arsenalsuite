
MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

SOURCES	+= \
	main.cpp \
	spewer.cpp

HEADERS	+= \
	spewer.h

INCLUDEPATH+=../../lib/classes/autocore ../../lib/classes/autoimp ../../lib/stone ../../lib/stone/include ../../lib/classes

win32{
	LIBS+=-L..\..\libblurqt -lblurqt
	LIBS+=-lpsapi
	LIBS+=-lauto
}

unix{
	LIBS+=-Wl,-rpath . -L../../lib/stone -L../../lib/classes -Wl,-dn -lclasses -lstone -Wl,-dy
}

TEMPLATE=app
CONFIG+=qt thread console warn_on debug
#CONFIG+=qt thread release
TARGET=rnd

