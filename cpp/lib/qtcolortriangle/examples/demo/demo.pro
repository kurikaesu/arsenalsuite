TEMPLATE = app
CONFIG -= moc
INCLUDEPATH += .

include(../../src/qtcolortriangle.pri)

# Input
SOURCES += main.cpp colordialog.cpp sketchpad.cpp
HEADERS += colordialog.h sketchpad.h
