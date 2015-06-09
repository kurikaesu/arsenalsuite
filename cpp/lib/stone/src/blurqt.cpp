
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include <qglobal.h>

#ifdef Q_OS_WIN
#include "windows.h"
#include "shellapi.h"
#include "lm.h"
#include "lmerr.h"
#endif

#include "pyembed.h"
 
#include <qcoreapplication.h>
#include <stdlib.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qprocess.h>
#include <qfiledialog.h>
#include <qlibrary.h>
#include <qpainter.h>
#include <qbitmap.h>

#include "blurqt.h"
#include "database.h"
#include "freezercore.h"
#include "iniconfig.h"
#include "interval.h"
#include "multilog.h"
#include "record.h"
#include "recordlist.h"
#include "schema.h"

IniConfig * sConfig, * sUserConfig;
static bool sUserConfigRead = false;
static QString sConfigName, sLogFile;

void initStone( int argc, char ** argv )
{
	QStringList args;
	for( int i = 1; i<argc; i++ )
		args << QString::fromLatin1( argv[i] );
	initStone(args);
}

void initStone( const QStringList & args )
{
	IniConfig & cfg = config();
	int argc = args.size();
	for( int i=0; i < argc; i++ ) {
		QString arg = args[i];
		bool hasNext = i+1<argc;
		if( arg == "-db-host" && hasNext ) {
			cfg.pushSection( "Database" );
			cfg.writeString( "Host", args[++i] );
			cfg.popSection();
		} else
		if( arg == "-db-port" && hasNext ) {
			cfg.pushSection( "Database" );
			cfg.writeString( "Port", args[++i] );
			cfg.popSection();
		} else
		if( arg == "-db-name" && hasNext ) {
				cfg.pushSection( "Database" );
				cfg.writeString( "DatabaseName", args[++i] );
				cfg.popSection();
		}
		if( arg == "-db-user" && hasNext ) {
			cfg.pushSection( "Database" );
			cfg.writeString( "User", args[++i] );
			cfg.popSection();
		} else
		if( arg == "-db-password" && hasNext ) {
			cfg.pushSection( "Database" );
			cfg.writeString( "Password", args[++i] );
			cfg.popSection();
		} else
		if( arg == "-rum-host" && hasNext ) {
			cfg.pushSection( "Update Server" );
			cfg.writeString( "Host", args[++i] );
			cfg.popSection();
		} else
		if( arg == "-rum-port" && hasNext ) {
			cfg.pushSection( "Update Server" );
			cfg.writeString( "Port", args[++i] );
			cfg.popSection();
		} else
		if( arg == "-show-sql" ) {
			cfg.pushSection( "Database" );
			cfg.writeString( "EchoMode", "EchoSelect,EchoUpdate,EchoInsert,EchoDelete" );
			cfg.popSection();
		}
		if( arg == "-log-file" && hasNext ) {
			sLogFile = args[++i];
		}
/*		else if( arg == "-create-database" ){
			printf( "VALIDATING/CREATING Tables" );
			printf( Database::current()->createTables() ? "Success" : "Failure" );
			return;
		}
		else if( arg == "-verify-database" ){
			printf( "VALIDATING Tables" );
			printf( Database::current()->verifyTables() ? "Success" : "Failure" );
			return;
		}
		else if( arg == "-output-schema" && (i+1 < argc) ){
			Database::current()->schema()->writeXmlSchema( args[++i] );
			return;
		} */
	}
}

QString stoneOptionsHelp()
{
	QStringList ret;
	// Keep this list in sync with the docs in blurqt.h
	ret << "-show-sql";
	ret << "\tOutputs all executed sql to stdout";
	ret << "-show-time";
	ret << "\tOutputs time information at program termination, include sql and index time";
	ret << "-create-database";
	ret << "\tCreates all missing tables in the database, then exits";
	ret << "-verify-database";
	ret << "\tVerifies all tables in the database, then exits";
	ret << "-output-schema FILE";
	ret << "\tOutputs the database schema in xml format to FILE";
	ret << "-db-host HOST";
	ret << "\tSet the database host to HOST. Can be either a hostname or an ipv4 address";
	ret << "-db-port PORT";
	ret << "\tSet the database port to PORT";
	ret << "-db-user USER";
	ret << "\tSet the database username to USER";
	ret << "-db-password PASS";
	ret << "\tSet the database password to PASS";
	ret << "-rum-host HOST";
	ret << "\tSet the update server host to HOST. Can be either a hostname or an ipv4 address";
	ret << "-rum-port PORT";
	ret << "\tSet the update server port to PORT";
	return ret.join("\n");
}

/// Initialises the stone library using a supplied configuration file. Returns if it was able to read the configuration file successfully
/// This function can be called multiple times.  The first time all needed Qt configuration(addLibraryPath,qRegisterMetaType) is done
/// regardless of the config file being found.  Any subsequent times the config file will be read.
/// The first config file that is found will become the primary and will be the destination
/// of saved settings when shutdown is called.

