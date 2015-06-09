SOURCES += \
	Qt.cpp \
	handlers.cpp

INCLUDEPATH += ../smoke
INCLUDEPATH += ../../stone/include ../../stone/.out ../../classes/autocore ../../classes

# Red Hat / CentOS
INCLUDEPATH += /usr/include/stone /usr/include/classes
LIBS+=-L/usr/lib/stone -L/usr/lib/classes
LIBS+=-L../../stone
LIBS+=-L../../classes
LIBS+=-L../smoke/qt
LIBS+=-lsmokeqt
LIBS+=-lclasses -lstone

win32 {
	INCLUDEPATH += C:/Perl/lib/CORE 
	LIBS+=-lpsapi -lMpr -lws2_32
	LIBS+=-LC:\Perl\lib\CORE
	LIBS+=-LC:\Perl\bin
	LIBS+=-lperl58
	LIBS+=-lPerlEz
}
unix {
	INCLUDEPATH += /usr/lib/perl5/5.8.8/i686-linux/CORE/
	INCLUDEPATH += /usr/lib/perl/5.8.8/CORE
	INCLUDEPATH += /usr/lib/perl/5.8.7/CORE
	INCLUDEPATH += /usr/lib/perl/5.8.5/CORE
	INCLUDEPATH += /usr/lib/perl/5.8.4/CORE
	INCLUDEPATH += /usr/lib/perl/5.8.3/CORE
	INCLDUEPATH += /usr/lib/perl5/5.8.8/i386-linux-thread-multi/CORE/
	INCLUDEPATH += /usr/lib/perl5/5.8.7/i386-linux-thread-multi/CORE/
	INCLUDEPATH += /usr/lib/perl5/5.8.5/i386-linux-thread-multi/CORE/
	INCLUDEPATH += /usr/lib/perl5/5.8.4/i386-linux-thread-multi/CORE/
	INCLUDEPATH += /usr/lib/perl5/5.8.3/i386-linux-thread-multi/CORE/
	INCLUDEPATH += /usr/lib/perl5/5.8.8/i386-linux-thread-multi/CORE/
	INCLUDEPATH += /usr/lib64/perl/5.8.3/CORE
	INCLUDEPATH += /usr/lib64/perl/5.8.4/CORE
	INCLUDEPATH += /usr/lib64/perl/5.8.5/CORE
	INCLUDEPATH += /usr/lib64/perl5/5.8.3/x86_64-linux-thread-multi/CORE/
	INCLUDEPATH += /usr/lib64/perl5/5.8.4/x86_64-linux-thread-multi/CORE/
	INCLUDEPATH += /usr/lib64/perl5/5.8.5/x86_64-linux-thread-multi/CORE/
	LIBS+=-L/usr/lib/perl5/5.8.8/i386-linux-thread-multi/CORE/
	LIBS+=-L/usr/lib/perl5/5.8.7/i386-linux-thread-multi/CORE/
	LIBS+=-L/usr/lib/perl5/5.8.5/i386-linux-thread-multi/CORE/
	LIBS+=-L/usr/lib/perl5/5.8.4/i386-linux-thread-multi/CORE/
	LIBS+=-L/usr/lib/perl5/5.8.3/i386-linux-thread-multi/CORE/
	LIBS+=-L/usr/lib64/perl5/5.8.3/x86_64-linux-thread-multi/CORE/
	LIBS+=-L/usr/lib64/perl5/5.8.4/x86_64-linux-thread-multi/CORE/
	LIBS+=-L/usr/lib64/perl5/5.8.5/x86_64-linux-thread-multi/CORE/
	LIBS+=-lperl

}

QMAKE_CXXFLAGS+=-fno-strict-aliasing

TEMPLATE=lib
CONFIG+=qt thread console dll
QT+=sql network xml

TARGET=Qt

