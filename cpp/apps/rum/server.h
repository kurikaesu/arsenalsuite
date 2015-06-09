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


#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <qmutex.h>

#include "transactionmgr.h"

using namespace Stone;
class Connection;

class Server : public QTcpServer
{
Q_OBJECT
public:

	Server( QObject * parent );

	bool initialize();

	////////////////////////////////////////////
	//////// thread safe ///////////////////////
	////////////////////////////////////////////

//	void setVersion( Connection * conn, uint version, uint oldversion );
	
	void connectionClosed( Connection * );

	TransactionMgr * transactionMgr() const { return mTransactionMgr; }
	
	void incomingConnection(int socketDescriptor);
	////////////////////////////////////////////
	/////// end thread safe ////////////////////
	////////////////////////////////////////////

protected:
	QMutex mConnectionListMutex;
	QList<Connection*> mConnections;
	
	TransactionMgr * mTransactionMgr;
};


#endif // SERVER_H

