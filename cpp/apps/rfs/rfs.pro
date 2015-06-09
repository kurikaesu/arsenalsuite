MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

INCLUDEPATH = ../../lib/stone/include ../../lib/classes/autocore ../../lib/classes/autoimp ../../lib/stone/.out ../../lib/stone ../../lib/classes

HEADERS += \
	scanner.h

SOURCES += \
	scanner.cpp \
	main.cpp

#INTERFACES +=


TARGET = rfs
CONFIG += qt thread warn_on debug console
QT+=qt3support network sql xml
LIBS+=-Wl,-rpath . -L/usr/lib/qt-3.3/lib -L../../lib/stone -L../../lib/classes -Wl,-dn -lclasses -lstone -Wl,-dy
