
MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

#DEFINES += COMPILE_AFTER_EFFECTS_BURNER
DEFINES += COMPILE_BATCH_BURNER
win32:DEFINES += COMPILE_MAX7_BURNER
win32:DEFINES += COMPILE_MAXSCRIPT_BURNER
DEFINES += COMPILE_MAYA_BURNER
DEFINES += COMPILE_MENTALRAY_BURNER
#DEFINES += COMPILE_RENDERMAN_BURNER
#DEFINES += COMPILE_RIBGEN_BURNER
#DEFINES += COMPILE_SHAKE_BURNER
#DEFINES += COMPILE_SYNC_BURNER
#DEFINES += COMPILE_AUTODESK_BURNER

unix:DEFINES += USE_TIME_WRAP
unix:!macx:DEFINES += USE_ACCOUNTING_INTERFACE

INCLUDEPATH+=include
INCLUDEPATH+=../../lib/classes/autocore ../../lib/classes
INCLUDEPATH+=../../lib/stone/include
INCLUDEPATH+=../../lib/stonegui/include
INCLUDEPATH+=idle

isEmpty( PYTHON ) {
    PYTHON="python"
}

unix {
	INCLUDEPATH+=/usr/include/stone
	INCLUDEPATH+=/usr/include/classes
	INCLUDEPATH+=/usr/include/stonegui

	PY_VERSION=$$system($$PYTHON " -c \"from distutils.sysconfig import get_python_version; print get_python_version()\"")
	message(Python Version is $$PY_VERSION)
	INCLUDEPATH+=$$system($$PYTHON " -c \"from distutils.sysconfig import get_python_inc; print get_python_inc()\"")
	LIBS+=-lpython$${PY_VERSION}
}

macx:CONFIG-=app_bundle

SOURCES += \
	src/batchburner.cpp \
	src/builtinburnerplugin.cpp \
	src/common.cpp \
	src/jobburnerfactory.cpp \
	src/jobburner.cpp \
	src/killdialog.cpp \
	src/maindialog.cpp \
	src/main.cpp \
	src/mapwarningdialog.cpp \
	src/aftereffectsburner.cpp \
	src/max7burner.cpp \
	src/maxscriptburner.cpp \
	src/mayaburner.cpp \
	src/shakeburner.cpp \
	src/settingsdialog.cpp \
	src/slave.cpp \
	src/spooler.cpp \
	src/abstractdownload.cpp \
	src/wincopy.cpp \
	src/win32sharemanager.cpp

HEADERS += \
	include/batchburner.h \
	include/builtinburnerplugin.h \
	include/common.h \
	include/jobburnerfactory.h \
	include/jobburnerplugin.h \
	include/jobburner.h \
	include/mapwarningdialog.h \
	include/aftereffectsburner.h \
	include/max7burner.h \
	include/maxscriptburner.h \
	include/mayaburner.h \
	include/shakeburner.h \
	include/killdialog.h \
	include/maindialog.h \
	include/settingsdialog.h \
	include/slave.h \
	include/spooler.h \
	include/abstractdownload.h \
	include/wincopy.h \
	include/win32sharemanager.h

win32 {
	SOURCES += src/oopburner.cpp
	HEADERS += include/oopburner.h
}

INTERFACES += \
	ui/killdialogui.ui \
	ui/maindialogui.ui \
	ui/mapwarningdialogui.ui \
	ui/settingsdialogui.ui

DEPENDPATH+=src include ui

RESOURCES += burner.qrc

# Idle - taken from Psi
include( "idle/idle.pri" )

# Python modules
debug:win32 {
    LIBS+=-LsipBurner -lBurner
    LIBS+=-L../../lib/classes/sipClasses -lpyClasses_d
    LIBS+=-L../../lib/stone/sipStone -lpyStone_d
    LIBS+=-L../../lib/sip/siplib -lsip_d
} else {
    LIBS+=-LsipBurner -lBurner
    win32 {
	LIBS+=-L../../lib/classes/sipClasses -lpyClasses
	LIBS+=-L../../lib/stone/sipStone -lpyStone
	LIBS+=-L../../lib/sip/siplib -lsip
    }
}

# Stone and classes
LIBS+=-L../../lib/stonegui -lstonegui
LIBS+=-L../../lib/classes -lclasses
LIBS+=-L../../lib/stone -lstone

!win32:LIBS+=-lutil


win32 {
	PY_PATH=$$system("python -c \"from distutils.sysconfig import get_config_vars; print get_config_vars()['prefix']\"")
	INCLUDEPATH+=$$system("python -c \"from distutils.sysconfig import get_python_inc; print get_python_inc()\"")
	PY_VERSION=$$system("python -c \"from distutils.sysconfig import get_python_version; print get_python_version().replace('.','')\"")
	message(Python Version is $$PY_VERSION Python lib path is $$PY_LIB_PATH)
	LIBS+=-L$${PY_PATH}\\libs -lpython$${PY_VERSION}
	LIBS += -lpsapi -lMpr -lws2_32 -lgdi32
	QMAKE_CXXFLAGS+=/Zi
	QMAKE_LFLAGS+=/DEBUG /OPT:REF /OPT:ICF
}

unix:!macx {
	LIBS += -lXss -L/usr/X11R6/lib -L/usr/X11R6/lib64
}

CONFIG += qt thread warn_on rtti exceptions
QT+=network sql xml
RC_FILE = burner.rc

TARGET=burner

win32 {
	debug {
		CONFIG += console
	}
}

DESTDIR=./
unix {
	target.path=$$(DESTDIR)/usr/local/bin
}
win32 {
	target.path=$$(DESTDIR)/blur/common/
}

INSTALLS += target
