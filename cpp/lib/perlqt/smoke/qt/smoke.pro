
SOURCES += \
	x_1.cpp \
	x_2.cpp \
	x_3.cpp \
	x_4.cpp \
	x_5.cpp \
	x_6.cpp \
	x_7.cpp \
	x_8.cpp \
	x_9.cpp \
	x_10.cpp \
	x_11.cpp \
	x_12.cpp \
	x_13.cpp \
	x_14.cpp \
	x_15.cpp \
	x_16.cpp \
	x_17.cpp \
	x_18.cpp \
	x_19.cpp \
	x_20.cpp \
	smokedata.cpp

HEADERS += ../smoke.h

INCLUDEPATH+=.. .
INCLUDEPATH+=/usr/include/stone
INCLUDEPATH+=/usr/include/classes
INCLUDEPATH+=../../../stone/include
INCLUDEPATH+=../../../classes/autocore
INCLUDEPATH+=../../../classes/base
INCLUDEPATH+=../../../stone/.out
INCLUDEPATH+=../../../stone/
INCLUDEPATH+=../../../classes/autoimp
INCLUDEPATH+=../../../classes

QMAKE_CXXFLAGS+=-fno-strict-aliasing

TEMPLATE=lib
CONFIG+=qt thread staticlib
TARGET=smokeqt
DESTDIR=.
QT+=sql network xml

