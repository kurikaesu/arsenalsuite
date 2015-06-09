
#include <qapplication.h>

#include "session.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "blurqt.h"
#include "freezercore.h"

int main(int argc, char * argv[])
{
	if( argc >= 2  && QString(argv[1]).contains("debug") ){
		qWarning("Starting in debug mode\n");
	}else{
		qWarning("Starting in deamon mode\n");
		if( fork() )
			exit(0);
		setsid();
		if( fork() )
			exit(0);
	}
	QApplication a(argc, argv, false);
	initConfig( "resin.ini" );
	FreezerCore::instance();	
	Session * session = new Session();
	session->connectToHost("jabber.blur.com",/*port*/ 5222,/*login*/ "groupserver",/*pass*/ "groupserver",/*resource*/ "RPCServer");
	return a.exec();
}
