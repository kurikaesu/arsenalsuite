

SOURCES+=kdatepicker.cpp \
		kdatetbl.cpp \
		kcalendarsystem.cpp \
		kcalendarsystemgregorian.cpp \

HEADERS+=kdatepicker.h \
		kdatetbl.h \
		kcalendarsystem.h \
		kcalendarsystemgregorian.h


TEMPLATE=lib
CONFIG+=qt thread release staticlib
TARGET=datepicker
QT+=qt3support

DESTDIR=./
