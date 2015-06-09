
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

#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include <qevent.h>
#include <qlist.h>
#include <qobject.h>
#include <qstring.h>
#include <qtcpsocket.h>

#include "record.h"
#include "packetsocket.h"
#include "qds.h"

class QDataStream;
class QTimer;

class RecordList;
class PacketSocket;

namespace Stone {
class Database;

static const quint32 ACK = 0x0;
static const quint32 REPLAY = 0x1;
static const quint32 ADDED = 0x2;
static const quint32 REMOVED = 0x3;
static const quint32 UPDATED = 0x4;
static const quint32 LISTEN = 0x5;

/**
 *  This class communicates with the Resin Update Manager(RUM)
 *  server to send data changes performed locally, and recieve
 *  data changes that are a result of other RUM clients.
 *
 *  This enables the application to immediatly know if important
 *  data changes, so that the interface can be updated, or the
 *  user informed, etc.., without having to poll the database.
 *
 *  Since this class communicates directly with the \ref Table class,
 *  all the member functions are protected.
 * \ingroup Stone
 */
class STONE_EXPORT UpdateManager : public QObject
{
Q_OBJECT
public:
	static UpdateManager * instance();

signals:
	void statusChanged( bool connected );

protected slots:
	void connectToHost();
	void connected();
	void error( QAbstractSocket::SocketError );
	void connectionClosed();
	void processCommands();
	void ack();
	
protected:
	UpdateManager();

	friend class Stone::Table;
	friend class Stone::Database;

	void flushPending();
	void clearPending();

	// Called from the tables
	void recordsAdded( Table * table, const RecordList & recs );
	void recordUpdated( Table * table, Record cur, Record old, bool sendAllFields = false );
	void recordsDeleted( Table * table, const RecordList & recs );
	void recordsDeleted( const QString & table, QList<uint>  );

	void handshake();
	void listen();
	void replay( uint transactionStart, uint transactionEnd = 0 );

	// This command takes ownership of the bytearray
	// and will delete it after it is sent
	void queuePacket( const Packet & pkt );

	void parsePacket( const Packet & pkt );
	void parseAdded( QDS &, uint transaction );
	void parseUpdated( QDS &, uint transaction );
	void parseDeleted( QDS &, uint transaction );
	
	bool mEnabled;
	
	QString mHost;
	uint mPort;
	uint mRetryRate;
	uint mMaxRetrys;
	bool mExecOfflineCommands;
	
	uint mLastTransactionProcessed;
	QString mLastTransactionFile;
	
	static UpdateManager * mSelf;
	PacketSocket * mSocket;
	
	QTimer * mAckTimer;

	enum Status {
		StatusConnected,
		StatusDisconnected
	};

	Status mStatus;
	bool mFlushAfterConnect;
	QList<Packet> mPendingPackets;
};

} //namespace

using Stone::UpdateManager;

#endif // UPDATE_MANAGER_H