bool initConfig( const QString & configName, const QString & logfile )
{
	if( !QCoreApplication::instance() ) {
		fprintf( stderr, "Calling initConfig before creating a QApplication object is not recommended\nCreating a QCoreApplication object now to avoid a crash\n" );
		int argc = 0;
		new QCoreApplication(argc, (char**)0);
	}

	if( !sConfig ) {
		sConfig = new IniConfig();
		qRegisterMetaType<Record>("Record");
		qRegisterMetaType<RecordList>("RecordList");
		qRegisterMetaType<Interval>("Interval");
#ifdef Q_OS_WIN
#ifdef Q_OS_WIN64
		// Used for 64 bit dlls, won't show up under syswow64
		QCoreApplication::addLibraryPath("c:/windows/system32/blur64/");
#else
		QCoreApplication::addLibraryPath("c:/blur/common/");
#endif
#endif // Q_OS_WIN
	}

	// Check to see if the configuration file exists
	bool configExists = QFile::exists( configName );
	if( !configExists )
		printf("Could not find %s\n", qPrintable(configName));

	if( configExists ) {
		// The first config file that is found will become the primary and will be the destination
		// of saved settings when shutdown is called.
		if( sConfigName.isEmpty() )
			sConfigName = configName;
		sConfig->setFileName( configName );
		sConfig->readFromFile();
	}
	
	if( !logfile.isEmpty() ) {
		sLogFile = logfile;
		if( !sLogFile.isEmpty() && sLogFile.right(4) != ".log" )
			sLogFile = sLogFile + ".log";
	}

	return configExists;
}

void initUserConfig( const QString & fileName )
{
	if( !sUserConfigRead ) {
		sUserConfigRead = true;
		sUserConfig = new IniConfig();
		sUserConfig->setFileName( fileName );
		sUserConfig->readFromFile();
	}
}

static Multilog * mLog = 0;

void shutdown()
{
	FreezerCore::shutdown();
//	Database::shutdown();
	sConfig->writeToFile();
	if( sUserConfigRead )
		sUserConfig->writeToFile();
	delete sConfig;
	sConfig = 0;
	delete sUserConfig;
	sUserConfig = 0;
	delete mLog;
	mLog = 0;
}

IniConfig & config()
{
	if( !sConfig ) {
		Log( "config() function called without a preceding initConfig call." );
		sConfig = new IniConfig();
	}
	return *sConfig;
}

IniConfig & userConfig()
{
	if( !sUserConfig ) {
		Log( "userConfig() function called without a preceding initUserConfig call." );
		sUserConfig = new IniConfig();
	}
	return *sUserConfig;
}

static bool sLoggingEnabled = true;

Multilog * log()
{
	if( sLoggingEnabled && !mLog ) {
		if( sConfig ) {
			sConfig->pushSection( "Logging" );
			sLoggingEnabled = sConfig->readBool( "Enabled", true );
			if( sLoggingEnabled ) {
				mLog = new Multilog(
				sLogFile.isEmpty() ? sConfig->readString( "File", sConfigName.replace(".ini","") + ".log" ) : sLogFile, // Filename
				sConfig->readBool( "EchoStdOut", true ),
				sConfig->readInt( "MaxSeverity", 5 ), // Only critical and important errors by default
				sConfig->readInt( "MaxFiles", 10 ),
				sConfig->readInt( "MaxSize", 1024 * 1024 ) ); // One megabyte
			}
			sConfig->popSection();
		} else
			sLoggingEnabled = false;
	}
	return mLog;
}

void Log( const QString & message, int severity, const QString & file )
{
	if( sLoggingEnabled && log() ) {
		log()->log( severity, message, file );
	} else
		printf( qPrintable( file + ": " + message ) );
}

bool sendEmail( QStringList recipients, const QString & subject, const QString & body, const QString & sender, QStringList attachments )
{
	bool success = runPythonFunction( "blur.email", "send", VarList() << sender << recipients << subject << body << attachments ).toBool();
	if( !success ) {
		LOG_1( "Unable to send email: \nSubject:\t" + subject + "\nRecipients:\t" + recipients.join(",")
		 + "\nBody:\t" + body + "\nattachments:\t" + attachments.join(","));
	}
	return success;
}

#ifdef Q_OS_WIN

QString getUserName()
{
	char buf[100];
	int size=100;
	GetUserNameA(buf, (LPDWORD)(&size));
	buf[size]=0;
	return QString(buf);
}

QStringList getLoggedInUsers()
{
	LPWKSTA_USER_INFO_0 pBuf = NULL;
	LPWKSTA_USER_INFO_0 pTmpBuf;
	NET_API_STATUS nStatus;

	DWORD dwEntriesRead = 0;
	DWORD dwTotalEntries = 0;

	nStatus = NetWkstaUserEnum(NULL, 0, (LPBYTE*)&pBuf, MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries, NULL);

	QStringList users;

	if ((nStatus == 0) || (nStatus == ERROR_MORE_DATA))
	{
		if ((pTmpBuf = pBuf) != NULL)
		{
			for (DWORD i = 0; i < dwEntriesRead; ++i)
			{
				if (pTmpBuf == NULL)
				{
					// Spit out an error
					LOG_1("Error occurred when attempting to retrived logged in user list");
					break;
				}

				users.append(QString::fromWCharArray(pTmpBuf->wkui0_username));
				++pTmpBuf;
			}
		}
	}

	return users;
}

#else

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

QString getUserName()
{
	struct passwd * ps = getpwuid(getuid());
	if( !ps )
		return QString::null;
	return QString(ps->pw_name);
}

QStringList getLoggedInUsers()
{
	QProcess * proc = new QProcess();
	QStringList arguments;
	proc->start("who", arguments);

	QStringList output;

	bool ret = proc->waitForFinished(1000);
	if( !ret ) return output;

	while (proc->canReadLine())
		output.append(proc->readLine());

	QStringList users;
	foreach( QString whoLine, output )
	{
		if (!whoLine.contains(":0"))
			continue;
		QStringList parts = whoLine.split(' ');
		if( parts[1] == ":0" ) {
			if (users.indexOf(parts[0]) == -1)
				users.append(parts[0]);
		}
	}

	return users;
}

#endif // Q_WS_WIN

