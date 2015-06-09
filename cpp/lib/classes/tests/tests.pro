

SOURCES+=test.cpp

INCLUDEPATH+=../../stone/include ../autocore ..
LIBS+=-L../../stone -L../ -lclasses -lstone
TARGET=tests
CONFIG=qt thread warn_on debug
QT+=xml network qt3support sql gui
DESTDIR=./
