
SOURCES  += plugin.cpp \
	kdatepicker.cpp \
	kdatetbl.cpp \
	kcalendarsystem.cpp \
	kcalendarsystemgregorian.cpp 
		
HEADERS  += plugin.h \
	kdatepicker.h \
	kdatetbl.h \
	kcalendarsystem.h \
	kcalendarsystemgregorian.h
	
TARGET    = datepickerplugin

TEMPLATE     = lib
CONFIG      += plugin qt warn_on qt_no_compat_warning designer
INCLUDEPATH += include
QT += qt3support

