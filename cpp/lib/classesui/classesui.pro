
MOC_DIR=.out
OBJECTS_DIR=.out
UI_DIR=.out

#FORMS+=

SOURCES += \
	src/abadminplugin.cpp \
	src/assetdialog.cpp \
	src/assettemplatecombo.cpp \
	src/assettemplatedialog.cpp \
	src/assettemplatesdialog.cpp \
	src/assettypecombo.cpp \
	src/assettypedialog.cpp \
	src/addnotedialog.cpp \
	src/datechooserwidget.cpp \
	src/dialogfactory.cpp \
	src/elementmodel.cpp \
	src/elementschedulecontroller.cpp \
	src/elementui.cpp \
	src/eventdialog.cpp \
	src/filetrackerdialog.cpp \
	src/graphitesavedialog.cpp \
	src/groupsdialog.cpp \
	src/hostselector.cpp \
	src/hostdialog.cpp \
	src/hosthistoryview.cpp \
	src/hostinterfacedialog.cpp \
	src/hostlistsdialog.cpp \
	src/jobassignmentwidget.cpp \
	src/jobassignmentwindow.cpp \
	src/notificationroutedialog.cpp \
	src/notificationrouteview.cpp \
	src/notificationview.cpp \
	src/notificationwidget.cpp \
	src/pathtemplatecombo.cpp \
	src/pathtemplatedialog.cpp \
	src/pathtemplatesdialog.cpp \
	src/permsdialog.cpp \
	src/projectcombo.cpp \
	src/projectdialog.cpp \
	src/projectstoragedialog.cpp \
	src/resincrashdialog.cpp \
	src/resinerror.cpp \
	src/resolutiondialog.cpp \
	src/scenedialog.cpp \
	src/scheduledialog.cpp \
	src/schedulewidget.cpp \
	src/scheduleheader.cpp \
	src/schedulepanel.cpp \
	src/scheduleprintdialog.cpp \
	src/schedulerow.cpp \
	src/scheduleentry.cpp \
	src/scheduleselection.cpp \
	src/servicedialog.cpp \
	src/shotdialog.cpp \
	src/statussetdialog.cpp \
	src/statusdialog.cpp \
	src/tasktypecombo.cpp \
	src/threadview.cpp \
	src/thumbnailui.cpp \
	src/timeentrydialog.cpp \
	src/userdialog.cpp \
	src/userroledialog.cpp \
	src/usertaskdialog.cpp \
	src/usernotifydialog.cpp \
	src/versiontrackerdialog.cpp

HEADERS += \
	include/abadminplugin.h \
	include/addnotedialog.h \
	include/assetdialog.h \
	include/assettemplatecombo.h \
	include/assettemplatesdialog.h \
	include/assettemplatedialog.h \
	include/assettypecombo.h \
	include/assettypedialog.h \
	include/datechooserwidget.h \
	include/dialogfactory.h \
	include/elementmodel.h \
	include/elementschedulecontroller.h \
	include/elementui.h \
	include/eventdialog.h \
	include/filetrackerdialog.h \
	include/graphitesavedialog.h \
	include/groupsdialog.h \
	include/hostselector.h \
	include/hostdialog.h \
	include/hosthistoryview.h \
	include/hostlistsdialog.h \
	include/hostinterfacedialog.h \
	include/jobassignmentwidget.h \
	include/jobassignmentwindow.h \
	include/notificationroutedialog.h \
	include/notificationrouteview.h \
	include/notificationview.h \
	include/notificationwidget.h \
	include/pathtemplatecombo.h \
	include/pathtemplatedialog.h \
	include/pathtemplatesdialog.h \
	include/permsdialog.h \
	include/projectcombo.h \
	include/projectdialog.h \
	include/projectstoragedialog.h \
	include/resincrashdialog.h \
	include/resinerror.h \
	include/resolutiondialog.h \
	include/scenedialog.h \
	include/scheduledialog.h \
	include/schedulewidget.h \
	include/scheduleheader.h \
	include/schedulepanel.h \
	include/scheduleprintdialog.h \
	include/schedulerow.h \
	include/scheduleentry.h \
	include/scheduleselection.h \
	include/shotdialog.h \
	include/servicedialog.h \
	include/statussetdialog.h \
	include/statusdialog.h \
	include/tasktypecombo.h \
	include/timeentrydialog.h \
	include/threadview.h \
	include/thumbnailui.h \
	include/userdialog.h \
	include/usertaskdialog.h \
	include/userroledialog.h \
	include/usernotifydialog.h \
	include/versiontrackerdialog.h

FORMS += \
	ui/addnotedialogui.ui \
	ui/assetdialogui.ui \
	ui/assettemplatedialogui.ui \
	ui/assettasksdialogui.ui \
	ui/assettypedialogui.ui \
	ui/editassettypedialogui.ui \
	ui/eventdialogui.ui \
	ui/assettemplatesdialogui.ui \
	ui/assettemplatedialogui.ui \
	ui/filetrackerdialogui.ui \
	ui/graphitesavedialogui.ui \
	ui/groupsdialogui.ui \
	ui/hostdialogui.ui \
	ui/hostlistsdialogui.ui \
	ui/hostinterfacedialogui.ui \
	ui/hostselectorbase.ui \
	ui/jobassignmentwidgetui.ui \
	ui/notificationroutedialogui.ui \
	ui/notificationwidgetui.ui \
	ui/pathtemplatedialogui.ui \
	ui/pathtemplatesdialogui.ui \
	ui/permsdialogui.ui \
	ui/projectdialogui.ui \
	ui/projectstoragedialogui.ui \
	ui/resincrashdialogui.ui \
	ui/resolutiondialogui.ui \
	ui/savelistdialogui.ui \
	ui/scenedialogui.ui \
	ui/scheduledialogui.ui \
	ui/scheduleprintdialogui.ui \
	ui/servicedialogui.ui \
	ui/shotdialogui.ui \
	ui/statussetdialogui.ui \
	ui/statusdialogui.ui \
	ui/threadviewui.ui \
	ui/timeentrydialogui.ui \
	ui/usertaskui.ui \
	ui/userroleui.ui \
	ui/userdialogui.ui \
	ui/usernotifyui.ui \
	ui/versiontrackerdialogui.ui

RESOURCES += \
	ui/threadview.qrc

INCLUDEPATH+=src include .out ../stone/include ../stone/.out
INCLUDEPATH+=../stonegui/include ../stonegui/.out
INCLUDEPATH+=../classes ../classes/autocore

DEPENDPATH+=src ui

win32 {
	LIBS += -lPsapi -lMpr -ladvapi32 -lshell32
	LIBS+=-Lc:\\IntelLib
	LIBS+=-L..\\classes -lclasses
	LIBS+=-L..\\stonegui -lstonegui
	LIBS+=-L..\\stone -lstone
	QMAKE_CXXFLAGS+=/Zi
	QMAKE_LFLAGS+=/DEBUG /OPT:REF /OPT:ICF
}

unix {
	INCLUDEPATH+=/usr/include/stone
	INCLUDEPATH+=/usr/include/classes
}

macx {
	LIBS+=-L../classes -lclasses
	LIBS+=-L../stonegui -lstonegui
	LIBS+=-L../stone -lstone
}

DEFINES+=CLASSESUI_MAKE_DLL

TEMPLATE=lib
TARGET=classesui

unix {
	target.path=$$(DESTDIR)/usr/local/lib
}
win32 {
	target.path=$$(DESTDIR)/blur/common/
}

INSTALLS += target

CONFIG+=qt thread
QT+=sql xml gui network

DESTDIR=./
