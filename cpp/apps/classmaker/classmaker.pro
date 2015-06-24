
HEADERS += \
	includes/mainwindow.h \
	includes/sourcegen.h \
	includes/tabledialog.h \
	includes/fielddialog.h \
	includes/indexdialog.h \
	includes/createdatabasedialog.h \

SOURCES += \
	src/mainwindow.cpp \
	src/sourcegen.cpp \
	src/main.cpp \
	src/tabledialog.cpp \
	src/fielddialog.cpp \
	src/indexdialog.cpp \
	src/createdatabasedialog.cpp \

FORMS += \
	ui/mainwindowui.ui \
	ui/tabledialogui.ui \
	ui/fielddialogui.ui \
	ui/indexdialogui.ui \
	ui/createdatabasedialogui.ui \
	ui/docsdialogui.ui
	
TARGET=classmaker

INCLUDEPATH+=includes
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
