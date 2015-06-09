MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

INCLUDEPATH = ../../lib/stone/include ../../lib/classes/autocore ../../lib/classes/autoimp ../../lib/stone/.out ../../lib/stone ../../lib/classes

HEADERS += \
	thrasher.h

SOURCES += \
	thrasher.cpp \
	main.cpp

#INTERFACES +=


TARGET = rfm
CONFIG += qt thread warn_on debug console
QT+=qt3support network xml sql
LIBS+=-Wl,-rpath . -L/usr/lib/qt-3.3/lib -L../../lib/stone -L../../lib/classes -Wl,-dn -lstone -lclasses -Wl,-dy
