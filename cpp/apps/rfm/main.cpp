/*
 *
 * Copyright 2003, 2004 Blur Studio Inc.
 *
 * This file is part of the Resin software package.
 *
 * Resin is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Resin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Resin; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <qapplication.h>
#include <qdir.h>

#include "blurqt.h"
#include "thrasher.h"

extern void classes_loader();

int main( int argc, char * argv [] )
{
        #ifndef Q_OS_WIN
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
        #endif // Q_OS_WIN
	if( !QFile::exists( "/etc/rfm.ini" ) && QFile::exists( "rfm.ini" ) )
		initConfig( "rfm.ini" );
	else
		initConfig( "/etc/rfm.ini" );
	classes_loader();
	QApplication a( argc, argv, false );
	Thrasher * t = new Thrasher();
	int result = a.exec();
	delete t;
	shutdown();
	return result;
}

