

MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

CONFIG += psics
PSICS_CPP = ../blurt/cutestuff
include(../blurt/psics.pri)

DEFINES += QCA_STATIC
QCA_PREFIX = ../blurt/qca
INCLUDEPATH += $$QCA_PREFIX
HEADERS += $$QCA_PREFIX/qca.h $$QCA_PREFIX/qcaprovider.h
SOURCES += $$QCA_PREFIX/qca.cpp

# include Iris XMPP library
IRIS_BASE = ../blurt/iris
include(../blurt/iris.pri)

SOURCES += main.cpp session.cpp server.cpp
HEADERS += session.h server.h

TARGET = rjd

INCLUDEPATH += ../../lib/stone ../../lib/stone/include ../../lib/classes/autocore ../../lib/classes/autoimp ../../lib/classes/

unix {
	LIBS+=-lXss
}

CONFIG=qt debug thread static x11

LIBS+=-L../../lib/stone -L../../lib/classes -lclasses -lstone 
