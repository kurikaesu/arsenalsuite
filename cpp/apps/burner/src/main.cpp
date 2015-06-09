
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

/*
 * $Id$
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <qapplication.h>
#include <qdir.h>
#include <qfile.h>
#include <qlibrary.h>
#include <qprocess.h>
#include <qsettings.h>

#include "maindialog.h"
#include "common.h"
#include "blurqt.h"
#include "database.h"
#include "freezercore.h"
#include "slave.h"
#include "process.h"

#include "jobbatch.h"
#include "host.h"
#include "hoststatus.h"

#include "stonegui.h"
#include "classes.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <winerror.h>

#ifndef KEY_WOW64_64KEY
#define KEY_WOW64_64KEY 0x0100
#endif

const char * ABPSMON = "abpsmon.exe";

// Force a "crash" so that we get a backtrace through drmingw
// qt uses abort in many places when fatal errors are detected
// which can be caused by memory corruption etc.
void abrt_handler(int )
{
	printf("abort handler\n");
	raise(SIGSEGV);
}

#else // !Q_OS_WIN

#include <unistd.h>
const char * ABPSMON = "abpsmon";

#endif


// Returns true if there isn't another burner.exe running
bool startup( bool usePsmon, bool daemonize, bool useSingleProcessMutex )
{
#ifdef Q_OS_WIN
	if( useSingleProcessMutex ) {
		hMutex = CreateMutex( NULL, true, L"FreezerSingleProcess");
		if (hMutex == NULL) {
			LOG_3( "Error: Couldn't create mutex, exiting" );
			return false;
		}
		if( GetLastError() == ERROR_ALREADY_EXISTS ) {
			LOG_3( "Error: Another process owns the mutex, exiting" );
			return false;
		}
	}
#ifndef Q_OS_WIN64
	QLibrary excdll( "exchndl.dll" );
	if( !excdll.load() ) {
		LOG_1( "Error Loading exchndl.dll(Crash Handler) Error Was: " + excdll.errorString() );
	}
#endif
	disableWindowsErrorReporting( "burner.exe" );
	//signal(SIGABRT,abrt_handler);
#endif
	if( !daemonize && usePsmon && pidsByName( ABPSMON ) == 0 )
		QProcess::startDetached( ABPSMON );
#ifndef Q_OS_WIN
	if( daemonize )
		return daemon( 0, 0 ) == 0;
#endif
	return true;
}

static QCoreApplication * a = 0;

//
// With qt4 the app freezes upon logout,
// no matter what i do in the mainwindow's
// closeEvent.  This function makes the
// app close properly
//
#ifdef Q_OS_WIN
static bool winEventFilter( void * message, long * result )
{
	MSG * msg = (MSG*)message;
	if( msg->message == WM_QUERYENDSESSION || msg->message == WM_ENDSESSION ) {
		LOG_5( "Quitting application in response to WM_QUERYENDSESSION|WM_ENDSESSION message." );
		if( a )
			a->quit();
		*result = TRUE;
		return true;
	}
	return false;
}

#else

static int setup_unix_signal_handlers()
{
    struct sigaction sigint, sigterm;
 
    sigint.sa_handler = Slave::termSignalHandler;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags |= SA_RESTART;
 
    if (sigaction(SIGINT, &sigint, 0) > 0)
        return 1;

    sigterm.sa_handler = Slave::termSignalHandler;
    sigemptyset(&sigterm.sa_mask);
    sigterm.sa_flags |= SA_RESTART;
 
    if (sigaction(SIGTERM, &sigterm, 0) > 0)
        return 2;
 
    return 0;
}

#endif // !Q_OS_WIN

int main(int argc, char * argv[])
{
	bool gui = true;
	bool showSql = false;
	bool createDb = false;
	bool verifyDb = false;
	bool autoRegister = false;
	bool usePsmon = true;
	bool daemonize = false;
	bool inNewLogonSession = false;
	bool burnOnlyMode = false;
	// Burn Only param
	int jobAssignmentKey = 0;
#ifdef Q_OS_WIN
	QString configFile = "burner.ini";
#else
	QString configFile = "/etc/ab/burner.ini";
#endif
	for( int i = 1; i<argc; i++ ){
		QString arg( argv[i] );
		if( arg == "-h" || arg == "--help" )
		{
			printf( "Options:\n" );
			printf( "-config <file>" );
			printf( "\tLocation of configuration .ini file\n" );
			printf( "-nogui" );
			printf( "\tRun without a gui, for headless slaves\n" );
			printf( "-show-sql" );
			printf( "\tOutputs all executed sql to stdout\n" );
			printf( "-create-database" );
			printf( "\tCreates all missing tables in the database, then exits\n" );
			printf( "-verify-database" );
			printf( "\tVerifies all tables in the database, then exits\n" );
			printf( "-output-schema FILE" );
			printf( "\tOutputs the database schema in xml format to FILE\n" );
			printf( "-auto-register" );
			printf( "\tIf no Host record exists, create it\n" );
			printf( "-no-psmon" );
			printf( "\tDo not run the abpsmon utility, responsible for restarting AB ourself\n" );
#ifndef Q_OS_WIN
			printf( "-daemonize" );
			printf( "\tDetach from terminal, this option also disables the gui and the psmon utility.\n" );
#endif
			return 0;
		}
		else if( arg == "-show-sql" )
			showSql = true;
		else if( arg == "-create-database" )
			createDb = true;
		else if( arg == "-verify-database" )
			verifyDb = true;
		else if( arg == "-output-schema" && (i+1 < argc) )
			return 0;
		else if( arg == "-nogui" )
			gui = false;
		else if( arg == "-auto-register" )
			autoRegister = true;
		else if( arg == "-no-psmon" )
			usePsmon = false;
		else if( arg == "-daemonize" ) {
			daemonize = true;
			usePsmon = gui = false;
		}
		else if( arg == "-config" )
			configFile = argv[i+1];
		else if( arg == "-in-new-logon-session" )
			inNewLogonSession = true;
		else if( arg == "-burn" ) {
			++i;
			if( i >= argc ) {
				printf( "-burn requires a job assignment key as the next argument\n" );
				return 1;
			}
			burnOnlyMode = true;
			gui = autoRegister = usePsmon = daemonize = false;
			jobAssignmentKey = QString(argv[++i]).toInt();
		}
	}

	if( gui )
		a = new QApplication( argc, argv );
	else
		a = new QCoreApplication( argc, argv );

	// Use default log file from config file unless we are in burn only mode
	QString logFile;
	if( burnOnlyMode )
		logFile = "burner_burn_" + QString::number( jobAssignmentKey ) + ".log";

#ifdef Q_OS_WIN
	if( !initConfig( configFile, logFile ) )
        return -1;
	initUserConfig( QDir::homePath() + "/.burner.ini" );
#else
	if( !initConfig( configFile ) )
        return -1;
	config().readFromFile( "/etc/ab/burner.ini", false );
	initUserConfig( QDir::homePath() + "/.burner.ini" );
#endif

	initStone( argc, argv );

#ifdef Q_OS_WIN
	if( !inNewLogonSession ) {
		IniConfig & ini = config();
		ini.pushSection( "LogonSession" );
		if ( ini.readBool( "UseNewSession", false ) ) {
			Win32Process * proc = new Win32Process();
			proc->setLogonFlag( Win32Process::LogonWithProfile );
			QString user = ini.readString( "Username" ), domain = ini.readString( "Domain" );
			proc->setLogon( user, ini.readString( "Password" ), domain );
			proc->start( "c:\\blur\\burner\\burner.exe", QStringList() << "-in-new-logon-session" );
			if( proc->isRunning() ) {
				if( !domain.isEmpty() ) user += "/" + domain;
				LOG_1( "Burner started in new logon session as user " + user + ", pid is " + QString::number( proc->pid()->dwProcessId ) );
				return 0;
			}
			LOG_5( "Unable to start burner in new logon session" );
			return 1;
		}
		ini.popSection();
	}
#endif // Q_OS_WIN

	bool do_start = startup(usePsmon,daemonize,!burnOnlyMode);

	IniConfig & cfg = config();
	cfg.pushSection("Assburner");

	QString appName = cfg.readString("ApplicationName","Assburner");
	a->setApplicationName( appName );
	LOG_1( appName+" version " + VERSION + ", build " + QString("$Date$").remove(QRegExp("[^\\d]")) + " starting" );

	classes_loader();
	initStoneGui();

	if( showSql ) {
		showSql = true;
		Database::current()->setEchoMode(
			Database::EchoSelect
			| Database::EchoUpdate
			| Database::EchoInsert
			| Database::EchoDelete );
	}
	if( createDb ) {
		printf( "VALIDATING/CREATING Tables" );
		printf( Database::current()->createTables() ? "Success" : "Failure" );
		return 0;
	}
	if( verifyDb ) {
		printf( "VALIDATING Tables" );
		printf( Database::current()->verifyTables() ? "Success" : "Failure" );
		return 0;
	}

	if( usePsmon && !do_start ) {
		Host h = Host::currentHost();
		HostStatus hs = h.hostStatus();
		QString s = hs.slaveStatus();
		if( s == "offline" || s == "stopping" || s == "restart" ) {
			hs.setSlaveStatus( "starting" );
			// This call commits the host status record
			hs.returnSlaveFrames();
		}
		return 1;
	}

	Slave * s = new Slave(gui, autoRegister, burnOnlyMode);
	s->setInOwnLogonSession( inNewLogonSession );

	int res = 0;
	if( gui ) {
#ifdef Q_OS_WIN
		a->setEventFilter( winEventFilter );
#endif // Q_OS_WIN

		MainDialog * md = new MainDialog(s);
		//md->show();
		//don't show so that client-updates when status offline don't pop up md
		
		((QApplication*)a)->setQuitOnLastWindowClosed(false);
		res = a->exec();
		LOG_5( "::main: a->exec() returned " + QString(res) );
		LOG_5( "::main: Deleting MainDialog" );
		delete md;
	} else {
		LOG_3( "starting event loop" );
#ifndef Q_OS_WIN
		setup_unix_signal_handlers();
#endif
		res = a->exec();
		LOG_5( "::main: a->exec() returned " + QString(res) );
	}

#ifdef Q_OS_WIN
	LOG_5( "::main: Closing mutex" );
	
	if( hMutex )
		CloseHandle( hMutex );
#endif
	// Host will stay at restart status until the new instance
  // sets it's status to online
	if( usePsmon && Host::currentHost().hostStatus().slaveStatus() != "restart" )
		killAll( ABPSMON );
	LOG_5( "::main: Calling shutdown()" );
	shutdown();
	return res;
}

