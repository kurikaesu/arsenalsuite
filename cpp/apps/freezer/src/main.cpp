
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Arsenal.
 *
 * Arsenal is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Arsenal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arsenal; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Python.h"
#include "signal.h"

#include <qapplication.h>
#include <qdir.h>
#include <qhostaddress.h>
#include <qlibrary.h>
#include <qpalette.h>

#include "blurqt.h"
#include "database.h"
#include "freezercore.h"
#include "schema.h"
#include "process.h"
#include "path.h"

#include "stonegui.h"
#include "tardstyle.h"

#include "classes.h"
#include "host.h"
#include "jobassignment.h"
#include "user.h"

#include "afcommon.h"
#include "joblistwidget.h"
#include "mainwindow.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif

extern void classes_loader();

#ifdef Q_OS_WIN
BOOL CALLBACK AFEnumWindowsProc( HWND hwnd, LPARAM otherProcessId )
{
	int pid = (int)otherProcessId;
	int winPid;
	GetWindowThreadProcessId( hwnd, (DWORD*)&winPid );
	LOG_5( "Found window with process id: " + QString::number( winPid ) );
	if( winPid == pid ) {
		LOG_5( "Raising window" );
		if( IsIconic( hwnd ) )
			OpenIcon( hwnd );
		SetForegroundWindow( hwnd );
	//	SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE );
	}
	return true;
}

HANDLE hMutex;
#endif

#ifdef Q_OS_WIN
extern "C" void initsip(void);
extern "C" void initStone(void);
extern "C" void initClasses(void);
extern "C" void initStonegui(void);
extern "C" void initClassesui(void);
extern "C" void initFreezer(void);
extern "C" void initabsubmit(void);
#endif

// Return the named attribute object from the named module.
PyObject * getModuleAttr(const char *module, const char *attr)
{
#if PY_VERSION_HEX >= 0x02050000
    PyObject *mod = PyImport_ImportModule(module);
#else
    PyObject *mod = PyImport_ImportModule(const_cast<char *>(module));
#endif

    if (!mod)
    {
        PyErr_Print();
        return 0;
    }

#if PY_VERSION_HEX >= 0x02050000
    PyObject *obj = PyObject_GetAttrString(mod, attr);
#else
    PyObject *obj = PyObject_GetAttrString(mod, const_cast<char *>(attr));
#endif

    Py_DECREF(mod);

    if (!obj)
    {
        PyErr_Print();
        return 0;
    }

    return obj;
}

