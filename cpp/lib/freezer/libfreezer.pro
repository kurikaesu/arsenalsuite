
MOC_DIR=.out
OBJECTS_DIR=.out
UI_DIR=.out

DEPENDPATH+=src include ui

SOURCES+= \
	src/abadminplugins.cpp \
	src/afcommon.cpp \
	src/assfreezerview.cpp \
	src/assfreezermenus.cpp \
	src/batchsubmitdialog.cpp \
	src/displayprefsdialog.cpp \
	src/errorlistwidget.cpp \
	src/framenthdialog.cpp \
	src/glwindow.cpp \
	src/graphiteview.cpp \
	src/hosterrorwindow.cpp \
	src/hostlistwidget.cpp \
	src/hostservicematrix.cpp \
	src/userservicematrix.cpp \
	src/frameviewerplugin.cpp \
	src/multiframeviewerplugin.cpp \
	src/hostviewerplugin.cpp \
	src/imagecache.cpp \
	src/imageview.cpp \
	src/items.cpp \
	src/jobaftereffectssettingswidget.cpp \
	src/jobbatchsettingswidget.cpp \
	src/jobenvironmentwindow.cpp \
	src/jobfusionsettingswidget.cpp \
	src/jobhistoryview.cpp \
	src/joblistwidget.cpp \
	src/jobmaxsettingswidget.cpp \
	src/jobmaxscriptsettingswidget.cpp \
	src/jobmayasettingswidget.cpp \
	src/jobrealflowsettingswidget.cpp \
	src/jobshakesettingswidget.cpp \
	src/jobsettingswidget.cpp \
	src/jobstatwidget.cpp \
	src/jobsettingswidgetplugin.cpp \
	src/jobxsisettingswidget.cpp \
	src/jobviewerplugin.cpp \
	src/joberrorswidgetplugin.cpp \
	src/jobframestabwidgetplugin.cpp \
	src/mainwindow.cpp \
	src/projectreservedialog.cpp \
	src/projectreserveview.cpp \
	src/servicechecktree.cpp \
	src/projectweightdialog.cpp \
	src/projectweightview.cpp \
	src/servicestatusview.cpp \
	src/settingsdialog.cpp \
	src/tabtoolbar.cpp \
	src/threadtasks.cpp \
	src/viewmanager.cpp \
	src/webview.cpp

HEADERS+= \
	include/abadminplugins.h \
	include/afcommon.h \
	include/assfreezerview.h \
	include/assfreezermenus.h \
	include/batchsubmitdialog.h \
	include/displayprefsdialog.h \
	include/errorlistwidget.h \
	include/framenthdialog.h \
	include/glwindow.h \
	include/graphiteview.h \
	include/hosterrorwindow.h \
	include/hostlistwidget.h \
	include/hostservicematrix.h \
	include/userservicematrix.h \
	include/frameviewerplugin.h \
	include/frameviewerfactory.h \
	include/multiframeviewerplugin.h \
	include/multiframeviewerfactory.h \
	include/hostviewerplugin.h \
	include/hostviewerfactory.h \
	include/imagecache.h \
	include/imageview.h \
	include/items.h \
	include/jobaftereffectssettingswidget.h \
	include/jobbatchsettingswidget.h \
	include/jobenvironmentwindow.h \
	include/jobfusionsettingswidget.h \
	include/jobhistoryview.h \
	include/joblistwidget.h \
	include/jobmaxsettingswidget.h \
	include/jobmaxscriptsettingswidget.h \
	include/jobmayasettingswidget.h \
	include/jobrealflowsettingswidget.h \
	include/jobshakesettingswidget.h \
	include/jobstatwidget.h \
	include/jobsettingswidget.h \
	include/jobsettingswidgetplugin.h \
	include/jobxsisettingswidget.h \
	include/jobviewerfactory.h \
	include/jobviewerplugin.h \
	include/joberrorswidgetplugin.h \
	include/joberrorswidgetfactory.h \
	include/jobframestabwidgetplugin.h \
	include/jobframestabwidgetfactory.h \
	include/mainwindow.h \
	include/projectweightdialog.h \
	include/projectweightview.h \
	include/projectreservedialog.h \
	include/projectreserveview.h \
	include/servicechecktree.h \
	include/servicestatusview.h \
	include/settingsdialog.h \
	include/tabtoolbar.h \
	include/threadtasks.h \
	include/viewmanager.h \
	include/webview.h

