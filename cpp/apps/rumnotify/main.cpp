
#include <qapplication.h>

#include "blurqt.h"
#include "notifier.h"
#include "freezercore.h"
#include "taskuser.h"
#include "elementthread.h"

static void printOptions()
{
	qWarning( "Usage: rumnotify [OPTIONS] COMMAND table keylist" );
	qWarning( "COMMAND is one of INSERT, UPDATE, DELETE" );
	qWarning( "Options:" );
	qWarning( "-host HOST\tThe rum host to connect to" );
	qWarning( "-port PORT\tThe rum port to connect to" );
}

int main( int argc, char * argv[] )
{
	QApplication a( argc, argv, false );
	initConfig( "resin.ini" );
	IniConfig & config = initConfig();
	config.pushSection( "Database" );
	
	QString command, table;
	Q3ValueList<uint> keys;
	int i = 1;

	
	if( argc < 3 ) {
		qWarning( "Insufficient arguments" );
		printOptions();
		return 1;
	}
	for( ; i<argc; i++ ) {
		QString arg( argv[i] );
		if( arg == "-host" ) {
			i++;
			if( i == argc ) {
				qWarning( "-host without hostname" );
				return 1;
			}
			config.writeString( "Host", QString( argv[i] ) );
		}
		else if( arg == "-port" ) {
			++i;
			if( i == argc ) {
				qWarning( "-port without port" );
				return 1;
			}
			config.writeInt( "Port", QString( argv[i] ).toUInt() );
		}
		else if( arg.upper() == "INSERT" || arg.upper() == "UPDATE" || arg.upper() == "DELETE" ) {
			command = arg.upper();
			++i;
			break;
		} else {
			qWarning( "Invalid argument: " + arg );
			printOptions();
			return 1;
		}
	}
	
	if( i == argc ) {
		qWarning( command + " without table" );
		printOptions();
		return 1;
	}
	
	table = QString(argv[i++]);
	
	if( i == argc ) {
		qWarning( command + " on " + table + " without keys" );
		printOptions();
		return 1;
	}
	
	for( ; i<argc; i++ ) {
		bool valid;
		uint key = QString( argv[i] ).toUInt( &valid );
		if( !valid ) {
			qWarning( QString( argv[i] ) + " is not a valid key" );
			printOptions();
			return 1;
		}
		keys += key;
	}
	
	if( !FreezerCore::instance()->isConnected() ) {
		qWarning( "Couldn't connect to database" );
		return 1;
	}
	
	TaskUser::table();
	Thread::table();
	
	new Notifier( command, table, keys );
	
	a.exec();
}

