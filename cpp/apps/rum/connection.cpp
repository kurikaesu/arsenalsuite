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
#ifndef Q_OS_WIN
#include <signal.h>
#endif 

#include <QTcpSocket>
#include <qdatastream.h>

#include "qds.h"

#include "server.h"
#include "connection.h"

static const quint32 ACK = 0x0;
static const quint32 REPLAY = 0x1;
static const quint32 ADDED = 0x2;
static const quint32 REMOVED = 0x3;
static const quint32 UPDATED = 0x4;
static const quint32 LISTEN = 0x5;

static const int SEND_BUFFER_SIZE = 1024*32;

quint32 packetType( QByteArray * ba )
{
	quint32 size, tran, ret;
	QDataStream s( ba, QIODevice::ReadOnly );
	s >> size >> tran >> ret;
//	qWarning( QString("PacketType: %1, %3").arg(size).arg(ret) );
	return ret;
}

static const char * packetTypeToString [] = 
{
	"ACK",
	"REPLAY",
	"ADDED",
	"REMOVED",
	"UPDATED",
	"LISTEN"
};

QString packetTypeString( quint32 packetType )
{
	return packetTypeToString[packetType];
}

Connection::Connection( PacketSocket * socket, Server * server )
: QThread()
, mListenMode( false )
, mTimeSinceRecv( 0 )
, mLastTransactionSent( 0 )
, mPacketSize( 0 )
, mBytesToRead( 0 )
, mCurrentBuffer( 0 )
, mVersion( 0 )
, mServer( server )
, mSocket( socket )
{
#ifndef Q_OS_WIN
	// Ignore signals from broken pipe(lost connection)
	signal( SIGPIPE, SIG_IGN );
#endif
	mSocket->setParent( 0 );
	mSocket->moveToThread( this );

//	mSocketDevice = new QSocketDevice( socket, QSocketDevice::Stream );
//	mSocketDevice->setAddressReusable( true );
//	mSocketDevice->setSendBufferSize( SEND_BUFFER_SIZE );
}

void Connection::run()
{
	dataLoop();
	msg( "Closing connection" );
	mServer->connectionClosed( this );
	delete mSocket;
	mSocket = 0;
}

void Connection::sendPacket( const QByteArray & tran )
{
	if( !tran.size() ) {
		msg( "Connection::sendPacket( QByteArray * ) got a null packet" );
		return;
	}
	
	int size( tran.size() );
//	msg( "Sending packet of size " + QString::number( size ) );
	for( int sentTotal=0; sentTotal < size; )
	{
		int sent = mSocket->write( tran.constData() + sentTotal, qMin( SEND_BUFFER_SIZE, size - sentTotal ) );
		if( sent == -1 ) {
			msg( "Error writing to socket" );
			return;
		}
		sentTotal += sent;
		msg( QString("Sent %1 bytes, %2 total").arg(sent).arg(sentTotal) );
	}
	return;
}

void Connection::msg( const QString & msg )
{
	qWarning( qPrintable(QString::number( mSocket->peerPort() ) + " " + msg) );
}

void Connection::dataLoop()
{
	TransactionMgr * mgr = mServer->transactionMgr();
	while( 1 ) {
		
		// Send any transactions that have occured
		uint lastTrans = mgr->lastTransaction();
		if( mListenMode && lastTrans > mLastTransactionSent ) {
			//msg( "Transactions to send" );
			for( uint i = mLastTransactionSent+1; i <= lastTrans; i++ ){
				if( mMyTransactions.contains( i ) ){
					mMyTransactions.removeAll( i );
				} else {
					QByteArray trans = mgr->getTransaction( i, mVersion );
					qWarning( qPrintable(QString("Sending transaction %1, %2 bytes" ).arg( i ).arg( trans.size() )) ); 
					sendPacket( trans );
				}
				if( !mSocket->isValid() )
					return;
			}
			mLastTransactionSent = lastTrans;
		}
		
		// Wait for data
		bool canRead = mSocket->waitForNextPacket( 500 );
		
		if( !mSocket->isValid() ) {
			msg( "Connection closed" );
			return;
		}
		
		// Oops, we timed out
		if( !canRead ) {
			mTimeSinceRecv += 30;
			
			// We wait a full hour to time out, cause there is no need
			// for the client to send us any data unless they have
			// data to send
			if( mTimeSinceRecv > 1000 /*sec*/ * 60 /* min */ * 60 /* hour */ ){
				msg( "Socket timed out, closing connection" );
				return;
			}
			continue;
		}

		while( mSocket->availablePacketCount() ) {

			// We got some data, so reset the timeout variable
			mTimeSinceRecv = 0;

			Packet pkt = mSocket->nextPacket();
			QDS ds(pkt.data);
			if( !mVersion ) {
				// First packet must be magic
				if( pkt.id != 123456789 ) {
					msg( "Connection didn't have the magic" );
					return;
				}
				
				ds >> mVersion;
				msg( "Handshake successful, Got Version: " + QString::number( mVersion ) );
				continue;
			}

			if( pkt.id == ACK ) {
			} else if( pkt.id == REPLAY ) {
				quint32 start, end;
				ds >> start >> end;
				if( end == 0 )
					end = mgr->lastTransaction();
				msg( QString( "Replaying transactions from %1 to %2" ).arg( start ).arg( end ) );
				if( start <= mgr->lastTransaction() ) {
					end = qMin( mgr->lastTransaction(), end );
					for( uint i = start; i <= end; i++ ) {
						QByteArray trans = mgr->getTransaction( i, mVersion );
						qWarning( qPrintable(QString("Sending transaction %1, %2 bytes" ).arg( i ).arg( trans.size() ) ) ); 
						sendPacket( trans );
						if( !mSocket->isValid() )
							return;
					}
				}
			} else if( pkt.id == LISTEN ) {
				quint32 start;
				ds >> start;
				
				msg( "LISTEN " + QString::number( start ) );
				
				// listen 0 starts at the current point
				if( start == 0 )
					start = mgr->lastTransaction();
				else
					start = qMin( mgr->lastTransaction(), start );
					
				mLastTransactionSent = start;
				mListenMode = true;
			} else {
				// Get a unique transaction id from the server thread
				quint32 id = mgr->getTransactionId();
				
				msg( "Transaction got id " + QString::number( id ) );
				
				QDS transaction;
				// Packet header, since we are not using sendPacket
				transaction << pkt.id;
				transaction << quint32(pkt.data.size() + sizeof(quint32));

				// Original payload with transaction id prepended
				transaction << quint32(id);
				transaction << pkt.data;

				// Hand the transaction off to the server
				mgr->setTransaction( id, transaction.data() );
				
				// msg( "Gave transaction to manager" );
				
				// Loop until we update the lasttransaction id
				// We have to loop because we might be waiting
				// for another thread that has a lower transaction
				// id
				while( !mgr->updateLastTransaction( id ) )
					msleep( 5 );
					
				mMyTransactions += id;
			//	msg( "Finished with transaction" );
			
			}
		}
	}
}
