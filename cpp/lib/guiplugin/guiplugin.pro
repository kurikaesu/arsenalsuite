
SOURCES  += plugin.cpp
HEADERS  += plugin.h
TARGET    = blurwidgets

INCLUDEPATH+=../stonegui/include ../stone/include
LIBS+=-L../stone -lstone -L../stonegui -lstonegui

TEMPLATE     = lib
CONFIG      += qt warn_on designer plugin thread
INCLUDEPATH += include
DBFILE       = plugin.db
LANGUAGE     = C++
DEFINES += DESIGNER_PLUGIN

target.path=$$(QTDIR)/plugins/designer/
INSTALLS += target

DESTDIR=./
