MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

INCLUDEPATH = ../../lib/stone/include ../../lib/stone/.out
LIBS+= -L../../lib/stone -lstone

HEADERS += \
	server.h \
	connection.h

SOURCES += \
	server.cpp \
	connection.cpp \
	main.cpp

TARGET = rum
CONFIG += qt thread warn_on console
QT+=network xml
target.path=/usr/local/bin
INSTALLS += target
