
include(svnrev.pri)

MOC_DIR=.out
OBJECTS_DIR=.out
UI_DIR=.out

SOURCES += \
	src/blurqt.cpp \
	src/changeset.cpp \
	src/connection.cpp \
	src/dateutil.cpp \
	src/database.cpp \
	src/expression.cpp \
	src/field.cpp \
	src/freezercore.cpp \
	src/graphite.cpp \
	src/index.cpp \
	src/indexschema.cpp \
	src/iniconfig.cpp \
	src/interval.cpp \
	src/joinedselect.cpp \
	src/path.cpp \
	src/packetsocket.cpp \
	src/pgconnection.cpp \
	src/process.cpp \
	src/process_win.cpp \
	src/pyembed.cpp \
	src/quickdatastream.cpp \
	src/recentvalues.cpp \
	src/record.cpp \
	src/recordlist.cpp \
	src/recordimp.cpp \
	src/recordproxy.cpp \
	src/recordxml.cpp \
	src/resultset.cpp \
	src/remotelogserver.cpp \
	src/schema.cpp \
	src/sqlerrorhandler.cpp \
	src/table.cpp \
	src/tableschema.cpp \
	src/transactionmgr.cpp \
	src/updatemanager.cpp \
	src/multilog.cpp \
	src/md5_globalstuff.cpp \
	src/md5.cpp \
	src/bt.cpp

	
HEADERS += \
	include/blurqt.h \
	include/changeset.h \
	include/connection.h \
	include/dateutil.h \
	include/database.h \
	include/expression.h \
	include/field.h \
	include/freezercore.h \
	include/graphite.h \
	include/graphite_p.h \
	include/index.h \
	include/indexschema.h \
	include/iniconfig.h \
	include/interval.h \
	include/joinedselect.h \
	include/path.h \
	include/packetsocket.h \
	include/pgconnection.h \
	include/process.h \
	include/pyembed.h \
	include/quickdatastream.h \
	include/recentvalues.h \
	include/record.h \
	include/recordimp.h \
	include/recordlist.h \
	include/recordproxy.h \
	include/recordxml.h \
	include/remotelogserver.h \
	include/resultset.h \
	include/schema.h \
	include/sqlerrorhandler.h \
	include/table.h \
	include/tableschema.h \
	include/transactionmgr.h \
	include/trigger.h \
	include/updatemanager.h \
	include/multilog.h \
	include/md5_bithelp.h \
	include/md5_globalstuff.h \ 
	include/md5.h
	
INCLUDEPATH+=include src .out

DEPENDPATH+=src include

DEFINES+=STONE_MAKE_DLL

win32 {
	LIBS+=-lPsapi -lMpr -ladvapi32 -lshell32 -luser32 -lpdh -lUserenv -lnetapi32 -lGdi32
	LIBS+=-Lc:\\IntelLib
	PY_PATH=$$system("python -c \"from distutils.sysconfig import get_config_vars; print get_config_vars()['prefix']\"")
	INCLUDEPATH+=$$system("python -c \"from distutils.sysconfig import get_python_inc; print get_python_inc()\"")
	PY_VERSION=$$system("python -c \"from distutils.sysconfig import get_python_version; print get_python_version().replace('.','')\"")
	message(Python Version is $$PY_VERSION Python lib path is $${PY_PATH}\\libs)
	LIBS+=-L$${PY_PATH}\\libs -lpython$${PY_VERSION}

	QMAKE_CXXFLAGS+=/Zi
	QMAKE_LFLAGS+=/DEBUG /OPT:REF /OPT:ICF
}

unix {
	INCLUDEPATH+=$$system("python -c \"from distutils.sysconfig import get_python_inc; print get_python_inc()\"")
	PY_VERSION=$$system("python -c \"from distutils.sysconfig import get_python_version; print get_python_version()\"")

	#QMAKE_CXXFLAGS+=-std=c++0x
	message(Python Version is $$PY_VERSION)
	INCLUDEPATH += /usr/include/python$${PY_VERSION}/
	INCLUDEPATH += ../sip/siplib/
	LIBS+=-lpython$${PY_VERSION}
}

macx{
  INCLUDEPATH += /opt/local/include/python$${PY_VERSION}/
}

TEMPLATE=lib
contains( DEFINES, versioned ) {
	TARGET=stone$$SVNREV
} else {
	TARGET=stone
}

unix {
	target.path=$$(DESTDIR)/usr/local/lib
}
win32 {
	target.path=$$(DESTDIR)/blur/common/
}

INSTALLS += target
CONFIG+=qt thread
QT+=sql xml network

DESTDIR=./
