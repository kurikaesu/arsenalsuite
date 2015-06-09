
Blur C++ repository, general build and dependancy notes

prerequsites for building on Win2K/XP --
	(see H:\public\abe\blur_build_prerequisites for downloads)
	1. Download/install MinGW, see http://mingw.sourceforge.net
		- use c:\mingw as install dir
		- also download/install mingw32-make(can copy mingw32-make.exe to make.exe for convenience)
		- add C:\MinGW\bin to your %PATH% envornment variable
	2. Download/install Postgres (still optional at this point)
		see http://www.postgresql.org/ftp/binary
		- when installing, drop-down the 'Development' option
		and make sure to include the Include files and
		Library files options. For a simpler install,
		exclude the 'Database Server' itself
	3. Download/install QT4.1.x,
		see http://www.trolltech.com/download/opensource.html
	4. Resin ActiveX Requirements
		NOTE: The resin activex control requires activeqt which comes
		in the Qt Desktop Edition commercial licensed Qt.
		It also requires the midl compiler, provided either with
		visual studio, or the free to download microsoft sdk
	5. Build qt-postgres lib
		specific cmd line:
		C:\Qt\4.0.1\src\plugins\sqldrivers\psql>qmake \
		"INCLUDEPATH+=C:\postgres\postgresql-8.0.1\src\interfaces\libpq C:\postgres\postgresql-8.0.1\src\include" "LIBS+=-LC:\postgres\postgresql-8.0.1\src\interfaces\libpq -lpq"
		C:\Qt\4.0.1\src\plugins\sqldrivers\psql>make
	6. Get your buildin' on.


--------------- BUILD INSTRUCTIONS -------------------------

linux>
python2.4 build.py TARGETS OPTIONS
win32>
build.py TARGETS OPTIONS

TARGETS ->
	all
	assburner
	resin
	resinax
	absubmit
	freezer
	pyqt
	pyqt_modules
	qt_installer

OPTIONS ->
	debug
	console
	clean

Target Specific Options
specified as TARGET:OPTION

current target specific options include
TARGET:skip - skips building this target, even if it is a dependancy of another target

Examples:

Building a clean release build of assburner
build.py assburner clean

Re-building resin, skipping pyqt and pyclassesstatic
build.py resin pyqt:skip pyclassesstatic:skip

Building everything clean
build.py all clean