INTERFACES+= \
	ui/aboutdialog.ui \
	ui/batchsubmitdialogui.ui \
	ui/displayprefsdialogui.ui \
	ui/hostservicematrixwindowui.ui \
	ui/userservicematrixwindowui.ui \
	ui/hostlistwidgetui.ui \
	ui/hostservicematrixwidgetui.ui \
	ui/framenthdialogui.ui \
	ui/jobaftereffectssettingswidgetui.ui \
	ui/jobbatchsettingswidgetui.ui \
	ui/jobenvironmentwindowui.ui \
	ui/jobfusionsettingswidgetui.ui \
	ui/jobfusionvideomakersettingswidgetui.ui \
	ui/joblistwidgetui.ui \
	ui/jobmaxscriptsettingswidgetui.ui \
	ui/jobmaxsettingswidgetui.ui \
	ui/jobmayasettingswidgetui.ui \
	ui/jobrealflowsettingswidgetui.ui \
	ui/jobsettingswidgetui.ui \
	ui/jobshakesettingswidgetui.ui \
	ui/jobxsisettingswidgetui.ui \
	ui/projectweightdialogui.ui \
	ui/projectreservedialogui.ui \
	ui/settingsdialogui.ui

LIBS+=-L../absubmit -labsubmit
LIBS+=-L../classesui -lclassesui
LIBS+=-L../classes -lclasses
LIBS+=-L../stonegui -lstonegui
LIBS+=-L../stone -lstone

# In tree include paths
INCLUDEPATH += .out include
INCLUDEPATH += ../stone/include ../stone
INCLUDEPATH += ../stonegui/include ../stonegui/.out/
INCLUDEPATH += ../classes/autocore ../classes/autoimp ../classes
INCLUDEPATH += ../classesui/include ../classesui/.out/
INCLUDEPATH += ../absubmit/include ../absubmit/.out

isEmpty( PYTHON ) {
	PYTHON="python"
}

win32 {
	PY_VERSION=$$system("python -c \"from distutils.sysconfig import get_python_version; print get_python_version().replace('.','')\"")
} else {
	PY_VERSION=$$system("python -c \"from distutils.sysconfig import get_python_version; print get_python_version()\"")
}

PY_PATH=$$system("python -c \"from distutils.sysconfig import get_config_vars; print get_config_vars()['prefix']\"")
INCLUDEPATH+=$$system("python -c \"from distutils.sysconfig import get_python_inc; print get_python_inc()\"")

message(Python Version is $$PY_VERSION Python lib path is $$PY_PATH\\libs)
win32:LIBS+=-L$${PY_PATH}\\libs
LIBS+=-lpython$${PY_VERSION}

win32 {
	LIBS+=-lPsapi
	INCLUDEPATH+=c:/source/sip/siplib
	LIBS += -lpsapi -lMpr -lws2_32 -lgdi32
	QMAKE_CXXFLAGS+=/Zi
	QMAKE_LFLAGS+=/DEBUG /OPT:REF /OPT:ICF
}


# Linux out of tree include paths
unix {
	INCLUDEPATH += /usr/include/stone /usr/include/stonegui /usr/include/classes /usr/include/classesui /usr/include/absubmit
}

macx{
	#INCLUDEPATH+=/Developer/SDKs/MacOSX10.5u.sdk/usr/X11R6/include/
	QMAKE_MAC_SDK=/Developer/SDKs/MacOSX10.5.sdk
	QMAKE_MACOSX_DEPLOYMENT_TARGET=10.5
}

unix{
	PY_VERSION=$$system($$PYTHON " -c \"from distutils.sysconfig import get_python_version; print get_python_version()\"")
	message(Python Version is $$PY_VERSION)
	INCLUDEPATH+=$$system($$PYTHON " -c \"from distutils.sysconfig import get_python_inc; print get_python_inc()\"")
	INCLUDEPATH += ../sip/siplib/
	LIBS+=-lpython$${PY_VERSION}
}

DEFINES+=FREEZER_MAKE_DLL

#unix:DEFINES+=USE_GRAPHVIZ

#DEFINES+=USE_IMAGE_MAGICK
contains( DEFINES, USE_IMAGE_MAGICK ) {
	unix:LIBS+=-L$$(MAGICK_ROOT)/lib
	unix:INCLUDEPATH+=$$(MAGICK_ROOT)/include/ImageMagick

	unix:LIBS+=-Wl,-rpath,$$(MAGICK_ROOT)/lib

	unix:LIBS+=-lMagick++

	#macx:INCLUDEPATH+=/usr/local/include
	#macx:LIBS+=-lMagick++

	win32:LIBS+=-lMagick++
	win32:LIBS+=-L/ImageMagick/lib
	win32:INCLUDEPATH+=/ImageMagick/include
}

TEMPLATE=lib

CONFIG += qt thread opengl
QT+=gui xml sql opengl network webkit

TARGET=freezer

unix {
	target.path=$$(DESTDIR)/usr/local/lib
}
win32 {
	target.path=$$(DESTDIR)/blur/common/
}

INSTALLS += target

DESTDIR=./
