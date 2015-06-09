
MOC_DIR=.out
OBJECTS_DIR=.out
UI_DIR=.out

INCLUDEPATH += ../stone/include ../stonegui/include .out include ../classes/autocore ../classes/autoimp ../stone ../classes ../classesui/include

DEPENDPATH+=src include ui

SOURCES+= \
	src/graphdsdialog.cpp \
	src/snafuwidget.cpp \
	src/snafucmdthread.cpp \
	src/snafugraphwidget.cpp \
	src/snafuhttpthread.cpp \
	src/snafurrdthread.cpp \
	src/displayprefsdialog.cpp \
	src/items.cpp \
	src/servicechecktree.cpp \
	src/softwarelicensetree.cpp \
	src/softwaredialog.cpp \
	src/licensewidget.cpp \
	src/licensedialog.cpp \
	src/hostsoftwaretree.cpp \
	src/softwarechecktree.cpp \
	src/syslogtablewidgetitem.cpp

HEADERS+= \
	include/graphdsdialog.h \
	include/snafuwidget.h \
	include/snafucmdthread.h \
	include/snafugraphwidget.h \
	include/snafuhttpthread.h \
	include/snafurrdthread.h \
	include/displayprefsdialog.h \
	include/items.h \
	include/servicechecktree.h \
	include/softwarechecktree.h \
	include/softwarelicensetree.h \
	include/softwaredialog.h \
	include/licensewidget.h \
	include/licensedialog.h \
	include/hostsoftwaretree.h \
	include/syslogtablewidgetitem.h

INTERFACES+= \
	ui/graphdsdialogui.ui \
	ui/displayprefsdialogui.ui \
	ui/licensewidget.ui \
	ui/licensedialog.ui \
	ui/softwaredialog.ui \
	ui/snafuwidgetui.ui

win32{
#	INCLUDEPATH+=c:\nvidia\cg\include
#	LIBS+=-Lc:\nvidia\cg\lib -lcgGL -lcg
	LIBS+=-lPsapi
	LIBS+=-L..\classesui -lclassesui
	LIBS+=-L..\classes -lclasses
	LIBS+=-L..\stonegui -lstonegui
	LIBS+=-L..\stone -lstone
#	LIBS+=-Lc:\IntelLib
}

unix{
	LIBS+=-L../classesui -lclassesui
	LIBS+=-L../classes -lclasses
	LIBS+=-L../stonegui -lstonegui
	LIBS+=-L../stone -lstone
}

DEFINES+=SNAFU_MAKE_DLL

TEMPLATE=lib

CONFIG += qt thread opengl console debug
QT+=sql opengl network
TARGET=snafu
target.path=/usr/local/lib
INSTALLS += target

DESTDIR=./