void loadPythonPlugins()
{
	LOG_5( "Loading Python" );

#ifdef Q_OS_WIN
	/*
	 * This structure specifies the names and initialisation functions of
	 * the builtin modules.
	 */
	struct _inittab builtin_modules[] = {
		{"sip", initsip},
		{"blur.Stone",initStone},
		{"blur.Classes",initClasses},
		{"blur.Stonegui",initStonegui},
		{"blur.Classesui",initClassesui},
		{"blur.Freezer", initFreezer},
		{"blur.absubmit", initabsubmit},
		{NULL, NULL}
	};

	PyImport_ExtendInittab(builtin_modules);
#endif
	
	Py_Initialize();
	// Initialise python threads
	PyEval_InitThreads();

	const char * builtinModuleImportStr = 
#ifdef Q_OS_WIN
		"import imp,sys\n"
		"class MetaLoader(object):\n"
		"\tdef __init__(self):\n"
		"\t\tself.modules = {}\n"
		"\t\tself.modules['blur.Stone'] = imp.load_module('blur.Stone',None,'',('','',imp.C_BUILTIN))\n"
		"\t\tself.modules['blur.Classes'] = imp.load_module('blur.Classes',None,'',('','',imp.C_BUILTIN))\n"
		"\t\tself.modules['blur.Stonegui'] = imp.load_module('blur.Stonegui',None,'',('','',imp.C_BUILTIN))\n"
		"\t\tself.modules['blur.Classesui'] = imp.load_module('blur.Classesui',None,'',('','',imp.C_BUILTIN))\n"
		"\t\tself.modules['blur.Freezer'] = imp.load_module('blur.Freezer',None,'',('','',imp.C_BUILTIN))\n"
		"\t\tself.modules['blur.absubmit'] = imp.load_module('blur.absubmit',None,'',('','',imp.C_BUILTIN))\n"
		"\tdef find_module(self,fullname,path=None):\n"
		"\t\tif fullname in self.modules:\n"
		"\t\t\treturn self.modules[fullname]\n"
		"sys.meta_path.append(MetaLoader())\n"
		"import blur\n"
		"blur.RedirectOutputToLog()\n";
#else
		"import blur\n"
		"blur.RedirectOutputToLog()\n";
#endif
	PyRun_SimpleString(builtinModuleImportStr);

	LOG_5( "Loading python afplugins" );

    PyObject * sys_path = getModuleAttr("sys", "path");

    if (!sys_path) {
        LOG_1( "Python Initialization Failure: Failed to get sys.path" );
        return;
    }

    QString dir = QDir::currentPath();

    // Convert the directory to a Python object with native separators.
#if QT_VERSION >= 0x040200
    dir = QDir::toNativeSeparators(dir);
#else
    dir = QDir::convertSeparators(dir);
#endif

#if PY_MAJOR_VERSION >= 3
    // This is a copy of qpycore_PyObject_FromQString().

    PyObject *dobj = PyUnicode_FromUnicode(0, dir.length());

    if (!dobj)
    {
        PyErr_Print();
        return;
    }

    Py_UNICODE *pyu = PyUnicode_AS_UNICODE(dobj);

    for (int i = 0; i < dir.length(); ++i)
        *pyu++ = dir.at(i).unicode();
#else
        PyObject *dobj = PyString_FromString(dir.toAscii().constData());

        if (!dobj)
        {
            PyErr_Print();
            return;
        }
#endif

    // Add the directory to sys.path.
    int rc = PyList_Append(sys_path, dobj);
    Py_DECREF(dobj);

    if (rc < 0)
    {
        PyErr_Print();
        return;
    }
	
	PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject *plug_mod = PyImport_ImportModule("afplugins");
    if (!plug_mod)
    {
        PyErr_Print();
        return;
    }
    Py_DECREF(plug_mod);
	
	PyGILState_Release(gstate);

    PyEval_SaveThread();
}

void oops_handler(int )
{
	printf("abort handler\n");
    printBackTrace();
    exit(1);
}

