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

#include "blurqt.h"

#include "server.h"

int main( int argc, char * argv [] )
{

#ifndef Q_OS_WIN
	if( argc >= 2  && QString(argv[1]).contains("debug") ){
#endif
		LOG_3("Starting in debug mode\n");
#ifndef Q_OS_WIN
	}else{
		LOG_3("Starting in deamon mode\n");
		if( fork() )
			exit(0);
		setsid();
		if( fork() )
			exit(0);
	}
#endif
	QApplication a( argc, argv, false );

	initConfig( "rum.ini", "/var/log/ab/rum.log" );

	Server * server = new Server( 0 );
	if( !server->initialize() )
		return 1;

	LOG_1( "Server Running on " + server->serverAddress().toString() );
	return a.exec();
}

