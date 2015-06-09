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

#include <qfile.h>
#include <qtextstream.h>

#include "blurqt.h"

#include "connection.h"
#include "server.h"

Server::Server( QObject * parent )
: QTcpServer( parent )
{
	mTransactionMgr = new TransactionMgr();
}

bool Server::initialize()
{
	if( !mTransactionMgr->initialize( "/var/spool/rum/transaction" ) )
		return false;

	mTransactionMgr->start();

	if( !listen( QHostAddress::Any, 25565 ) ) {
		LOG_1( "Unable to listen on port 25565" );
		return false;
	}
	return true;
}

// thread safe
void Server::connectionClosed( Connection * connection )
{
	QMutexLocker ml( &mConnectionListMutex );
	mConnections.removeAll( connection );
}

void Server::incomingConnection(int socketDescriptor)
{
	LOG_5( "New connection created" );

	PacketSocket * socket = new PacketSocket(this);
	socket->setSocketDescriptor(socketDescriptor);

	Connection * conn = new Connection( socket, this );
	mConnectionListMutex.lock();
	mConnections += conn;
	mConnectionListMutex.unlock();
	conn->start();
}


