INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
SOURCES += $$PWD/qtcolortriangle.cpp
HEADERS += $$PWD/qtcolortriangle.h

win32:contains(TEMPLATE, lib):contains(CONFIG, shared) {
    DEFINES += QT_COLORTRIANGLE_EXPORT=__declspec(dllexport)
}
