
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of Bach.
 *
 * Bach is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Bach is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bach; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id: main.cpp 9408 2010-03-03 22:35:49Z brobison $
 */

#include <stdlib.h>

#include <qapplication.h>
#include <qsplashscreen.h>
#include <qdir.h>
#include <QTcpSocket>
#include <stdio.h>
#include <time.h>

#include "blurqt.h"
#include "stonegui.h"
#include "iniconfig.h"
#include "process.h"
#include "database.h"
#include "table.h"
#include "freezercore.h"
#include "updatemanager.h"

#include "bachmainwindow.h"
#include "ffimagesequenceprovider.h"

extern void bach_loader();

// TODO: remove this terrible hack, and do a read-only mode properly.
bool bach_editor_mode = false;

#ifdef Q_OS_WIN
const char * PID_FILE="C:\\Bach\\siren.pid";
const char * PROCESS_NAME="siren.exe";
#else
const char * PID_FILE="/tmp/bach.pid";
const char * PROCESS_NAME="bach";
#endif // Q_OS_WIN
QString CONFIG_FILE="bachro.ini";

int main( int argc, char * argv[] )
{
	srand( time( NULL ) );
	QApplication a( argc, argv );
	bool showSplash = true, allowDupWindow=false;

#ifdef Q_OS_WIN
	if( QDir::current() != QDir( "C:/blur/bach" ) )
		QDir::setCurrent( "C:/blur/bach" );
#endif // Q_OS_WIN

	QString url;
	bach_editor_mode = false;

	for( int i = 1; i<argc; i++ ){
		QString arg( argv[i] );
		//bool hasNext = i+1<argc;
		if( arg == "-h" || arg == "--help" )
		{
			printf( (QString("Bach v")).toLatin1() );
			printf( "Options:" );
			printf( "-no-splash" );
			printf( "\tDon't show the splash screen\n" );
			printf( "-editor|--editor" );
			printf( "\tChange to Editor mode (from read-only mode) : TODO: this ONLY changes the titlebar, need better method\n" );
			printf( "-config <file>|--config <file>" );
			printf( "\tSets the config file to use (default: %s)\n", CONFIG_FILE.toLatin1().data() );
			printf( "-cwd" );
			printf( "\tSets the current working directory to c:/Bach\n" );
			printf( "-generateAllThumbs" );
			printf( "\tGenerates thumbnails for every asset, and populates caches\n" );
			printf( "-dup\tAllows opening multiple windows, instead of bringing already open window to font\n" );
			printf( stoneOptionsHelp().toLatin1() );
			printf( "\n\n" );
			return 0;
		}
		else if( arg == "-no-splash" )
		{
			showSplash = false;
		}
		else if( arg == "-editor" || arg == "--editor" )
		{
			bach_editor_mode = true;
		}
		else if( arg == "-config" || arg == "--config" )
		{
			printf( "config count: [%s][%d][%d]\n", argv[ i ], i, argc );
			if ( ++i < argc )
			{
				printf( "config: [%s][%s]\n", arg.toLatin1().data(), argv[ i ] );
				CONFIG_FILE = argv[ i ];
			}
		}
		else if( arg == "-cwd" )
		{
			QDir::setCurrent("c:/Bach/");
		}
		else if( arg == "-dup" )
		{
			allowDupWindow = true;
		}
		else
		{
			printf(("got arg: " + arg).toLatin1());
			url = arg;
		}
	}

	if ( !QFile::exists( CONFIG_FILE ) )
	{
		QMessageBox::critical( NULL, "Config File", "Can't find config file: "+CONFIG_FILE );
		return -1;
	}

	initConfig( CONFIG_FILE );
	initStone( argc, argv );

#ifdef Q_OS_WIN
	initUserConfig( "h:/public/" + getUserName() + "/bach.ini" );
#else
	initUserConfig( QDir::homePath() + "/.bach.ini" );
#endif

	/* This will create the worker thread
	 * and connect to the database, must
	 * be done after QApplication construction,
	 * and before MainWindow construction */
	bach_loader();
	initStoneGui();
	//IniConfig & cfg = config();

	FreezerCore::setDatabaseForThread( bachDb(), Connection::createFromIni( config(), "Database" ) );
	// well... we don't have an updateManager running on our network right now.
	// UpdateManager::instance();

	int pid = pidFromFile(PID_FILE);
	//  Valid PID    Make sure pid is not another prog, don't kill ourself
	if( !allowDupWindow && (pid > 0) && isRunning(pid, PROCESS_NAME) && (pid != processID()) ){
		printf("Bach is already running with pid %i", processID());
		// connect to running Bach and send it the URL
		printf( ("Telling running Bach("+ QString::number(pid) +") to go to: " + url).toLatin1() );
		QTcpSocket * socket = new QTcpSocket();
		socket->connectToHost( "127.0.0.1", 31104 );
		QTextStream os(socket);
		os << url << "\n";
		socket->flush();
		socket->close();
		delete socket;
		shutdown();
		return 0;
	}

	// Write new client_pid.txt
	if( !pidToFile(PID_FILE) )
	{
		printf((QString("Couldn't write pid to ") + PID_FILE).toLatin1());
	}

	int result;
	// Scoped so that MainWindow destructer is called before shutdown()
	// so that the settings saved in the blur destructor are saved to file
	{
		/* Show splash screen unless -no-splash option is given */
		QSplashScreen * splash=0;
		if( showSplash )
		{
			QString s;
			s.sprintf( "images/splash_%02i.png", rand() % 3 );
			QPixmap pixmap( s );
			splash = new QSplashScreen( pixmap );
			splash->show();
		}

		// These tables should always keep a reference to any
		// loaded records with the key cache, so that they
		// can be loaded by primary key multiple times
		// without hitting the database
		//Database::current()->setUndoEnabled( true );
#ifdef USE_FFMPEG
		registerFFImageSequenceProviderPlugin();
#endif

		BachMainWindow mw(splash);
		if ( bach_editor_mode )
			mw.setWindowTitle( mw.windowTitle() + " :: EDITOR-MODE" );
		else
			mw.setWindowTitle( mw.windowTitle() + " :: READ-ONLY" );
		IniConfig & uc = userConfig();
		uc.pushSection( "BachMainWindow" );
		QStringList fg = uc.readString( "FrameGeometry" ).split(',');
		uc.popSection();
		if( fg.size()==4 ){
			mw.resize( QSize( fg[2].toInt(), fg[3].toInt() ) );
			mw.move( QPoint( fg[0].toInt(), fg[1].toInt() ) );
		}
		mw.show();

		/* Finish the splash screen when MainWindow is shown */
		if( splash )
			splash->finish( &mw );

		/* Enter event loop */
		result = a.exec();
	}

	shutdown();

	return result;
}

