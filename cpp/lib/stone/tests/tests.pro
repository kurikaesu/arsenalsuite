

SOURCES+=test.cpp

unix:QMAKE_CXXFLAGS+=-std=c++0x
INCLUDEPATH+=../include
LIBS+=-L.. -lstone
TARGET=tests
CONFIG=qt thread warn_on debug qtestlib
QT+=xml network sql gui
DESTDIR=./
