
include( svnrev.pri )
include( auto.pri )

MOC_DIR=.out
OBJECTS_DIR=.out
UI_DIR=.out

INCLUDES += \
	classes.h

SOURCES += \
	classes.cpp

INCLUDEPATH+=../stone/include autocore autoimp base /usr/include/stone .

win32 {
	LIBS+=-lMpr -lws2_32 -ladvapi32 -luser32
	PY_PATH=$$system("python -c \"from distutils.sysconfig import get_config_vars; print get_config_vars()['prefix']\"")
	INCLUDEPATH+=$$system("python -c \"from distutils.sysconfig import get_python_inc; print get_python_inc()\"")
	PY_VERSION=$$system("python -c \"from distutils.sysconfig import get_python_version; print get_python_version().replace('.','')\"")
	message(Python Version is $$PY_VERSION Python lib path is $$PY_PATH\\libs)
	LIBS+=-L$${PY_PATH}\\libs -lpython$${PY_VERSION}
	QMAKE_CXXFLAGS+=/Zi
	QMAKE_LFLAGS+=/DEBUG /OPT:REF /OPT:ICF
}

unix {
        INCLUDEPATH+=$$system("python -c \"from distutils.sysconfig import get_python_inc; print get_python_inc()\"")
        PY_VERSION=$$system("python -c \"from distutils.sysconfig import get_python_version; print get_python_version()\"")

        message(Python Version is $$PY_VERSION)
        INCLUDEPATH += /usr/include/python$${PY_VERSION}/
        INCLUDEPATH += ../sip/siplib/
        LIBS+=-lpython$${PY_VERSION}
}

# Python modules
#debug {
#    #LIBS+=-L../../lib/sip/siplib -lsip_d
#} else {
    #LIBS+=-L../../lib/sip/siplib -lsip
#}


DEFINES+=CLASSES_MAKE_DLL
TEMPLATE=lib
CONFIG+=qt thread
contains( DEFINED, versioned ) {
	TARGET=classes$$SVNREV
	LIBS+=-L../stone -lstone$$SVNREV
} else {
	TARGET=classes
	LIBS+=-L../stone -lstone
}

unix {
	target.path=$$(DESTDIR)/usr/local/lib
}
win32 {
	target.path=$$(DESTDIR)/blur/common/
}

INSTALLS += target

QT+=xml sql network

DESTDIR=./
