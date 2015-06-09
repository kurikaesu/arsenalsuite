
HEADERS += \
	mainwindow.h \
	sourcegen.h \
	tabledialog.h \
	fielddialog.h \
	indexdialog.h \
	createdatabasedialog.h \

SOURCES += \
	mainwindow.cpp \
	sourcegen.cpp \
	main.cpp \
	tabledialog.cpp \
	fielddialog.cpp \
	indexdialog.cpp \
	createdatabasedialog.cpp \

FORMS += \
	mainwindowui.ui \
	tabledialogui.ui \
	fielddialogui.ui \
	indexdialogui.ui \
	createdatabasedialogui.ui \
	docsdialogui.ui
	
TARGET=classmaker

INCLUDEPATH+=../../lib/stonegui/include ../../lib/stonegui/.out
INCLUDEPATH+=../../lib/stone/ ../../lib/stone/include ../../lib/stone/.out 
INCLUDEPATH+=/usr/include/stone /usr/include/stonegui

LIBS+=-L../../lib/stonegui -lstonegui
LIBS+=-L../../lib/stone/ -lstone

unix:LIBS+=-lutil

win32:LIBS += -lpsapi -lMpr
macx:CONFIG-=app_bundle

TEMPLATE=app

CONFIG+=qt thread
QT+=sql xml gui network
DESTDIR=./
MOC_DIR        = .out
OBJECTS_DIR    = .out
UI_DIR         = .out

RC_FILE=classmaker.rc

unix {
	target.path=$$(DESTDIR)/usr/local/bin
}
win32 {
	QMAKE_CXXFLAGS+=/Zi
	QMAKE_LFLAGS+=/DEBUG /OPT:REF /OPT:ICF
	target.path=$$(DESTDIR)/blur/common
}
INSTALLS += target
