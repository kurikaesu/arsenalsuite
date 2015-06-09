

#include <qapplication.h>

#include "spewer.h"
#include "blurqt.h"

int main( int argc, char * argv [] )
{
	QApplication a( argc, argv, false );
	initConfig( "resin.ini" );
	Spewer * spewer = new Spewer();
	return a.exec();
}

