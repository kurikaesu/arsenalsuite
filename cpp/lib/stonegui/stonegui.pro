
MOC_DIR=.out
OBJECTS_DIR=.out
UI_DIR=.out

RESOURCES += stonegui.qrc

FORMS+= \
	ui/addlinkdialog.ui \
	ui/lostconnectiondialogui.ui \
	ui/configdbdialogui.ui \
	ui/graphitedialogui.ui \
	ui/graphitesourceswidgetui.ui \
	ui/graphiteoptionswidgetui.ui \
	ui/imagesequencewidget.ui \
	ui/remotetailwindowui.ui

SOURCES += \
	src/actions.cpp \
	src/busywidget.cpp \
	src/configdbdialog.cpp \
	src/extgraphicsscene.cpp \
	src/exttreewidgetitem.cpp \
	src/ffimagesequenceprovider.cpp \
#	src/phononimagesequenceprovider.cpp \
	src/fieldcheckbox.cpp \
	src/fieldlineedit.cpp \
	src/fieldtextedit.cpp \
	src/fieldspinbox.cpp \
	src/filteredit.cpp \
	src/glutil.cpp \
	src/graphitedialog.cpp \
	src/graphitesource.cpp \
	src/graphitesourceswidget.cpp \
	src/graphiteoptionswidget.cpp \
	src/graphitewidget.cpp \
	src/htmlhighlighter.cpp \
	src/iconserver.cpp \
	src/imagesequenceprovider.cpp \
	src/lostconnectiondialog.cpp \
	src/modelgrouper.cpp \
	src/modeliter.cpp \
	src/qvariantcmp.cpp \
	src/recordcombo.cpp \
	src/recorddelegate.cpp \
	src/recorddrag.cpp \
	src/recordlistmodel.cpp \
	src/recordlistview.cpp \
#	src/tardstyle.cpp \
	src/recentvaluesui.cpp \
	src/recordfilterwidget.cpp \
	src/recordpropvalmodel.cpp \
	src/recordpropvaltree.cpp \
	src/recordsupermodel.cpp \
	src/recordtreeview.cpp \
	src/richtexteditor.cpp \
	src/stonegui.cpp \
	src/supermodel.cpp \
	src/remotetailwidget.cpp \
	src/remotetailwindow.cpp \
	src/imagesequencewidget.cpp \
	src/undotoolbutton.cpp \
	src/viewcolors.cpp

HEADERS += \
	include/actions.h \
	include/busywidget.h \
	include/configdbdialog.h \
	include/extgraphicsscene.h \
	include/exttreewidgetitem.h \
	include/ffimagesequenceprovider.h \
#	include/phononimagesequenceprovider.h \
	include/fieldcheckbox.h \
	include/fieldlineedit.h \
	include/fieldtextedit.h \
	include/fieldspinbox.h \
	include/filteredit.h \
	include/glutil.h \
	include/graphitedialog.h \
	include/graphitesource.h \
	include/graphitesourceswidget.h \
	include/graphiteoptionswidget.h \
	include/graphitewidget.h \
	include/htmlhighlighter.h \
	include/iconserver.h \
	include/imagesequenceprovider.h \
	include/lostconnectiondialog.h \
	include/modelgrouper.h \
	include/modeliter.h \
	include/qvariantcmp.h \
	include/recentvaluesui.h \
	include/recordcombo.h \
	include/recorddelegate.h \
	include/recorddrag.h \
	include/recordfilterwidget.h \
	include/recordlistmodel.h \
	include/recordlistview.h \
	include/recordpropvalmodel.h \
	include/recordpropvaltree.h \
	include/recordsupermodel.h \
#	include/tardstyle.h \
	include/recordtreeview.h \
	include/richtexteditor.h \
	include/stonegui.h \
	include/supermodel.h \
	include/remotetailwidget.h \
	include/remotetailwindow.h \
	include/imagesequencewidget.h \
	include/undotoolbutton.h \
	include/viewcolors.h

INCLUDEPATH+=include src .out ../stone/include ../stone/.out

DEPENDPATH+=src include ui

win32 {
	LIBS += -lPsapi -lMpr -ladvapi32 
	LIBS+=-Lc:\\IntelLib
	LIBS+=-L../stone -lstone
	QMAKE_CXXFLAGS+=/Zi
	QMAKE_LFLAGS+=/DEBUG /OPT:REF /OPT:ICF
}

unix {
	INCLUDEPATH+=/usr/include/stone
}

macx{
	INCLUDEPATH += /Developer/SDKs/MacOSX10.5.sdk/usr/X11R6/include/
	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.5.sdk
	LIBS+=-L../stone -lstone 
}

# FFmpeg support
#unix:DEFINES+=USE_FFMPEG
#win32:DEFINES+=USE_FFMPEG

contains( DEFINES, USE_FFMPEG ) {
	include(ffmpeg.pri)
}

# Phonon support
unix:DEFINES-=USE_PHONON
contains( DEFINES, USE_PHONON ) {
	QT+=phonon
}

# Grphviz support
#unix:DEFINES+=USE_GRAPHVIZ
contains( DEFINES, USE_GRAPHVIZ ) {
	INCLUDES += gvgraph.h
	SOURCES += gvgraph.cpp
	INCLUDEPATH += /usr/include
	LIBS+=-lgraph -lgvc
}

DEFINES+=STONEGUI_MAKE_DLL
TEMPLATE=lib
TARGET=stonegui

unix {
	target.path=$$(DESTDIR)/usr/local/lib
}
win32 {
	target.path=$$(DESTDIR)/blur/common/
}

INSTALLS += target
CONFIG+=qt thread
QT+=sql xml gui network opengl

DESTDIR=./
