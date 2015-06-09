
MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

INCLUDEPATH += .out include
INCLUDEPATH += ../../lib/stone/include ../../lib/stone
INCLUDEPATH += ../../lib/classes/autocore ../../lib/classes/autoimp ../../lib/classes
INCLUDEPATH += ../../lib/absubmit/include ../../lib/absubmit/.out

INCLUDEPATH += /usr/include/stone /usr/include/classes /usr/include/absubmit

SOURCES += \
	src/main.cpp

LIBS += -L../../lib/classes -lclasses -L../../lib/stone -lstone -L../../lib/absubmit -labsubmit
!win32:LIBS += -lutil
win32:LIBS += -lpsapi -lMpr
unix:!macx:LIBS += -Wl,-rpath .
macx:CONFIG-=app_bundle

CONFIG += qt thread warn_on
QT += xml sql network

TARGET=absubmit

DESTDIR=./

unix {
	target.path=$$(DESTDIR)/usr/local/bin
}
win32 {
	QMAKE_CXXFLAGS+=/Zi
	QMAKE_LFLAGS+=/DEBUG /OPT:REF /OPT:ICF
	target.path=$$(DESTDIR)/blur/common/
}

INSTALLS += target
