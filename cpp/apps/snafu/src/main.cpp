
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Snafu.
 *
 * Snafu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Snafu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Snafu; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: main.cpp 8442 2009-06-09 03:46:10Z brobison $
 */

#include <qapplication.h>
#include <qpalette.h>
#include <qdir.h>
#include <QSplashScreen>

#include "stonegui.h"
#include "mainwindow.h"
#include "snafuwidget.h"
//#include "afcommon.h"
#include "blurqt.h"
#include "database.h"
#include "freezercore.h"
#include "tardstyle.h"
#include "host.h"
#include "user.h"
#include "process.h"
#include "classes.h"
#include "schema.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif

extern void classes_loader();

#ifdef Q_OS_WIN
BOOL CALLBACK SNAFUEnumWindowsProc( HWND hwnd, LPARAM otherProcessId )
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

int main( int argc, char * argv[] )
{
	int result=0;

#ifdef Q_OS_WIN
	hMutex = CreateMutex( NULL, true, L"SnafuSingleProcessMutex");
	if (hMutex == NULL) {
		LOG_5( "Error: Couldn't create mutex, exiting" );
		return false;
	}
	if( GetLastError() == ERROR_ALREADY_EXISTS ) {
		LOG_5( "Error: Another process owns the mutex, exiting" );
		QList<int> pids;
		if( pidsByName( "snafu.exe", &pids ) ) {
			int otherProcessId = pids[0];
			if( otherProcessId == processID() ) {
				if( pids.size() < 2 )
					return false;
				otherProcessId = pids[1];
			}
			LOG_5( "Trying to find window with process pid of " + QString::number( otherProcessId ) );
			EnumWindows( SNAFUEnumWindowsProc, LPARAM(otherProcessId) );
		}
		return false;
	}
#endif

	QApplication a(argc, argv);

	initConfig( "snafu.ini" );
#ifdef Q_OS_WIN
	QString cp = "h:/public/" + getUserName() + "/Blur";
	if( !QDir( cp ).exists() )
		cp = "C:/Documents and Settings/" + getUserName();
	initUserConfig( cp + "/snafu.ini" );
#else
	initUserConfig( QDir::homePath() + "/.snafu" );
#endif
	classes_loader();
	initStoneGui();

	// Share the database across threads, each with their own connection
	FreezerCore::setDatabaseForThread( classesDb(), Connection::createFromIni( config(), "Database" ) );
	{
		IniConfig & cfg = config();
		bool showTime = false;
	
		for( int i = 1; i<argc; i++ ){
			QString arg( argv[i] );
			bool hasNext = i+1<argc;
			if( arg == "-h" || arg == "--help" )
			{
				LOG_5( QString("Snafu v") + VERSION );
				LOG_5( "Options:" );
							LOG_5( "-current-render" );
							LOG_5( "\tShow the current job that is rendering on this machine\n" );
				LOG_5( "-show-sql" );
				LOG_5( "\tOutputs all executed sql to stdout\n" );
				LOG_5( "-show-time" );
				LOG_5( "\tOutputs all executed sql to stdout\n" );
				LOG_5( "-create-database" );
				LOG_5( "\tCreates all missing tables in the database, then exits\n" );
				LOG_5( "-verify-database" );
				LOG_5( "\tVerifies all tables in the database, then exits\n" );
				LOG_5( "-output-schema FILE" );
				LOG_5( "\tOutputs the database schema in xml format to FILE\n" );
				LOG_5( "-db-host HOST" );
				LOG_5( "\tSet the database host to HOST. Can be either a hostname or an ipv4 address" );
				LOG_5( "-db-port PORT" );
				LOG_5( "\tSet the database port to PORT" );
				LOG_5( "-db-user USER" );
				LOG_5( "\tSet the database username to USER" );
				LOG_5( "-db-password PASS" );
				LOG_5( "\tSet the database password to PASS" );
				LOG_5( "-user USER" );
				LOG_5( "\tSet the logged in user to USER: Requires Admin Privs" );
				return 0;
			} else
			if( arg == "-db-host" && hasNext ) {
				cfg.pushSection( "Database" );
				cfg.writeString( "Host", argv[++i] );
				cfg.popSection();
			} else
			if( arg == "-db-port" && hasNext ) {
				cfg.pushSection( "Database" );
				cfg.writeString( "Port", argv[++i] );
				cfg.popSection();
			} else
			if( arg == "-db-user" && hasNext ) {
				cfg.pushSection( "Database" );
				cfg.writeString( "User", argv[++i] );
				cfg.popSection();
			} else
			if( arg == "-db-password" && hasNext ) {
				cfg.pushSection( "Database" );
				cfg.writeString( "Password", argv[++i] );
				cfg.popSection();
			} else
			if( arg == "-show-sql" )
				Database::current()->setEchoMode(
					Database::EchoSelect
					| Database::EchoUpdate
					| Database::EchoInsert
					| Database::EchoDelete );
			else if( arg == "-show-time" )
				showTime = true;
			else if( arg == "-create-database" ){
				LOG_5( "VALIDATING/CREATING Tables" );
				LOG_5( Database::current()->createTables() ? "Success" : "Failure" );
				return 0;
			}
			else if( arg == "-verify-database" ){
				LOG_5( "VALIDATING Tables" );
				LOG_5( Database::current()->verifyTables() ? "Success" : "Failure" );
				return 0;
			}
			else if( arg == "-output-schema" && (i+1 < argc) ){
				classesSchema()->writeXmlSchema( argv[++i] );
				return 0;
			}
			else if( arg == "-user" && (i+1 < argc) ) {
				QString impersonate( argv[++i] );
				if( User::hasPerms( "User", true ) ) // If you can edit users, you can login as anyone
					User::setCurrentUser( impersonate );
			}
		}
	
		
		{
			//FreezerCore::instance();
			Database * db = Database::current();
			QDir schemaDir = QDir::current();
			schemaDir.cd( "schemas" );
			if( schemaDir.exists() ) {
				QStringList schemas = schemaDir.entryList( QStringList() << "*.xml" );
				st_foreach( QStringList::Iterator, it, schemas )
					classesSchema()->mergeXmlSchema( schemaDir.path() + "/" + *it );
			}
					
			QPixmap pixmap("images/splash.png");
			QSplashScreen *splash = new QSplashScreen(pixmap);
			splash->show();

			MainWindow * m = new MainWindow;
			IniConfig & cfg = userConfig();
			cfg.pushSection( "MainWindow" );
			QStringList fg = cfg.readString( "FrameGeometry", "" ).split(',');
			cfg.popSection();
			if( fg.size()==4 ) {
				m->resize( QSize( fg[2].toInt(), fg[3].toInt() ) );
				m->move( QPoint( fg[0].toInt(), fg[1].toInt() ) );
			}
			m->show();
			splash->finish(m);

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