int main( int argc, char * argv[] )
{
	int result=0;

#ifdef Q_OS_WIN
	hMutex = CreateMutex( NULL, true, L"FreezerSingleProcessMutex");
	if (hMutex == NULL) {
		LOG_5( "Error: Couldn't create mutex, exiting" );
		return false;
	}
	if( GetLastError() == ERROR_ALREADY_EXISTS ) {
		LOG_5( "Error: Another process owns the mutex, exiting" );
		QList<int> pids;
		if( pidsByName( "freezer.exe", &pids ) ) {
			int otherProcessId = pids[0];
			if( otherProcessId == processID() ) {
				if( pids.size() < 2 )
					return false;
				otherProcessId = pids[1];
			}
			LOG_5( "Trying to find window with process pid of " + QString::number( otherProcessId ) );
			EnumWindows( AFEnumWindowsProc, LPARAM(otherProcessId) );
		}
		return false;
	}

	QLibrary excdll( "exchndl.dll" );
	if( !excdll.load() ) {
		qWarning( excdll.errorString().toLatin1().constData() );
	}
	disableWindowsErrorReporting( "assburner.exe" );

#endif

    signal(SIGSEGV, oops_handler);
    signal(SIGABRT, oops_handler);
	QApplication a(argc, argv);

    if( !initConfig( "freezer.ini" ) ) {
#ifndef Q_OS_WIN
        // Fallback if the config file does not exist in the current folder
        if( !initConfig( "/etc/ab/freezer.ini" ) )
#endif
        return -1;
    }

#ifdef Q_OS_WIN
	QString cp = "h:/public/" + getUserName() + "/Blur";
	if( !QDir( cp ).exists() )
		cp = "C:/Documents and Settings/" + getUserName();
	initUserConfig( cp + "/freezer.ini" );
#else
	initUserConfig( QDir::homePath() + "/.freezer" );
#endif

    initStone( argc, argv );
    classes_loader();
    initStoneGui();
	{
		JobList showJobs;
		bool showTime = false;
        QString currentView;
        QStringList loadViewFiles;

		for( int i = 1; i<argc; i++ ){
			QString arg( argv[i] );
			if( arg == "-h" || arg == "--help" )
			{
				LOG_1( QString("Freezer v") + VERSION );
				LOG_1( "Options:" );
				LOG_1( "-current-render" );
				LOG_1( "\tShow the current job that is rendering on this machine\n" );
				LOG_1( "-show-time" );
				LOG_1( "\tOutputs summary of time executed for all sql statement at program close\n" );
				LOG_1( "-user USER" );
				LOG_1( "\tSet the logged in user to USER: Requires Admin Privs" );
				LOG_1( "-current-view VIEWNAME" );
				LOG_1( "\tMake VIEWNAME the active view, once they are all loaded" );
				LOG_1( "-load-view FILE" );
				LOG_1( "\tRead a saved view config from FILE" );
				LOG_1( stoneOptionsHelp() );
				return 0;
			}
			else if( arg.endsWith("-show-time") )
				showTime = true;
			else if( arg.endsWith( "-current-render" ) ) {
				showJobs = Host::currentHost().activeAssignments().jobs();
			}
			else if( arg.endsWith("-user") && (i+1 < argc) ) {
				QString impersonate( argv[++i] );
				if( User::hasPerms( "User", true ) ) // If you can edit users, you can login as anyone
					User::setCurrentUser( impersonate );
			}
            else if( arg.endsWith("-current-view") && (i+1 < argc) ) {
				currentView = QString( argv[++i] );
            }
            else if( arg.endsWith("-load-view") && (i+1 < argc) ) {
                loadViewFiles << QString(argv[++i]);
            }
		}

		// Share the database across threads, each with their own connection
		FreezerCore::setDatabaseForThread( classesDb(), Connection::createFromIni( config(), "Database" ) );
		
		{
            loadPythonPlugins();
			MainWindow m;
			IniConfig & cfg = userConfig();
			cfg.pushSection( "MainWindow" );
			QStringList fg = cfg.readString( "FrameGeometry", "" ).split(',');
			cfg.popSection();
			if( fg.size()==4 ) {
				m.resize( QSize( fg[2].toInt(), fg[3].toInt() ) );
				m.move( QPoint( fg[0].toInt(), fg[1].toInt() ) );
			}
			if( showJobs.size() )
				m.jobPage()->setJobList( showJobs );
            foreach( QString viewFile, loadViewFiles )
                m.loadViewFromFile( viewFile );
            if( !currentView.isEmpty() )
                m.setCurrentView( currentView );
			m.show();
			result = a.exec();
			if( showTime ){
				Database * tm = Database::current();
				LOG_5( 			"                  Sql Time Elapsed" );
				LOG_5(			"|   Select  |   Update  |  Insert  |  Delete  |  Total  |" );
				LOG_5( 			"-----------------------------------------------" );
				LOG_5( QString(	"|     %1    |     %2    |    %3    |    %4    |    %5   |\n")
					.arg( tm->elapsedSqlTime( Table::SqlSelect ) )
					.arg( tm->elapsedSqlTime( Table::SqlUpdate ) )
					.arg( tm->elapsedSqlTime( Table::SqlInsert ) )
					.arg( tm->elapsedSqlTime( Table::SqlDelete ) )
					.arg( tm->elapsedSqlTime() )
				);
				LOG_5( 			"                  Index Time Elapsed" );
				LOG_5(			"|   Added  |   Updated  |  Incoming  |  Deleted  |  Search  |  Total  |" );
				LOG_5( 			"-----------------------------------------------" );
				LOG_5( QString(	"|     %1     |     %2    |    %3    |    %4   |    %5    |   %6    |\n")
					.arg( tm->elapsedIndexTime( Table::IndexAdded ) )
					.arg( tm->elapsedIndexTime( Table::IndexUpdated ) )
					.arg( tm->elapsedIndexTime( Table::IndexIncoming ) )
					.arg( tm->elapsedIndexTime( Table::IndexRemoved ) )
					.arg( tm->elapsedIndexTime( Table::IndexSearch ) )
					.arg( tm->elapsedIndexTime() )
				);
				tm->printStats();
			}
		}
	}
	shutdown();
#ifdef Q_OS_WIN
	CloseHandle( hMutex );
#endif
	return result;
}

