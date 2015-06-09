
#include <QCoreApplication>
#include <QFile>
#include <QRegExp>
#include <QFileInfo>
#include <qlibrary.h>
#include <qlist.h>
#include <qtimer.h>

#include "blurqt.h"
#include "classes.h"
#include "database.h"
#include "freezercore.h"

#include "submitter.h"

#ifdef Q_OS_WIN
#include "windows.h"
#endif

extern void classes_loader();

void printHelp()
{
  printf( "Options:\n" );
  printf( "Please visit the following URL for doco:\n" );
  printf( "\thttp://rover.deimos.org/mediawiki/index.php/Absubmit\n" );
	exit(0);
}

int main(int argc, char * argv[])
{
	QCoreApplication a(argc, argv);

	QStringList argList = a.arguments();
	int argCount = argList.size();
	if( argCount == 1 || argList.contains("--help") || argList.contains("-h") )
		printHelp();

	QMap<QString,QString> argMap;
	for( int i=1; i< argCount; i+=2 ) {
		if( i+1 >= argCount ) break;
		argMap[argList[i]] = argList[i+1];
		LOG_5( "Setting key " + argList[i] + " to " + argList[i+1] );
	}

#ifdef Q_OS_WIN
	QString configFile = "c:/blur/absubmit/absubmit.ini";
#else
	QString configFile = "/etc/absubmit.ini";
#endif // Q_OS_WIN
	if( argMap.contains("-config") ) {
		configFile = argMap["-config"];
		argMap.remove("-config");
	}

#ifdef Q_OS_WIN
	QString logFile = "c:/blur/absubmit/absubmit.log";
#else
	QString logFile = "/var/log/ab/absubmit.log";
#endif // Q_OS_WIN
	if( argMap.contains("-log") ) {
		logFile = argMap["-log"];
		argMap.remove("-log");
	}

	initConfig( configFile, logFile );

#ifdef Q_OS_WIN
	QLibrary excdll( "exchndl.dll" );
	if( !excdll.load() )
		LOG_1( "Unable to load exchndl.dll, error was: " + excdll.errorString() );
#endif

	LOG_3("absubmit starting");

	initStone( argc, argv );

	classes_loader();

	Connection * conn = classesDb()->connection();
	if( !conn->checkConnection() )
		return writeErrorFile( "Unable to connect to the database, Connection String Was: " + conn->connectString() + "  Error Was: " + conn->lastErrorText() );

	// Uncomment this if absubmit ever accesses the db from more than the main thread
	// Share the database across threads, each with their own connection
	// FreezerCore::setDatabaseForThread( classesDb(), Connection::createFromIni( config(), "Database" ) );

	Submitter * submitter = new Submitter();
	submitter->applyArgs( argMap );
	submitter->setExitAppOnFinish( true );

	QTimer::singleShot( 0, submitter, SLOT( submit() ) );

	int result = a.exec();

	LOG_3("absubmit done. returning value '" + QString::number(result) + "' to caller");
	shutdown();

	return result;
}

